/********************************************************************************
*                                                                               *
*                    O p e n G L   C a n v a s   C h e c k                      *
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
* along with this program.  If not, see <http://www.gnu.org/licenses/>.         *
********************************************************************************/
#include "fx.h"

#if defined(HAVE_GL_H)
#include <GL/gl.h>

/*
  Notes:

  - Draws one red frame into an FXGLCanvas, reads a pixel back and exits 0 when
    it is red.  On Wayland the canvas is an EGL window on a wl_subsurface; on
    X11 and Win32 it is a native GL canvas.
  - Exit 1 means the context did not become current or the pixel was not what
    was drawn; exit 2 means the test was built without OpenGL.
*/

class GLCheck : public FXMainWindow {
  FXDECLARE(GLCheck)
protected:
  FXGLCanvas* canvas;
  FXGLVisual* visual;
  FXint       tries;
protected:
  GLCheck(){}
public:
  long onTimeout(FXObject*,FXSelector,void*);
  enum { ID_TIMEOUT=FXMainWindow::ID_LAST };
public:
  GLCheck(FXApp* a):FXMainWindow(a,"GL Canvas Check",nullptr,nullptr,DECOR_ALL,0,0,320,240),canvas(nullptr),visual(nullptr),tries(0){
    visual=new FXGLVisual(a,VISUAL_DOUBLE_BUFFER);
    canvas=new FXGLCanvas(this,visual,(FXObject*)nullptr,0,LAYOUT_FILL_X|LAYOUT_FILL_Y);
    }
  virtual void create() override {
    FXMainWindow::create();
    getApp()->addTimeout(this,ID_TIMEOUT,200000000);
    }
  virtual ~GLCheck(){ delete canvas; delete visual; }
  };

FXDEFMAP(GLCheck) GLCheckMap[]={
  FXMAPFUNC(SEL_TIMEOUT,GLCheck::ID_TIMEOUT,GLCheck::onTimeout)
  };

FXIMPLEMENT(GLCheck,FXMainWindow,GLCheckMap,ARRAYNUMBER(GLCheckMap))


long GLCheck::onTimeout(FXObject*,FXSelector,void*){
  FXuchar pixel[4]={0,0,0,0};
  if(canvas->makeCurrent()){
    glViewport(0,0,canvas->getWidth(),canvas->getHeight());
    glClearColor(1.0f,0.0f,0.0f,1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glFinish();
    glReadPixels(4,4,1,1,GL_RGBA,GL_UNSIGNED_BYTE,pixel);
    // Swap the way an application does: only when the visual says it is double
    if(visual->isDoubleBuffer()) canvas->swapBuffers();
    canvas->makeNonCurrent();
    if(pixel[0]>250 && pixel[1]<5 && pixel[2]<5 && pixel[3]>250){
      fxmessage("glcanvas: GL canvas draws, pixel %u %u %u %u\n",pixel[0],pixel[1],pixel[2],pixel[3]);
      getApp()->stop(0);
      return 1;
      }
    }
  if(++tries>20){
    fxmessage("glcanvas: the GL canvas did not draw, last pixel %u %u %u %u\n",pixel[0],pixel[1],pixel[2],pixel[3]);
    getApp()->stop(1);
    return 1;
    }
  getApp()->addTimeout(this,ID_TIMEOUT,150000000);
  return 1;
  }


int main(int argc,char* argv[]){
  FXApp app("glcanvas","FoxTest");
  app.init(argc,argv);
  GLCheck* window=new GLCheck(&app);
  app.create();
  window->show(PLACEMENT_SCREEN);
  return app.run();
  }

#else

int main(int argc,char* argv[]){
  fxmessage("glcanvas: built without OpenGL; use a build with --opengl\n");
  return 2;
  }

#endif
