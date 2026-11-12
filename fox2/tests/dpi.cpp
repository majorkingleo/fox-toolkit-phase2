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
* Every DPI, monitor, position and size change is appended to a log file, so    *
* a session on a multi-monitor machine can be reviewed afterwards.  The log     *
* name defaults to dpi-log.txt in the current directory; set FOX_DPI_LOG to      *
* write elsewhere.  "Snapshot" writes the current state on demand.              *
*                                                                               *
* Run with "simulate" as argument to send a synthetic 144-DPI change and exit.  *
*                                                                               *
********************************************************************************/

#include "fx.h"
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>


#if defined(WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#ifndef WM_DPICHANGED
#define WM_DPICHANGED 0x02E0
#endif
typedef UINT (WINAPI *GETDPIFORWINDOWFUNC)(HWND hwnd);
typedef HRESULT (WINAPI *GETDPIFORMONITORFUNC)(HMONITOR monitor,UINT type,UINT* dpix,UINT* dpiy);
#define DPI_MAX_MONITORS 16
#endif

#define DPI_SCALE_MIN   70
#define DPI_SCALE_MAX   150
#define DPI_SCALE_STEP  10

// How often the window monitor, DPI, position and size are sampled
#define DPI_POLL_PERIOD 250
#define DPI_POLL_INTERVAL (FXTime(DPI_POLL_PERIOD)*1000000)


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


#if defined(WIN32)

// One display monitor as reported by the window system
struct DPIMonitor {
  HMONITOR handle;
  RECT     rect;
  RECT     work;
  FXuint   flags;
  FXint    dpi;
  FXchar   name[64];
  };

// The monitors of this machine
struct DPIMonitorList {
  FXint     count;
  DPIMonitor monitors[DPI_MAX_MONITORS];
  };


// Locate GetDpiForMonitor once; it is not in the MinGW import libraries
static GETDPIFORMONITORFUNC dpiGetDpiForMonitorFunc(){
  static GETDPIFORMONITORFUNC func=nullptr;
  static FXbool looked=false;
  HMODULE shcore;
  if(!looked){
    looked=true;
    shcore=LoadLibraryA("shcore.dll");
    if(shcore) func=(GETDPIFORMONITORFUNC)GetProcAddress(shcore,"GetDpiForMonitor");
    }
  return func;
  }


// Locate GetDpiForWindow once
static GETDPIFORWINDOWFUNC dpiGetDpiForWindowFunc(){
  static GETDPIFORWINDOWFUNC func=nullptr;
  static FXbool looked=false;
  HMODULE user32;
  if(!looked){
    looked=true;
    user32=GetModuleHandleA("user32.dll");
    if(user32) func=(GETDPIFORWINDOWFUNC)GetProcAddress(user32,"GetDpiForWindow");
    }
  return func;
  }


// Native DPI of a window, or 0 when the API is unavailable
static FXint dpiWindowDPI(HWND hwnd){
  GETDPIFORWINDOWFUNC func=dpiGetDpiForWindowFunc();
  return (func && hwnd)?(FXint)func(hwnd):0;
  }


// Collect one monitor
static BOOL CALLBACK dpiMonitorProc(HMONITOR monitor,HDC, LPRECT,LPARAM param){
  DPIMonitorList* list=(DPIMonitorList*)param;
  MONITORINFOEXA info;
  GETDPIFORMONITORFUNC getdpi;
  DPIMonitor* rec;
  UINT dpix=0;
  UINT dpiy=0;
  if(!list || list->count>=DPI_MAX_MONITORS) return TRUE;
  memset(&info,0,sizeof(info));
  info.cbSize=sizeof(info);
  if(!GetMonitorInfoA(monitor,&info)) return TRUE;
  rec=&list->monitors[list->count++];
  rec->handle=monitor;
  rec->rect=info.rcMonitor;
  rec->work=info.rcWork;
  rec->flags=info.dwFlags;
  rec->dpi=0;
  memset(rec->name,0,sizeof(rec->name));
  strncpy(rec->name,info.szDevice,sizeof(rec->name)-1);
  getdpi=dpiGetDpiForMonitorFunc();
  if(getdpi && SUCCEEDED(getdpi(monitor,0,&dpix,&dpiy))) rec->dpi=(FXint)dpix;
  return TRUE;
  }


// Enumerate the monitors
static void dpiEnumMonitors(DPIMonitorList& list){
  list.count=0;
  EnumDisplayMonitors(nullptr,nullptr,dpiMonitorProc,(LPARAM)&list);
  }


// Index of the monitor a window is on, -1 when unknown
static FXint dpiMonitorIndex(const DPIMonitorList& list,HWND hwnd){
  HMONITOR monitor=MonitorFromWindow(hwnd,MONITOR_DEFAULTTONEAREST);
  FXint i;
  for(i=0; i<list.count; i++){
    if(list.monitors[i].handle==monitor) return i;
    }
  return -1;
  }


// Textual geometry of the monitor layout, used to detect changes
static FXString dpiLayoutSignature(const DPIMonitorList& list){
  FXString result;
  FXint i;
  for(i=0; i<list.count; i++){
    result+=FXString::value("%s %s dpi=%d rect=%ld,%ld %ldx%ld\n",
      list.monitors[i].name,list.monitors[i].flags&MONITORINFOF_PRIMARY?"primary":"secondary",
      list.monitors[i].dpi,
      (long)list.monitors[i].rect.left,(long)list.monitors[i].rect.top,
      (long)(list.monitors[i].rect.right-list.monitors[i].rect.left),
      (long)(list.monitors[i].rect.bottom-list.monitors[i].rect.top));
    }
  return result;
  }

#endif


// Append-only session log; every line is flushed so a crash keeps the data
class DPILog {
private:
  FILE*    file;                        // Open log file
  FXString filename;                    // Path of the log file
  FXTime   started;                     // Session start time
private:
  DPILog(const DPILog&);
  DPILog& operator=(const DPILog&);
public:
  DPILog():file(nullptr),started(FXThread::time()){}
  ~DPILog(){ close(); }
  FXbool isOpen() const { return file!=nullptr; }
  const FXString& getFilename() const { return filename; }
  FXbool open(const FXString& path);
  void close();
  void line(const FXchar* format,...);
  void separator(const FXchar* title);
  };


// Open the log; existing content is kept so several sessions can be compared
FXbool DPILog::open(const FXString& path){
  close();
  filename=path;
  file=fopen(filename.text(),"at");
  started=FXThread::time();
  return file!=nullptr;
  }


// Close the log
void DPILog::close(){
  if(file){
    fclose(file);
    file=nullptr;
    }
  }


// Write one timestamped line
void DPILog::line(const FXchar* format,...){
  FXString message;
  FXTime now=FXThread::time();
  time_t walltime=time(nullptr);
  struct tm stamp;
  va_list arguments;
  if(!file) return;
  va_start(arguments,format);
  message.vformat(format,arguments);
  va_end(arguments);
#if defined(WIN32)
  localtime_s(&stamp,&walltime);
#else
  localtime_r(&walltime,&stamp);
#endif
  fprintf(file,"%04d-%02d-%02d %02d:%02d:%02d.%03d +%8lldms | %s\n",
    stamp.tm_year+1900,stamp.tm_mon+1,stamp.tm_mday,stamp.tm_hour,stamp.tm_min,stamp.tm_sec,
    (FXint)((now%1000000000)/1000000),(long long)((now-started)/1000000),message.text());
  fflush(file);
  }


// Write a rule with a title
void DPILog::separator(const FXchar* title){
  line("==================== %s ====================",title);
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
  FXLabel       *logLabel;              // Log file readout
  FXint          clicks;                // Sample button clicks
  DPILog        *log;                   // Session log
  FXString       laststate;             // Last logged window state
  FXString       pendingstate;          // State waiting to settle
  FXString       lastlayout;            // Last logged monitor layout
  FXint          lastdpi;               // Last logged window DPI
private:
  DPITest(const DPITest&);
  DPITest& operator=(const DPITest&);
public:
  DPITest():app(nullptr),window(nullptr),slider(nullptr),scaleLabel(nullptr),awarenessLabel(nullptr),dpiLabel(nullptr),sizeLabel(nullptr),clickLabel(nullptr),logLabel(nullptr),clicks(0),log(nullptr),lastdpi(0){}
  void setApplication(FXApp* a,FXMainWindow* w){ app=a; window=w; }
  void setWidgets(FXSlider* s,FXLabel* sl,FXLabel* aw,FXLabel* dp,FXLabel* sz,FXLabel* ck){
    slider=s; scaleLabel=sl; awarenessLabel=aw; dpiLabel=dp; sizeLabel=sz; clickLabel=ck;
    }
  void setLog(DPILog* l,FXLabel* ll){ log=l; logLabel=ll; }
  void updateLabels();
  void update();
  void snapshot(const FXchar* reason);
  void poll();
public:
  enum {
    ID_SLIDER=1,                        // Scale slider
    ID_DECREASE,                        // Decrease by one step
    ID_INCREASE,                        // Increase by one step
    ID_RESET,                           // Back to 100%
    ID_AUTO,                            // Follow the monitor DPI
    ID_SAMPLE,                          // Sample button click
    ID_SNAPSHOT,                        // Write the state to the log
    ID_POLL,                            // Periodic state sampling
    ID_LAST
    };
public:
  long onScale(FXObject*,FXSelector,void*);
  long onCmdDecrease(FXObject*,FXSelector,void*);
  long onCmdIncrease(FXObject*,FXSelector,void*);
  long onCmdReset(FXObject*,FXSelector,void*);
  long onCmdAuto(FXObject*,FXSelector,void*);
  long onCmdSample(FXObject*,FXSelector,void*);
  long onCmdSnapshot(FXObject*,FXSelector,void*);
  long onTimeout(FXObject*,FXSelector,void*);
  };


FXDEFMAP(DPITest) DPITestMap[]={
  FXMAPFUNC(SEL_CHANGED,DPITest::ID_SLIDER,DPITest::onScale),
  FXMAPFUNC(SEL_COMMAND,DPITest::ID_SLIDER,DPITest::onScale),
  FXMAPFUNC(SEL_COMMAND,DPITest::ID_DECREASE,DPITest::onCmdDecrease),
  FXMAPFUNC(SEL_COMMAND,DPITest::ID_INCREASE,DPITest::onCmdIncrease),
  FXMAPFUNC(SEL_COMMAND,DPITest::ID_RESET,DPITest::onCmdReset),
  FXMAPFUNC(SEL_COMMAND,DPITest::ID_AUTO,DPITest::onCmdAuto),
  FXMAPFUNC(SEL_COMMAND,DPITest::ID_SAMPLE,DPITest::onCmdSample),
  FXMAPFUNC(SEL_COMMAND,DPITest::ID_SNAPSHOT,DPITest::onCmdSnapshot),
  FXMAPFUNC(SEL_TIMEOUT,DPITest::ID_POLL,DPITest::onTimeout),
  };


FXIMPLEMENT(DPITest,FXObject,DPITestMap,ARRAYNUMBER(DPITestMap))


// Refresh the information labels
void DPITest::updateLabels(){
  FXint percent=FXint(app->getDisplayScale()*100.0+0.5);
  awarenessLabel->setText(FXString::value("Process awareness: %s (%d)",awarenessName(app->getDPIAwareness()),app->getDPIAwareness()));
  dpiLabel->setText(FXString::value("System DPI: %d    Window DPI: %d",app->getDPI(),window->getWindowDPI()));
  scaleLabel->setText(FXString::value("%d%%",percent));
  sizeLabel->setText(FXString::value("Window size: %d x %d",window->getWidth(),window->getHeight()));
  }


// Refresh the labels and keep the whole test UI visible as it scales
void DPITest::update(){
  FXdouble scale=app->getDisplayScale();
  FXint wantw,wanth;
  updateLabels();

  // Keep the whole test UI visible as it scales
  if(scale>0.0){
    wantw=FXMAX(FXint(520.0*scale+0.5),window->getDefaultWidth());
    wanth=FXMAX(FXint(320.0*scale+0.5),window->getDefaultHeight());
    if(window->getWidth()!=wantw || window->getHeight()!=wanth){
      window->resize(wantw,wanth);
      }
    }

  window->recalc();
  window->update();
  }


// Write the current DPI and monitor state to the log
void DPITest::snapshot(const FXchar* reason){
  FXint percent=FXint(app->getDisplayScale()*100.0+0.5);
  if(!log || !log->isOpen()) return;
  log->separator(reason);
  log->line("awareness=%s(%d) system_dpi=%d",awarenessName(app->getDPIAwareness()),app->getDPIAwareness(),app->getDPI());
  log->line("theme_scale=%.3f (%d%%) auto=%s",app->getDisplayScale(),percent,app->getDisplayScaleAuto()?"yes":"no");
  log->line("window_dpi_fox=%d window_scale=%.3f",window->getWindowDPI(),window->getScale());
  log->line("window_pos=%d,%d window_size=%dx%d",window->getX(),window->getY(),window->getWidth(),window->getHeight());
  lastdpi=window->getWindowDPI();
#if defined(WIN32)
  {
    DPIMonitorList list;
    HWND hwnd=(HWND)window->id();
    RECT frame;
    FXint index;
    FXint i;
    dpiEnumMonitors(list);
    if(hwnd && GetWindowRect(hwnd,&frame)){
      log->line("window_dpi_native=%d frame=%ld,%ld %ldx%ld",dpiWindowDPI(hwnd),
        (long)frame.left,(long)frame.top,(long)(frame.right-frame.left),(long)(frame.bottom-frame.top));
      }
    else{
      log->line("window_dpi_native=0 frame=unknown");
      }
    log->line("monitors=%d",list.count);
    for(i=0; i<list.count; i++){
      log->line("  monitor[%d] name=%s %s dpi=%d rect=%ld,%ld %ldx%ld work=%ld,%ld %ldx%ld",
        i,list.monitors[i].name,list.monitors[i].flags&MONITORINFOF_PRIMARY?"primary":"secondary",
        list.monitors[i].dpi,
        (long)list.monitors[i].rect.left,(long)list.monitors[i].rect.top,
        (long)(list.monitors[i].rect.right-list.monitors[i].rect.left),
        (long)(list.monitors[i].rect.bottom-list.monitors[i].rect.top),
        (long)list.monitors[i].work.left,(long)list.monitors[i].work.top,
        (long)(list.monitors[i].work.right-list.monitors[i].work.left),
        (long)(list.monitors[i].work.bottom-list.monitors[i].work.top));
      }
    index=hwnd?dpiMonitorIndex(list,hwnd):-1;
    if(0<=index && index<list.count){
      log->line("  window_monitor=%d name=%s dpi=%d",index,list.monitors[index].name,list.monitors[index].dpi);
      }
    else{
      log->line("  window_monitor=unknown");
      }
  }
#endif
  }


// Sample the window once and log the state after it held still
void DPITest::poll(){
#if defined(WIN32)
  DPIMonitorList list;
  HWND hwnd=(HWND)window->id();
  FXString state;
  FXString layout;
  RECT frame;
  FXint index;
  if(!hwnd || !log || !log->isOpen()) return;
  dpiEnumMonitors(list);
  layout=dpiLayoutSignature(list);
  index=dpiMonitorIndex(list,hwnd);
  GetWindowRect(hwnd,&frame);
  state=FXString::value("%d|%d|%d|%d|%ld|%ld|%ld|%ld|%d",
    window->getWindowDPI(),window->getX(),window->getY(),window->getWidth(),window->getHeight(),
    (long)frame.left,(long)frame.top,(long)(frame.right-frame.left),(long)(frame.bottom-frame.top),index);
  if(lastlayout.empty()){
    lastlayout=layout;
    }
  else if(layout!=lastlayout){
    lastlayout=layout;
    log->line("monitor layout changed");
    snapshot("monitor layout");
    }
  if(state==laststate) return;
  if(state!=pendingstate){
    pendingstate=state;
    return;
    }
  if(lastdpi && lastdpi!=window->getWindowDPI()){
    log->line("window DPI changed %d -> %d",lastdpi,window->getWindowDPI());
    }
  laststate=state;
  lastdpi=window->getWindowDPI();
  snapshot("window moved, resized or changed DPI");
  updateLabels();
#endif
  }


// Periodic state sampling
long DPITest::onTimeout(FXObject*,FXSelector,void*){
  poll();
  app->addTimeout(this,ID_POLL,DPI_POLL_INTERVAL);
  return 1;
  }


// Write a snapshot on demand
long DPITest::onCmdSnapshot(FXObject*,FXSelector,void*){
  snapshot("manual snapshot");
  return 1;
  }


// Slider moved
long DPITest::onScale(FXObject*,FXSelector,void* ptr){
  FXint percent=(FXint)(FXival)ptr;
  if(percent<DPI_SCALE_MIN) percent=DPI_SCALE_MIN;
  if(percent>DPI_SCALE_MAX) percent=DPI_SCALE_MAX;
  app->setDisplayScale(FXdouble(percent)/100.0);
  if(log) log->line("user: display scale %d%%",percent);
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
  if(log) log->line("user: display scale auto (system DPI %d)",app->getDPI());
  update();
  return 1;
  }


// Sample button clicked; proves the hit target moved and scaled with the UI
long DPITest::onCmdSample(FXObject*,FXSelector,void*){
  clicks++;
  clickLabel->setText(FXString::value("Button clicks: %d  (hit test at %d%%)",clicks,FXint(app->getDisplayScale()*100.0+0.5)));
  if(log) log->line("user: sample click %d at %d%%, window dpi %d",clicks,FXint(app->getDisplayScale()*100.0+0.5),window->getWindowDPI());
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
  FXLabel *logLabel;
  FXVerticalFrame *content;
  FXVerticalFrame *info;
  FXHorizontalFrame *scaleRow;
  FXHorizontalFrame *buttonRow;
  FXGroupBox *sample;
  DPITest tester;
  DPILog log;
  FXString logpath;
  FXbool simulate=false;
  FXint status;
  const FXchar* logname;
  if(argc>1 && FXString::comparecase(argv[1],"simulate")==0) simulate=true;
  application.init(argc,argv);

  // Session log; FOX_DPI_LOG overrides the default file name
  logname=getenv("FOX_DPI_LOG");
  logpath=(logname && logname[0])?FXString(logname):FXString("dpi-log.txt");
  log.open(logpath);

  main=new FXMainWindow(&application,"FOX DPI test",nullptr,nullptr,DECOR_ALL,0,0,520,320);
  content=new FXVerticalFrame(main,LAYOUT_FILL_X|LAYOUT_FILL_Y,0,0,0,0,0,0,0,0);

  new FXLabel(content,"FOX DPI test",nullptr,LABEL_NORMAL|LAYOUT_FILL_X);

  info=new FXVerticalFrame(content,LAYOUT_FILL_X|FRAME_GROOVE,0,0,0,0);
  awarenessLabel=new FXLabel(info,"",nullptr,LABEL_NORMAL|LAYOUT_FILL_X);
  dpiLabel=new FXLabel(info,"",nullptr,LABEL_NORMAL|LAYOUT_FILL_X);
  logLabel=new FXLabel(info,"",nullptr,LABEL_NORMAL|LAYOUT_FILL_X);

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
  new FXButton(buttonRow,"Snapshot",nullptr,&tester,DPITest::ID_SNAPSHOT,FRAME_RAISED|FRAME_THICK|BUTTON_NORMAL);

  sample=new FXGroupBox(content,"Sample",GROUPBOX_TITLE_LEFT|FRAME_GROOVE|LAYOUT_FILL_X|LAYOUT_FILL_Y);
  new FXButton(sample,"Click me",nullptr,&tester,DPITest::ID_SAMPLE,BUTTON_NORMAL|LAYOUT_LEFT);
  new FXLabel(sample,"Text and padding follow the theme scale.",nullptr,LABEL_NORMAL|LAYOUT_FILL_X);

  clickLabel=new FXLabel(content,"Button clicks: 0",nullptr,LABEL_NORMAL|LAYOUT_FILL_X);
  sizeLabel=new FXLabel(content,"",nullptr,LABEL_NORMAL|LAYOUT_FILL_X);

  tester.setApplication(&application,main);
  tester.setWidgets(slider,scaleLabel,awarenessLabel,dpiLabel,sizeLabel,clickLabel);
  tester.setLog(&log,logLabel);

  application.create();
  main->show(PLACEMENT_SCREEN);
  tester.update();

  // Session header, then the first complete state
  log.separator("session start");
  log.line("cwd=%s",FXSystem::getCurrentDirectory().text());
  log.line("mode=%s trace_env=%s",simulate?"simulate":"interactive",
    getenv("FOX_TRACE_LEVEL")?getenv("FOX_TRACE_LEVEL"):"unset");
  tester.snapshot("startup");
  logLabel->setText(FXString::value("Log: %s",FXPath::name(log.getFilename()).text()));
  logLabel->setHelpText(log.getFilename());

  printf("FOX DPI test\n");
  printf("  process awareness : %d (%s)\n",application.getDPIAwareness(),awarenessName(application.getDPIAwareness()));
  printf("  system DPI        : %d\n",application.getDPI());
  printf("  theme scale       : %.2f\n",application.getDisplayScale());
  printf("  window DPI (FOX)  : %d\n",main->getWindowDPI());
  printf("  log file          : %s\n",log.isOpen()?log.getFilename().text():"(could not be opened)");

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
    tester.snapshot("after WM_DPICHANGED 144");
    printf("  after WM_DPICHANGED (144 dpi):\n");
    printf("    theme scale       : %.2f\n",application.getDisplayScale());
    printf("    window DPI (FOX)  : %d\n",main->getWindowDPI());
    printf("    window bounds     : %d,%d %dx%d\n",main->getX(),main->getY(),main->getWidth(),main->getHeight());
    fflush(stdout);

    // Round trip: move back to the system DPI
    GetWindowRect(hwnd,&rect);
    rect.right=rect.left+(rect.right-rect.left)*2/3;
    rect.bottom=rect.top+(rect.bottom-rect.top)*2/3;
    SendMessage(hwnd,WM_DPICHANGED,MAKEWPARAM(application.getDPI(),application.getDPI()),(LPARAM)&rect);
    tester.update();
    tester.snapshot("after WM_DPICHANGED back to system DPI");
    printf("  after WM_DPICHANGED (%d dpi):\n",application.getDPI());
    printf("    theme scale       : %.2f\n",application.getDisplayScale());
    printf("    window DPI (FOX)  : %d\n",main->getWindowDPI());
    printf("    window bounds     : %d,%d %dx%d\n",main->getX(),main->getY(),main->getWidth(),main->getHeight());
    fflush(stdout);
    application.addTimeout(&application,FXApp::ID_QUIT,2000000000);
    }
#endif

  // Start sampling the window state for the log
  if(!simulate) application.addTimeout(&tester,DPITest::ID_POLL,DPI_POLL_INTERVAL);

  status=application.run();
  log.separator("session end");
  log.line("run() returned %d",status);
  return status;
  }
