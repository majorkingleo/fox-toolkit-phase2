#include "ChartDemoWindow.h"
#include <cstdio>
#include <cstring>

#if defined(WIN32)
#include <windows.h>
#endif

namespace FXCHARTDEMO {

using namespace FX;

#if defined(WIN32)
static void attachConsole(){
  if(AttachConsole(ATTACH_PARENT_PROCESS)){
    std::freopen("CONOUT$","w",stdout);
    std::freopen("CONOUT$","w",stderr);
    }
  }
#endif


int run(int argc,char* argv[]){
  FXint out=1;
  for(FXint i=1; i<argc; i++){
    if(std::strcmp(argv[i],"--no-dialogs")!=0) argv[out++]=argv[i];
    }
  argc=out;
  argv[argc]=nullptr;
  for(FXint i=1; i+1<argc; i++){
    if(std::strcmp(argv[i],"--check-import")==0){
#if defined(WIN32)
      attachConsole();
#endif
      FXCHART::CSVTable table;
      FXString error;
      if(!FXCHART::readCSV(argv[i+1],table,error)){
        std::fprintf(stderr,"%s\n",error.text());
        return 1;
        }
      FXint numeric=0,timestamps=0;
      for(FXuval c=0; c<table.columns.size(); c++){
        if(table.columns[c].numeric) numeric++;
        if(table.columns[c].timestamp) timestamps++;
        }
      std::printf("%d rows, %d columns, %d numeric, %d timestamp; delimiter=%s; encoding=%s\n",table.rows,(FXint)table.columns.size(),numeric,timestamps,table.delimiter.text(),table.encoding.text());
      return 0;
      }
    }
  FXApp application("ChartDemo","FOX Chart Demo");
  application.init(argc,argv);
  ChartDemoWindow* window=new ChartDemoWindow(&application);
  application.create();
  window->show(PLACEMENT_SCREEN);
  for(FXint i=1; i<argc; i++) window->openFile(argv[i]);
  return application.run();
  }

}


int main(int argc,char* argv[]){
  return FXCHARTDEMO::run(argc,argv);
  }
