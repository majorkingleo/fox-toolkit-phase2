/********************************************************************************
*                                                                               *
*                                   Dictionary Test                             *
*                                                                               *
*********************************************************************************
* Copyright (C) 2008,2026 by Jeroen van der Zijp.   All Rights Reserved.        *
********************************************************************************/
#include "fx.h"
#include "testargs.h"

/*
  Notes:
  - Tests for FXDictionary.
  - Without arguments, the words to insert, look up and remove are read from
    stdin, and the times are printed (a benchmark).
  - With --test, a fixed set of generated keys is used, and the result of every
    operation is checked; the exit code is 0 only if all checks held.
*/

static inline FXTime timesource(){
  return FXThread::ticks();
//  return FXThread::time();
  }


/*******************************************************************************/

static FXint failures=0;


// Record a failed check
static void check(FXbool ok,const FXchar* what){
  if(!ok){
    failures++;
    if(failures<=20) printf("BAD: %s\n",what);
    }
  }


// Generate unique key number i
static FXString makekey(FXint i){
  FXchar buf[64];
  snprintf(buf,sizeof(buf),"key-%d-%x",i,(FXuint)((FXuint)i*2654435761u));
  return FXString(buf);
  }


// Value stored for key number i
static inline void* makevalue(FXint i){
  return (void*)(FXival)(i+1);
  }


// Check that exactly the keys with number i%step==rem are present with value(i)
static void checkContents(const FXDictionary& dict,FXint n,FXint step,FXint rem,const FXchar* what){
  FXint i;
  for(i=0; i<n; i++){
    FXString key=makekey(i);
    if(i%step==rem){
      FXival pos=dict.find(key);
      check(0<=pos && dict.key(pos)==key && dict.data(pos)==makevalue(i),what);
      check(dict.has(key) && dict.at(key)==makevalue(i),what);
      }
    else{
      check(!dict.has(key) && dict.find(key)<0 && dict.at(key)==nullptr,what);
      }
    }
  }


// Test the dictionary with generated keys
static void testdictionary(){
  const FXint N=20000;
  FXDictionary dict;
  FXbool thrown;
  FXival pos;
  FXint i,count;
  FXuval sum;
  void* old;

  // Empty dictionary
  check(dict.empty(),"new dictionary is empty");
  check(dict.used()==0,"new dictionary has no entries");
  check(!dict.has("a") && dict.find("a")<0,"empty dictionary does not have a key");
  check(dict.remove("a")==nullptr,"remove from empty dictionary returns null");

  // Null and empty keys are refused
  thrown=false;
  try{ dict.find(""); } catch(const FXRangeException&){ thrown=true; }
  check(thrown,"find of empty key throws");
  thrown=false;
  try{ dict.at(""); } catch(const FXRangeException&){ thrown=true; }
  check(thrown,"at of empty key throws");
  thrown=false;
  try{ dict.remove(""); } catch(const FXRangeException&){ thrown=true; }
  check(thrown,"remove of empty key throws");
  check(dict.used()==0,"refused keys leave the dictionary empty");

  // Insertions (the table grows several times)
  for(i=0; i<N; i++){
    old=dict.insert(makekey(i),makevalue(i));
    check(old==nullptr,"insert of a new key returns null");
    }
  check(!dict.empty() && dict.used()==N,"used count after insertions");
  checkContents(dict,N,1,0,"lookup after insertions");
  check(dict.find("no-such-key")<0 && !dict.has("no-such-key"),"missing key is not found");
  check(dict.used()==N,"lookups do not insert");

  // Replace values
  for(i=0; i<N; i+=3){
    old=dict.insert(makekey(i),makevalue(i+N));
    check(old==makevalue(i),"insert of an existing key returns the old value");
    check(dict.at(makekey(i))==makevalue(i+N),"replaced value is stored");
    old=dict.insert(makekey(i),makevalue(i));
    check(old==makevalue(i+N),"second replace returns the replaced value");
    }
  check(dict.used()==N,"replacing does not change the used count");
  checkContents(dict,N,1,0,"lookup after replacing");

  // Walk all slots
  count=0;
  sum=0;
  for(pos=0; pos<dict.no(); pos++){
    if(!dict.empty(pos)){ count++; sum+=(FXuval)dict.data(pos); }
    }
  check(count==N,"number of occupied slots equals the used count");
  check(sum==(FXuval)N*(FXuval)(N+1)/2,"sum of all values");

  // Remove every second key
  for(i=0; i<N; i+=2){
    old=dict.remove(makekey(i));
    check(old==makevalue(i),"remove returns the old value");
    check(dict.remove(makekey(i))==nullptr,"second remove returns null");
    }
  check(dict.used()==N/2,"used count after removing half");
  checkContents(dict,N,2,1,"lookup after removing half");

  // Insert them again; voided slots are reused
  for(i=0; i<N; i+=2){
    old=dict.insert(makekey(i),makevalue(i));
    check(old==nullptr,"insert after remove returns null");
    }
  check(dict.used()==N,"used count after reinserting");
  checkContents(dict,N,1,0,"lookup after reinserting");

  // Copy, assign and adopt
  {
  FXDictionary copy(dict);
  FXDictionary assigned;
  FXDictionary adopted;
  assigned=dict;
  check(copy.used()==N && assigned.used()==N,"copies have the same used count");
  checkContents(copy,N,1,0,"lookup in constructed copy");
  checkContents(assigned,N,1,0,"lookup in assigned copy");
  copy.remove(makekey(7));
  check(!copy.has(makekey(7)) && dict.has(makekey(7)) && assigned.has(makekey(7)),"copies are independent");
  adopted.adopt(assigned);
  check(adopted.used()==N && assigned.used()==0,"adopt takes over the table");
  checkContents(adopted,N,1,0,"lookup in adopted dictionary");
  }

  // Erase by position
  pos=dict.find(makekey(0));
  check(0<=pos,"key 0 is present before erase");
  old=dict.erase(pos);
  check(old==makevalue(0) && !dict.has(makekey(0)),"erase removes the entry");
  check(dict.erase(pos)==nullptr,"erase of a voided slot returns null");
  check(dict.used()==N-1,"used count after erase");
  thrown=false;
  try{ dict.erase(-1); } catch(const FXRangeException&){ thrown=true; }
  check(thrown,"erase out of range throws");

  // Remove everything; the table shrinks
  for(i=0; i<N; i++){
    dict.remove(makekey(i));
    }
  check(dict.used()==0,"used count after removing everything");
  checkContents(dict,N,1,-1,"lookup after removing everything");

  // Usable again after having shrunk
  for(i=0; i<100; i++){
    dict.insert(makekey(i),makevalue(i));
    }
  check(dict.used()==100,"used count after refilling");
  checkContents(dict,100,1,0,"lookup after refilling");

  // Clear
  check(dict.clear(),"clear succeeds");
  check(dict.empty() && dict.used()==0,"cleared dictionary is empty");
  check(!dict.has(makekey(1)),"cleared dictionary does not have the key");
  }


/*******************************************************************************/

int main(int argc,char* argv[]){
  fxtestargs(argc,argv);
  FXTime beg,end,tot,max,ins;
  FXchar buf[1000];
  FXDictionary dict;

  // Generated data, checked
  if(1<argc && strcmp(argv[1],"--test")==0){
    testdictionary();
    if(failures){
      printf("FAILED: %d check(s)\n",failures);
      return 1;
      }
    printf("all checks passed\n");
    return 0;
    }

  max=0;
  tot=0;
  ins=0;

  // Insertions
  while(fgets(buf,sizeof(buf),stdin)){
    if(strchr(buf,'\n')) *strchr(buf,'\n')='\0';

    beg=timesource();
    dict.insert(buf,nullptr);
    end=timesource();
    tot+=(end-beg);
    max=(end-beg)>max?(end-beg):max;
    ins++;
    }

  printf("insertions: %lld avg: %lld max: %lld\n",ins,ins?tot/ins:0,max);


  max=0;
  tot=0;
  ins=0;
  rewind(stdin);

  // Lookups
  while(fgets(buf,sizeof(buf),stdin)){
    if(strchr(buf,'\n')) *strchr(buf,'\n')='\0';

    beg=timesource();
    dict.at(buf);
    end=timesource();
    tot+=(end-beg);
    max=(end-beg)>max?(end-beg):max;
    ins++;
    }

  printf("Lookups: %lld avg: %lld max: %lld\n",ins,ins?tot/ins:0,max);

  max=0;
  tot=0;
  ins=0;
  rewind(stdin);

  // Removes
  while(fgets(buf,sizeof(buf),stdin)){
    if(strchr(buf,'\n')) *strchr(buf,'\n')='\0';

    beg=timesource();
    dict.remove(buf);
    end=timesource();
    tot+=(end-beg);
    max=(end-beg)>max?(end-beg):max;
    ins++;
    }

  printf("Removes: %lld avg: %lld max: %lld\n",ins,ins?tot/ins:0,max);
  printf("no=%lld used=%lld\n",(long long)dict.no(),(long long)dict.used());

#if 0
  for(int i=0; i<dict.no(); ++i){
    if(!dict.empty(i)){
      printf("%d %s\n",i,dict.key(i).text());
      }
    }
#endif

  return 0;
  }

