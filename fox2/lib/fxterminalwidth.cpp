/********************************************************************************
*                                                                               *
*                  T e r m i n a l   C h a r a c t e r   W i d t h              *
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
#include "FXString.h"
#include "fxunicode.h"
#include "fxterminalwidth.h"

using namespace FX;

/*******************************************************************************/

namespace FX {

// East Asian Wide and Fullwidth, and the emoji ranges that terminals treat as wide.
// The table stays deliberately small; ambiguous characters are one cell.
struct FXTerminalWideRange {
  FXwchar first;
  FXwchar last;
  };

static const FXTerminalWideRange fxterminalwideranges[]={
  {0x1100,0x115F},
  {0x2329,0x232A},
  {0x2E80,0x303E},
  {0x3040,0xA4CF},
  {0xAC00,0xD7A3},
  {0xF900,0xFAFF},
  {0xFE10,0xFE19},
  {0xFE30,0xFE6F},
  {0xFF00,0xFF60},
  {0xFFE0,0xFFE6},
  {0x1F000,0x1F02F},
  {0x1F0A0,0x1F0FF},
  {0x1F100,0x1F1FF},
  {0x1F200,0x1F2FF},
  {0x1F300,0x1F64F},
  {0x1F680,0x1F6FF},
  {0x1F900,0x1F9FF},
  {0x1FA70,0x1FAFF},
  {0x20000,0x3FFFD}
  };


// Return true if the codepoint is in a wide range
static FXbool fxterminalIsWide(FXwchar ch){
  FXint lo=0;
  FXint hi=(FXint)(sizeof(fxterminalwideranges)/sizeof(fxterminalwideranges[0]))-1;
  FXint mid;
  while(lo<=hi){
    mid=(lo+hi)/2;
    if(ch<fxterminalwideranges[mid].first){
      hi=mid-1;
      }
    else if(ch>fxterminalwideranges[mid].last){
      lo=mid+1;
      }
    else{
      return true;
      }
    }
  return false;
  }


// Return true if the codepoint combines with the preceding cell
FXbool fxterminalIsCombining(FXwchar ch){
  FXuint cat=Unicode::charCategory(ch);
  if(cat==CatMarkNonSpacing || cat==CatMarkSpacingCombining || cat==CatMarkEnclosing) return true;
  if(cat==CatFormat || cat==CatControl || cat==CatSurrogate || cat==CatNotAssigned) return true;
  return Unicode::charCombining(ch)!=0;
  }


// Return the number of terminal cells a codepoint occupies
FXint fxterminalCharWidth(FXwchar ch){
  if(ch==0) return 0;
  if(ch<0x20 || (0x7F<=ch && ch<0xA0)) return 0;
  if(fxterminalIsCombining(ch)) return 0;
  if(fxterminalIsWide(ch)) return 2;
  return 1;
  }

}

/*******************************************************************************/
