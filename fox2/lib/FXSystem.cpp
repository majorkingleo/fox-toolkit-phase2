/********************************************************************************
*                                                                               *
*         M i s c e l l a n e o u s   S y s t e m   F u n c t i o n s           *
*                                                                               *
*********************************************************************************
* Copyright (C) 2005,2026 by Jeroen van der Zijp.   All Rights Reserved.        *
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
#include "fxascii.h"
#include "FXArray.h"
#include "FXHash.h"
#include "FXStream.h"
#include "FXString.h"
#include "FXIO.h"
#include "FXFile.h"
#include "FXPath.h"
#include "FXProcess.h"
#include "FXSystem.h"
#include "FXStat.h"

/*
  Notes:

  - A bric-a-brack of various functions we could not place anywhere else.

  - Some ISO 8601 Formats:

     Date:                        2019-09-28
     Date and time in UTC:        2019-09-28T15:33:50+00:00
                                  2019-09-28T15:33:50Z
                                  20190928T153350Z
     Week:                        2019-W39
     Date with week number:       2019-W39-6
     Date without year:           --09-28[1]
     Ordinal date:                2019-271
*/


using namespace FX;

/*******************************************************************************/

namespace FX {

// Return permissions string
FXString FXSystem::modeString(FXuint mode){
  FXchar result[11];
  result[0]=(mode&FXIO::SymLink) ? 'l' : (mode&FXIO::File) ? '-' : (mode&FXIO::Directory) ? 'd' : (mode&FXIO::Character) ? 'c' : (mode&FXIO::Block) ? 'b' : (mode&FXIO::Fifo) ? 'p' : (mode&FXIO::Socket) ? 's' : '?';
  result[1]=(mode&FXIO::OwnerRead) ? 'r' : '-';
  result[2]=(mode&FXIO::OwnerWrite) ? 'w' : '-';
  result[3]=(mode&FXIO::SetUser) ? 's' : (mode&FXIO::OwnerExec) ? 'x' : '-';
  result[4]=(mode&FXIO::GroupRead) ? 'r' : '-';
  result[5]=(mode&FXIO::GroupWrite) ? 'w' : '-';
  result[6]=(mode&FXIO::SetGroup) ? 's' : (mode&FXIO::GroupExec) ? 'x' : '-';
  result[7]=(mode&FXIO::OtherRead) ? 'r' : '-';
  result[8]=(mode&FXIO::OtherWrite) ? 'w' : '-';
  result[9]=(mode&FXIO::Sticky) ? 't' : (mode&FXIO::OtherExec) ? 'x' : '-';
  result[10]=0;
  return FXString(result);
  }


// Get executable path
FXString FXSystem::getExecPath(){
  return FXSystem::getEnvironment("PATH");
  }


// Return host name
FXString FXSystem::getHostName(){
  FXchar name[MAXHOSTNAMELEN];
  if(gethostname(name,sizeof(name))==0){
    name[MAXHOSTNAMELEN-1]='\0';
    return FXString(name);
    }
  return "localhost";
  }

}

