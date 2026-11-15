/*
 * xgrab - save one X11 window as a binary PPM image.
 *
 *   xgrab 0x1200112 out.ppm
 *
 * When a compositor redirects the window (Xwayland under sway or KWin) the
 * window itself cannot be read with XGetImage; the contents live in the
 * window pixmap, so XCompositeNameWindowPixmap() is tried first and a plain
 * XGetImage is the fallback for an unredirected server.  The alpha channel
 * is dropped, which is what a screenshot wants.
 *
 * Build: make -C tools xgrab   (needs libX11 and libXcomposite)
 */
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/Xcomposite.h>
#include <stdio.h>
#include <stdlib.h>

static int xfailed=0;

static int on_error(Display*,XErrorEvent*){
  xfailed=1;
  return 0;
  }

int main(int argc,char** argv){
  if(argc!=3){
    fprintf(stderr,"usage: xgrab WINDOWID OUT.ppm\n");
    return 2;
    }
  Display *dpy=XOpenDisplay(0);
  if(!dpy){ fprintf(stderr,"xgrab: cannot open display\n"); return 1; }
  Window win=(Window)strtoul(argv[1],0,0);
  XWindowAttributes wa;
  if(!XGetWindowAttributes(dpy,win,&wa)){
    fprintf(stderr,"xgrab: no such window 0x%lx\n",win);
    return 1;
    }
  XSetErrorHandler(on_error);

  Drawable src=win;
  Pixmap pixmap=None;
  XSync(dpy,False);
  xfailed=0;
  pixmap=XCompositeNameWindowPixmap(dpy,win);
  XSync(dpy,False);
  if(pixmap && !xfailed){
    src=pixmap;
    }
  else{
    pixmap=None;
    }

  xfailed=0;
  XImage *img=XGetImage(dpy,src,0,0,wa.width,wa.height,AllPlanes,ZPixmap);
  XSync(dpy,False);
  if(!img || xfailed){
    fprintf(stderr,"xgrab: cannot read window 0x%lx\n",win);
    return 1;
    }
  FILE *fp=fopen(argv[2],"wb");
  if(!fp){ fprintf(stderr,"xgrab: cannot write %s\n",argv[2]); return 1; }
  fprintf(fp,"P6\n%d %d\n255\n",wa.width,wa.height);
  for(int y=0; y<wa.height; y++){
    for(int x=0; x<wa.width; x++){
      unsigned long pix=XGetPixel(img,x,y);
      unsigned char rgb[3];
      rgb[0]=(unsigned char)((pix&wa.visual->red_mask)>>16);
      rgb[1]=(unsigned char)((pix&wa.visual->green_mask)>>8);
      rgb[2]=(unsigned char)(pix&wa.visual->blue_mask);
      fwrite(rgb,1,3,fp);
      }
    }
  fclose(fp);
  XDestroyImage(img);
  if(pixmap) XFreePixmap(dpy,pixmap);
  XCloseDisplay(dpy);
  return 0;
  }
