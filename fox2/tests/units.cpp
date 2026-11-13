/********************************************************************************
*                                                                               *
*                        U n i t - C o n v e r s i o n s                        *
*                                                                               *
*********************************************************************************
* Copyright (C) 2026 by Jeroen van der Zijp.   All Rights Reserved.             *
********************************************************************************/
#include "fx.h"
#include "testargs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
  Notes:

  - Units test program.  The new FXUnits feature in FOX provides general
    physics conversion.

  - You can convert units between any arbitrary expression of S.I. and
    other common units, and any other expression.

  - FXUnits tries to match dimensions if both source-units and destination-
    units are specified.

  - When converting from (or to) S.I. units, you should be more careful as
    only the destination (or source) unit's dimensions are determined.

  - FXUnits also knows about common prefixes such as G[iga], M[ega], etc.

  - Data have been obtained from recent N.I.S.T. publications and S.I.
    standards organization.  However, if you spot problems please communicate
    these to jeroen@fox-toolkit.com.

*/

/*******************************************************************************/

// Print options
void printusage(const FXchar* prog){
  fxmessage("%s options:\n",prog);
  fxmessage("  --check-table            Check table.\n");
  fxmessage("  --dump-table             Dump table.\n");
  fxmessage("  --value <value>          Start value.\n");
  fxmessage("  --from <units>           Increment value .\n");
  fxmessage("  --to <units>             Add increment this many times.\n");
  fxmessage("  --dims <units>           Print dimensions-vector for units.\n");
  fxmessage("  --units <dims>           Print units for dimensions-vector.\n");
  fxmessage("  --canonical <units>      Print canonical S.I. units for given input units.\n");
  fxmessage("  --span <units>           Dump every so many times.\n");
  fxmessage("  --trace-topics <topics>  Set trace topics.\n");
  fxmessage("  -h, --help               Print help.\n");
  }


// Table should be remain sorted
void checktable(){
  for(FXuint u=1; u<FXUnits::NumUnits; ++u){
    if(strcmp(FXUnits::symbol[u],FXUnits::symbol[u-1])<=0){
      fxmessage("Units table: %s > %s is false\n",FXUnits::symbol[u],FXUnits::symbol[u-1]);
      return;
      }
    }
  fxmessage("Units table is OK\n");
  }


// Dump table
void dumptable(){
  for(FXuint u=0; u<FXUnits::NumUnits; ++u){
    fxmessage("%5d\t%-10s\t%-20.16lg\t%-18s\n",u,FXUnits::symbol[u],FXUnits::factor[u],FXUnits::expression[u]);
    }
  }


// Build table
int main(int argc,char *argv[]){
  fxtestargs(argc,argv);
  FXdouble originalvalue=1.0;
  FXdouble convertedvalue=1.0;
  FXString fm_unit;
  FXString to_unit;
  FXString string;
  FXint    length=0;
  FXulong  dims;


  // Grab a few arguments
  for(int arg=1; arg<argc; ++arg){
    if(FXString::compare(argv[arg],"-h")==0 || FXString::compare(argv[arg],"--help")==0){
      printusage(argv[0]);
      return 0;
      }
    else if(FXString::compare(argv[arg],"--trace-topics")==0){
      if(++arg>=argc){ fxwarning("Missing argument for --trace-topics option.\n"); return 1; }
      setTraceTopics(argv[arg]);
      }
    else if(FXString::compare(argv[arg],"--check-table")==0){
      checktable();
      }
    else if(FXString::compare(argv[arg],"--dump-table")==0){
      dumptable();
      }
    else if(FXString::compare(argv[arg],"--value")==0){
      if(++arg>=argc){ fxwarning("Missing argument for --value option.\n"); return 1; }
      originalvalue=strtod(argv[arg],nullptr);
      }
    else if(FXString::compare(argv[arg],"--from")==0){
      if(++arg>=argc){ fxwarning("Missing argument for --from option.\n"); return 1; }
      fm_unit=argv[arg];
      }
    else if(FXString::compare(argv[arg],"--to")==0){
      if(++arg>=argc){ fxwarning("Missing argument for --to option.\n"); return 1; }
      to_unit=argv[arg];
      }
    else if(FXString::compare(argv[arg],"--dims")==0){
      if(++arg>=argc){ fxwarning("Missing argument for --dims option.\n"); return 1; }
      dims=FXUnits::dimensions(argv[arg]);
      fxmessage("dimensions(%s) = 0x%012llx\n",argv[arg],dims);
      }
    else if(FXString::compare(argv[arg],"--units")==0){
      if(++arg>=argc){ fxwarning("Missing argument for --units option.\n"); return 1; }
      dims=strtoull(argv[arg],nullptr,0);
      fxmessage("units(0x%012llx) = %s\n",dims,FXUnits::canonical(dims).text());
      }
    else if(FXString::compare(argv[arg],"--canonical")==0){
      if(++arg>=argc){ fxwarning("Missing argument for --canonical option.\n"); return 1; }
      fxmessage("canonical(%s) = %s\n",argv[arg],FXUnits::canonical(argv[arg]).text());
      }
    else if(FXString::compare(argv[arg],"--span")==0){
      if(++arg>=argc){ fxwarning("Missing argument for --span option.\n"); return 1; }
      length=FXUnits::span(argv[arg]);
      fxmessage("span(%s) = %d\n",argv[arg],length);
      }
    else{
      fxwarning("Bad argument.\n");
      printusage(argv[0]);
      return 1;
      }
    }

  // Conversion
  if(!fm_unit.empty() || !to_unit.empty()){
    FXUnits units;

    // Conversion from fm_unit to to_unit (and back)
    if(!fm_unit.empty() && !to_unit.empty()){

      units=FXUnits::convertFromTo(fm_unit,to_unit);
      if(units){
        convertedvalue=units(originalvalue);
        fxmessage("value %.14lg %s -> %.14lg %s [%s]\n",originalvalue,fm_unit.text(),convertedvalue,to_unit.text(),FXUnits::canonical(fm_unit).text());
        }
      else{
        fxmessage("failed to convert: %s [%s] -> %s [%s]\n",fm_unit.text(),FXUnits::canonical(fm_unit).text(),to_unit.text(),FXUnits::canonical(to_unit).text());
        }

      units=FXUnits::convertToFrom(to_unit,fm_unit);
      if(units){
        convertedvalue=units(originalvalue);
        fxmessage("value %.14lg %s -> %.14lg %s [%s]\n",originalvalue,fm_unit.text(),convertedvalue,to_unit.text(),FXUnits::canonical(fm_unit).text());
        }
      else{
        fxmessage("failed to convert: %s [%s] -> %s [%s]\n",fm_unit.text(),FXUnits::canonical(fm_unit).text(),to_unit.text(),FXUnits::canonical(to_unit).text());
        }
      }

    // Convert from fm_unit to S.I.
    else if(!fm_unit.empty()){
      units=FXUnits::convertFromTo(fm_unit);
      if(units){
        convertedvalue=units(originalvalue);
        fxmessage("value %.14lg %s -> %.14lg %s [S.I.]\n",originalvalue,fm_unit.text(),convertedvalue,FXUnits::canonical(fm_unit).text());
        }
      else{
        fxmessage("failed to convert from: %s [%s]\n",fm_unit.text(),FXUnits::canonical(fm_unit).text());
        }
      }

    // Convert from S.I. to to_unit
    else if(!to_unit.empty()){
      units=FXUnits::convertToFrom(to_unit);
      if(units){
        convertedvalue=units(originalvalue);
        fxmessage("value %.14lg %s [S.I.] -> %.14lg %s\n",originalvalue,FXUnits::canonical(to_unit).text(),convertedvalue,to_unit.text());
        }
      else{
        fxmessage("failed to convert to: %s [%s]\n",to_unit.text(),FXUnits::canonical(to_unit).text());
        }
      }
    }

  return 0;
  }
