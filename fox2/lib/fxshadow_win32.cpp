/********************************************************************************
*                                                                               *
*                          Shadow   ( W i n 3 2 )                               *
*                                                                               *
*********************************************************************************
* Copyright (C) 2026 by Jeroen van der Zijp.   All Rights Reserved.             *
*********************************************************************************
* This library is free software; you can redistribute it and/or modify          *
* it under the terms of the GNU Lesser General Public License as published by   *
* the Free Software Foundation; either version 3 of the License, or             *
* (at your option) any later version.                                           *
*                                                                               *
* This library is distributed in the hope that it will be useful,               *
* but WITHOUT ANY WARRANTY; without even the implied warranty of                *
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the                 *
* GNU Lesser General Public License for more details.                           *
*                                                                               *
* You should have received a copy of the GNU Lesser General Public License      *
* along with this program.  If not, see <http://www.gnu.org/licenses/>          *
********************************************************************************/
#include "xincs.h"
#include "fxver.h"
#include "fxdefs.h"
#include "fxmath.h"
#include "FXElement.h"
#include "FXArray.h"
#include "FXHash.h"
#include "FXMutex.h"
#include "FXSize.h"
#include "FXPoint.h"
#include "FXRectangle.h"
#include "FXString.h"
#include "FXStringDictionary.h"
#include "FXSettings.h"
#include "FXRegistry.h"
#include "FXEvent.h"
#include "FXRegion.h"
#include "FXImage.h"
#include "FXBitmap.h"
#include "FXIcon.h"
#include "FXDC.h"
#include "FXWindow.h"
#include "FXApp.h"
#include "fxraster.h"
#include "fxpriv.h"

#if defined(WIN32)

/*
  Notes:

  - A Win32 window has no alpha channel, so a translucent drop shadow cannot be
    painted into the window it belongs to.  The shadow therefore lives in a
    second, layered window (WS_EX_LAYERED and UpdateLayeredWindow) that carries
    a premultiplied ARGB bitmap, and is kept around the popup it belongs to.
  - The ring lies outside the popup rather than inside a margin: the popup keeps
    the size the shared widget code gives it, so no layout changes and nothing
    has to be clipped away.  The middle of the bitmap is fully transparent, so
    the popup shows through it whatever the stacking order turns out to be.
  - The ring is written with FXRaster, the same rasterizer the Wayland surface
    uses, so the falloff matches the other backends.  The faded end of the ring
    is kept at alpha 1: a layered window is hit tested per pixel, and a fully
    transparent margin would let a click fall through a tool tip onto the window
    below it.
  - The state hangs off the owner window as a window property, so no member has
    to be added to FXWindow and the shadow dies with the window it belongs to.
*/

#define FXSHADOWPROP     TEXT("FOXShadow")
#define FXSHADOWCLASS    TEXT("FOXShadowWindow")

using namespace FX;

/*******************************************************************************/

namespace FX {

namespace {

// One layered window carrying the shadow ring of another window
struct FXShadowWindow {
  HWND     hwnd;        // The layered window
  HDC      dc;          // Memory DC of the bitmap
  HBITMAP  bitmap;      // 32 bpp top down DIB section
  HBITMAP  oldbitmap;   // Bitmap the DC had before
  void    *pixels;      // Bitmap pixels
  FXint    width;       // Bitmap width
  FXint    height;      // Bitmap height
  FXint    margin;      // Width of the ring
  FXint    lastx;       // Where the ring was last placed
  FXint    lasty;
  FXbool   dirty;       // Bitmap changed since it was presented
  };


// The shadow of a window, or nullptr
FXShadowWindow* getShadow(HWND window){
  return (FXShadowWindow*)GetProp(window,FXSHADOWPROP);
  }


// Window class of the layered shadow; registered once
ATOM shadowClass(){
  static ATOM atom=0;
  if(atom==0){
    WNDCLASSEX wc;
    clearElms(&wc,1);
    wc.cbSize=sizeof(WNDCLASSEX);
    wc.lpfnWndProc=DefWindowProc;
    wc.hInstance=GetModuleHandle(nullptr);
    wc.lpszClassName=FXSHADOWCLASS;
    atom=RegisterClassEx(&wc);
    }
  return atom;
  }


// Size the bitmap to the window plus the ring around it
FXbool resizeShadow(FXShadowWindow* shadow,FXint width,FXint height){
  BITMAPINFO info;
  if(shadow->bitmap && shadow->width==width && shadow->height==height) return true;
  if(shadow->bitmap){
    SelectObject(shadow->dc,shadow->oldbitmap);
    DeleteObject(shadow->bitmap);
    shadow->bitmap=nullptr;
    shadow->pixels=nullptr;
    }
  clearElms(&info,1);
  info.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);
  info.bmiHeader.biWidth=width;
  info.bmiHeader.biHeight=-height;
  info.bmiHeader.biPlanes=1;
  info.bmiHeader.biBitCount=32;
  info.bmiHeader.biCompression=BI_RGB;
  shadow->bitmap=CreateDIBSection(shadow->dc,&info,DIB_RGB_COLORS,&shadow->pixels,nullptr,0);
  if(!shadow->bitmap) return false;
  shadow->oldbitmap=(HBITMAP)SelectObject(shadow->dc,shadow->bitmap);
  shadow->width=width;
  shadow->height=height;
  return true;
  }


// Place the ring around the window, and present the bitmap when it changed
void presentShadow(HWND owner,FXShadowWindow* shadow){
  POINT origin,source;
  SIZE size;
  BLENDFUNCTION blend;
  RECT rect;
  if(!IsWindowVisible(owner)){
    ShowWindow(shadow->hwnd,SW_HIDE);
    return;
    }
  GetClientRect(owner,&rect);
  origin.x=rect.left-shadow->margin;
  origin.y=rect.top-shadow->margin;
  ClientToScreen(owner,&origin);
  if(shadow->dirty || shadow->lastx!=origin.x || shadow->lasty!=origin.y || !IsWindowVisible(shadow->hwnd)){
    source.x=0;
    source.y=0;
    size.cx=shadow->width;
    size.cy=shadow->height;
    blend.BlendOp=AC_SRC_OVER;
    blend.BlendFlags=0;
    blend.SourceConstantAlpha=255;
    blend.AlphaFormat=AC_SRC_ALPHA;
    UpdateLayeredWindow(shadow->hwnd,nullptr,&origin,&size,shadow->dc,&source,0,&blend,ULW_ALPHA);
    shadow->lastx=origin.x;
    shadow->lasty=origin.y;
    shadow->dirty=false;
    }
  if(!IsWindowVisible(shadow->hwnd)) ShowWindow(shadow->hwnd,SW_SHOWNOACTIVATE);
  SetWindowPos(shadow->hwnd,owner,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE|SWP_NOOWNERZORDER);
  }


// Create the layered window for a window; the property owns it from here
FXShadowWindow* createShadow(HWND owner){
  FXShadowWindow *shadow;
  shadow=new FXShadowWindow;
  clearElms(shadow,1);
  if(shadowClass()==0){ delete shadow; return nullptr; }
  shadow->hwnd=CreateWindowEx(WS_EX_LAYERED|WS_EX_TOOLWINDOW|WS_EX_NOACTIVATE,FXSHADOWCLASS,nullptr,WS_POPUP,0,0,1,1,nullptr,nullptr,GetModuleHandle(nullptr),nullptr);
  if(!shadow->hwnd){ delete shadow; return nullptr; }
  shadow->dc=CreateCompatibleDC(nullptr);
  if(!shadow->dc){
    DestroyWindow(shadow->hwnd);
    delete shadow;
    return nullptr;
    }
  SetProp(owner,FXSHADOWPROP,(HANDLE)shadow);
  return shadow;
  }

}

// Create or update the drop shadow around a window: the ring lies in a margin
// around the whole client area, which is what the popup paints
void fxshadowSet(FXID window,FXint radius,FXint margin){
  HWND owner=(HWND)window;
  FXShadowWindow *shadow;
  FXRaster raster;
  RECT rect;
  if(!owner || margin<=0 || radius<=0){ fxshadowDestroy(window); return; }
  if(!IsWindow(owner)) return;
  GetClientRect(owner,&rect);
  if(rect.right<=0 || rect.bottom<=0) return;
  shadow=getShadow(owner);
  if(!shadow){
    shadow=createShadow(owner);
    if(!shadow) return;
    }
  shadow->margin=margin;
  if(!resizeShadow(shadow,rect.right+(margin<<1),rect.bottom+(margin<<1))) return;
  raster.setTarget((FXColor*)shadow->pixels,shadow->width,shadow->height);
  memset(shadow->pixels,0,(size_t)shadow->width*(size_t)shadow->height*sizeof(FXColor));
  fxShadowRing(raster,margin,margin,rect.right,rect.bottom,radius,margin,1);
  shadow->dirty=true;
  presentShadow(owner,shadow);
  }


// Follow the window the shadow belongs to: position, stacking order, visibility
void fxshadowSync(FXID window){
  FXShadowWindow *shadow;
  if(!window) return;
  shadow=getShadow((HWND)window);
  if(shadow){
    presentShadow((HWND)window,shadow);
    }
  }


// Destroy the shadow of a window
void fxshadowDestroy(FXID window){
  FXShadowWindow *shadow;
  if(!window) return;
  shadow=(FXShadowWindow*)RemoveProp((HWND)window,FXSHADOWPROP);
  if(!shadow) return;
  if(shadow->hwnd) DestroyWindow(shadow->hwnd);
  if(shadow->bitmap){
    SelectObject(shadow->dc,shadow->oldbitmap);
    DeleteObject(shadow->bitmap);
    }
  if(shadow->dc) DeleteDC(shadow->dc);
  delete shadow;
  }

}

#endif
