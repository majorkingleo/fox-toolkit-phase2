/********************************************************************************
*                                                                               *
*                         Utility Functions   ( X 1 1 )                         *
*                                                                               *
*********************************************************************************
* Copyright (C) 1998,2026 by Jeroen van der Zijp.   All Rights Reserved.        *
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
#include "fxchar.h"
#include "fxmath.h"
#include "fxendian.h"
#include "fxascii.h"
#include "FXElement.h"
#include "FXHash.h"
#include "FXStream.h"
#include "FXString.h"
#include "FXElement.h"

#if !defined(WIN32)

/*
  Notes:

  - Handy global utility functions.
*/

// Initial space allocated for message; it will grow if needed
#ifndef INITIALMESSAGESIZE
#define INITIALMESSAGESIZE 512
#endif

using namespace FX;

/*******************************************************************************/

namespace FX {

// Return true if console application
FXbool fxisconsole(const FXchar*){
  return true;
  }

/*******************************************************************************/

// Log message to [typically] stderr
void fxmessage(const FXchar* format,...){
  FXString message('\0',INITIALMESSAGESIZE);
  va_list arguments;
  va_start(arguments,format);
  message.vformat(format,arguments);
  va_end(arguments);
  fputs(message.text(),stderr);
  fflush(stderr);
  }


// Error routine
void fxerror(const FXchar* format,...){
  FXString message('\0',INITIALMESSAGESIZE);
  va_list arguments;
  va_start(arguments,format);
  message.vformat(format,arguments);
  va_end(arguments);
  if(!fxdialogsenabled()){
    fputs(message.text(),stdout);
    fflush(stdout);
    abort();
    }
  fputs(message.text(),stderr);
  fflush(stderr);
  abort();
  }


// Warning routine
void fxwarning(const FXchar* format,...){
  FXString message('\0',INITIALMESSAGESIZE);
  va_list arguments;
  va_start(arguments,format);
  message.vformat(format,arguments);
  va_end(arguments);
  if(!fxdialogsenabled()){
    fputs(message.text(),stdout);
    fflush(stdout);
    return;
    }
  fputs(message.text(),stderr);
  fflush(stderr);
  }

/*******************************************************************************/

// Assert failed routine
void fxassert(const FXchar* expression,const FXchar* filename,unsigned int lineno){
  if(isatty(fileno(stderr))){
    fxmessage("%s:%d: \033[1;31mFXASSERT\033[1;33m(%s)\033[1;31m failed.\033[0m\n",filename,lineno,expression);
    }
  else{
    fxmessage("%s:%d: FXASSERT(%s) failed.\n",filename,lineno,expression);
    }
  }


// Verify failed routine
void fxverify(const FXchar* expression,const FXchar* filename,unsigned int lineno){
  if(isatty(fileno(stderr))){
    fxmessage("%s:%d: \033[1;31mFXVERIFY\033[1;33m(%s)\033[1;31m failed.\033[0m\n",filename,lineno,expression);
    }
  else{
    fxmessage("%s:%d: FXVERIFY(%s) failed.\n",filename,lineno,expression);
    }
  }


extern FXAPI FILE *fxopen(const char *filename,const char *mode);

FILE *fxopen(const char *filename,const char *mode){
  return fopen(filename,mode);
  }

extern FXAPI FILE *fxreopen(const char *filename,const char *mode,FILE * stream);

FILE *fxreopen(const char *filename,const char *mode,FILE * stream){
  return freopen(filename,mode,stream);
  }

}

#endif
