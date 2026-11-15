/*
 * xlist - list the X11 window tree, with what a test script needs to know.
 *
 *   xlist                       mapped windows, with title and absolute place
 *   xlist -all                  every window, mapped or not
 *   xlist -title "FOX Calculator"   only the subtree of that window
 *   xlist -geom -title TITLE    print "ID X Y W H" of that window
 *   xlist -tree                 indented tree (the default)
 *
 * The position is in root coordinates and the size in pixels, so a script can
 * add a widget-relative point to it and hand the result to an input injector.
 * A title is matched with XFetchName, which is what the toolkit sets.
 *
 * Build: make -C tools xlist   (needs libX11)
 */
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static Display *dpy;
static int all=0;
static const char *want=0;

/* Find the window with the given title, depth first */
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

/* Print one window and, when asked, its subtree */
static void walk(Window w,int depth){
  Window rootret,parent,*children=0;
  unsigned int nchildren=0;
  XWindowAttributes wa;
  char *name=0;
  int ax=0,ay=0;
  Window child;
  if(!XGetWindowAttributes(dpy,w,&wa)) return;
  if(all || wa.map_state==IsViewable){
    XTranslateCoordinates(dpy,w,DefaultRootWindow(dpy),0,0,&ax,&ay,&child);
    XFetchName(dpy,w,&name);
    printf("%*s0x%lx %dx%d+%d+%d map=%d depth=%d %s\n",
           depth*2,"",w,wa.width,wa.height,ax,ay,(int)wa.map_state,wa.depth,
           name?name:"-");
    if(name) XFree(name);
    }
  if(!XQueryTree(dpy,w,&rootret,&parent,&children,&nchildren)) return;
  for(unsigned int i=0; i<nchildren; i++) walk(children[i],depth+1);
  if(children) XFree(children);
  }

int main(int argc,char** argv){
  int geom=0;
  int i;
  Window win;
  for(i=1; i<argc; i++){
    if(strcmp(argv[i],"-all")==0) all=1;
    else if(strcmp(argv[i],"-geom")==0) geom=1;
    else if(strcmp(argv[i],"-title")==0 && i+1<argc) want=argv[++i];
    else if(strncmp(argv[i],"-title=",7)==0) want=argv[i]+7;
    else{
      fprintf(stderr,"usage: xlist [-all] [-geom] [-title TITLE]\n");
      return 2;
      }
    }
  dpy=XOpenDisplay(0);
  if(!dpy){ fprintf(stderr,"xlist: cannot open display\n"); return 1; }
  win=want? find_window(DefaultRootWindow(dpy),want):DefaultRootWindow(dpy);
  if(!win){
    fprintf(stderr,"xlist: no window titled '%s'\n",want);
    return 1;
    }
  if(geom){
    XWindowAttributes wa;
    int ax=0,ay=0;
    Window child;
    if(!XGetWindowAttributes(dpy,win,&wa)) return 1;
    XTranslateCoordinates(dpy,win,DefaultRootWindow(dpy),0,0,&ax,&ay,&child);
    printf("0x%lx %d %d %d %d\n",win,ax,ay,wa.width,wa.height);
    }
  else{
    walk(win,0);
    }
  XCloseDisplay(dpy);
  return 0;
  }
