#include "fx.h"
#include "fxkeys.h"
#include "StopWatch.h"


int main(int argc,char* argv[]){
  if(fxversion[0]!=FOX_MAJOR || fxversion[1]!=FOX_MINOR || fxversion[2]!=FOX_LEVEL){
    fxerror("FOX Library mismatch; expected version: %d.%d.%d, but found version: %d.%d.%d.\n",FOX_MAJOR,FOX_MINOR,FOX_LEVEL,fxversion[0],fxversion[1],fxversion[2]);
    }

  FXApp application("Stopwatch");
  application.init(argc,argv);
  new StopWatch(&application);
  new FXToolTip(&application,0);
  application.create();
  return application.run();
  }
