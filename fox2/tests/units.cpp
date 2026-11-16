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

  - With --test, a built-in table of conversions with expected values is checked,
    both directions of each conversion, conversions which must be refused, the
    dimension vectors and the table order; the exit code is 0 only if all hold.

*/

/*******************************************************************************/

// Conversion of value from unit fm to unit to gives expected
struct UnitCase {
  const FXchar* fm;
  const FXchar* to;
  FXdouble      value;
  FXdouble      expected;
  };


static const UnitCase cases[]={
  {"m","cm",1.0,100.0},
  {"km","m",2.0,2000.0},
  {"mm","m",1500.0,1.5},
  {"in","cm",1.0,2.54},
  {"ft","in",1.0,12.0},
  {"yd","ft",1.0,3.0},
  {"mi","km",1.0,1.609344},
  {"min","s",1.0,60.0},
  {"h","s",1.0,3600.0},
  {"day","h",1.0,24.0},
  {"kg","g",1.0,1000.0},
  {"lb","kg",1.0,0.45359237},
  {"oz","g",1.0,28.349523125},
  {"L","mL",1.0,1000.0},
  {"gal","L",1.0,3.785411784},
  {"atm","Pa",1.0,101325.0},
  {"bar","kPa",1.0,100.0},
  {"psi","Pa",1.0,6894.757293168},
  {"cal","J",1.0,4.1868},
  {"kW","W",1.5,1500.0},
  {"hp","W",1.0,745.69987158227},
  {"kph","mph",1.0,0.62137119223733},
  {"mph","m/s",1.0,0.44704},
  {"eV","J",1.0,1.602176634E-19},
  {"\xC2\xB0""C","K",0.0,273.15},
  {"\xC2\xB0""C","\xC2\xB0""F",100.0,212.0},
  {"\xC2\xB0""F","\xC2\xB0""C",32.0,0.0},
  {nullptr,nullptr,0.0,0.0}
  };


// Units which must not convert to each other
static const FXchar *const refused[][2]={
  {"m","s"},
  {"kg","m"},
  {"W","J"},
  {"mph","m"}
  };


// Relative closeness
static FXbool closeto(FXdouble a,FXdouble b){
  return Math::fabs(a-b)<=1E-9*Math::fabs(b) || Math::fabs(a-b)<=1E-12;
  }


// Table should be remain sorted
FXbool checktable(){
  for(FXuint u=1; u<FXUnits::NumUnits; ++u){
    if(strcmp(FXUnits::symbol[u],FXUnits::symbol[u-1])<=0){
      fxmessage("Units table: %s > %s is false\n",FXUnits::symbol[u],FXUnits::symbol[u-1]);
      return false;
      }
    }
  fxmessage("Units table is OK\n");
  return true;
  }


// Run the tables, return the number of failed checks
static FXint testunits(){
  FXint failures=0;
  FXUnits units,back;
  FXdouble result;
  FXint i;

  // Table order is needed for the look-up
  if(!checktable()){ printf("BAD: units table is not sorted\n"); failures++; }

  // Every symbol is found again
  for(FXint u=0; u<FXUnits::NumUnits; u++){
    if(FXUnits::lookup(FXUnits::symbol[u])!=u){
      printf("BAD: symbol %d (%s) is not found by lookup\n",u,FXUnits::symbol[u]);
      failures++;
      }
    }
  if(FXUnits::lookup("nosuchunit")!=-1){ printf("BAD: unknown unit is found\n"); failures++; }
  if(FXUnits::span("km")!=2){ printf("BAD: span(km) is %d\n",FXUnits::span("km")); failures++; }
  if(FXUnits::span("m/s")!=3){ printf("BAD: span(m/s) is %d\n",FXUnits::span("m/s")); failures++; }

  // Conversions, in both ways of asking
  for(i=0; cases[i].fm; i++){
    const UnitCase& c=cases[i];
    units=FXUnits::convertFromTo(c.fm,c.to);
    if(!units){
      printf("BAD: %s -> %s is refused\n",c.fm,c.to);
      failures++;
      continue;
      }
    result=units(c.value);
    if(!closeto(result,c.expected)){
      printf("BAD: %.14g %s -> %s gives %.14g, expected %.14g\n",c.value,c.fm,c.to,result,c.expected);
      failures++;
      }
    back=FXUnits::convertToFrom(c.to,c.fm);
    if(!back || !closeto(back(c.value),result)){
      printf("BAD: convertToFrom(%s,%s) disagrees with convertFromTo\n",c.to,c.fm);
      failures++;
      }
    if(!closeto(units.invert()(result),c.value)){
      printf("BAD: %s -> %s does not invert\n",c.fm,c.to);
      failures++;
      }
    }

  // Conversions between different kinds of quantity
  for(i=0; i<(FXint)(sizeof(refused)/sizeof(refused[0])); i++){
    units=FXUnits::convertFromTo(refused[i][0],refused[i][1]);
    if(units){
      printf("BAD: %s -> %s is not refused\n",refused[i][0],refused[i][1]);
      failures++;
      }
    }

  // Dimensions
  if(FXUnits::dimensions("m")!=FXUnits::dimensions("in")){ printf("BAD: m and in differ in dimensions\n"); failures++; }
  if(FXUnits::dimensions("m")!=FXUnits::dimensions("km")){ printf("BAD: m and km differ in dimensions\n"); failures++; }
  if(FXUnits::dimensions("m")==FXUnits::dimensions("s")){ printf("BAD: m and s have the same dimensions\n"); failures++; }
  if(FXUnits::dimensions("W")!=FXUnits::dimensions("hp")){ printf("BAD: W and hp differ in dimensions\n"); failures++; }
  if(FXUnits::canonical(FXUnits::dimensions("J"))!=FXUnits::canonical("J")){ printf("BAD: canonical units of J differ\n"); failures++; }
  return failures;
  }

// Print options
void printusage(const FXchar* prog){
  fxmessage("%s options:\n",prog);
  fxmessage("  --check-table            Check table.\n");
  fxmessage("  --dump-table             Dump table.\n");
  fxmessage("  --test                   Check built-in conversions.\n");
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
    else if(FXString::compare(argv[arg],"--test")==0){
      FXint failures=testunits();
      if(failures){
        printf("FAILED: %d check(s)\n",failures);
        return 1;
        }
      printf("all checks passed\n");
      return 0;
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
