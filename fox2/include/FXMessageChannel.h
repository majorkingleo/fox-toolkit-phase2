/********************************************************************************
*                                                                               *
*         I n t e r - T h r e a d    M e s s a g i n g    C h a n n e l         *
*                                                                               *
*********************************************************************************
* Copyright (C) 2006,2026 by Jeroen van der Zijp.   All Rights Reserved.        *
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
#ifndef FXMESSAGECHANNEL_H
#define FXMESSAGECHANNEL_H

#ifndef FXOBJECT_H
#include "FXObject.h"
#endif

#ifndef FXMUTEX_H
#include "FXMutex.h"
#endif

namespace FX {

class FXApp;


/**
* FXMessageChannel asynchronously dispatches messages from worker threads to
* objects handled by the FXApp event loop.
*/
class FXAPI FXMessageChannel : public FXObject {
  FXDECLARE(FXMessageChannel)
private:
  FXApp* app;
  FXMessageChannel();
  FXMessageChannel(const FXMessageChannel&);
  FXMessageChannel& operator=(const FXMessageChannel&);
protected:
  FXInputHandle h[3];
  FXMutex m;
public:
  enum {
    ID_IO_READ=1,
    ID_LAST
    };
public:
  long onMessage(FXObject*,FXSelector,void*);
  FXMessageChannel(FXApp* a);
  FXApp* getApp() const { return app; }
  FXbool message(FXObject* target,FXSelector msg,const void* data=nullptr,FXint size=0);
  virtual ~FXMessageChannel();
  };

}

#endif
