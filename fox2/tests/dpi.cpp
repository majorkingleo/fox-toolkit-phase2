/********************************************************************************
*                                                                               *
*                    D P I    T E S T    A P P L I C A T I O N                   *
*                                                                               *
*********************************************************************************
*                                                                               *
* Interactive DPI test: shows the process awareness and the window DPI, and     *
* lets the user scale the whole UI with a slider (70% .. 150%) or preset        *
* buttons.  "Auto" returns to following the monitor DPI.                        *
*                                                                               *
* Run with "simulate" as argument to send a synthetic 144-DPI change and exit.  *
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

#define DPI_SCALE_MIN   70
#define DPI_SCALE_MAX   150
#define DPI_SCALE_STEP  10


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


// Message target of the interactive controls
class DPITest : public FXObject {
  FXDECLARE(DPITest)
private:
  FXApp         *app;                   // Application
  FXMainWindow  *window;                // Test window
  FXSlider      *slider;                // Scale slider
  FXLabel       *scaleLabel;            // Current scale
  FXLabel       *awarenessLabel;        // Process awareness
  FXLabel       *dpiLabel;              // System and window DPI
  FXLabel       *sizeLabel;             // Window size
  FXLabel       *clickLabel;            // Hit-testing readout
  FXint          clicks;                // Sample button clicks
private:
  DPITest(const DPITest&);
  DPITest& operator=(const DPITest&);
public:
  DPITest():app(nullptr),window(nullptr),slider(nullptr),scaleLabel(nullptr),awarenessLabel(nullptr),dpiLabel(nullptr),sizeLabel(nullptr),clickLabel(nullptr),clicks(0){}
  void setApplication(FXApp* a,FXMainWindow* w){ app=a; window=w; }
  void setWidgets(FXSlider* s,FXLabel* sl,FXLabel* aw,FXLabel* dp,FXLabel* sz,FXLabel* ck){
    slider=s; scaleLabel=sl; awarenessLabel=aw; dpiLabel=dp; sizeLabel=sz; clickLabel=ck;
    }
  void update();
public:
  enum {
    ID_SLIDER=1,                        // Scale slider
    ID_DECREASE,                        // Decrease by one step
    ID_INCREASE,                        // Increase by one step
    ID_RESET,                           // Back to 100%
    ID_AUTO,                            // Follow the monitor DPI
    ID_SAMPLE,                          // Sample button click
    ID_LAST
    };
public:
  long onScale(FXObject*,FXSelector,void*);
  long onCmdDecrease(FXObject*,FXSelector,void*);
  long onCmdIncrease(FXObject*,FXSelector,void*);
  long onCmdReset(FXObject*,FXSelector,void*);
  long onCmdAuto(FXObject*,FXSelector,void*);
  long onCmdSample(FXObject*,FXSelector,void*);
  };


FXDEFMAP(DPITest) DPITestMap[]={
  FXMAPFUNC(SEL_CHANGED,DPITest::ID_SLIDER,DPITest::onScale),
  FXMAPFUNC(SEL_COMMAND,DPITest::ID_SLIDER,DPITest::onScale),
  FXMAPFUNC(SEL_COMMAND,DPITest::ID_DECREASE,DPITest::onCmdDecrease),
  FXMAPFUNC(SEL_COMMAND,DPITest::ID_INCREASE,DPITest::onCmdIncrease),
  FXMAPFUNC(SEL_COMMAND,DPITest::ID_RESET,DPITest::onCmdReset),
  FXMAPFUNC(SEL_COMMAND,DPITest::ID_AUTO,DPITest::onCmdAuto),
  FXMAPFUNC(SEL_COMMAND,DPITest::ID_SAMPLE,DPITest::onCmdSample),
  };


FXIMPLEMENT(DPITest,FXObject,DPITestMap,ARRAYNUMBER(DPITestMap))


// Refresh the information labels
void DPITest::update(){
  FXint percent=FXint(app->getDisplayScale()*100.0+0.5);
  FXdouble scale=app->getDisplayScale();
  FXint wantw,wanth;
  awarenessLabel->setText(FXString::value("Process awareness: %s (%d)",awarenessName(app->getDPIAwareness()),app->getDPIAwareness()));
  dpiLabel->setText(FXString::value("System DPI: %d    Window DPI: %d",app->getDPI(),window->getWindowDPI()));
  scaleLabel->setText(FXString::value("%d%%",percent));

  // Keep the whole test UI visible as it scales
  if(scale>0.0){
    wantw=FXMAX(FXint(520.0*scale+0.5),window->getDefaultWidth());
    wanth=FXMAX(FXint(320.0*scale+0.5),window->getDefaultHeight());
    if(window->getWidth()!=wantw || window->getHeight()!=wanth){
      window->resize(wantw,wanth);
      }
    }

  sizeLabel->setText(FXString::value("Window size: %d x %d",window->getWidth(),window->getHeight()));
  window->recalc();
  window->update();
  }


// Slider moved
long DPITest::onScale(FXObject*,FXSelector,void* ptr){
  FXint percent=(FXint)(FXival)ptr;
  if(percent<DPI_SCALE_MIN) percent=DPI_SCALE_MIN;
  if(percent>DPI_SCALE_MAX) percent=DPI_SCALE_MAX;
  app->setDisplayScale(FXdouble(percent)/100.0);
  update();
  return 1;
  }


// Decrease by one step
long DPITest::onCmdDecrease(FXObject*,FXSelector,void*){
  slider->setValue(FXMAX(slider->getValue()-DPI_SCALE_STEP,DPI_SCALE_MIN),true);
  return 1;
  }


// Increase by one step
long DPITest::onCmdIncrease(FXObject*,FXSelector,void*){
  slider->setValue(FXMIN(slider->getValue()+DPI_SCALE_STEP,DPI_SCALE_MAX),true);
  return 1;
  }


// Back to 100%
long DPITest::onCmdReset(FXObject*,FXSelector,void*){
  slider->setValue(100,true);
  return 1;
  }


// Follow the monitor DPI again
long DPITest::onCmdAuto(FXObject*,FXSelector,void*){
  app->setDisplayScaleAuto();
  slider->setValue(FXint(app->getDisplayScale()*100.0+0.5),false);
  update();
  return 1;
  }


// Sample button clicked; proves the hit target moved and scaled with the UI
long DPITest::onCmdSample(FXObject*,FXSelector,void*){
  clicks++;
  clickLabel->setText(FXString::value("Button clicks: %d  (hit test at %d%%)",clicks,FXint(app->getDisplayScale()*100.0+0.5)));
  return 1;
  }


// Entry point
int main(int argc,char* argv[]){
  FXApp application("DPITest","FoxTest");
  FXMainWindow *main;
  FXSlider *slider;
  FXLabel *scaleLabel;
  FXLabel *awarenessLabel;
  FXLabel *dpiLabel;
  FXLabel *sizeLabel;
  FXLabel *clickLabel;
  FXVerticalFrame *content;
  FXVerticalFrame *info;
  FXHorizontalFrame *scaleRow;
  FXHorizontalFrame *buttonRow;
  FXGroupBox *sample;
  DPITest tester;
  FXbool simulate=false;
  if(argc>1 && FXString::comparecase(argv[1],"simulate")==0) simulate=true;
  application.init(argc,argv);

  main=new FXMainWindow(&application,"FOX DPI test",nullptr,nullptr,DECOR_ALL,0,0,520,320);
  content=new FXVerticalFrame(main,LAYOUT_FILL_X|LAYOUT_FILL_Y,0,0,0,0,0,0,0,0);

  new FXLabel(content,"FOX DPI test",nullptr,LABEL_NORMAL|LAYOUT_FILL_X);

  info=new FXVerticalFrame(content,LAYOUT_FILL_X|FRAME_GROOVE,0,0,0,0);
  awarenessLabel=new FXLabel(info,"",nullptr,LABEL_NORMAL|LAYOUT_FILL_X);
  dpiLabel=new FXLabel(info,"",nullptr,LABEL_NORMAL|LAYOUT_FILL_X);

  scaleRow=new FXHorizontalFrame(content,LAYOUT_FILL_X,0,0,0,0,0,0,0,0);
  new FXLabel(scaleRow,"Scale:",nullptr,LABEL_NORMAL);
  slider=new FXSlider(scaleRow,&tester,DPITest::ID_SLIDER,SLIDER_HORIZONTAL|LAYOUT_FILL_X|FRAME_SUNKEN|FRAME_THICK,0,0,0,0);
  slider->setRange(DPI_SCALE_MIN,DPI_SCALE_MAX,false);
  slider->setValue(100,false);
  slider->setHelpText("Display scale in percent");
  scaleLabel=new FXLabel(scaleRow,"100%",nullptr,LABEL_NORMAL);

  buttonRow=new FXHorizontalFrame(content,LAYOUT_FILL_X|PACK_UNIFORM_WIDTH,0,0,0,0,0,0,0,0);
  new FXButton(buttonRow,"-10%",nullptr,&tester,DPITest::ID_DECREASE,FRAME_RAISED|FRAME_THICK|BUTTON_NORMAL);
  new FXButton(buttonRow,"+10%",nullptr,&tester,DPITest::ID_INCREASE,FRAME_RAISED|FRAME_THICK|BUTTON_NORMAL);
  new FXButton(buttonRow,"100%",nullptr,&tester,DPITest::ID_RESET,FRAME_RAISED|FRAME_THICK|BUTTON_NORMAL);
  new FXButton(buttonRow,"Auto",nullptr,&tester,DPITest::ID_AUTO,FRAME_RAISED|FRAME_THICK|BUTTON_NORMAL);

  sample=new FXGroupBox(content,"Sample",GROUPBOX_TITLE_LEFT|FRAME_GROOVE|LAYOUT_FILL_X|LAYOUT_FILL_Y);
  new FXButton(sample,"Click me",nullptr,&tester,DPITest::ID_SAMPLE,BUTTON_NORMAL|LAYOUT_LEFT);
  new FXLabel(sample,"Text and padding follow the theme scale.",nullptr,LABEL_NORMAL|LAYOUT_FILL_X);

  clickLabel=new FXLabel(content,"Button clicks: 0",nullptr,LABEL_NORMAL|LAYOUT_FILL_X);
  sizeLabel=new FXLabel(content,"",nullptr,LABEL_NORMAL|LAYOUT_FILL_X);

  tester.setApplication(&application,main);
  tester.setWidgets(slider,scaleLabel,awarenessLabel,dpiLabel,sizeLabel,clickLabel);

  application.create();
  main->show(PLACEMENT_SCREEN);
  tester.update();

  printf("FOX DPI test\n");
  printf("  process awareness : %d (%s)\n",application.getDPIAwareness(),awarenessName(application.getDPIAwareness()));
  printf("  system DPI        : %d\n",application.getDPI());
  printf("  theme scale       : %.2f\n",application.getDisplayScale());
  printf("  window DPI (FOX)  : %d\n",main->getWindowDPI());

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
    tester.update();
    printf("  after WM_DPICHANGED (144 dpi):\n");
    printf("    theme scale       : %.2f\n",application.getDisplayScale());
    printf("    window DPI (FOX)  : %d\n",main->getWindowDPI());
    printf("    window bounds     : %d,%d %dx%d\n",main->getX(),main->getY(),main->getWidth(),main->getHeight());
    fflush(stdout);
    application.addTimeout(&application,FXApp::ID_QUIT,2000000000);
    }
#endif

  application.run();
  return 0;
  }
