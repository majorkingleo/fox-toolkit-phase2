/********************************************************************************
*                                                                               *
*                                T i m e   S t u f f                            *
*                                                                               *
*********************************************************************************
* Copyright (C) 2019,2026 by Jeroen van der Zijp.   All Rights Reserved.        *
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
#include "fxascii.h"
#include "FXAtomic.h"
#include "FXElement.h"
#include "FXString.h"
#include "FXSystem.h"

#if !defined(WIN32)

/*
  Notes:
  - Platform implementation of the FXSystem time zone members; the portable calendar
    functions stay in FXSystemTime.cpp.
*/

#define TOPIC_DEBUG     1002

using namespace FX;

/*******************************************************************************/

namespace FX {

// Time units in terms of nanoseconds
static const FXTime seconds=1000000000;
static const FXTime minutes=60*seconds;
static const FXTime hours=60*minutes;

// Time zone variables, set only once
static volatile FXuint local_zone_set=0;



// Call tzset() only once
static void setuplocaltimezone(){
  if(atomicSet(&local_zone_set,1)==0){
    tzset();
    }
  }


// Return offset between standard local time zone to UTC, in nanoseconds
FXTime FXSystem::localTimeZoneOffset(){
  setuplocaltimezone();
#if defined(__FreeBSD__) || defined(__OpenBSD__)
  return 0;     // FIXME
#else
  return seconds*timezone;
#endif
  }


// Return offset daylight savings time to standard time, in nanoseconds
FXTime FXSystem::daylightSavingsOffset(){
  setuplocaltimezone();
#if defined(__FreeBSD__) || defined(__OpenBSD__)
  return 0;     // FIXME
#else
  return -hours*daylight;
#endif
  }


// Return time zone name (or daylight savings time time zone name)
FXString FXSystem::localTimeZoneName(FXbool dst){
  setuplocaltimezone();
  return FXString(tzname[dst]);
  }




// Return 1 if daylight savings time is active at utc in nanoseconds since Unix Epoch
FXTime FXSystem::daylightSavingsActive(FXTime utc){
  setuplocaltimezone();
#if defined(HAVE_LOCALTIME_R)
  struct tm tmresult;
  time_t tmp=(time_t)(utc/seconds);
  struct tm* ptm=localtime_r(&tmp,&tmresult);
  FXTRACE(TOPIC_DEBUG,"FXSystem::daylightSavingsActive(%lld) = %d\n",utc,ptm && ptm->tm_isdst!=0);
  return ptm && ptm->tm_isdst!=0;
#else
  time_t tmp=(time_t)(utc/seconds);
  struct tm* ptm=localtime(&tmp);
  FXTRACE(TOPIC_DEBUG,"FXSystem::daylightSavingsActive(%lld) = %d\n",utc,ptm && ptm->tm_isdst!=0);
  return ptm && ptm->tm_isdst!=0;
#endif
  }

}

#endif
