/********************************************************************************
*                                                                               *
*                         P s e u d o   T e r m i n a l   T e s t               *
*                                                                               *
*********************************************************************************
* Copyright (C) 2026 by Jeroen van der Zijp.   All Rights Reserved.             *
*********************************************************************************/
#include "fx.h"
#include "fxpty.h"
#include <stdio.h>
#include <string.h>

#if defined(WIN32)
#include <windows.h>
#endif

using namespace FX;

/*******************************************************************************/

// Child: wait for one line and answer it
static int child(){
  FXchar line[256];
  fputs("READY\r\n",stdout);
  fflush(stdout);
  if(!fgets(line,sizeof(line),stdin)) return 3;
  while(strlen(line) && (line[strlen(line)-1]=='\n' || line[strlen(line)-1]=='\r')){
    line[strlen(line)-1]='\0';
    }
  fprintf(stdout,"GOT:%s\r\n",line);
  fflush(stdout);
  return 7;
  }


// Drain the PTY into the accumulated text
static void drain(FXPty& pty,FXString& output){
  FXchar buffer[1024];
  FXival count;
  while((count=pty.read(buffer,sizeof(buffer)))>0){
    output.append(buffer,(FXint)count);
    }
  }


// Wait for a marker in the output
static FXbool waitFor(FXPty& pty,FXString& output,const FXchar* marker){
  FXint i;
  for(i=0; i<500; i++){
    drain(pty,output);
    if(output.find(marker)>=0) return true;
    FXThread::sleep(10000000);
    }
  return false;
  }


// Parent: start the test binary as a child on a PTY
static int parent(const FXchar* executable){
  FXPty pty;
  FXTerminalLaunch launch;
  FXString error;
  FXString output;
  FXint i;
#if defined(WIN32)
  {
    HMODULE ntdll=GetModuleHandleW(L"ntdll.dll");
    if(ntdll && GetProcAddress(ntdll,"wine_get_version")){
      printf("SKIP: ConPTY is not functional under Wine\n");
      return 0;
      }
  }
#endif
  launch.shell=executable;
  launch.arguments="--child";
  launch.directory=FXSystem::getCurrentDirectory();
  launch.term="xterm-256color";
  launch.kind=TERMINAL_XTERM;
  launch.columns=80;
  launch.rows=24;
  if(!pty.start(launch,error)){
    if(error.find("not available")>=0){
      printf("SKIP: %s\n",error.text());
      return 0;
      }
    printf("FAIL: start: %s\n",error.text());
    return 1;
    }
  if(!waitFor(pty,output,"READY")){
    printf("FAIL: child did not become ready; output [%s]\n",output.text());
    return 1;
    }
  if(pty.write("hello\n",6)!=6){
    printf("FAIL: could not queue input\n");
    return 1;
    }
  pty.flushWrites();
  if(!waitFor(pty,output,"GOT:hello")){
    printf("FAIL: child did not receive input; output [%s]\n",output.text());
    return 1;
    }
  pty.resize(100,30);
  for(i=0; i<500 && pty.isRunning(); i++){
    drain(pty,output);
    FXThread::sleep(10000000);
    }
  drain(pty,output);
  if(pty.getExitCode()!=7){
    printf("FAIL: exit code %d, expected 7; output [%s]\n",pty.getExitCode(),output.text());
    return 1;
    }
  printf("pseudo terminal test passed\n");
  return 0;
  }


// Main
int main(int argc,FXchar* argv[]){
  if(1<argc && strcmp(argv[1],"--child")==0) return child();
  return parent(FXSystem::getExecFilename().text());
  }
