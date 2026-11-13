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

#if defined(WIN32)

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

// Is year a leap-year
static FXint is_leap(FXint year){
  return !(year%4) && ((year%100) || !(year%400));
  }


// Time zone variables, set only once
static volatile FXuint local_zone_set=0;

//
// The Windows TIME_ZONE_INFORMATION struct contains:
//
//   struct TIME_ZONE_INFORMATION {
//     LONG       Bias;                 // UTC = localtime + bias (minutes)
//     WCHAR      StandardName[32];     // Standard time name
//     SYSTEMTIME StandardDate;         // Time when daylight saving to standard time occurs
//     LONG       StandardBias;         // Value added to Bias during standard time (most zones = 0)
//     WCHAR      DaylightName[32];     // Daylight savings time name
//     SYSTEMTIME DaylightDate;         // Time when standard time to daylight savings time occurs
//     LONG       DaylightBias;         // Value added to Bias during daylight savings time (most zones = -60)
//     };
//
// While the Windows SYSTEMTIME struct contains:
//
//   struct SYSTEMTIME {
//     WORD wYear;                      // Year, 1601..
//     WORD wMonth;                     // Month, january..december (1..12)
//     WORD wDayOfWeek;                 // Day of week, sunday..monday (0..7)
//     WORD wDay;                       // Day of month, (1..31) [SEE NOTES BELOW]
//     WORD wHour;                      // Hour (0..23)
//     WORD wMinute;                    // Minutes (0..59)
//     WORD wSecond;                    // Seconds (0..59)
//     WORD wMilliseconds;              // Milliseconds (0..999)
//     };
//
static TIME_ZONE_INFORMATION tzi;


// Call tzset() only once
static void setuplocaltimezone(){
  if(atomicSet(&local_zone_set,1)==0){
    GetTimeZoneInformation(&tzi);
    }
  }


// Return offset between standard local time zone to UTC, in nanoseconds
FXTime FXSystem::localTimeZoneOffset(){
  setuplocaltimezone();
  return minutes*tzi.Bias;              // +minutes*tzi.StandardBias;
  }


// Return offset daylight savings time to standard time, in nanoseconds
FXTime FXSystem::daylightSavingsOffset(){
  setuplocaltimezone();
  return minutes*tzi.DaylightBias;      // Or difference between standard and daylight bias.
  }


// Return time zone name (or daylight savings time time zone name)
FXString FXSystem::localTimeZoneName(FXbool dst){
  setuplocaltimezone();
  return FXString(dst?tzi.DaylightName:tzi.StandardName);
  }



// Convert utc (ns) since 01/01/1970 to 100ns since 01/01/1601
//static inline FXTime fxwintime(FXTime utc){
//  const FXTime BIAS=FXLONG(116444736000000000);
//  const FXTime MULT=FXLONG(100);
//  return BIAS+utc/MULT;
//  }

// Convert utc (ns) since 01/01/1970 to 100ns since 01/01/1601
static inline FILETIME fxwintime(FXTime ut){
  const FXTime BIAS=FXLONG(116444736000000000);
  const FXTime MULT=FXLONG(100);
  FILETIME ft;
  ut=BIAS+ut/MULT;
  ft.dwLowDateTime=(DWORD)ut;
  ft.dwHighDateTime=(DWORD)(ut>>32);
  return ft;
  }


// Compare incoming date cur against changeover date chg; there are
// two ways to compare the numbers.
// When chg.wYear!=0, the change over occurs only once on the indicated
// absolute time.
// If chg.wYear==0, the switch between standard and daylight savings time
// occurs yearly, on the nth occurrence of wDayOfWeek, where n is given
// in the wDay variable of the TIME_ZONE_INFORMATION StandardDate or
// DaylightDate struct.
static FXint compare_zone_switch_over(const SYSTEMTIME& cur,const SYSTEMTIME& chg){
  static const FXint month_lengths[12]={31,28,31,30,31,30,31,31,30,31,30,31};
  FXint cursecs,chgsecs,chgday,first,last;

  // Absolute date
  if(chg.wYear){
    if(cur.wYear==chg.wYear){
      if(cur.wMonth==chg.wMonth){
        if(cur.wDay==chg.wDay){
          cursecs=(cur.wHour*60+cur.wMinute)*60+cur.wSecond;
          chgsecs=(chg.wHour*60+chg.wMinute)*60+chg.wSecond;
          return cursecs-chgsecs;
          }
        return cur.wDay-chg.wDay;
        }
      return cur.wMonth-chg.wMonth;
      }
    return cur.wYear-chg.wYear;
    }

  // Relative date
  if(cur.wMonth==chg.wMonth){

    // Calculate the day of the first wDayOfWeek in the month
    first=(6+chg.wDayOfWeek-cur.wDayOfWeek+cur.wDay)%7+1;

    // Check needed for the 5th weekday of the month
    last=month_lengths[cur.wMonth-1]+(cur.wMonth==2 && is_leap(cur.wYear));

    // Switch at the nth occurrence (value in chg.wDay) of certain day of week
    chgday=first+7*(chg.wDay-1);
    if(chgday>last) chgday-=7;

    chgsecs=((chgday*24+chg.wHour)*60+chg.wMinute)*60+chg.wSecond;
    cursecs=((cur.wDay*24+cur.wHour)*60+cur.wMinute)*60+cur.wSecond;
    return cursecs-chgsecs;
    }
  return (FXint)cur.wMonth-(FXint)chg.wMonth;
  }


// Determine if daylight savings in effect
// The daylight savings time switch is tied to local time, and local
// time's year is tied to time zone.  However, time zone is a function
// of the local year [time zone info can change from year to year].
// We assume that the time zone is relatively stable, but we may
// call GetTimeZoneInformationForYear() in a future revision to
// obtain only DST/STD time switches, which are changed more rapidly.
static FXint daylightSavingsState(FXTime utc,FXbool local){

  // Expanded date/time
  SYSTEMTIME loc;

  // Assume local time
  FILETIME ftloc=fxwintime(utc);
  FILETIME ftdst=ftloc;
  FILETIME ftstd=ftloc;

  // If UTC, convert to local using time zone information
  if(!local){
    ftloc=fxwintime(utc-tzi.Bias*minutes);
    ftdst=fxwintime(utc-tzi.Bias*minutes-tzi.DaylightBias*minutes);
    ftstd=fxwintime(utc-tzi.Bias*minutes-tzi.StandardBias*minutes);
    }

  // Get expanded date/time
  FileTimeToSystemTime(&ftloc,&loc);

  // If wMonth is zero then no daylight savings in effect
  if(tzi.DaylightDate.wMonth && tzi.StandardDate.wMonth){

    // Expanded date/time
    SYSTEMTIME dst;
    SYSTEMTIME std;

    // Get expanded date/time
    FileTimeToSystemTime(&ftdst,&dst);
    FileTimeToSystemTime(&ftstd,&std);

    // Daylight savings time prior to switch to standard time
    FXbool before_std_date=(compare_zone_switch_over(dst,tzi.StandardDate)<0);

    // Standard time after switch to daylight savings time
    FXbool after_dst_date=(compare_zone_switch_over(std,tzi.DaylightDate)>=0);

    // Northern hemisphere
    if(tzi.DaylightDate.wMonth<tzi.StandardDate.wMonth){
      if(before_std_date && after_dst_date) return 1;
      }

    // Southern hemisphere
    else{
      if(before_std_date || after_dst_date) return 1;
      }
    }
  return 0;
  }



// Return 1 if daylight savings time is active at utc in nanoseconds since Unix Epoch
FXTime FXSystem::daylightSavingsActive(FXTime utc){
  setuplocaltimezone();
  return daylightSavingsState(utc,false);
  }

}

#endif
