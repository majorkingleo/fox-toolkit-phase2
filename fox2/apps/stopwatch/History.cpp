#include "History.h"

History::History(){
  count=0;
  }


void History::clear(){
  count=0;
  }


void History::add(FXTime when,FXTime elapsed){
  FXint i=FXMIN(count,MAXENTRIES-1);
  for(;i>0;i--) entries[i]=entries[i-1];
  entries[0].when=when;
  entries[0].elapsed=elapsed;
  if(count<MAXENTRIES) count++;
  }


void History::load(FXSettings& reg){
  FXint i;
  count=FXCLAMP(0,(FXint)reg.readIntEntry("STOPWATCH","count",0),MAXENTRIES);
  for(i=0;i<count;i++){
    FXString key;
    key.format("when%d",i);
    entries[i].when=reg.readLongEntry("STOPWATCH",key,0);
    key.format("elapsed%d",i);
    entries[i].elapsed=reg.readLongEntry("STOPWATCH",key,0);
    }
  }


void History::save(FXSettings& reg) const{
  FXint i;
  reg.writeIntEntry("STOPWATCH","count",count);
  for(i=0;i<count;i++){
    FXString key;
    key.format("when%d",i);
    reg.writeLongEntry("STOPWATCH",key,entries[i].when);
    key.format("elapsed%d",i);
    reg.writeLongEntry("STOPWATCH",key,entries[i].elapsed);
    }
  // Remove rows left over from a longer history
  for(i=count;i<MAXENTRIES;i++){
    FXString key;
    key.format("when%d",i);
    reg.deleteEntry("STOPWATCH",key);
    key.format("elapsed%d",i);
    reg.deleteEntry("STOPWATCH",key);
    }
  }


// "MM:SS.hh" below one hour, "HH:MM:SS.hh" from one hour on
FXString History::formatElapsed(FXTime ns){
  FXString text;
  if(ns<0) ns=0;
  FXlong total=ns/10000000;     // hundredths of a second
  FXint hh=(FXint)(total/360000);
  FXint mm=(FXint)((total/6000)%60);
  FXint ss=(FXint)((total/100)%60);
  FXint hs=(FXint)(total%100);
  if(hh>0)
    text.format("%02d:%02d:%02d.%02d",hh,mm,ss,hs);
  else
    text.format("%02d:%02d.%02d",mm,ss,hs);
  return text;
  }


FXString History::formatWhen(FXTime when){
  return FXSystem::localTime(when,"%Y-%m-%d %H:%M:%S");
  }
