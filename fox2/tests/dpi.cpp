/********************************************************************************
*                                                                               *
*                    D P I    D I A G N O S T I C S                              *
*                                                                               *
*********************************************************************************
*                                                                               *
* Reports the process DPI awareness, the system DPI, and the device bounds of   *
* a top-level window.  The window closes itself after two seconds.              *
*                                                                               *
********************************************************************************/

#include "fx.h"
#include <stdio.h>


#if defined(WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#ifndef WM_DPICHANGED
#define WM_DPICHANGED 0x02E0
#endif
typedef UINT (WINAPI *GETDPIFORWINDOWFUNC)(HWND hwnd);
#endif


// Textual name of a DPI awareness level
static const FXchar* awarenessName(FXint awareness){
  switch(awareness){
    case DPIAWARENESS_UNAWARE: return "unaware";
    case DPIAWARENESS_SYSTEM: return "system";
    case DPIAWARENESS_PER_MONITOR: return "per-monitor";
    case DPIAWARENESS_PER_MONITOR_V2: return "per-monitor-v2";
    default: break;
    }
  return "unknown";
  }


// Entry point
int main(int argc,char* argv[]){
  FXApp application("DPITest","FoxTest");
  FXMainWindow *main;
  FXbool simulate=false;
  if(argc>1 && FXString::comparecase(argv[1],"simulate")==0) simulate=true;
  application.init(argc,argv);
  main=new FXMainWindow(&application,"DPI diagnostic",nullptr,nullptr,DECOR_ALL);
  new FXLabel(main,"DPI diagnostic",nullptr,LABEL_NORMAL|LAYOUT_CENTER_X);
  application.create();
  main->show(PLACEMENT_SCREEN);

  printf("FOX DPI diagnostic\n");
  printf("  process awareness : %d (%s)\n",application.getDPIAwareness(),awarenessName(application.getDPIAwareness()));
  printf("  system DPI        : %d\n",application.getDPI());
  printf("  theme scale       : %.2f\n",application.getTheme()->getScale());
  printf("  window bounds     : %d,%d %dx%d\n",main->getX(),main->getY(),main->getWidth(),main->getHeight());
  printf("  window DPI (FOX)  : %d\n",main->getWindowDPI());
  printf("  window scale      : %.2f\n",main->getScale());

#if defined(WIN32)
  {
    GETDPIFORWINDOWFUNC getdpiforwindow=nullptr;
    HMODULE user32dll=GetModuleHandleA("user32.dll");
    HWND hwnd=(HWND)main->id();
    RECT rect;
    if(user32dll){
      getdpiforwindow=(GETDPIFORWINDOWFUNC)GetProcAddress(user32dll,"GetDpiForWindow");
      }
    printf("  window DPI        : %d\n",(getdpiforwindow && hwnd)?(FXint)getdpiforwindow(hwnd):0);
    if(hwnd && GetWindowRect(hwnd,&rect)){
      printf("  device rect       : %ld,%ld %ldx%ld\n",(long)rect.left,(long)rect.top,(long)(rect.right-rect.left),(long)(rect.bottom-rect.top));
      }
    }
#endif

  fflush(stdout);

#if defined(WIN32)
  if(simulate){
    HWND hwnd=(HWND)main->id();
    RECT rect;
    GetWindowRect(hwnd,&rect);
    rect.right=rect.left+(rect.right-rect.left)*3/2;
    rect.bottom=rect.top+(rect.bottom-rect.top)*3/2;
    SendMessage(hwnd,WM_DPICHANGED,MAKEWPARAM(144,144),(LPARAM)&rect);
    printf("  after WM_DPICHANGED (144 dpi):\n");
    printf("    theme scale       : %.2f\n",application.getTheme()->getScale());
    printf("    window DPI (FOX)  : %d\n",main->getWindowDPI());
    printf("    window bounds     : %d,%d %dx%d\n",main->getX(),main->getY(),main->getWidth(),main->getHeight());
    fflush(stdout);
    }
#endif

  // Close the window by itself so the diagnostic can run unattended
  application.addTimeout(&application,FXApp::ID_QUIT,2000000000);
  application.run();
  return 0;
  }
