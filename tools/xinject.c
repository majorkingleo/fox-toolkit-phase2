/*
 * xinject - drive an X11 application from a script.
 *
 * XSendEvent mode (the default): every event is synthesized and delivered to
 * the deepest X window at the requested point, exactly as the toolkit would
 * see it from the server.  This is the only way to drive an application on
 * KWin/Wayland, where XTest does not move the real pointer.
 *
 * XTest mode (the x... commands): the events are injected through XTest, which
 * is what a nested X server (Xwayland under sway, Xvfb) needs when the
 * application listens to the real pointer and keyboard.
 *
 * Window coordinates are relative to the toplevel with the given title; the
 * absolute coordinates of the x... commands are that toplevel's origin plus
 * the given point.
 *
 *   xinject -title TITLE id                     print the shell window id
 *   xinject -title TITLE tree                   print the window tree
 *   xinject -title TITLE focus                  synthesize FocusIn
 *   xinject -title TITLE click X Y              synthesized click (XSendEvent)
 *   xinject -title TITLE move X Y               EnterNotify and motion
 *   xinject -title TITLE type TEXT              type text (synthesized)
 *   xinject -title TITLE key KEYSYMNAME         one keysym (synthesized)
 *   xinject -title TITLE sleep SECONDS
 *   xinject -title TITLE xclick X Y             click through XTest
 *   xinject -title TITLE xmove X Y              motion through XTest
 *   xinject -title TITLE xtype TEXT             type through XTest
 *   xinject -title TITLE xkey KEYSYMNAME        one key through XTest
 *
 * Build: make -C tools xinject   (needs libX11 and libXtst)
 */
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <X11/Xatom.h>
#include <X11/XKBlib.h>
#include <X11/extensions/XTest.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static Display *dpy;

static int ignore_error(Display*,XErrorEvent*){ return 0; }

/* Find the top-level shell window with the given title */
static Window find_window(Window root,const char *title){
  Window rootret,parent,*children=0;
  unsigned int nchildren=0;
  char *name=0;
  if(XFetchName(dpy,root,&name) && name){
    int hit=strcmp(name,title)==0;
    XFree(name);
    if(hit) return root;
    }
  if(!XQueryTree(dpy,root,&rootret,&parent,&children,&nchildren)) return 0;
  for(unsigned int i=0; i<nchildren; i++){
    Window w=find_window(children[i],title);
    if(w){ if(children) XFree(children); return w; }
    }
  if(children) XFree(children);
  return 0;
  }

/* Descend to the deepest child at the coordinate, translating it along */
static Window deepest(Window win,int x,int y,int *ox,int *oy){
  for(;;){
    Window rootret,parent,*children=0;
    unsigned int nchildren=0;
    Window found=0;
    if(!XQueryTree(dpy,win,&rootret,&parent,&children,&nchildren)) break;
    for(unsigned int i=0; i<nchildren; i++){
      XWindowAttributes wa;
      if(XGetWindowAttributes(dpy,children[i],&wa) && wa.map_state==IsViewable){
        int cx=0,cy=0;
        Window child;
        XTranslateCoordinates(dpy,win,children[i],x,y,&cx,&cy,&child);
        if(cx>=0 && cy>=0 && cx<wa.width && cy<wa.height){
          found=children[i];
          x=cx; y=cy;
          break;
          }
        }
      }
    if(children) XFree(children);
    if(!found) break;
    win=found;
    }
  *ox=x; *oy=y;
  return win;
  }

/* Absolute root position of a window-relative point */
static void root_pos(Window shell,int x,int y,int *rx,int *ry){
  Window child;
  XTranslateCoordinates(dpy,shell,DefaultRootWindow(dpy),x,y,rx,ry,&child);
  }

static void send_event(Window w,XEvent *ev,long mask){
  ev->xany.display=dpy;
  ev->xany.window=w;
  ev->xany.send_event=True;
  XSendEvent(dpy,w,False,mask,ev);
  XSync(dpy,False);
  }

static void do_focus(Window shell){
  XEvent ev;
  memset(&ev,0,sizeof(ev));
  ev.xfocus.type=FocusIn;
  ev.xfocus.mode=NotifyNormal;
  ev.xfocus.detail=NotifyAncestor;
  send_event(shell,&ev,FocusChangeMask);
  }

static void do_crossing(Window shell,Window w,int x,int y,XEvent *ev,int type){
  memset(ev,0,sizeof(*ev));
  ev->xcrossing.type=type;
  ev->xcrossing.mode=NotifyNormal;
  ev->xcrossing.detail=NotifyAncestor;
  ev->xcrossing.window=w;
  ev->xcrossing.root=DefaultRootWindow(dpy);
  ev->xcrossing.subwindow=None;
  ev->xcrossing.time=CurrentTime;
  ev->xcrossing.x=x;
  ev->xcrossing.y=y;
  root_pos(w,x,y,&ev->xcrossing.x_root,&ev->xcrossing.y_root);
  ev->xcrossing.same_screen=True;
  ev->xcrossing.focus=False;
  ev->xcrossing.state=0;
  send_event(w,ev,EnterWindowMask|LeaveWindowMask);
  }

static void do_motion(Window w,int x,int y){
  XEvent ev;
  memset(&ev,0,sizeof(ev));
  ev.xmotion.type=MotionNotify;
  ev.xmotion.window=w;
  ev.xmotion.root=DefaultRootWindow(dpy);
  ev.xmotion.subwindow=None;
  ev.xmotion.time=CurrentTime;
  ev.xmotion.x=x;
  ev.xmotion.y=y;
  root_pos(w,x,y,&ev.xmotion.x_root,&ev.xmotion.y_root);
  ev.xmotion.same_screen=True;
  send_event(w,&ev,PointerMotionMask);
  }

static void do_button(Window w,int x,int y,int type,unsigned state){
  XEvent ev;
  memset(&ev,0,sizeof(ev));
  ev.xbutton.type=type;
  ev.xbutton.window=w;
  ev.xbutton.root=DefaultRootWindow(dpy);
  ev.xbutton.subwindow=None;
  ev.xbutton.time=CurrentTime;
  ev.xbutton.x=x;
  ev.xbutton.y=y;
  root_pos(w,x,y,&ev.xbutton.x_root,&ev.xbutton.y_root);
  ev.xbutton.state=state;
  ev.xbutton.button=Button1;
  ev.xbutton.same_screen=True;
  send_event(w,&ev,ButtonPressMask|ButtonReleaseMask);
  }

static void do_click(Window shell,int x,int y){
  Window target;
  int tx,ty;
  XEvent ev;
  do_focus(shell);
  target=deepest(shell,x,y,&tx,&ty);
  do_crossing(shell,target,tx,ty,&ev,EnterNotify);
  usleep(20000);
  do_motion(target,tx,ty);
  usleep(20000);
  do_button(target,tx,ty,ButtonPress,0);
  usleep(40000);
  do_button(target,tx,ty,ButtonRelease,Button1Mask);
  usleep(40000);
  }

static int key_for(KeySym ks,KeyCode *code,int *shift){
  int min,max;
  XDisplayKeycodes(dpy,&min,&max);
  for(int kc=min; kc<=max; kc++){
    for(int level=0; level<2; level++){
      if(XkbKeycodeToKeysym(dpy,(KeyCode)kc,0,level)==ks){
        *code=(KeyCode)kc;
        *shift=level;
        return 1;
        }
      }
    }
  return 0;
  }

static void do_tap_level(Window shell,KeySym ks,int level){
  KeyCode code;
  int shift;
  XEvent ev;
  if(!key_for(ks,&code,&shift)){
    fprintf(stderr,"xinject: no keycode for keysym 0x%lx\n",(unsigned long)ks);
    return;
    }
  shift=level;
  memset(&ev,0,sizeof(ev));
  ev.xkey.keycode=code;
  ev.xkey.root=DefaultRootWindow(dpy);
  ev.xkey.subwindow=None;
  ev.xkey.time=CurrentTime;
  ev.xkey.same_screen=True;
  ev.xkey.type=KeyPress;
  ev.xkey.state=shift?ShiftMask:0;
  send_event(shell,&ev,KeyPressMask);
  usleep(20000);
  ev.xkey.type=KeyRelease;
  ev.xkey.state=shift?ShiftMask:0;
  send_event(shell,&ev,KeyReleaseMask);
  usleep(20000);
  }

static void do_tap(Window shell,KeySym ks){
  KeyCode code;
  int shift=0;
  if(!key_for(ks,&code,&shift)){
    fprintf(stderr,"xinject: no keycode for keysym 0x%lx\n",(unsigned long)ks);
    return;
    }
  do_tap_level(shell,ks,shift);
  }

static KeySym char_keysym(char c){
  switch(c){
    case ' ':  return XK_space;
    case '\n': return XK_Return;
    case '\t': return XK_Tab;
    case '.':  return XK_period;
    case ',':  return XK_comma;
    case '+':  return XK_plus;
    case '-':  return XK_minus;
    case '*':  return XK_asterisk;
    case '/':  return XK_slash;
    case '=':  return XK_equal;
    case '(':  return XK_parenleft;
    case ')':  return XK_parenright;
    case '%':  return XK_percent;
    case '&':  return XK_ampersand;
    case '|':  return XK_bar;
    case '^':  return XK_asciicircum;
    case '~':  return XK_asciitilde;
    case '!':  return XK_exclam;
    case '<':  return XK_less;
    case '>':  return XK_greater;
    default:
      if(c>='0' && c<='9') return XK_0+(c-'0');
      if(c>='a' && c<='z') return XK_a+(c-'a');
      if(c>='A' && c<='Z') return XK_a+(c-'A');
      return NoSymbol;
    }
  }

static void print_tree(Window w,int depth){
  Window rootret,parent,*children=0;
  unsigned int nchildren=0;
  char *name=0;
  XWindowAttributes wa;
  if(!XGetWindowAttributes(dpy,w,&wa)) return;
  XFetchName(dpy,w,&name);
  printf("%*s0x%lx %dx%d+%d+%d %s\n",depth*2,"",w,wa.width,wa.height,wa.x,wa.y,name?name:"(none)");
  if(name) XFree(name);
  if(!XQueryTree(dpy,w,&rootret,&parent,&children,&nchildren)) return;
  for(unsigned int i=0; i<nchildren; i++) print_tree(children[i],depth+1);
  if(children) XFree(children);
  }

static void xtap_key(KeySym ks,int level){
  KeyCode code;
  int shift;
  if(!key_for(ks,&code,&shift)){
    fprintf(stderr,"xinject: no keycode for keysym 0x%lx\n",(unsigned long)ks);
    return;
    }
  shift=level;
  KeyCode shiftcode=XKeysymToKeycode(dpy,XK_Shift_L);
  if(shift) XTestFakeKeyEvent(dpy,shiftcode,True,CurrentTime);
  XTestFakeKeyEvent(dpy,code,True,CurrentTime);
  XTestFakeKeyEvent(dpy,code,False,CurrentTime);
  if(shift) XTestFakeKeyEvent(dpy,shiftcode,False,CurrentTime);
  XSync(dpy,False);
  usleep(20000);
  }

static void xtype_text(const char* s){
  for(; *s; s++){
    KeySym ks=char_keysym(*s);
    if(ks==NoSymbol){ fprintf(stderr,"xinject: no keysym for '%c'\n",*s); continue; }
    xtap_key(ks,(*s>='A' && *s<='Z')?1:0);
    }
  }

int main(int argc,char **argv){
  const char *title=0;
  int i=1;
  if(argc>2 && strcmp(argv[1],"-title")==0){ title=argv[2]; i=3; }
  if(i>=argc){ fprintf(stderr,"usage: xinject [-title TITLE] CMD ...\n"); return 2; }
  dpy=XOpenDisplay(0);
  if(!dpy){ fprintf(stderr,"xinject: cannot open display\n"); return 1; }
  XSetErrorHandler(ignore_error);
  Window win=title? find_window(DefaultRootWindow(dpy),title) : DefaultRootWindow(dpy);
  if(!win){ fprintf(stderr,"xinject: no window titled '%s'\n",title); return 1; }
  Window child;
  int wx=0,wy=0;
  if(title) XTranslateCoordinates(dpy,win,DefaultRootWindow(dpy),0,0,&wx,&wy,&child);
  const char *cmd=argv[i++];
  if(strcmp(cmd,"id")==0){ printf("0x%lx\n",win); return 0; }
  if(strcmp(cmd,"tree")==0){ print_tree(win,0); return 0; }
  if(strcmp(cmd,"focus")==0){ do_focus(win); return 0; }
  if(strcmp(cmd,"sleep")==0){ usleep((useconds_t)(atof(argv[i++])*1000000.0)); return 0; }
  if(strcmp(cmd,"click")==0){
    int x=atoi(argv[i++]),y=atoi(argv[i++]);
    do_click(win,x,y);
    return 0;
    }
  if(strcmp(cmd,"move")==0){
    int x=atoi(argv[i++]),y=atoi(argv[i++]);
    Window target; int tx,ty; XEvent ev;
    do_focus(win);
    target=deepest(win,x,y,&tx,&ty);
    do_crossing(win,target,tx,ty,&ev,EnterNotify);
    do_motion(target,tx,ty);
    return 0;
    }
  if(strcmp(cmd,"xclick")==0 || strcmp(cmd,"xmove")==0){
    int x=atoi(argv[i++]),y=atoi(argv[i++]);
    XTestFakeMotionEvent(dpy,-1,wx+x,wy+y,CurrentTime);
    XSync(dpy,False);
    usleep(60000);
    if(strcmp(cmd,"xclick")==0){
      XTestFakeButtonEvent(dpy,1,True,CurrentTime);
      XSync(dpy,False);
      usleep(30000);
      XTestFakeButtonEvent(dpy,1,False,CurrentTime);
      XSync(dpy,False);
      }
    return 0;
    }
  if(strcmp(cmd,"xtype")==0){
    xtype_text(argv[i++]);
    return 0;
    }
  if(strcmp(cmd,"xkey")==0){
    KeySym ks=XStringToKeysym(argv[i++]);
    if(ks==NoSymbol){ fprintf(stderr,"xinject: bad keysym\n"); return 1; }
    xtap_key(ks,0);
    return 0;
    }
  if(strcmp(cmd,"key")==0){
    KeySym ks=XStringToKeysym(argv[i++]);
    if(ks==NoSymbol){ fprintf(stderr,"xinject: bad keysym\n"); return 1; }
    do_focus(win);
    do_tap(win,ks);
    return 0;
    }
  if(strcmp(cmd,"type")==0){
    const char *s=argv[i++];
    do_focus(win);
    for(; *s; s++){
      KeySym ks=char_keysym(*s);
      if(ks==NoSymbol){ fprintf(stderr,"xinject: no keysym for '%c'\n",*s); continue; }
      if(*s>='A' && *s<='Z') do_tap_level(win,ks,1);
      else do_tap(win,ks);
      }
    return 0;
    }
  fprintf(stderr,"xinject: unknown command '%s'\n",cmd);
  return 2;
  }
