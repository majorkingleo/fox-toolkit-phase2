/********************************************************************************
*                                                                               *
*                              F o x T e r m                                    *
*                                                                               *
*********************************************************************************
* Copyright (C) 2026 by Jeroen van der Zijp.   All Rights Reserved.             *
*********************************************************************************
* This program is free software: you can redistribute it and/or modify          *
* it under the terms of the GNU General Public License as published by          *
* the Free Software Foundation, either version 3 of the License, or             *
* (at your option) any later version.                                           *
*                                                                               *
* This program is distributed in the hope that it will be useful,               *
* but WITHOUT ANY WARRANTY; without even the implied warranty of                *
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the                 *
* GNU General Public License for more details.                                  *
*                                                                               *
* You should have received a copy of the GNU General Public License             *
* along with this program.  If not, see <http://www.gnu.org/licenses/>.         *
********************************************************************************/
#include "fx.h"
#include "TerminalWindow.h"
#include "Profiles.h"
#include <stdio.h>
#include <string.h>
#include <signal.h>

using namespace FX;

/*******************************************************************************/

// Quote one command line argument
static FXString quoteArgument(const FXString& argument){
  FXString result;
  FXbool quote=false;
  if(argument.find(' ')>=0 || argument.find('\t')>=0 || argument.find('"')>=0) quote=true;
  if(!quote) return argument;
  result+='"';
  for(FXint i=0; i<argument.length(); i++){
    if(argument[i]=='"' || argument[i]=='\\') result+='\\';
    result+=argument[i];
    }
  result+='"';
  return result;
  }


// Print usage
static void usage(const FXchar* program){
  printf("Usage: %s [--profile NAME] [--term vt100|xterm]\n",program);
  printf("       [--working-directory DIR] [--execute COMMAND [ARG...]]\n");
  printf("\n");
  printf("  --profile NAME          select a profile (default)\n");
  printf("  --term vt100|xterm      select the emulation profile\n");
  printf("  --working-directory DIR start the shell in DIR\n");
  printf("  --opacity PERCENT       set whole-window opacity (0..100)\n");
  printf("  --execute COMMAND       run COMMAND directly without a shell\n");
  printf("  --help                  show this help\n");
  printf("  --version               show the FOX version\n");
  }


// Start the application
int main(int argc,char* argv[]){
  FXTerminalLaunch launch;
  FXString profile;
  FXString execute;
  FXString arguments;
  FXString directory;
  FXString term;
  FXint opacity=100;
  FXint kind=TERMINAL_XTERM;
  FXApp application("FoxTerm");
  TerminalWindow *window;
  FXint i;
  if(fxversion[0]!=FOX_MAJOR || fxversion[1]!=FOX_MINOR || fxversion[2]!=FOX_LEVEL){
    fxerror("FOX Library mismatch; expected version: %d.%d.%d, but found version: %d.%d.%d.\n",FOX_MAJOR,FOX_MINOR,FOX_LEVEL,fxversion[0],fxversion[1],fxversion[2]);
    }
  for(i=1; i<argc; i++){
    if(strcmp(argv[i],"--help")==0){
      usage(argv[0]);
      return 0;
      }
    if(strcmp(argv[i],"--version")==0){
      printf("%d.%d.%d\n",FOX_MAJOR,FOX_MINOR,FOX_LEVEL);
      return 0;
      }
    if(strcmp(argv[i],"--profile")==0 && i+1<argc){
      profile=argv[++i];
      continue;
      }
    if(strcmp(argv[i],"--term")==0 && i+1<argc){
      term=argv[++i];
      if(term=="vt100") kind=TERMINAL_VT100;
      else if(term=="xterm"){ kind=TERMINAL_XTERM; term="xterm-256color"; }
      else{
        fprintf(stderr,"%s: unknown terminal type %s\n",argv[0],term.text());
        return 2;
        }
      continue;
      }
    if(strcmp(argv[i],"--opacity")==0 && i+1<argc){
      FXint parsed=FXString(argv[++i]).toInt();
      opacity=FXCLAMP(0,parsed,100);
      continue;
      }
    if(strcmp(argv[i],"--working-directory")==0 && i+1<argc){
      directory=argv[++i];
      continue;
      }
    if(strcmp(argv[i],"--execute")==0){
      if(i+1>=argc){
        fprintf(stderr,"%s: --execute needs a command\n",argv[0]);
        return 2;
        }
      execute=argv[++i];
      for(i=i+1; i<argc; i++){
        if(!arguments.empty()) arguments+=' ';
        arguments+=quoteArgument(argv[i]);
        }
      break;
      }
    fprintf(stderr,"%s: unknown option %s\n",argv[0],argv[i]);
    usage(argv[0]);
    return 2;
    }
  application.init(argc,argv);
  {
    Profiles profiles;
    FXint selected;
    TerminalProfile chosen;
    profiles.load(application.reg());
    if(profile.empty()) profile=profiles.getActive();
    selected=profiles.find(profile);
    if(selected<0){
      fprintf(stderr,"%s: unknown profile %s\n",argv[0],profile.text());
      return 2;
      }
    chosen=profiles.at(selected);
    if(!directory.empty()) chosen.directory=directory;
    if(!term.empty()){
      chosen.term=term;
      chosen.kind=kind;
      }
    if(!execute.empty()){
      chosen.shell=execute;
      chosen.arguments=arguments;
      }
    launch=profiles.launch(chosen);
    window=new TerminalWindow(&application,launch,chosen.id);
    }
  application.addSignal(SIGINT,window,TerminalWindow::ID_QUIT);
  new FXToolTip(&application,0);
  application.create();
  window->setOpacity(opacity);
  return application.run();
  }
