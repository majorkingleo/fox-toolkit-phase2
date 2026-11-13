/********************************************************************************
*                                                                               *
*                       Utility Functions   ( W i n 3 2 )                       *
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

#if defined(WIN32)

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
FXbool fxisconsole(const FXchar *path){
  IMAGE_OPTIONAL_HEADER optional_header;
  IMAGE_FILE_HEADER     file_header;
  IMAGE_DOS_HEADER      dos_header;
  DWORD                 dwCoffHeaderOffset;
  DWORD                 dwNewOffset;
  DWORD                 dwMoreDosHeader[16];
  ULONG                 ulNTSignature;
  HANDLE                hImage;
  DWORD                 dwBytes;
  FXbool                flag=false;     // Assume false on Windows is safest!

  // Open the application file.
  hImage=CreateFileA(path,GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
  if(hImage!=INVALID_HANDLE_VALUE){

    // Read MS-Dos image header.
    if(ReadFile(hImage,&dos_header,sizeof(IMAGE_DOS_HEADER),&dwBytes,nullptr)==0) goto x;

    // Test bytes read
    if(dwBytes!=sizeof(IMAGE_DOS_HEADER)) goto x;

    // Test signature
    if(dos_header.e_magic!=IMAGE_DOS_SIGNATURE) goto x;

    // Read more MS-Dos header.
    if(ReadFile(hImage,dwMoreDosHeader,sizeof(dwMoreDosHeader),&dwBytes,nullptr)==0) goto x;

    // Test bytes read
    if(dwBytes!=sizeof(dwMoreDosHeader)) goto x;

    // Move the file pointer to get the actual COFF header.
    dwNewOffset=SetFilePointer(hImage,dos_header.e_lfanew,nullptr,FILE_BEGIN);
    dwCoffHeaderOffset=dwNewOffset+sizeof(ULONG);
    if(dwCoffHeaderOffset==0xFFFFFFFF) goto x;

    // Read NT signature of the file.
    if(ReadFile(hImage,&ulNTSignature,sizeof(ULONG),&dwBytes,nullptr)==0) goto x;

    // Test bytes read
    if(dwBytes!=sizeof(ULONG)) goto x;

    // Test NT signature
    if(ulNTSignature!=IMAGE_NT_SIGNATURE) goto x;

    if(ReadFile(hImage,&file_header,IMAGE_SIZEOF_FILE_HEADER,&dwBytes,nullptr)==0) goto x;

    // Test bytes read
    if(dwBytes!=IMAGE_SIZEOF_FILE_HEADER) goto x;

    // Read the optional header of file.
    if(ReadFile(hImage,&optional_header,sizeof(IMAGE_OPTIONAL_HEADER),&dwBytes,nullptr)==0) goto x;

    // Test bytes read
    if(dwBytes!=sizeof(IMAGE_OPTIONAL_HEADER)) goto x;

    // Switch on systems
    switch(optional_header.Subsystem){
      case IMAGE_SUBSYSTEM_WINDOWS_GUI:     // Windows GUI (2)
      case IMAGE_SUBSYSTEM_WINDOWS_CE_GUI:  // Windows CE GUI (9)
        flag=false;
        break;
      case IMAGE_SUBSYSTEM_WINDOWS_CUI:     // Windows Console (3)
      case IMAGE_SUBSYSTEM_OS2_CUI:         // OS/2 Console (5)
      case IMAGE_SUBSYSTEM_POSIX_CUI:       // Posix Console (7)
        flag=true;
        break;
      case IMAGE_SUBSYSTEM_NATIVE:          // Native (1)
      case IMAGE_SUBSYSTEM_NATIVE_WINDOWS:  // Native Win9x (8)
      case IMAGE_SUBSYSTEM_UNKNOWN:         // Unknown (0)
      default:
        flag=false;
        break;
      }
x:  CloseHandle(hImage);
    }
  return flag;
  }

/*******************************************************************************/

// Log message to [typically] stderr
void fxmessage(const FXchar* format,...){
  FXString message('\0',INITIALMESSAGESIZE);
  va_list arguments;
  va_start(arguments,format);
  message.vformat(format,arguments);
  va_end(arguments);
#if defined(_WINDOWS)
  OutputDebugStringA(message.text());
  fputs(message.text(),stderr);         // if a console is available
  fflush(stderr);
#else
  fputs(message.text(),stderr);
  fflush(stderr);
#endif
  }


// Error routine
void fxerror(const FXchar* format,...){
  FXString message('\0',INITIALMESSAGESIZE);
  va_list arguments;
  va_start(arguments,format);
  message.vformat(format,arguments);
  va_end(arguments);
#if defined(_WINDOWS)
  OutputDebugStringA(message.text());
  fputs(message.text(),stderr);         // if a console is available
  fflush(stderr);
  MessageBoxA(nullptr,message.text(),nullptr,MB_OK|MB_ICONEXCLAMATION|MB_APPLMODAL);
  DebugBreak();
#else
  fputs(message.text(),stderr);
  fflush(stderr);
  abort();
#endif
  }


// Warning routine
void fxwarning(const FXchar* format,...){
  FXString message('\0',INITIALMESSAGESIZE);
  va_list arguments;
  va_start(arguments,format);
  message.vformat(format,arguments);
  va_end(arguments);
#if defined(_WINDOWS)
  OutputDebugStringA(message.text());
  fputs(message.text(),stderr);         // if a console is available
  fflush(stderr);
  MessageBoxA(nullptr,message.text(),nullptr,MB_OK|MB_ICONINFORMATION|MB_APPLMODAL);
#else
  fputs(message.text(),stderr);
  fflush(stderr);
#endif
  }

/*******************************************************************************/

// Assert failed routine
void fxassert(const FXchar* expression,const FXchar* filename,unsigned int lineno){
  if(_isatty(_fileno(stderr))){
    fxmessage("%s(%d): \033[1;31mFXASSERT\033[1;33m(%s)\033[1;31m failed.\033[0m\n",filename,lineno,expression);
    }
  else{
    fxmessage("%s(%d): FXASSERT(%s) failed.\n",filename,lineno,expression);
    }
  }


// Verify failed routine
void fxverify(const FXchar* expression,const FXchar* filename,unsigned int lineno){
  if(_isatty(_fileno(stderr))){
    fxmessage("%s(%d): \033[1;31mFXVERIFY\033[1;33m(%s)\033[1;31m failed.\033[0m\n\n",filename,lineno,expression);
    }
  else{
    fxmessage("%s(%d): FXVERIFY(%s) failed.\n",filename,lineno,expression);
    }
  }


extern FXAPI FILE *fxopen(const char *filename,const char *mode);

FILE *fxopen(const char *filename,const char *mode){
#if defined(UNICODE)
  FXnchar unifile[MAXPATHLEN],unimode[8];
  utf2ncs(unifile,filename,ARRAYNUMBER(unifile));
  utf2ncs(unimode,mode,ARRAYNUMBER(unimode));
  return _wfopen(unifile,unimode);
#else
  return fopen(filename,mode);
#endif
  }

extern FXAPI FILE *fxreopen(const char *filename,const char *mode,FILE * stream);

FILE *fxreopen(const char *filename,const char *mode,FILE * stream){
#if defined(UNICODE)
  FXnchar unifile[MAXPATHLEN],unimode[8];
  utf2ncs(unifile,filename,ARRAYNUMBER(unifile));
  utf2ncs(unimode,mode,ARRAYNUMBER(unimode));
  return _wfreopen(unifile,unimode,stream);
#else
  return freopen(filename,mode,stream);
#endif
  }

}

#endif
