/********************************************************************************
*                                                                               *
*                        Unicode  Character  Database  Tests                    *
*                                                                               *
*********************************************************************************
* Copyright (C) 2011,2026 by Jeroen van der Zijp.   All Rights Reserved.        *
********************************************************************************/
#include "fx.h"
#include "testargs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>



const FXchar *const category[]={
  "Cn",
  "Cc",
  "Cf",
  "Cs",
  "Co",
  "Mn",
  "Mc",
  "Me",
  "Zs",
  "Zl",
  "Zp",
  "Lu",
  "Ll",
  "Lt",
  "Lm",
  "Lo",
  "Nl",
  "Nd",
  "No",
  "Pc",
  "Pd",
  "Ps",
  "Pe",
  "Pi",
  "Pf",
  "Po",
  "Sm",
  "Sc",
  "Sk",
  "So"
  };


const FXchar *const direction[]={
  "L",
  "LRE",
  "LRO",
  "R",
  "AL",
  "RLE",
  "RLO",
  "PDF",
  "EN",
  "ES",
  "ET",
  "AN",
  "CS",
  "NSM",
  "BN",
  "B",
  "S",
  "WS",
  "ON"
  };

const FXchar *const linebreak[]={
  "XX",

  "BK",
  "CR",
  "LF",
  "CM",
  "NL",
  "SG",
  "WJ",
  "ZW",
  "GL",
  "SP",

  "B2",
  "BA",
  "BB",
  "HY",
  "CB",

  "CL",
  "CP",
  "EX",
  "IN",
  "NS",
  "OP",
  "QU",

  "IS",
  "NU",
  "PO",
  "PR",
  "SY",

  "AI",
  "AL",
  "H2",
  "H3",
  "ID",

  "JL",
  "JV",
  "JT",
  "SA"
  };


const FXchar *const scriptcode[]={
  "Zzzz",
  "Zyyy",
  "Zinh",
  "Latn",
  "Grek",
  "Cyrl",
  "Armn",
  "Hebr",
  "Arab",
  "Syrc",
  "Thaa",
  "Deva",
  "Beng",
  "Guru",
  "Gujr",
  "Orya",
  "Taml",
  "Telu",
  "Knda",
  "Mlym",
  "Sinh",
  "Thai",
  "Laoo",
  "Tibt",
  "Mymr",
  "Geor",
  "Hang",
  "Ethi",
  "Cher",
  "Cans",
  "Ogam",
  "Runr",
  "Khmr",
  "Mong",
  "Hira",
  "Kana",
  "Bopo",
  "Hani",
  "Yiii",
  "Ital",
  "Goth",
  "Dsrt",
  "Tglg",
  "Hano",
  "Buhd",
  "Tagb",
  "Limb",
  "Tale",
  "Linb",
  "Ugar",
  "Shaw",
  "Osma",
  "Cprt",
  "Brai",
  "Bugi",
  "Copt",
  "Talu",
  "Glag",
  "Tfng",
  "Sylo",
  "Xpeo",
  "Khar",
  "Bali",
  "Xsux",
  "Phnx",
  "Phag",
  "Nkoo",
  "Sund",
  "Lepc",
  "Olck",
  "Vaii",
  "Saur",
  "Kali",
  "Rjng",
  "Lyci",
  "Cari",
  "Lydi",
  "Cham",
  "Lana",
  "Tavt",
  "Avst",
  "Egyp",
  "Samr",
  "Lisu",
  "Bamu",
  "Java",
  "Mtei",
  "Armi",
  "Sarb",
  "Prti",
  "Phli",
  "Orkh",
  "Kthi",
  "Batk",
  "Brah",
  "Mand"
  };

const FXchar *const joiningtype[]={
  "NJ",
  "RJ",
  "DJ",
  "JC",
  "LJ",
  "JT"
  };


const FXchar *const decomposetype[]={
  "None",
  "Cann",
  "Comp",
  "Font",
  "NBrk",
  "Init",
  "Medi",
  "Fina",
  "Isol",
  "Circ",
  "Supr",
  "Sub",
  "Vert",
  "Wide",
  "Narr",
  "Smll",
  "Sqre",
  "Frac"
  };


// Number of failed checks
static FXint failures=0;


// Check character category
static void checkCategory(FXwchar uc,const FXchar* cat){
  const FXchar* got=category[Unicode::charCategory(uc)];
  if(strcmp(got,cat)!=0){
    failures++;
    printf("BAD: category of U+%04X is %s, expected %s\n",uc,got,cat);
    }
  }


// Check script
static void checkScript(FXwchar uc,const FXchar* script){
  const FXchar* got=scriptcode[Unicode::scriptType(uc)];
  if(strcmp(got,script)!=0){
    failures++;
    printf("BAD: script of U+%04X is %s, expected %s\n",uc,got,script);
    }
  }


// Check a value
static void checkValue(const FXchar* what,FXwchar uc,FXint got,FXint expected){
  if(got!=expected){
    failures++;
    printf("BAD: %s of U+%04X is %d, expected %d\n",what,uc,got,expected);
    }
  }


// Values taken from the Unicode character database
static void checkKnownValues(){
  checkCategory(0x0000,"Cc");
  checkCategory(0x000A,"Cc");
  checkCategory(0x0020,"Zs");
  checkCategory(0x002B,"Sm");
  checkCategory(0x0030,"Nd");
  checkCategory(0x0041,"Lu");
  checkCategory(0x0061,"Ll");
  checkCategory(0x00E9,"Ll");
  checkCategory(0x01C5,"Lt");
  checkCategory(0x0301,"Mn");
  checkCategory(0x03B1,"Ll");
  checkCategory(0x20AC,"Sc");
  checkCategory(0x2028,"Zl");
  checkCategory(0x2029,"Zp");
  checkCategory(0xD800,"Cs");
  checkCategory(0xE000,"Co");
  checkCategory(0x10400,"Lu");
  checkCategory(0x10FFFF,"Cn");
  checkScript(0x0041,"Latn");
  checkScript(0x03B1,"Grek");
  checkScript(0x0430,"Cyrl");
  checkScript(0x05D0,"Hebr");
  checkScript(0x0627,"Arab");
  checkValue("digit value",0x0035,Unicode::digitValue(0x0035),5);
  checkValue("digit value",0x002B,Unicode::digitValue(0x002B),-1);
  checkValue("combining class",0x0301,Unicode::charCombining(0x0301),230);
  checkValue("combining class",0x0041,Unicode::charCombining(0x0041),0);
  checkValue("to lower",0x0041,Unicode::toLower(0x0041),0x0061);
  checkValue("to upper",0x0061,Unicode::toUpper(0x0061),0x0041);
  checkValue("to upper",0x00E9,Unicode::toUpper(0x00E9),0x00C9);
  checkValue("to upper",0x03B1,Unicode::toUpper(0x03B1),0x0391);
  checkValue("to lower",0x0416,Unicode::toLower(0x0416),0x0436);
  checkValue("to lower",0x10400,Unicode::toLower(0x10400),0x10428);
  checkValue("is upper",0x0041,Unicode::isUpper(0x0041),1);
  checkValue("is upper",0x0061,Unicode::isUpper(0x0061),0);
  checkValue("is lower",0x0061,Unicode::isLower(0x0061),1);
  checkValue("is lower",0x0041,Unicode::isLower(0x0041),0);
  }


// Start the whole thing
int main(int argc,char* argv[]){
  fxtestargs(argc,argv);
  FXchar buffer[2048];
  FXint len;
  FXwchar uc;
  FXchar ch;

  // Check against known values
  checkKnownValues();
  if(failures){
    printf("FAILED: %d check(s)\n",failures);
    return 1;
    }
  printf("all checks passed\n");

  // Dump character properties only on request: unicode --dump
  if(argc<2 || strcmp(argv[1],"--dump")!=0) return 0;

  // Dump character properties
  for(uc=0; uc<0x110000; ++uc){
    if(uc%32==0){
      printf("\nCode\tChar\tCat\tDir\tBreak\tValue\tScript\tComb\tJoin\tDecom\tNumdec\tCase\n");
      printf("============================================================================================\n");
      }
    printf("%04X\t",uc);
    if(' '<=uc){
      len=wc2utf(buffer,uc); buffer[len]=0;
      printf("%s\t",buffer);
      }
    else{
      printf(" \t");
      }
    printf("%2s\t",category[Unicode::charCategory(uc)]);
    printf("%3s\t",direction[Unicode::charDirection(uc)]);
    printf("%2s\t",linebreak[Unicode::lineBreakType(uc)]);
    if(0<=Unicode::digitValue(uc)){
      printf("%2d\t",Unicode::digitValue(uc));
      }
    else{
      printf("--\t");
      }
    printf("%4s\t",scriptcode[Unicode::scriptType(uc)]);
    printf("%4d\t",Unicode::charCombining(uc));
    printf("%3s\t",joiningtype[Unicode::joiningType(uc)]);
    printf("%4s\t",decomposetype[Unicode::decomposeType(uc)]);
    printf("%2d\t",Unicode::charNumDecompose(uc));
    ch='-';
    if(Unicode::isUpper(uc)) ch='U';
    else if(Unicode::isLower(uc)) ch='L';
    else if(Unicode::isTitle(uc)) ch='T';
    printf("%c\t",ch);
    printf("\n");
    }
  return 0;
  }
