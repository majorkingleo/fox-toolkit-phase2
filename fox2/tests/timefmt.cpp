/********************************************************************************
*                                                                               *
*                             String Format I/O Test                            *
*                                                                               *
*********************************************************************************
* Copyright (C) 2007,2026 by Jeroen van der Zijp.   All Rights Reserved.        *
********************************************************************************/
#include "fx.h"
#include "testargs.h"
#include <time.h>


/*
  Notes:
  - Test battery for time formatting and parsing.
*/


/*******************************************************************************/


// Time print formats
const FXchar *timeformat[]={
  "%H:%M:%S.%fm",
  "%l:%M:%S %P",
  "%A %B %d %Y %I:%M:%S.%fu %p %z",
  "%Y/%m/%d",
  "%G-W%V-%u",
  "%Y %j",
  "%Y %U %w",
  "%Y %W %u",
  "%Y %j %w",
  "%Y %U %w",
  "%T",
  "%Z",
  "%FT%T%z",
  "%s"
  };


// Time values to print
const FXTime timevalues[]={
  FXLONG(-885744000000000000),
  FXLONG(-192799800000000000),
  FXLONG(                  0),
  FXLONG( 798282120000000000),
  FXLONG(1000197960000000000),
  FXLONG(1571388300000000000),
  FXLONG(1588006865264666745)
  };


// Time parse formats
const FXchar *timeparse[]={
  "%Z",                 // Zone offset
  "%Y",                 // Year
  "%Y %j",              // Year, day of year
  "%Y/%m/%d",           // Year, month, day
  "%b %d %Y",           // Month, day, year
  "%Y %U %w",           // Year, week, day of week
  "%Y %U",              // Year, week
  "%H%M%S",             // Hours, minutes, seconds
  "%s",                 // Seconds since Epoch
  "%z",                 // Zone offset
  "%z",                 // Zone offset
  "%F",                 // ISO date format
  "%H:%M:%S.%fm"
  };


// Time strings to parse
const FXchar* timestrings[]={
  "CST",
  "2020",
  "2020 38",
  "2020/2/7",
  "Feb  7 2020",
  "2020 3 1",
  "2020 5",
  "112233",
  "1579508820",
  "+0300",
  "-03",
  "2020-2-7",
  "18:50:19.734"
  };


// ISO week/year checks
const FXint week_number_cases[][3]={
  {2005, 1, 1},     // Sat  1 Jan 2005      2005-01-01      2004-W53-6
  {2005, 1, 2},     // Sun  2 Jan 2005      2005-01-02      2004-W53-7
  {2005,12,31},     // Sat 31 Dec 2005      2005-12-31      2005-W52-6
  {2006, 1, 1},     // Sun  1 Jan 2006      2006-01-01      2005-W52-7
  {2006, 1, 2},     // Mon  2 Jan 2006      2006-01-02      2006-W01-1
  {2006,12,31},     // Sun 31 Dec 2006      2006-12-31      2006-W52-7
  {2007, 1, 1},     // Mon  1 Jan 2007      2007-01-01      2007-W01-1
  {2007,12,30},     // Sun 30 Dec 2007      2007-12-30      2007-W52-7
  {2007,12,31},     // Mon 31 Dec 2007      2007-12-31      2008-W01-1
  {2008, 1, 1},     // Tue  1 Jan 2008      2008-01-01      2008-W01-2
  {2008,12,28},     // Sun 28 Dec 2008      2008-12-28      2008-W52-7
  {2008,12,29},     // Mon 29 Dec 2008      2008-12-29      2009-W01-1
  {2008,12,30},     // Tue 30 Dec 2008      2008-12-30      2009-W01-2
  {2008,12,31},     // Wed 31 Dec 2008      2008-12-31      2009-W01-3
  {2009, 1, 1},     // Thu  1 Jan 2009      2009-01-01      2009-W01-4
  {2009,12,31},     // Thu 31 Dec 2009      2009-12-31      2009-W53-4
  {2010, 1, 1},     // Fri  1 Jan 2010      2010-01-01      2009-W53-5
  {2010, 1, 2},     // Sat  2 Jan 2010      2010-01-02      2009-W53-6
  {2010, 1, 3},     // Sun  3 Jan 2010      2010-01-03      2009-W53-7
  {2020, 4,27}      // Mon 27 Apr 2020      2020-04-27      2020-W18-1
  };


// ISO week/year checks
const FXchar* isoweek_date_strings[]={
  "2004-W53-6",     // Sat  1 Jan 2005      2005-01-01      2004-W53-6
  "2004-W53-7",     // Sun  2 Jan 2005      2005-01-02      2004-W53-7
  "2005-W52-6",     // Sat 31 Dec 2005      2005-12-31      2005-W52-6
  "2005-W52-7",     // Sun  1 Jan 2006      2006-01-01      2005-W52-7
  "2006-W01-1",     // Mon  2 Jan 2006      2006-01-02      2006-W01-1
  "2006-W52-7",     // Sun 31 Dec 2006      2006-12-31      2006-W52-7
  "2007-W01-1",     // Mon  1 Jan 2007      2007-01-01      2007-W01-1
  "2007-W52-7",     // Sun 30 Dec 2007      2007-12-30      2007-W52-7
  "2008-W01-1",     // Mon 31 Dec 2007      2007-12-31      2008-W01-1
  "2008-W01-2",     // Tue  1 Jan 2008      2008-01-01      2008-W01-2
  "2008-W52-7",     // Sun 28 Dec 2008      2008-12-28      2008-W52-7
  "2009-W01-1",     // Mon 29 Dec 2008      2008-12-29      2009-W01-1
  "2009-W01-2",     // Tue 30 Dec 2008      2008-12-30      2009-W01-2
  "2009-W01-3",     // Wed 31 Dec 2008      2008-12-31      2009-W01-3
  "2009-W01-4",     // Thu  1 Jan 2009      2009-01-01      2009-W01-4
  "2009-W53-4",     // Thu 31 Dec 2009      2009-12-31      2009-W53-4
  "2009-W53-5",     // Fri  1 Jan 2010      2010-01-01      2009-W53-5
  "2009-W53-6",     // Sat  2 Jan 2010      2010-01-02      2009-W53-6
  "2009-W53-7",     // Sun  3 Jan 2010      2010-01-03      2009-W53-7
  "2020-W18-1"      // Mon 27 Apr 2020      2020-04-27      2020-W18-1
  };


// Test week number stuff
FXString weeknumber(FXint year,FXint month,FXint day){
  const FXTime seconds=1000000000;
  const FXTime DAY=86400*seconds;
  FXSystem::Time st={0,0,0,0,0,0,0,0,0,0};
  FXSystem::systemTimeFromTime(st,FXSystem::daysFromCivil(year,month,day)*DAY);
  return systemTimeFormat(st,"%a %e %b %Y    %Y-%m-%d    %G-W%V-%u (doy=%j)");
  }


// Failures found
static FXint failures=0;


// Report a failure, but not too many of them
static void bad(const FXchar* fmt,...) FX_PRINTF(1,2);
static void bad(const FXchar* fmt,...){
  va_list ap;
  failures++;
  if(failures<=40){
    printf("BAD: ");
    va_start(ap,fmt);
    vprintf(fmt,ap);
    va_end(ap);
    printf("\n");
    }
  }


// Expect string
static void expectstr(const FXchar* what,const FXString& got,const FXchar* expected){
  if(strcmp(got.text(),expected)!=0) bad("%s: expected \"%s\" got \"%s\"",what,expected,got.text());
  }


// Expect number
static void expectnum(const FXchar* what,FXlong got,FXlong expected){
  if(got!=expected) bad("%s: expected %lld got %lld",what,expected,got);
  }


// Nanoseconds since Epoch, computed from parts
static FXTime ns(FXint y,FXint m,FXint d,FXint h=0,FXint mi=0,FXint s=0,FXint n=0){
  const FXTime sec=1000000000;
  return (((FXSystem::daysFromCivil(y,m,d)*24+h)*60+mi)*60+s)*sec+n;
  }


// System time from nanoseconds
static FXSystem::Time fromns(FXTime t){
  FXSystem::Time st={0,0,0,0,0,0,0,0,0,0};
  FXSystem::systemTimeFromTime(st,t);
  return st;
  }


// Days in month
static FXint daysinmonth(FXint y,FXint m){
  static const FXint dim[12]={31,28,31,30,31,30,31,31,30,31,30,31};
  return (m==2 && !(y%4) && ((y%100) || !(y%400))) ? 29 : dim[m-1];
  }


// Check broken down time
static void expectparts(const FXchar* what,const FXSystem::Time& st,FXint y,FXint mo,FXint d,FXint yd,FXint wd,FXint h,FXint mi,FXint s,FXint n){
  if(st.year!=y || st.month!=mo || st.mday!=d || st.yday!=yd || st.wday!=wd || st.hour!=h || st.min!=mi || st.sec!=s || st.nano!=n){
    bad("%s: expected %04d-%02d-%02d yday=%d wday=%d %02d:%02d:%02d.%09d got %04d-%02d-%02d yday=%d wday=%d %02d:%02d:%02d.%09d",what,y,mo,d,yd,wd,h,mi,s,n,st.year,st.month,st.mday,st.yday,st.wday,st.hour,st.min,st.sec,st.nano);
    }
  }


// Days to and from civil date
static void testcivil(){
  FXint y,m,d,py=0,pm=0,pd=0;
  FXlong z;
  FXSystem::civilFromDays(y,m,d,0);
  if(y!=1970 || m!=1 || d!=1) bad("civilFromDays(0) gave %04d-%02d-%02d",y,m,d);
  FXSystem::civilFromDays(y,m,d,-1);
  if(y!=1969 || m!=12 || d!=31) bad("civilFromDays(-1) gave %04d-%02d-%02d",y,m,d);
  FXSystem::civilFromDays(y,m,d,11017);
  if(y!=2000 || m!=3 || d!=1) bad("civilFromDays(11017) gave %04d-%02d-%02d",y,m,d);
  FXSystem::civilFromDays(y,m,d,18321);
  if(y!=2020 || m!=2 || d!=29) bad("civilFromDays(18321) gave %04d-%02d-%02d",y,m,d);
  expectnum("daysFromCivil 1970-01-01",FXSystem::daysFromCivil(1970,1,1),0);
  expectnum("daysFromCivil 2000-01-01",FXSystem::daysFromCivil(2000,1,1),10957);
  expectnum("daysFromCivil 1900-03-01",FXSystem::daysFromCivil(1900,3,1),-25508);
  expectnum("daysFromCivil 0001-01-01",FXSystem::daysFromCivil(1,1,1),-719162);
  expectnum("daysFromCivil 2020-03-01",FXSystem::daysFromCivil(2020,3,1),18322);
  for(z=-800000; z<=800000 && failures<10; ++z){
    FXSystem::civilFromDays(y,m,d,z);
    if(m<1 || 12<m || d<1 || daysinmonth(y,m)<d){ bad("civilFromDays(%lld) gave invalid %04d-%02d-%02d",z,y,m,d); continue; }
    if(FXSystem::daysFromCivil(y,m,d)!=z) bad("daysFromCivil(%04d-%02d-%02d) is not %lld",y,m,d,z);
    if(z>-800000){                                      // Successive days are successive dates
      FXbool next=(y==py && m==pm && d==pd+1) || (d==1 && ((y==py && m==pm+1) || (y==py+1 && m==1 && pm==12)));
      if(!next) bad("%04d-%02d-%02d does not follow %04d-%02d-%02d",y,m,d,py,pm,pd);
      }
    py=y; pm=m; pd=d;
    }
  }


// Broken down time from nanoseconds
static void testparts(){
  const FXTime sec=1000000000;
  FXTime t;
  FXSystem::Time st;
  expectparts("epoch",fromns(0),1970,1,1,1,4,0,0,0,0);
  expectparts("one nanosecond before epoch",fromns(-1),1969,12,31,365,3,23,59,59,999999999);
  expectparts("2000-01-01",fromns(ns(2000,1,1)),2000,1,1,1,6,0,0,0,0);
  expectparts("2000-12-31",fromns(ns(2000,12,31,23,59,59,5)),2000,12,31,366,0,23,59,59,5);
  expectparts("1900-12-31",fromns(ns(1900,12,31,12)),1900,12,31,365,1,12,0,0,0);
  expectparts("2020-02-29",fromns(ns(2020,2,29,1,2,3,4)),2020,2,29,60,6,1,2,3,4);
  expectparts("1588006865264666745",fromns(FXLONG(1588006865264666745)),2020,4,27,118,1,17,1,5,264666745);
  expectparts("-885744000000000000",fromns(FXLONG(-885744000000000000)),1941,12,7,341,0,8,0,0,0);
  for(t=-FXLONG(900000000000000000); t<FXLONG(900000000000000000) && failures<10; t+=FXLONG(1234567890123457)){
    st=fromns(t);
    if(st.hour<0 || 23<st.hour || st.min<0 || 59<st.min || st.sec<0 || 59<st.sec || st.nano<0 || 999999999<st.nano) bad("fields out of range for %lld",t);
    if(st.yday<1 || 366<st.yday || st.wday<0 || 6<st.wday) bad("day fields out of range for %lld",t);
    if(FXSystem::timeFromSystemTime(st)!=t) bad("time %lld does not survive a round trip, got %lld",t,FXSystem::timeFromSystemTime(st));
    if(st.nano!=(t%sec+sec)%sec) bad("nanoseconds of %lld are %d",t,st.nano);
    }
  }


// Out of range parts are carried over
static void testnormalize(){
  struct { FXint y,mo,d,h,mi,s,n; FXTime expected; } cases[]={
    {2021, 1, 32, 0, 0,   0,          0, ns(2021,2,1)},
    {2021, 1,100, 0, 0,   0,          0, ns(2021,4,10)},
    {2021, 2, 29, 0, 0,   0,          0, ns(2021,3,1)},
    {2020,12, 32, 0, 0,   0,          0, ns(2021,1,1)},
    {2021,14,  1, 0, 0,   0,          0, ns(2022,2,1)},
    {2021, 3,  1,25, 0,   0,          0, ns(2021,3,2,1)},
    {2021, 3,  1, 0,75,   0,          0, ns(2021,3,1,1,15)},
    {2021, 3,  1, 0, 0,3661,          0, ns(2021,3,1,1,1,1)},
    {2021, 3,  1, 0, 0,   0,1500000000, ns(2021,3,1,0,0,1,500000000)},
    {2020,12, 31,23,59,  60,          0, ns(2021,1,1)}
    };
  FXuint i;
  for(i=0; i<ARRAYNUMBER(cases); i++){
    FXSystem::Time st={cases[i].y,cases[i].mo,cases[i].d,0,0,cases[i].h,cases[i].mi,cases[i].s,cases[i].n,0};
    FXTime t=FXSystem::timeFromSystemTime(st);
    if(t!=cases[i].expected) bad("normalizing %04d-%02d-%02d %02d:%02d:%02d.%09d: expected %lld got %lld",cases[i].y,cases[i].mo,cases[i].d,cases[i].h,cases[i].mi,cases[i].s,cases[i].n,cases[i].expected,t);
    }
  }


// Formatting with known results
static void testformat(){
  const FXTime t=FXLONG(1588006865264666745);           // Mon 2020-04-27 17:01:05.264666745 UTC
  struct { const FXchar* format; const FXchar* expected; } cases[]={
    {"%Y-%m-%d %H:%M:%S",   "2020-04-27 17:01:05"},
    {"%a %A %b %B %h",      "Mon Monday Apr April Apr"},
    {"%j %e %k %l",         "118 27 17  5"},
    {"%I %p %P",            "05 PM pm"},
    {"%y %C %G %g",         "20 20 2020 20"},
    {"%u %w %U %W %V",      "1 1 17 17 18"},
    {"%D %F %T %R %r",      "04/27/20 2020-04-27 17:01:05 17:01 05:01:05 PM"},
    {"%x|%X",               "Apr Mon 27|17:01:05"},
    {"%c",                  "Apr Mon 27 17:01:05 GMT 2020"},
    {"%fm %fu %fn",         "264 264666 264666745"},
    {"%s",                  "1588006865"},
    {"%z %Z",               "+0000 GMT"},
    {"100%% %n%t.",         "100% \n\t."},
    {"at %H o'clock",       "at 17 o'clock"},
    {"%Ey %Od",             "20 27"},
    {"",                    ""}
    };
  FXSystem::Time st=fromns(t);
  FXString expected,format;
  FXint i;
  FXuint k;
  for(k=0; k<ARRAYNUMBER(cases); k++){
    expectstr(cases[k].format,FXSystem::systemTimeFormat(st,cases[k].format),cases[k].expected);
    expectstr(cases[k].format,FXSystem::universalTime(t,cases[k].format),cases[k].expected);
    }

  // Time of day on a 12-hour clock
  st=fromns(ns(2020,4,27,0,30));  expectstr("12-hour clock 00:30",FXSystem::systemTimeFormat(st,"%I %l %p %P %k %H"),"12 12 AM am  0 00");
  st=fromns(ns(2020,4,27,5,0));   expectstr("12-hour clock 05:00",FXSystem::systemTimeFormat(st,"%I %l %p %k"),"05  5 AM  5");
  st=fromns(ns(2020,4,27,12,0));  expectstr("12-hour clock 12:00",FXSystem::systemTimeFormat(st,"%I %l %p %H"),"12 12 PM 12");
  st=fromns(ns(2020,4,27,13,5));  expectstr("12-hour clock 13:05",FXSystem::systemTimeFormat(st,"%l:%M %P"), " 1:05 pm");
  st=fromns(ns(2020,4,27,23,59)); expectstr("12-hour clock 23:59",FXSystem::systemTimeFormat(st,"%I:%M %p"),"11:59 PM");

  // Small years and days
  st=fromns(ns(1905,3,4));        expectstr("year 1905",FXSystem::systemTimeFormat(st,"%Y %y %C %e %d"),"1905 05 19  4 04");
  st=fromns(ns(2005,1,1));        expectstr("2005-01-01 weeks",FXSystem::systemTimeFormat(st,"%U %W %V %G"),"00 00 53 2004");
  st=fromns(ns(2006,1,1));        expectstr("2006-01-01 weeks",FXSystem::systemTimeFormat(st,"%U %W %V %G"),"01 00 52 2005");

  // Time zone offsets
  st=fromns(t);
  st.offset=19800;                expectstr("+05:30",FXSystem::systemTimeFormat(st,"%z"),"+0530");
  st.offset=-7200;                expectstr("-02:00",FXSystem::systemTimeFormat(st,"%z"),"-0200");
  st.offset=-1800;                expectstr("-00:30",FXSystem::systemTimeFormat(st,"%z"),"-0030");
  st.offset=3600;                 expectstr("+01:00",FXSystem::systemTimeFormat(st,FXSystem::isoTimeFormat),"2020-04-27T17:01:05+0100");

  // Fractions are truncated, they never carry into the digits above
  st=fromns(ns(2020,4,27,23,59,59,999999999));
  expectstr("fractions of .999999999",FXSystem::systemTimeFormat(st,"%S.%fm %S.%fu %S.%fn"),"59.999 59.999999 59.999999999");
  st=fromns(ns(2020,4,27,0,0,0,5000000));
  expectstr("fractions of .005",FXSystem::systemTimeFormat(st,"%fm %fu %fn"),"005 005000 005000000");

  // Long result
  for(i=0; i<40; i++){
    format+="%F ";
    expected+="2020-04-27 ";
    }
  expectstr("long format",FXSystem::universalTime(t,format.text()),expected.text());

  // Standard formats
  expectstr("default format",FXSystem::universalTime(t),"04/27/2020 17:01:05");
  expectstr("accurate format",FXSystem::universalTime(t,FXSystem::mainTimeFormat),"2020-04-27 17:01:05.264666");
  }


// Compare with the C library
static void testnative(){
  const FXchar* spec="%Y|%m|%d|%H|%M|%S|%j|%U|%W|%V|%G|%g|%u|%w|%a|%A|%b|%B|%y|%C|%e|%I|%p|%D|%F|%T|%R";
  const FXTime sec=1000000000;
  FXlong t;
  FXint count=0;
  for(t=0; t<FXLONG(4200000000) && count<5; t+=FXLONG(86400)+FXLONG(60317)){
    time_t tt=(time_t)t;
    struct tm* ptm=gmtime(&tt);
    FXchar buffer[512];
    if(!ptm) continue;
    if(strftime(buffer,sizeof(buffer),spec,ptm)==0) continue;
    FXString ours=FXSystem::universalTime(t*sec,spec);
    if(strcmp(ours.text(),buffer)!=0){
      bad("time %lld: C library \"%s\" FOX \"%s\"",t,buffer,ours.text());
      count++;
      }
    }
  }


// ISO week/year in both directions
static void testiso(){
  FXuint i;
  for(i=0; i<ARRAYNUMBER(week_number_cases); i++){
    FXString string=weeknumber(week_number_cases[i][0],week_number_cases[i][1],week_number_cases[i][2]);
    FXString expected;
    expected.format("    %s (doy=",isoweek_date_strings[i]);
    if(string.find(expected)<0) bad("ISO week of %04d-%02d-%02d: expected %s in \"%s\"",week_number_cases[i][0],week_number_cases[i][1],week_number_cases[i][2],isoweek_date_strings[i],string.text());
    FXSystem::Time st={0,0,0,0,0,0,0,0,0,0};
    FXint n=FXSystem::systemTimeParse(st,isoweek_date_strings[i],"%G-W%V-%u");
    if(n!=(FXint)strlen(isoweek_date_strings[i])) bad("ISO parse of %s consumed %d",isoweek_date_strings[i],n);
    if(st.year!=week_number_cases[i][0] || st.month!=week_number_cases[i][1] || st.mday!=week_number_cases[i][2]){
      bad("ISO parse of %s gave %04d-%02d-%02d, expected %04d-%02d-%02d",isoweek_date_strings[i],st.year,st.month,st.mday,week_number_cases[i][0],week_number_cases[i][1],week_number_cases[i][2]);
      }
    else{
      FXSystem::Time ref=fromns(ns(st.year,st.month,st.mday));
      if(st.yday!=ref.yday || st.wday!=ref.wday) bad("ISO parse of %s: yday=%d wday=%d, expected yday=%d wday=%d",isoweek_date_strings[i],st.yday,st.wday,ref.yday,ref.wday);
      }
    }
  }


// Parsing with known results
static void testparse(){
  const FXTime sec=1000000000;
  const FXTime t=ns(2020,4,27,17,1,5);
  const FXTime day=ns(2020,4,27);
  struct { const FXchar* format; const FXchar* string; FXTime expected; } cases[]={
    {"%Y-%m-%d %H:%M:%S",             "2020-04-27 17:01:05",                  t},
    {"%F %T",                         "2020-04-27 17:01:05",                  t},
    {"%Y/%m/%d %H:%M:%S",             "2020/4/27 17:1:5",                     t},
    {"%d %b %Y %H:%M:%S",             "27 Apr 2020 17:01:05",                 t},
    {"%A, %B %d, %Y %I:%M:%S %p",     "Monday, April 27, 2020 05:01:05 PM",   t},
    {"%Y-%m-%d %I:%M %p",             "2020-04-27 12:30 AM",                  day+30*60*sec},
    {"%Y-%m-%d %I:%M %p",             "2020-04-27 12:30 PM",                  day+(12*60+30)*60*sec},
    {"%FT%T%z",                       "2020-04-27T19:01:05+0200",             t},
    {"%FT%T%z",                       "2020-04-27T12:01:05-05:00",            t},
    {"%FT%T%z",                       "2020-04-27T17:01:05Z",                 t},
    {"%FT%T%z",                       "2020-04-27T14:31:05-0230",             t},
    {"%FT%T%z",                       "2020-04-27T17:01:05+00",               t},
    {"%F %T %Z",                      "2020-04-27 12:01:05 EST",              t},
    {"%F %T %Z",                      "2020-04-27 11:01:05 CST",              t},
    {"%F %T %Z",                      "2020-04-27 12:01:05 CDT",              t},
    {"%F %T %Z",                      "2020-04-27 09:01:05 PST",              t},
    {"%F %T %Z",                      "2020-04-27 17:01:05 UTC",              t},
    {"%F %T %Z",                      "2020-04-27 17:01:05 GMT",              t},
    {"%s",                            "1588006865",                           t},
    {"%s.%fn",                        "1588006865.264666745",                 t+264666745},
    {"%F %T.%fm",                     "2020-04-27 17:01:05.264",              t+264000000},
    {"%F %T.%fu",                     "2020-04-27 17:01:05.264666",           t+264666000},
    {"%F %T.%fn",                     "2020-04-27 17:01:05.264666745",        t+264666745},
    {"%Y %j",                         "2020 118",                             day},
    {"%Y %U %w",                      "2020 17 1",                            day},
    {"%Y %W %u",                      "2020 17 1",                            day},
    {"%G-W%V-%u",                     "2020-W18-1",                           day},
    {"%y-%m-%d",                      "20-04-27",                             day},
    {"%y-%m-%d",                      "70-01-01",                             0},
    {"%y-%m-%d",                      "69-12-31",                             ns(1969,12,31)},
    {"%y-%m-%d",                      "68-12-31",                             ns(2068,12,31)},
    {"%C%y-%m-%d",                    "1999-12-31",                           ns(1999,12,31)},
    {"%D",                            "04/27/20",                             day},
    {"%Y%m%d %H%M%S",                 "20200427 170105",                      t},
    {"%Y-%m-%d%n%H:%M",               "2020-04-27 \t 17:01",                  t-5*sec},
    {"%Y-%m-%d %H:%M:%S",             "2020-02-29 23:59:60",                  ns(2020,3,1)},
    {"%b %e %Y",                      "Feb  7 2020",                          ns(2020,2,7)},
    {"%B %d %Y",                      "february 07 2020",                     ns(2020,2,7)}
    };
  const FXchar* failing[][2]={
    {"%Y/%m/%d",    "2020/13/01"},
    {"%Y/%m/%d",    "2020/00/01"},
    {"%Y/%m/%d",    "2020/04/32"},
    {"%Y/%m/%d",    "2020/04/00"},
    {"%Y-%m-%d",    "2020-04"},
    {"%Y-%m-%d",    "2020.04.27"},
    {"%H:%M",       "24:00"},
    {"%Y",          "abc"},
    {"%b",          "Foo"},
    {"%p",          "XM"},
    {"%FT%T%z",     "2020-04-27T17:01:05+0"},
    {"%FT%T%z",     "2020-04-27T17:01:05+0260"},
    {"%FT%T%z",     "2020-04-27T17:01:05 0100"},
    {"%s",          "x"},
    {"%s",          "99999999999999999999"},
    {"%d",          "0"},
    {"%Q",          "1"}
    };
  FXuint k;
  FXSystem::Time st;
  for(k=0; k<ARRAYNUMBER(cases); k++){
    FXTime r=FXSystem::universalTime(cases[k].string,cases[k].format);
    if(r!=cases[k].expected) bad("parse \"%s\" with \"%s\": expected %lld got %lld",cases[k].string,cases[k].format,cases[k].expected,r);
    }
  for(k=0; k<ARRAYNUMBER(failing); k++){
    FXTime r=FXSystem::universalTime(failing[k][1],failing[k][0]);
    if(r!=forever) bad("parse \"%s\" with \"%s\" should fail but gave %lld",failing[k][1],failing[k][0],r);
    }

  // Count of characters consumed, and fields
  st=fromns(0);
  expectnum("consumed",FXSystem::systemTimeParse(st,"2020-04-27 trailing","%F"),10);
  expectnum("consumed none",FXSystem::systemTimeParse(st,"nothing","%F"),0);
  expectnum("zone offset +0300",(FXlong)(FXSystem::systemTimeParse(st,"+0300","%z"),st.offset),10800);
  expectnum("zone offset -03",(FXlong)(FXSystem::systemTimeParse(st,"-03","%z"),st.offset),-10800);
  expectnum("zone offset CST",(FXlong)(FXSystem::systemTimeParse(st,"CST","%Z"),st.offset),-21600);
  FXSystem::systemTimeParse(st,"2020-04-27 17:01:05","%F %T");
  expectparts("parsed fields",st,2020,4,27,118,1,17,1,5,0);

  // Parse what we print
  const FXchar* formats[]={"%Y-%m-%d %H:%M:%S","%FT%T%z","%m/%d/%Y %H:%M:%S","%A %B %d %Y %I:%M:%S %p","%G-W%V-%u %T","%Y %j %T","%Y %U %w %T","%Y %W %u %T","%s"};
  const FXchar* parseback[]={"%Y-%m-%d %H:%M:%S","%FT%T%z","%m/%d/%Y %H:%M:%S","%A %B %d %Y %I:%M:%S %p","%G-W%V-%u %T","%Y %j %T","%Y %U %w %T","%Y %W %u %T","%s"};
  FXlong u;
  FXuint f;
  for(u=-FXLONG(2000000000); u<FXLONG(4000000000) && failures<20; u+=FXLONG(86400)*37+FXLONG(4021)){
    for(f=0; f<ARRAYNUMBER(formats); f++){
      FXString printed=FXSystem::universalTime(u*sec,formats[f]);
      FXTime   parsed=FXSystem::universalTime(printed,parseback[f]);
      if(parsed!=u*sec) bad("\"%s\" of %lld printed \"%s\" parsed to %lld",formats[f],u*sec,printed.text(),parsed);
      }
    }
  }


// Local time and zone offset are consistent
static void testlocal(){
  const FXTime sec=1000000000;
  const FXTime samples[]={ns(2020,1,15,12),ns(2020,7,15,12),ns(1999,12,31,23,59,59),ns(2038,6,1,3,2,1),FXLONG(1588006865264666745)};
  FXuint k;
  for(k=0; k<ARRAYNUMBER(samples); k++){
    FXTime t=samples[k];
    FXTime zone=FXSystem::localTimeZoneOffset()+FXSystem::daylightSavingsOffset()*FXSystem::daylightSavingsActive(t);    // Seconds west of UTC
    FXint  east=-(FXint)(zone/sec);
    FXint  mag=east<0?-east:east;
    FXchar expected[32];
    snprintf(expected,sizeof(expected),"%c%02d%02d",east<0?'-':'+',(mag/60)/60,(mag/60)%60);
    expectstr("local zone offset",FXSystem::localTime(t,"%z"),expected);

    // Local time is utc with offset applied
    FXString local=FXSystem::localTime(t,"%FT%T");
    FXString shifted=FXSystem::universalTime(t+east*sec,"%FT%T");
    expectstr("local time",local,shifted.text());

    // Printing local time and parsing it back gives the time, without the fraction
    FXSystem::Time st=fromns(t);
    FXTime whole=t-st.nano;
    FXString printed=FXSystem::localTime(t,FXSystem::isoTimeFormat);
    FXTime parsed=FXSystem::localTime(printed,FXSystem::isoTimeFormat);
    if(parsed!=whole) bad("local time \"%s\" of %lld parsed to %lld, expected %lld",printed.text(),t,parsed,whole);
    parsed=FXSystem::universalTime(printed,FXSystem::isoTimeFormat);
    if(parsed!=whole) bad("universal time \"%s\" of %lld parsed to %lld, expected %lld",printed.text(),t,parsed,whole);
    }
  }


// Self-check
static int selftest(){
  testcivil();
  testparts();
  testnormalize();
  testformat();
  testnative();
  testiso();
  testparse();
  testlocal();
  if(failures){
    printf("FAILED: %d check(s)\n",failures);
    return 1;
    }
  printf("all checks passed\n");
  return 0;
  }

// Print options
void printusage(const char* prog){
  fxmessage("%s options:\n",prog);
  fxmessage("  -h, --help               Print help.\n");
  fxmessage("  --tracelevel <level>     Set trace level.\n");
  fxmessage("  --year <year>            Set the year.\n");
  fxmessage("  --month <month>          Set the month (1..12).\n");
  fxmessage("  --mday <mday>            Set the day of month (1..31).\n");
  fxmessage("  --hour <hour>            Set the hour (0..23).\n");
  fxmessage("  --min <min>              Set the minute (0..59).\n");
  fxmessage("  --sec <sec>              Set the second (0..59).\n");
  fxmessage("  --nano <nano>            Set the nano second (0..999999999).\n");
  fxmessage("  --format <format>        Set format.\n");
  fxmessage("  --to-iso                 Test conversion to iso-week/day.\n");
  fxmessage("  --from-iso               Test converstion from iso-week/day.\n");
  }


// Start
int main(int argc,char* argv[]){
  fxtestargs(argc,argv);
  if(2<=argc && strcmp(argv[1],"--test")==0) return selftest();
  const FXTime seconds=1000000000;
  const FXTime DAY=86400*seconds;
  FXint  year,month,day;
  FXTime days;
  FXString string;
  FXTime z,ct0,ct1,tzoff,dsoff;
  FXuval x,y;

  // Current universal time
  ct0=FXThread::time();

  // Time zone offset
  tzoff=FXSystem::localTimeZoneOffset();

  // Daylight savings offset
  dsoff=FXSystem::daylightSavingsOffset();

  // Current local time
  ct1=ct0-tzoff-dsoff;

  // System Time in parts
  FXSystem::Time st={0,0,0,0,0,0,0,0,0,0};

  // Init structure
  FXSystem::systemTimeFromTime(st,ct0);

  // Grab a few arguments
  for(FXint arg=1; arg<argc; ++arg){
    if(strcmp(argv[arg],"-h")==0 || strcmp(argv[arg],"--help")==0){
      printusage(argv[0]);
      return 0;
      }
    else if(strcmp(argv[arg],"--tracelevel")==0){
      if(++arg>=argc){ fxmessage("Missing --tracelevel argument.\n"); exit(1); }
      setTraceLevel(strtoul(argv[arg],nullptr,0));
      }
    else if(strcmp(argv[arg],"--year")==0){
      if(++arg>=argc){ fxmessage("Missing --year argument.\n"); exit(1); }
      st.year=strtoul(argv[arg],nullptr,0);
      }
    else if(strcmp(argv[arg],"--month")==0){
      if(++arg>=argc){ fxmessage("Missing --month argument.\n"); exit(1); }
      st.month=strtoul(argv[arg],nullptr,0);
      }
    else if(strcmp(argv[arg],"--mday")==0){
      if(++arg>=argc){ fxmessage("Missing --mday argument.\n"); exit(1); }
      st.mday=strtoul(argv[arg],nullptr,0);
      }
    else if(strcmp(argv[arg],"--yday")==0){
      if(++arg>=argc){ fxmessage("Missing --mday argument.\n"); exit(1); }
      st.yday=strtoul(argv[arg],nullptr,0);
      }
    else if(strcmp(argv[arg],"--hour")==0){
      if(++arg>=argc){ fxmessage("Missing --mday argument.\n"); exit(1); }
      st.hour=strtoul(argv[arg],nullptr,0);
      }
    else if(strcmp(argv[arg],"--min")==0){
      if(++arg>=argc){ fxmessage("Missing --mday argument.\n"); exit(1); }
      st.min=strtoul(argv[arg],nullptr,0);
      }
    else if(strcmp(argv[arg],"--sec")==0){
      if(++arg>=argc){ fxmessage("Missing --mday argument.\n"); exit(1); }
      st.sec=strtoul(argv[arg],nullptr,0);
      }
    else if(strcmp(argv[arg],"--nano")==0){
      if(++arg>=argc){ fxmessage("Missing --mday argument.\n"); exit(1); }
      st.nano=strtoul(argv[arg],nullptr,0);
      }
    else if(strcmp(argv[arg],"--offset")==0){
      if(++arg>=argc){ fxmessage("Missing --mday argument.\n"); exit(1); }
      st.offset=strtoul(argv[arg],nullptr,0);
      }
    else if(strcmp(argv[arg],"--string")==0){
      if(++arg>=argc){ fxmessage("Missing --string argument.\n"); exit(1); }
      string=argv[arg];
      }
    else if(strcmp(argv[arg],"--format")==0){
      if(++arg>=argc){ fxmessage("Missing --format argument.\n"); exit(1); }
      string=FXSystem::systemTimeFormat(st,argv[arg]);
      fxmessage("format=%-16s output=%s\n",argv[arg],string.text());
      }
    else if(strcmp(argv[arg],"--to-iso")==0){           // Test ISO week and year
      for(y=0; y<ARRAYNUMBER(week_number_cases); y++){
        string=weeknumber(week_number_cases[y][0],week_number_cases[y][1],week_number_cases[y][2]);
        fxmessage("%s\n",string.text());
        }
      fxmessage("\n");
      return 0;
      }
    else if(strcmp(argv[arg],"--from-iso")==0){         // Test ISO week and year
      for(x=0; x<ARRAYNUMBER(isoweek_date_strings); x++){
        clearElms(&st,1);
        FXSystem::systemTimeParse(st,isoweek_date_strings[x],"%G-W%V-%u");
        z=FXSystem::timeFromSystemTime(st);
        fxmessage("%-16s -> ",isoweek_date_strings[x]);
        fxmessage("%04d/%02d/%02d %02d:%02d:%02d  (wday=%1d  yday=%03d)\n",st.year,st.month,st.mday,st.hour,st.min,st.sec,st.wday,st.yday);
        }
      return 0;
      }
    else{
      printusage(argv[0]);
      return 0;
      }
    }

#if 1
  // Calculate nanoseconds
  ct0=FXSystem::timeFromSystemTime(st);

  // Dump original and updated
  fxmessage("ct0=%'-16lld\n",ct0);
  fxmessage("ct1=%'-16lld\n",ct1);
  fxmessage("\n");

  fxmessage("tzoff=%'lld\n",tzoff);
  fxmessage("dsoff=%'lld\n",dsoff);
  fxmessage("\n");

  // Format every which way
  for(x=0; x<ARRAYNUMBER(timeformat); x++){
    string=FXSystem::localTime(ct0,timeformat[x]);
    fxmessage("format=%-16s output=%s\n",timeformat[x],string.text());
    }
  fxmessage("\n");
#endif

#if 1
  // Test new civilFromDays() and daysFromCivil()
  fxmessage("*** TEST civilFromDays() and daysFromCivil() ***\n");
  for(y=0; y<ARRAYNUMBER(timevalues); y++){
    days=(timevalues[y]>=0 ? timevalues[y] : (timevalues[y]-(DAY-1)))/DAY;
    FXSystem::civilFromDays(year,month,day,days);
    fxmessage("%12ld - > %04d/%02d/%02d  ",days,year,month,day);
    days=FXSystem::daysFromCivil(year,month,day);
    fxmessage("%04d/%02d/%02d - > %12ld\n",year,month,day,days);
    }
  fxmessage("\n");
#endif

#if 1
  // Test ISO week number and year
  fxmessage("*** TEST ISO Week Number ***\n");
  for(y=0; y<ARRAYNUMBER(week_number_cases); y++){
    string=weeknumber(week_number_cases[y][0],week_number_cases[y][1],week_number_cases[y][2]);
    fxmessage("%s\n",string.text());
    }
  fxmessage("\n");
#endif

#if 1
  // Testing time formats
  fxmessage("*** TEST Time Format ***\n");
  for(y=0; y<ARRAYNUMBER(timevalues); y++){
    fxmessage("time=%'-16lld\n",timevalues[y]);
    for(x=0; x<ARRAYNUMBER(timeformat); x++){
//      string=FXSystem::universalTime(timevalues[y],timeformat[x]);
      string=FXSystem::localTime(timevalues[y],timeformat[x]);
      fxmessage("format=%-16s output=%s\n",timeformat[x],string.text());
      }
    fxmessage("\n");
    }
#endif

#if KUL
  for(x=0; x<ARRAYNUMBER(timeparse); x++){
    clearElms(&st,1);
    FXSystem::systemTimeParse(st,timestrings[x],timeparse[x]);
    z=FXSystem::timeFromSystemTime(st);
    fxmessage("format=%-16s input=%-16s -> ",timeparse[x],timestrings[x]);
    string=FXSystem::localTime(z,"date: %Y/%m/%d time: %H:%M:%S wday: %w yday: %j week: %U epoch: %s");
    fxmessage("%s \n",string.text());
    }
  fxmessage("\n");
#endif

#if 1
  // Test time parsing
  fxmessage("*** TEST Time Parsing ***\n");
  for(x=0; x<ARRAYNUMBER(timeparse); x++){
    clearElms(&st,1);
    FXSystem::systemTimeParse(st,timestrings[x],timeparse[x]);
    z=FXSystem::timeFromSystemTime(st);
    fxmessage("format=%-16s input=%-16s -> ",timeparse[x],timestrings[x]);
    fxmessage("%04d/%02d/%02d %02d:%02d:%02d.%09d  (wday=%1d  yday=%03d gmt=%6d z=%lld)\n",st.year,st.month,st.mday,st.hour,st.min,st.sec,st.nano,st.wday,st.yday,st.offset,z);
    }
  fxmessage("\n");
#endif

#if 1
  // Test Daylight Savings / Standard Time switch
  fxmessage("*** TEST Daylight Savings / Standard Time switch ***\n");
  st.year=2020;
  st.month=1;
  st.mday=1;
  st.hour=0;
  st.min=0;
  st.sec=0;
  st.nano=0;
  z=timeFromSystemTime(st);
  for(FXint d=0; d<=365; ++d){
    FXSystem::systemTimeFromTime(st,z);
    string=FXSystem::systemTimeFormat(st,"%Y-%m-%d %H:%M:%S");
    fxmessage("date=%s dst=%lld\n",string.text(),FXSystem::daylightSavingsActive(z));
    z+=DAY;
    }
  fxmessage("\n");
#endif

  return 0;
  }

