/********************************************************************************
*                                                                               *
*                O p e r a t i n g   S y s t e m   V e r s i o n                *
*                                                                               *
*********************************************************************************
* Copyright (C) 2022,2026 by Jeroen van der Zijp.   All Rights Reserved.        *
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

#if !defined(WIN32)


/*
  Notes:
  - Platform implementation of fxosversion().
*/

#define TOPIC_DETAIL    1002

using namespace FX;

/*******************************************************************************/

namespace FX {

extern FXAPI FXint __snprintf(FXchar* string,FXint length,const FXchar* format,...);

#if defined(HAVE_UNAME)

// Get operating system version string
FXival fxosversion(FXchar version[],FXival len){   // FIXME will change
  if(0<len){
    struct utsname buffer;
    if(uname(&buffer)==0){
      FXTRACE(TOPIC_DETAIL,"fxosversion: sysname=%s\n",buffer.sysname);
      FXTRACE(TOPIC_DETAIL,"fxosversion: nodename=%s\n",buffer.nodename);
      FXTRACE(TOPIC_DETAIL,"fxosversion: release=%s\n",buffer.release);
      FXTRACE(TOPIC_DETAIL,"fxosversion: version=%s\n",buffer.version);
      return fxstrlcpy(version,buffer.release,len);
      }
    //return __snprintf(version,len,"%d.%d.%d",LINUX_VERSION_MAJOR,LINUX_VERSION_PATCHLEVEL,LINUX_VERSION_SUBLEVEL);
    version[0]='\0';
    }
  return 0;
  }

#else

// Get operating system version string
FXival fxosversion(FXchar version[],FXival len){
  if(0<len){
    version[0]='\0';
    }
  return 0;
  }

#endif

}

#endif
