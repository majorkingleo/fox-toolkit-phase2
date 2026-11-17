/*
 * Send a WM_DELETE_WINDOW message to the first window whose name contains the
 * given text, the way a window manager's close button does.
 *
 *   xclose "Table Widget Test"
 *
 * Used by the themed close gate: closing a window is the path that quits the
 * application, and quitting is where a display may no longer be asked to free
 * anything.
 */
#include <X11/Xlib.h>
#include <stdio.h>
#include <string.h>

static Window find(Display *dpy,Window win,const char *name){
  char *title=nullptr;
  Window root,parent,*kids=nullptr;
  unsigned int nkids=0;
  if(XFetchName(dpy,win,&title) && title){
    if(strstr(title,name)){ XFree(title); return win; }
    XFree(title);
    }
  if(!XQueryTree(dpy,win,&root,&parent,&kids,&nkids)) return 0;
  for(unsigned int i=0; i<nkids; i++){
    Window found=find(dpy,kids[i],name);
    if(found){ if(kids) XFree(kids); return found; }
    }
  if(kids) XFree(kids);
  return 0;
  }

int main(int argc,char**argv){
  Display *dpy;
  Window win;
  XEvent ev;
  if(argc<2){ fprintf(stderr,"usage: xclose NAME\n"); return 2; }
  if(!(dpy=XOpenDisplay(nullptr))){ fprintf(stderr,"xclose: cannot open the display\n"); return 2; }
  if(!(win=find(dpy,DefaultRootWindow(dpy),argv[1]))){
    fprintf(stderr,"xclose: no window matching \"%s\"\n",argv[1]);
    XCloseDisplay(dpy);
    return 1;
    }
  memset(&ev,0,sizeof(ev));
  ev.xclient.type=ClientMessage;
  ev.xclient.window=win;
  ev.xclient.message_type=XInternAtom(dpy,"WM_PROTOCOLS",False);
  ev.xclient.format=32;
  ev.xclient.data.l[0]=(long)XInternAtom(dpy,"WM_DELETE_WINDOW",False);
  ev.xclient.data.l[1]=CurrentTime;
  XSendEvent(dpy,win,False,NoEventMask,&ev);
  XFlush(dpy);
  XCloseDisplay(dpy);
  return 0;
  }
