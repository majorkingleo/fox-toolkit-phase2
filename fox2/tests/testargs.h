/*
  Common command line handling of the FOX tests.

  - "--no-dialogs" is removed from the argument list; errors and warnings are then
    written to stdout instead of opening message boxes.
*/
#ifndef TESTARGS_H
#define TESTARGS_H

#include <string.h>
#include "fx.h"

inline void fxtestargs(int& argc,char** argv){
  int i=1;
  while(i<argc){
    if(strcmp(argv[i],"--no-dialogs")==0){
      FX::fxdialogs(false);
      for(int j=i+1; j<=argc; j++) argv[j-1]=argv[j];
      argc--;
      }
    else{
      i++;
      }
    }
  }

#endif
