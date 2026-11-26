/********************************************************************************
*                                                                               *
*                         Test Console Line Buffer                              *
*                                                                               *
********************************************************************************/
#include "fx.h"
#include <stdio.h>

using namespace FX;

static FXbool check(FXbool condition,const FXchar* message){
  if(!condition){
    fprintf(stderr,"FAIL: %s\n",message);
    return false;
    }
  return true;
  }

int main(){
  FXConsoleBuffer buffer;
  FXbool ok=true;
  buffer.setHistoryLines(50);
  for(FXint i=0; i<100; i++){
    FXString line;
    line.format("line-%d",i);
    buffer.appendLine(line);
    }
  ok=check(buffer.getNumLines()==50,"capacity keeps 50 lines") && ok;
  ok=check(buffer.getLineText(0)=="line-50","oldest retained line") && ok;
  ok=check(buffer.getLineText(49)=="line-99","newest retained line") && ok;

  buffer.clear();
  buffer.appendText("par");
  buffer.appendText("tial\nnext");
  ok=check(buffer.getNumLines()==2,"newline splits lines") && ok;
  ok=check(buffer.getLineText(0)=="partial","partial line continues across appends") && ok;
  ok=check(buffer.getLineText(1)=="next","unterminated final line is retained") && ok;

  buffer.clear();
  buffer.appendText("a",3);
  FXuint styles[]={4,5};
  buffer.appendStyled("bc",styles);
  ok=check(buffer.getLineText(0)=="abc","styled append preserves text") && ok;
  ok=check(buffer.getStyle(0,0)==3 && buffer.getStyle(0,1)==4 && buffer.getStyle(0,2)==5,"styles stored per byte") && ok;

  buffer.clear();
  buffer.appendLine("alpha");
  buffer.appendLine("beta");
  buffer.appendLine("gamma");
  ok=check(buffer.getRange(0,2,1,2)=="pha\nbe","multi-line half-open range extraction") && ok;
  ok=check(buffer.getRange(1,2,0,2)=="pha\nbe","reverse endpoints normalize correctly") && ok;
  ok=check(buffer.getRange(0,0,2,5)=="alpha\nbeta\ngamma","whole-buffer range extraction") && ok;
  ok=check(buffer.getRange(1,2,1,2).empty(),"empty range extracts empty text") && ok;

  buffer.setHistoryLines(2);
  buffer.appendLine("delta");
  ok=check(buffer.getNumLines()==2 && buffer.getLineText(0)=="gamma","range model remains consistent after ring eviction") && ok;
  ok=check(buffer.getRange(0,0,1,5)=="gamma\ndelta","range uses retained logical line indices") && ok;

  return ok ? 0 : 1;
  }