/********************************************************************************
*                                                                               *
*                          M e m o r y   M a p   T e s t                        *
*                                                                               *
*********************************************************************************
* Copyright (C) 2004,2026 by Jeroen van der Zijp.   All Rights Reserved.        *
********************************************************************************/
#include "fx.h"
#include "testargs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
  Notes:

  - Test FXMappedFile capabilities.
  - With read or write, one file is mapped and the program waits for return
    between the steps, so the mapping can be looked at from outside.
  - With --test, a temporary file is written, read back, partially mapped,
    modified and grown through maps, and every result is checked; the exit
    code is 0 only if all checks held.

*/

/*******************************************************************************/

static FXint failures=0;


// Record a failed check
static void check(FXbool ok,const FXchar* what){
  if(!ok){
    failures++;
    printf("BAD: %s\n",what);
    }
  }


// Byte expected at position i of the test file
static inline FXuchar pattern(FXival i){
  return (FXuchar)((i*31+7)&0xff);
  }


// Check bytes of the map against the pattern; the map starts at file position start
static FXbool matches(const FXuchar* base,FXival n,FXival start){
  for(FXival i=0; i<n; i++){
    if(base[i]!=pattern(start+i)) return false;
    }
  return true;
  }


// Test file mapping on a temporary file
static void testmemmap(){
  const FXival length=100000;                   // Not a multiple of the page size
  const FXival extra=4096;
  FXString file=FXPath::absolute(FXSystem::getTempDirectory(),FXString::value("fox-memmap-test-%d.tmp",FXProcess::current()));
  FXMappedFile map;
  FXuchar* base;
  FXival i,gran;

  gran=FXMappedFile::granularity();
  check(0<gran,"mapping granularity is positive");

  // Missing file can not be mapped for reading
  base=(FXuchar*)map.open(file,FXIO::Reading);
  check(base==nullptr,"mapping a missing file fails");

  // Create the file through a map and fill it
  base=(FXuchar*)map.open(file,FXIO::Writing,FXIO::AllReadWrite,length,0);
  check(base!=nullptr,"map for writing opens");
  if(base){
    check(map.data()==base && map.length()==length && map.offset()==0,"map for writing reports data, length and offset");
    for(i=0; i<length; i++) base[i]=pattern(i);
    check(map.flush(),"flush succeeds");
    check(map.close(),"close of map for writing succeeds");
    check(map.data()==nullptr && map.length()==0,"closed map has no data");
    check(FXStat::size(file)==length,"file has the mapped size");

    // Map whole file for reading
    base=(FXuchar*)map.open(file,FXIO::Reading);
    check(base!=nullptr,"map for reading opens");
    if(base){
      check(map.length()==length && map.size()==length,"whole file is mapped when length is 0");
      check(matches(base,length,0),"contents read through the map");
      check(map.close(),"close of map for reading succeeds");
      }

    // Map a part of the file at an offset
    if(gran<length){
      base=(FXuchar*)map.open(file,FXIO::Reading,FXIO::AllReadWrite,1000,gran);
      check(base!=nullptr,"map at an offset opens");
      if(base){
        check(map.length()==1000 && map.offset()==gran,"map at an offset reports length and offset");
        check(matches(base,1000,gran),"contents read through the map at an offset");
        check(map.close(),"close of map at an offset succeeds");
        }
      }

    // Change the file through a read-write map
    base=(FXuchar*)map.open(file,FXIO::ReadWrite);
    check(base!=nullptr,"map for reading and writing opens");
    if(base){
      check(map.length()==length,"read-write map covers the whole file");
      base[0]=0xAA;
      base[length-1]=0x55;
      check(map.close(),"close of read-write map succeeds");
      }
    base=(FXuchar*)map.open(file,FXIO::Reading);
    check(base!=nullptr,"map reopens after change");
    if(base){
      check(base[0]==0xAA && base[length-1]==0x55,"changes made through the map are in the file");
      check(matches(base+1,length-2,1),"the rest of the file is unchanged");
      map.close();
      }

    // Make the file bigger through a map
    base=(FXuchar*)map.open(file,FXIO::ReadWrite,FXIO::AllReadWrite,length+extra,0);
    check(base!=nullptr,"map larger than the file opens");
    if(base){
      check(map.length()==length+extra,"larger map has the requested length");
      check(base[0]==0xAA && matches(base+1,length-2,1),"old contents survive growing");
      for(i=length; i<length+extra; i++){
        if(base[i]!=0){ check(false,"the added part of the file is zero"); break; }
        }
      check(map.close(),"close of larger map succeeds");
      check(FXStat::size(file)==length+extra,"file has grown");
      }
    }

  // Remove it
  check(FXFile::remove(file),"temporary file is removed");
  }


/*******************************************************************************/

enum ACTION {
  TEST_NONE,
  TEST_READ,
  TEST_WRITE
  };


void printusage(){
  fprintf(stderr,"Usage: \n\n");
  fprintf(stderr,"   memmap --test\n\n");
  fprintf(stderr," or\n\n");
  fprintf(stderr,"   memmap read <length> <filename>\n\n");
  fprintf(stderr," or\n\n");
  fprintf(stderr,"   memmap write <length> <filename>\n");
  }


// Start the whole thing
int main(int argc,char** argv){
  fxtestargs(argc,argv);
  FXString filename=FXString::null;
  ACTION action=TEST_READ;
  FXival index;
  FXival length=0;
  FXival offset=0;
  char* base;

  // What to test
  FXMappedFile map;

  // Generated data, checked
  if(1<argc && strcmp(argv[1],"--test")==0){
    testmemmap();
    if(failures){
      printf("FAILED: %d check(s)\n",failures);
      return 1;
      }
    printf("all checks passed\n");
    return 0;
    }

  // Show how to use
  if(argc<4){
    printusage();
    exit(0);
    }

  // Get action
  if(strcmp(argv[1],"read")==0){
    action=TEST_READ;
    }
  else if(strcmp(argv[1],"write")==0){
    action=TEST_WRITE;
    }
  else{
    printusage();
    exit(0);
    }

  // Get size
  if(sscanf(argv[2],"%td",&length)<1){
    printusage();
    exit(0);
    }

  // Get filename
  filename=argv[3];

  // Show process id
  fprintf(stderr,"Process id=%d\n",FXProcess::current());

  // Test reading
  if(action==TEST_READ){

    // Wait for return
    fprintf(stderr,"Press return to map:\n");
    //getchar();

    // Map
    base=(char*)map.open(filename,FXIO::Reading,FXIO::AllReadWrite,length,offset);
    if(base==nullptr){
      fprintf(stderr,"map.open returned NULL\n");
      exit(1);
      }

    fprintf(stderr,"actual length = %td\n",map.length());

    // Touch it
    for(index=0; index<map.length(); index++){
      if(base[index]!='Z') fprintf(stderr,"base[%td]=%d\n",index,base[index]);
      }

    // Wait for return
    fprintf(stderr,"Press return to unmap:\n");
    getchar();

    // Unmap
    map.close();

    // Wait for return
    fprintf(stderr,"Press return to quit:\n");
    getchar();
    }

  // Test writing
  else if(action==TEST_WRITE){

    // Wait for return
    fprintf(stderr,"Press return to map:\n");
   // getchar();

    // Map
    base=(char*)map.open(filename,FXIO::Writing,FXIO::AllReadWrite,length,offset);
    if(base==nullptr){
      fprintf(stderr,"openMap returned NULL\n");
      exit(1);
      }


    fprintf(stderr,"actual length = %td\n",map.length());
    fxmessage("base: %p\n", base);

    // Write it
    for(index=0; index<map.length(); index++){
      base[index]='Z';
      }

    // Wait for return
    fprintf(stderr,"Press return to unmap:\n");
    getchar();

    // Unmap
    map.close();

    // Wait for return
    fprintf(stderr,"Press return to quit:\n");
    getchar();
    }

  return 0;
  }

