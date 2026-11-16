/********************************************************************************
*                                                                               *
*                             Regular Expression Test                           *
*                                                                               *
*********************************************************************************
* Copyright (C) 1999,2026 by Jeroen van der Zijp.   All Rights Reserved.        *
********************************************************************************/
#include "fx.h"
#include "testargs.h"


/*
  Notes:

  - With --test, a built-in table of patterns, flags, subjects and expected match
    ranges and captures is run, as well as a table of patterns with syntax errors,
    and the exit code is 0 only if all of them hold.

  - Without --test the program parses the pattern on the command line and, if a string
    follows, searches it; the exit code is 1 only if the pattern has a syntax error.
*/

#define NCAP 10    // Must be less that or equal to 10


/*******************************************************************************/

// Pattern, parse and match flags, subject, expected result
struct RexCase {
  const FXchar* pattern;
  FXint         mode;
  const FXchar* subject;
  FXint         where;      // Expected position, or -1 if there is no match
  const FXchar* match;      // Expected matched text
  const FXchar* cap1;       // Expected first capture, or nullptr if not checked
  const FXchar* cap2;       // Expected second capture, or nullptr if not checked
  };


// Pattern with syntax error
struct RexError {
  const FXchar* pattern;
  FXint         mode;
  FXRex::Error  error;
  };


static const RexCase cases[]={
  {"abc",FXRex::Normal,"xxabcxx",2,"abc",nullptr,nullptr},
  {"abc",FXRex::Normal,"xxabxx",-1,nullptr,nullptr,nullptr},
  {"a.c",FXRex::Normal,"abc",0,"abc",nullptr,nullptr},
  {"a*",FXRex::Normal,"aaab",0,"aaa",nullptr,nullptr},
  {"a*",FXRex::Normal,"",0,"",nullptr,nullptr},
  {"a+b",FXRex::Normal,"xaaab",1,"aaab",nullptr,nullptr},
  {"a?b",FXRex::Normal,"ab",0,"ab",nullptr,nullptr},
  {"cat|dog",FXRex::Normal,"hotdog",3,"dog",nullptr,nullptr},
  {"[a-c]+",FXRex::Normal,"xxbcax",2,"bca",nullptr,nullptr},
  {"[^a-c]+",FXRex::Normal,"abxyc",2,"xy",nullptr,nullptr},
  {"\\d+",FXRex::Normal,"abc123def",3,"123",nullptr,nullptr},
  {"\\w+",FXRex::Normal," hello ",1,"hello",nullptr,nullptr},
  {"^abc",FXRex::Normal,"abc",0,"abc",nullptr,nullptr},
  {"^abc",FXRex::Normal,"xabc",-1,nullptr,nullptr,nullptr},
  {"abc$",FXRex::Normal,"xabc",1,"abc",nullptr,nullptr},
  {"abc$",FXRex::Normal,"abcx",-1,nullptr,nullptr,nullptr},
  {"a{2,3}",FXRex::Normal,"aaaa",0,"aaa",nullptr,nullptr},
  {"a{2}",FXRex::Normal,"a",-1,nullptr,nullptr,nullptr},
  {"ABC",FXRex::IgnoreCase,"xabc",1,"abc",nullptr,nullptr},
  {"ABC",FXRex::Normal,"xabc",-1,nullptr,nullptr,nullptr},
  {"abc",FXRex::Exact,"abcd",-1,nullptr,nullptr,nullptr},
  {"abc",FXRex::Exact,"abc",0,"abc",nullptr,nullptr},
  {"a*",FXRex::NotEmpty,"baaa",1,"aaa",nullptr,nullptr},
  {"a+?",FXRex::Normal,"aaa",0,"a",nullptr,nullptr},
  {"<.*>",FXRex::Normal,"<a><b>",0,"<a><b>",nullptr,nullptr},
  {"<.*?>",FXRex::Normal,"<a><b>",0,"<a>",nullptr,nullptr},
  {"a.b",FXRex::Normal,"a\nb",-1,nullptr,nullptr,nullptr},
  {"a.b",FXRex::Newline,"a\nb",0,"a\nb",nullptr,nullptr},
  {"cat",FXRex::Words,"concat cat",7,"cat",nullptr,nullptr},
  {"a.c",FXRex::Verbatim,"abc a.c",4,"a.c",nullptr,nullptr},
  {"a(?=b)",FXRex::Normal,"acab",2,"a",nullptr,nullptr},
  {"a(?!b)",FXRex::Normal,"abac",2,"a",nullptr,nullptr},
  {"(ab)+",FXRex::Capture,"ababab",0,"ababab","ab",nullptr},
  {"(a)(b)",FXRex::Capture,"xab",1,"ab","a","b"},
  {"(a)\\1",FXRex::Capture,"baa",1,"aa","a",nullptr},
  {"(?:a)(b)",FXRex::Capture,"ab",0,"ab","b",nullptr},
  {"(\\w+)@(\\w+)",FXRex::Capture,"mail bob@host now",5,"bob@host","bob","host"},
  {nullptr,0,nullptr,0,nullptr,nullptr,nullptr}
  };


static const RexError errcases[]={
  {"",FXRex::Normal,FXRex::ErrEmpty},
  {"(a",FXRex::Normal,FXRex::ErrParent},
  {"[a",FXRex::Normal,FXRex::ErrBracket},
  {"a{2",FXRex::Normal,FXRex::ErrBrace},
  {"[z-a]",FXRex::Normal,FXRex::ErrRange},
  {"*a",FXRex::Normal,FXRex::ErrNoAtom},
  {"(a)\\2",FXRex::Capture,FXRex::ErrBackRef},
  {"a(b)",FXRex::Normal,FXRex::ErrOK},
  {nullptr,0,FXRex::ErrOK}
  };


// Is text between beg and end the same as want
static FXbool same(const FXchar* text,FXint beg,FXint end,const FXchar* want){
  return 0<=beg && beg<=end && (FXint)strlen(want)==(end-beg) && strncmp(text+beg,want,end-beg)==0;
  }


// Run the tables, return the number of failed checks
static FXint testrex(){
  FXint failures=0;
  FXint beg[NCAP];
  FXint end[NCAP];
  FXRex::Error err;
  FXRex rex;
  FXint where,len,i;

  // Searches
  for(i=0; cases[i].pattern; i++){
    const RexCase& c=cases[i];
    len=strlen(c.subject);
    for(FXint k=0; k<NCAP; k++){ beg[k]=-1; end[k]=-1; }
    err=rex.parse(c.pattern,c.mode);
    if(err!=FXRex::ErrOK){
      printf("BAD: /%s/ does not parse: %s\n",c.pattern,FXRex::getError(err));
      failures++;
      continue;
      }
    where=rex.search(c.subject,len,0,len,c.mode,beg,end,3);
    if(where!=c.where){
      printf("BAD: /%s/ on \"%s\": found at %d, expected %d\n",c.pattern,c.subject,where,c.where);
      failures++;
      continue;
      }
    if(c.where<0) continue;
    if(!same(c.subject,beg[0],end[0],c.match)){
      printf("BAD: /%s/ on \"%s\": matched range %d:%d, expected \"%s\"\n",c.pattern,c.subject,beg[0],end[0],c.match);
      failures++;
      }
    if(c.cap1 && !same(c.subject,beg[1],end[1],c.cap1)){
      printf("BAD: /%s/ on \"%s\": capture 1 is %d:%d, expected \"%s\"\n",c.pattern,c.subject,beg[1],end[1],c.cap1);
      failures++;
      }
    if(c.cap2 && !same(c.subject,beg[2],end[2],c.cap2)){
      printf("BAD: /%s/ on \"%s\": capture 2 is %d:%d, expected \"%s\"\n",c.pattern,c.subject,beg[2],end[2],c.cap2);
      failures++;
      }
    }

  // Syntax errors
  for(i=0; errcases[i].pattern; i++){
    err=rex.parse(errcases[i].pattern,errcases[i].mode);
    if(err!=errcases[i].error){
      printf("BAD: /%s/ gives error \"%s\", expected \"%s\"\n",errcases[i].pattern,FXRex::getError(err),FXRex::getError(errcases[i].error));
      failures++;
      }
    }

  // Syntax check only does not leave a matcher
  err=rex.parse("a(b",FXRex::Syntax);
  if(err!=FXRex::ErrParent){ printf("BAD: syntax check misses the missing parenthesis\n"); failures++; }
  if(!rex.empty()){ printf("BAD: a failed parse leaves a matcher\n"); failures++; }

  // Substitution of captures
  err=rex.parse("(\\w+) (\\w+)",FXRex::Capture);
  where=rex.search("hello world",11,0,11,FXRex::Capture,beg,end,3);
  if(err!=FXRex::ErrOK || where!=0){ printf("BAD: substitution pattern does not match\n"); failures++; }
  else{
    FXString s=FXRex::substitute("hello world",11,beg,end,"\\2 \\1",3);
    if(s!="world hello"){ printf("BAD: substitute gives \"%s\", expected \"world hello\"\n",s.text()); failures++; }
    s=FXRex::substitute("hello world",11,beg,end,"<&>",3);
    if(s!="<hello world>"){ printf("BAD: substitute gives \"%s\", expected \"<hello world>\"\n",s.text()); failures++; }
    }
  return failures;
  }


// Print command line help
static void printusage(){
  fxmessage("Usage: rex [options] pat [string]\n");
  fxmessage("  options:\n");
  fxmessage("  -?, -h, --help                      Print help.\n");
  fxmessage("  -tracetopics <topics>               Enable trace topics.\n");
  fxmessage("  -u, --unicode                       Unicode mode.\n");
  fxmessage("  -s, --syntax                        Syntax check only.\n");
  fxmessage("  -v, --verbatim                      Verbatim mode.\n");
  fxmessage("  -c, --capture                       Capturing parentheses.\n");
  fxmessage("  -i, --ignore-case                   Case-insensitive.\n");
  fxmessage("  -nl, --newline                      Newline matches \"any\" character classes.\n");
  fxmessage("  -x, --exact                         Match should consume all of string.\n");
  fxmessage("  -n, --not-empty                     Match should be non-empty.\n");
  fxmessage("  -r, --reverse                       Reverse match.\n");
  fxmessage("  -w, --words                         Match whole words.\n");
  fxmessage("  -nb, --not-bol                      Start of string is not begin of line.\n");
  fxmessage("  -ne, --not-eol                      End of string is not end of line.\n");
  fxmessage("  -l <num>, --levels <num>            Capure levels.\n");
  }


// Start the whole thing
int main(int argc,char** argv){
  fxtestargs(argc,argv);
  FXint beg[NCAP]={-1,-1,-1,-1,-1,-1,-1,-1,-1,-1};
  FXint end[NCAP]={-1,-1,-1,-1,-1,-1,-1,-1,-1,-1};
  FXint mode=FXRex::Normal;
  FXint ncap=1;
  FXint where;
  FXint arg=1;
  FXRex rex;
  FXRex::Error err;

  // Self-check with the built-in tables
  if(1<argc && FXString::compare(argv[1],"--test")==0){
    FXint failures=testrex();
    if(failures){
      printf("FAILED: %d check(s)\n",failures);
      return 1;
      }
    printf("all checks passed\n");
    return 0;
    }

  // Parse options
  while(arg<argc && argv[arg][0]=='-'){
    if(FXString::compare(argv[arg],"-s")==0 || FXString::compare(argv[arg],"--syntax")==0){
      mode|=FXRex::Syntax;
      }
    else if(FXString::compare(argv[arg],"-v")==0 || FXString::compare(argv[arg],"--verbatim")==0){
      mode|=FXRex::Verbatim;
      }
    else if(FXString::compare(argv[arg],"-c")==0 || FXString::compare(argv[arg],"--capture")==0){
      mode|=FXRex::Capture;
      }
    else if(FXString::compare(argv[arg],"-i")==0 || FXString::compare(argv[arg],"--ignore-case")==0){
      mode|=FXRex::IgnoreCase;
      }
    else if(FXString::compare(argv[arg],"-nl")==0 || FXString::compare(argv[arg],"--newline")==0){
      mode|=FXRex::Newline;
      }
    else if(FXString::compare(argv[arg],"-x")==0 || FXString::compare(argv[arg],"--exact")==0){
      mode|=FXRex::Exact;
      }
    else if(FXString::compare(argv[arg],"-n")==0 || FXString::compare(argv[arg],"--not-empty")==0){
      mode|=FXRex::NotEmpty;
      }
    else if(FXString::compare(argv[arg],"-r")==0 || FXString::compare(argv[arg],"--reverse")==0){
      mode|=FXRex::Reverse;
      }
    else if(FXString::compare(argv[arg],"-w")==0 || FXString::compare(argv[arg],"--words")==0){
      mode|=FXRex::Words;
      }
    else if(FXString::compare(argv[arg],"-nb")==0 || FXString::compare(argv[arg],"--not-bol")==0){
      mode|=FXRex::NotBol;
      }
    else if(FXString::compare(argv[arg],"-ne")==0 || FXString::compare(argv[arg],"--not-eol")==0){
      mode|=FXRex::NotEol;
      }
    else if(FXString::compare(argv[arg],"-u")==0 || FXString::compare(argv[arg],"--unicode")==0){
      mode|=FXRex::Unicode;
      }
    else if(FXString::compare(argv[arg],"-l")==0 || FXString::compare(argv[arg],"--levels")==0){
      if(++arg>=argc){ fxwarning("rex: missing capture levels.\n"); return 1; }
      sscanf(argv[arg],"%d",&ncap);
      }
    else if(FXString::compare(argv[arg],"-?")==0 || FXString::compare(argv[arg],"-h")==0 || FXString::compare(argv[arg],"--help")==0){
      printusage();
      return 0;
      }
    else if(FXString::compare(argv[arg],"-tracetopics")==0){
      if(++arg>=argc){ fxwarning("rex: missing argument for -tracetopics.\n"); return 0; }
      setTraceTopics(argv[arg]);
      }
    arg++;
    }

  // Pattern
  if(arg<argc){

    // Parse expression
    err=rex.parse(argv[arg],mode);

    // Show result of parse
    fxmessage("parse(\"%s\") = %s\n",argv[arg],FXRex::getError(err));
    if(err!=FXRex::ErrOK) return 1;

    // Next
    arg++;

    // Input
    if(arg<argc){

      // Search string with pattern
      where=rex.search(argv[arg],strlen(argv[arg]),0,strlen(argv[arg]),mode,beg,end,ncap);

      // If found, show where
      if(0<=where){
        fxmessage("found at %d\n",where);
        for(FXint i=0; i<ncap; i++){
          fxmessage("capture at %d:%d\n",beg[i],end[i]);
          }
        for(FXint i=beg[0]; i<end[0]; i++){
          fxmessage("%c",argv[arg][i]);
          }
        fxmessage("\n");
        }
      else{
        fxmessage("no match\n");
        }

      // Next
      arg++;
      }
    }
  return 0;
  }

