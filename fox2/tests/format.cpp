/********************************************************************************
*                                                                               *
*                             String Format I/O Test                            *
*                                                                               *
*********************************************************************************
* Copyright (C) 2007,2026 by Jeroen van der Zijp.   All Rights Reserved.        *
********************************************************************************/
#include "fx.h"
#include "testargs.h"
//#include <locale.h>
#include <time.h>


/*
  Notes:
  - Test battery for fxprintf().
*/


/*******************************************************************************/

namespace FX {

extern FXint __snprintf(FXchar* string,FXint length,const FXchar* format,...);
extern FXint __sscanf(const FXchar* string,const FXchar* format,...);

}

const FXchar *floatformat[]={
  "%.15E",
  "%'.5e",
  "%10.5f",
  "%-10.5f",
  "%+10.5f",
  "% 10.5f",
  "%123.9f",
  "%+22.9f",
  "%+4.9f",
  "%01.3f",
  "%4f",
  "%3.1f",
  "%3.2f",
  "%.0f",
  "%.3f",
  "%'.8f",
  "%+.3g",
  "%#.3g",
  "%.g",
  "%#.g",
  "%g",
  "%#g",
  "%'.8g",
  "%12.4e",
  "%a",
  "%A",
  "%.a",
  "%#.a",
  "%+15.4a",
  "%.20a",
  };


const double floatnumbers[]={
  0.000000001,
  -1.5,
  0.8,
  1.0,                          // 0x1p+0
  10.0,
  100.0,
  1000.0,
  10000.0,
  999.0,
  1010.0,
  134.21,
  91340.2,
  341.1234,
  203.9,
  0.4,
  0.96,
  0.996,
  0.9996,
  1.996,
  4.136,
  6442452944.1234,
  1.23456789E+20,
  1.23456789E-205,
  6.4969530541989433e-17,
  0.99999999,
  0.000009995,
  0.123456789,
  2.2250738585072014e-308,      // 0x1p-1022
  -4.94065645841246544177e-324, // -0x0.0000000000001p-1022
  1.7976931348623157e+308,      // 0x1.fffffffffffffp+1023
  1.9382023e-03,
  5e-320,
  0.0,
  -0.0,
  8.6796844466580162e-315
  };


// Double Infinite
static const union{ FXulong u; FXdouble f; } doubleinf={
  FXULONG(0x7ff0000000000000)
  };

// Double NaN
static const union{ FXulong u; FXdouble f; } doublenan={
  FXULONG(0x7fffffffffffffff)
  };

// Double small
static const union{ FXulong u; FXdouble f; } doublesmall={
  FXULONG(0x0000000000002788)
  };


const FXchar *intformat[]={
  "%d",
  "%0d",
  "%'d",
  "%02x",
  "%0.2x",
  "%-8d",
  "%8d",
  "%08d",
  "%.6d",
  "%u",
  "%+i",
  "% i",
  "%x",
  "%#x",
  "%#08x",
  "%o",
  "%#o",
  "%.32b",
  };

const FXint intnumbers[]={
  0,
  1,
  -1,
  0x90,
  -34,
  2147483647,
  -2147483648
  };


const FXchar *positionalformat[]={
  "%d%d%d",
  "%3$d%2$d%1$d",
  "%2$*1$d%3$d"
  };

const FXchar *positionalformat2="%1$*2$.*3$lf";

const FXchar *positionalformat3="%3$d%1$d%2$d";


// Uncomment to revert to native version
//#define __snprintf snprintf

void specialcases(const char* fmt){
  FXchar buffer[1024];
  double num=1234567890.123456789;
  while(0.000001<=num){
    __snprintf(buffer,sizeof(buffer),fmt,num);
    fprintf(stdout,"format=\"%s\" output=\"%s\"\n",fmt,buffer);
    num*=0.1;
    }
  fprintf(stdout,"\n");
  }


/*******************************************************************************/

// Self-check, started with --test
static FXint failures=0;


// Check a formatted string
static void expectstr(const FXchar* fmt,const FXchar* got,const FXchar* want){
  if(strcmp(got,want)!=0){
    printf("BAD: format=\"%s\" output=\"%s\" expected \"%s\"\n",fmt,got,want);
    failures++;
    }
  }


// Check a condition
static void expecttrue(const FXchar* what,FXbool ok){
  if(!ok){
    printf("BAD: %s\n",what);
    failures++;
    }
  }


// Format with one int and compare with the expected string
static void checkint(const FXchar* fmt,FXint v,const FXchar* want){
  FXchar buffer[1024];
  __snprintf(buffer,sizeof(buffer),fmt,v);
  expectstr(fmt,buffer,want);
  }


// Format with one long and compare with the expected string
static void checklong(const FXchar* fmt,FXlong v,const FXchar* want){
  FXchar buffer[1024];
  __snprintf(buffer,sizeof(buffer),fmt,v);
  expectstr(fmt,buffer,want);
  }


// Format with one double and compare with the expected string
static void checkdbl(const FXchar* fmt,FXdouble v,const FXchar* want){
  FXchar buffer[1024];
  __snprintf(buffer,sizeof(buffer),fmt,v);
  expectstr(fmt,buffer,want);
  }


// Format with one string and compare with the expected string
static void checkstr(const FXchar* fmt,const FXchar* v,const FXchar* want){
  FXchar buffer[1024];
  __snprintf(buffer,sizeof(buffer),fmt,v);
  expectstr(fmt,buffer,want);
  }


// Formats which the C library may handle differently from us: thousands
// grouping depends on the locale, hexadecimal floats on the library, and %b
// does not exist in the C library.
static FXbool portable(const FXchar* fmt){
  return !strpbrk(fmt,"'aAb");
  }


// Compare against the C library
static void comparenative(const FXchar* fmt,FXint v){
  FXchar ours[1024],native[1024];
  __snprintf(ours,sizeof(ours),fmt,v);
  snprintf(native,sizeof(native),fmt,v);
  if(strcmp(ours,native)!=0){
    printf("BAD: format=\"%s\" value=%d ours=\"%s\" native=\"%s\"\n",fmt,v,ours,native);
    failures++;
    }
  }


// Our conversion is accurate to about 16 digits and rounds the shortest decimal
// representation, the C library is exact.  Both numbers must be the same otherwise.
static FXbool closeenough(const FXchar* ours,const FXchar* native){
  FXchar *e1,*e2;
  long double a=strtold(ours,&e1);
  long double b=strtold(native,&e2);
  if(*e1 || *e2 || a-a!=0.0L || b-b!=0.0L) return false;
  long double d=(a<b)?b-a:a-b;
  return d<=5E-16L*((b<0.0L)?-b:b);
  }


static void comparenative(const FXchar* fmt,FXdouble v){
  FXchar ours[1024],native[1024];
  __snprintf(ours,sizeof(ours),fmt,v);
  snprintf(native,sizeof(native),fmt,v);
  if(strcmp(ours,native)!=0 && !closeenough(ours,native)){
    printf("BAD: format=\"%s\" value=%.17g ours=\"%s\" native=\"%s\"\n",fmt,v,ours,native);
    failures++;
    }
  }


// Bit counting
static void testbits(){
  FXint k;
  for(k=0; k<32; k++){
    expecttrue("clz32(1<<k)",clz32((FXuint)1<<k)==(FXuint)(31-k));
    expecttrue("ctz32(1<<k)",ctz32((FXuint)1<<k)==(FXuint)k);
    expecttrue("pop32(1<<k)",pop32((FXuint)1<<k)==1);
    }
  for(k=0; k<64; k++){
    expecttrue("clz64(1<<k)",clz64((FXulong)1<<k)==(FXulong)(63-k));
    expecttrue("ctz64(1<<k)",ctz64((FXulong)1<<k)==(FXulong)k);
    expecttrue("pop64(1<<k)",pop64((FXulong)1<<k)==1);
    }
  expecttrue("clz32(0x12345678)",clz32(0x12345678)==3);
  expecttrue("clz32(0x00000123)",clz32(0x00000123)==23);
  expecttrue("clz64(0x0123456789ABCDEF)",clz64(FXULONG(0x0123456789ABCDEF))==7);
  expecttrue("ctz32(0x87654320)",ctz32(0x87654320)==5);
  expecttrue("ctz64(0x8FEDCBA987654000)",ctz64(FXULONG(0x8FEDCBA987654000))==14);
  expecttrue("pop32(0x12345678)",pop32(0x12345678)==13);
  expecttrue("pop64(0x123456789ABCDEF0)",pop64(FXULONG(0x123456789ABCDEF0))==32);
  expecttrue("pop32(0xffffffff)",pop32(0xffffffff)==32);
  expecttrue("pop64(all ones)",pop64(FXULONG(0xffffffffffffffff))==64);
  }


// Integer formats with known output
static void testints(){
  FXchar buffer[1024];
  checkint("%d",0,"0");
  checkint("%d",-34,"-34");
  checkint("%d",2147483647,"2147483647");
  checkint("%d",-2147483647-1,"-2147483648");
  checkint("%0d",1,"1");
  checkint("%02x",1,"01");
  checkint("%0.2x",0x90,"90");
  checkint("%-8d|",-1,"-1      |");
  checkint("%8d",-1,"      -1");
  checkint("%08d",-34,"-0000034");
  checkint("%.6d",-34,"-000034");
  checkint("%u",-1,"4294967295");
  checkint("%+i",5,"+5");
  checkint("% i",5," 5");
  checkint("%x",0x90,"90");
  checkint("%#x",0x90,"0x90");
  checkint("%#x",0,"0");
  checkint("%#08x",0x90,"0x000090");
  checkint("%o",8,"10");
  checkint("%#o",8,"010");
  checkint("%b",5,"101");
  checkint("%.32b",5,"00000000000000000000000000000101");
  checkint("%.0d",0,"");
  checkint("%'d",1234567,"1,234,567");                // Always groups, whatever the locale
  checkint("%c",65,"A");
  checkint("%%%d",5,"%5");
  checklong("%lld",FXLONG(-9223372036854775807)-1,"-9223372036854775808");
  checklong("%lld",FXLONG(9223372036854775807),"9223372036854775807");
  checklong("%llu",(FXlong)FXULONG(0xffffffffffffffff),"18446744073709551615");
  checklong("%llx",(FXlong)FXULONG(0x123456789abcdef0),"123456789abcdef0");
  checkstr("%s","hello","hello");
  checkstr("%8s|","abc","     abc|");
  checkstr("%-8s|","abc","abc     |");
  checkstr("%.2s","abc","ab");
  checkstr("%5.1s|","abc","    a|");
  __snprintf(buffer,sizeof(buffer),"%0*d",6,33);
  expectstr("%0*d",buffer,"000033");
  __snprintf(buffer,sizeof(buffer),"%-*d|",6,33);
  expectstr("%-*d|",buffer,"33    |");
  __snprintf(buffer,sizeof(buffer),"%.*d",5,33);
  expectstr("%.*d",buffer,"00033");
  }


// Floating point formats with known output
static void testfloats(){
  const FXdouble inf=doubleinf.f;
  const FXdouble nan=doublenan.f;
  checkdbl("%.15E",0.000000001,"1.000000000000000E-09");
  checkdbl("%10.5f",-1.5,"  -1.50000");
  checkdbl("%-10.5f|",-1.5,"-1.50000  |");
  checkdbl("%+10.5f",1.0,"  +1.00000");
  checkdbl("% 10.5f",1.0,"   1.00000");
  checkdbl("%01.3f",0.8,"0.800");
  checkdbl("%f",0.0,"0.000000");
  checkdbl("%f",-0.0,"-0.000000");
  checkdbl("%e",0.0,"0.000000e+00");
  checkdbl("%.0f",0.5,"1");                             // Rounds half away from zero
  checkdbl("%.0f",1.5,"2");
  checkdbl("%.0f",2.5,"3");
  checkdbl("%.0f",0.4,"0");
  checkdbl("%.3f",0.9996,"1.000");
  checkdbl("%3.1f",0.96,"1.0");
  checkdbl("%.2f",134.21,"134.21");
  checkdbl("%.1f",6442452944.1234,"6442452944.1");
  checkdbl("%.9f",0.000000001,"0.000000001");
  checkdbl("%.0f",1.23456789E+20,"123456789000000000000");
  checkdbl("%e",1.7976931348623157e+308,"1.797693e+308");
  checkdbl("%e",2.2250738585072014e-308,"2.225074e-308");
  checkdbl("%e",5e-320,"4.999944e-320");
  checkdbl("%g",100000.0,"100000");
  checkdbl("%g",1000000.0,"1e+06");
  checkdbl("%g",0.0001,"0.0001");
  checkdbl("%g",0.00001,"1e-05");
  checkdbl("%g",0.0,"0");
  checkdbl("%g",-0.0,"-0");
  checkdbl("%.3g",1234.5678,"1.23e+03");
  checkdbl("%+.3g",1.5,"+1.5");
  checkdbl("%#.3g",1.0,"1.00");
  checkdbl("%#.3g",0.0,"0.00");
  checkdbl("%.g",999.0,"1e+03");
  checkdbl("%#.g",999.0,"1.e+03");
  checkdbl("%#g",1.0,"1.00000");
  checkdbl("%12.4e",0.123456789,"  1.2346e-01");
  checkdbl("%'.5e",1234567890.123456789,"1.23457e+09");
  checkdbl("%'.5f",1234567.5,"1,234,567.50000");
  checkdbl("%f",inf,"inf");
  checkdbl("%f",-inf,"-inf");
  checkdbl("%10f",inf,"       inf");
  checkdbl("%-10f|",inf,"inf       |");
  checkdbl("%+f",inf,"+inf");
  checkdbl("%e",inf,"inf");
  checkdbl("%g",inf,"inf");
  checkdbl("%f",nan,"nan");
  checkdbl("%10.3f",nan,"       nan");
  checkdbl("%E",inf,"INF");
  checkdbl("%G",nan,"NAN");
  checkdbl("%010f",inf,"       inf");                  // Zero flag does not pad non-numbers

  // Hexadecimal
  checkdbl("%a",1.0,"0x1p+0");
  checkdbl("%a",0.5,"0x1p-1");
  checkdbl("%a",-2.0,"-0x1p+1");
  checkdbl("%a",0.0,"0x0p+0");
  checkdbl("%a",-0.0,"-0x0p+0");
  checkdbl("%a",1.5,"0x1.8p+0");
  checkdbl("%A",255.5,"0X1.FFP+7");
  checkdbl("%.3a",1.0,"0x1.000p+0");
  checkdbl("%.0a",1.5,"0x2p+0");
  checkdbl("%#.0a",1.0,"0x1.p+0");
  checkdbl("%+15.4a",1.0,"   +0x1.0000p+0");
  checkdbl("%a",2.2250738585072014e-308,"0x1p-1022");
  checkdbl("%a",1.7976931348623157e+308,"0x1.fffffffffffffp+1023");
  checkdbl("%a",-4.94065645841246544177e-324,"-0x0.0000000000001p-1022");
  checkdbl("%a",inf,"inf");
  checkdbl("%a",nan,"nan");
  checkdbl("%.20a",1.0,"0x1.00000000000000000000p+0");
  checkdbl("%a",(FXdouble)(5.960464477539062784e-08f),"0x1p-24");
  checkdbl("%a",1.0/(FXLONG(1)<<53),"0x1p-53");
  checkdbl("%a",doublesmall.f,"0x0.0000000002788p-1022");
  }


// Argument position and width/precision from arguments
static void testpositional(){
  FXchar buffer[1024];
  __snprintf(buffer,sizeof(buffer),"%d%d%d",10,20,30);
  expectstr("%d%d%d",buffer,"102030");
  __snprintf(buffer,sizeof(buffer),"%3$d%2$d%1$d",10,20,30);
  expectstr("%3$d%2$d%1$d",buffer,"302010");
  __snprintf(buffer,sizeof(buffer),"%3$d%1$d%2$d",10,20,30);
  expectstr("%3$d%1$d%2$d",buffer,"301020");
  __snprintf(buffer,sizeof(buffer),"%2$*1$d%3$d",5,20,30);
  expectstr("%2$*1$d%3$d",buffer,"   2030");
  __snprintf(buffer,sizeof(buffer),"%1$*2$.*3$lf",3.14159265358979,20,10);
  expectstr("%1$*2$.*3$lf",buffer,"        3.1415926536");
  __snprintf(buffer,sizeof(buffer),"%1$d %1$d",7);
  expectstr("%1$d %1$d",buffer,"7 7");
  }


// Return value and truncation
static void testtruncation(){
  FXchar buffer[16];
  FXint n;
  memset(buffer,'x',sizeof(buffer));
  n=__snprintf(buffer,sizeof(buffer),"%d",123456);
  expecttrue("length of 123456",n==6);
  expectstr("%d",buffer,"123456");
  memset(buffer,'x',sizeof(buffer));
  n=__snprintf(buffer,5,"%d",123456);
  expecttrue("untruncated length is returned",n==6);
  expecttrue("5 characters are written",memcmp(buffer,"12345",5)==0);   // Unlike C, no terminator when truncated
  expecttrue("nothing written past the buffer",buffer[5]=='x');
  memset(buffer,'x',sizeof(buffer));
  n=__snprintf(buffer,1,"%d",123456);
  expecttrue("size 1 returns length",n==6);
  expecttrue("size 1 holds first character",buffer[0]=='1' && buffer[1]=='x');
  memset(buffer,'x',sizeof(buffer));
  n=__snprintf(buffer,0,"%d",123456);
  expecttrue("size 0 returns length",n==6);
  expecttrue("size 0 writes nothing",buffer[0]=='x');
  n=__snprintf(nullptr,0,"%s-%s","ab","cd");
  expecttrue("null buffer returns length",n==5);
  }


// Everything the program prints must agree with the C library
static void testnative(){
  FXchar fmt[32];
  FXuval x,y;
  for(x=0; x<ARRAYNUMBER(intformat); x++){
    if(!portable(intformat[x])) continue;
    for(y=0; y<ARRAYNUMBER(intnumbers); y++){
      comparenative(intformat[x],intnumbers[y]);
      }
    }
  for(x=0; x<ARRAYNUMBER(floatformat); x++){
    if(!portable(floatformat[x])) continue;
    for(y=0; y<ARRAYNUMBER(floatnumbers); y++){
      comparenative(floatformat[x],floatnumbers[y]);
      }
    comparenative(floatformat[x],doubleinf.f);
    comparenative(floatformat[x],-doubleinf.f);
    comparenative(floatformat[x],doublenan.f);
    }

  // Every precision of every conversion for a few awkward numbers
  static const FXchar conversions[]="efgEG";
  static const FXdouble hard[]={0.3,1.7,2.2,0.123,9.4,99.6,0.9999999,123456789.987654321,1e22,1e-7,5e-324};
  FXint prec;
  for(x=0; x<sizeof(conversions)-1; x++){
    for(prec=0; prec<=20; prec++){
      for(y=0; y<ARRAYNUMBER(hard); y++){
        snprintf(fmt,sizeof(fmt),"%%.%d%c",prec,conversions[x]);
        comparenative(fmt,hard[y]);
        snprintf(fmt,sizeof(fmt),"%%#.%d%c",prec,conversions[x]);
        comparenative(fmt,hard[y]);
        }
      }
    }
  }


// Print and read back must give the same number
static void testroundtrip(){
  FXchar buffer[1024];
  FXdouble back;
  FXuval x;
  for(x=0; x<ARRAYNUMBER(floatnumbers); x++){
    FXdouble v=floatnumbers[x];
    if(Math::fabs(v)<1E300){                            // Decimal is good to about 16 digits
      back=0.5;
      __snprintf(buffer,sizeof(buffer),"%.17g",v);
      if(__sscanf(buffer,"%lg",&back)!=1 || Math::fabs(back-v)>5E-16*Math::fabs(v)){
        printf("BAD: \"%s\" (%.17g) does not scan back, got %.17g\n",buffer,v,back);
        failures++;
        }
      back=0.5;
      __snprintf(buffer,sizeof(buffer),"%.17e",v);
      if(__sscanf(buffer,"%le",&back)!=1 || Math::fabs(back-v)>5E-16*Math::fabs(v)){
        printf("BAD: \"%s\" (%.17g) does not scan back, got %.17g\n",buffer,v,back);
        failures++;
        }
      }    back=0.5;
    __snprintf(buffer,sizeof(buffer),"%a",v);
    if(__sscanf(buffer,"%la",&back)!=1 || Math::fpBits(back)!=Math::fpBits(v)){
      printf("BAD: \"%s\" (%.17g) does not scan back, got %.17g\n",buffer,v,back);
      failures++;
      }
    }
  back=0.0;
  __sscanf("0x0.0000000002788p-1023","%la",&back);
  expecttrue("scan of hexadecimal denormal",Math::fpBits(back)==FXULONG(0x00000000000013C4));
  }


static FXint selftest(){
  testbits();
  testints();
  testfloats();
  testpositional();
  testtruncation();
  testnative();
  testroundtrip();
  if(failures){
    printf("FAILED: %d check(s)\n",failures);
    return 1;
    }
  printf("all checks passed\n");
  return 0;
  }


// Start
int main(int argc,char* argv[]){
  fxtestargs(argc,argv);
  FXchar buffer[1024];
  FXuval x,y;
  FXdouble trouble;

  // Self-check
  if(2<=argc && strcmp(argv[1],"--test")==0) return selftest();

/*
*/
  fxmessage("clz32(0x12345678) = %d\n",clz32(0x12345678));      // 3
  fxmessage("clz32(0x01234567) = %d\n",clz32(0x01234567));      // 7
  fxmessage("clz32(0x00123456) = %d\n",clz32(0x00123456));      // 11
  fxmessage("clz32(0x00012345) = %d\n",clz32(0x00012345));      // 15
  fxmessage("clz32(0x00001234) = %d\n",clz32(0x00001234));      // 19
  fxmessage("clz32(0x00000123) = %d\n",clz32(0x00000123));      // 23
  fxmessage("clz32(0x00000012) = %d\n",clz32(0x00000012));      // 27
  fxmessage("clz32(0x00000001) = %d\n",clz32(0x00000001));      // 31
  fxmessage("\n");

  fxmessage("clz64(0x123456789ABCDEF0) = %lld\n",clz64(FXULONG(0x123456789ABCDEF0)));    // 3
  fxmessage("clz64(0x0123456789ABCDEF) = %lld\n",clz64(FXULONG(0x0123456789ABCDEF)));    // 7
  fxmessage("clz64(0x00123456789ABCDE) = %lld\n",clz64(FXULONG(0x00123456789ABCDE)));    // 11
  fxmessage("clz64(0x000123456789ABCD) = %lld\n",clz64(FXULONG(0x000123456789ABCD)));    // 15
  fxmessage("clz64(0x0000123456789ABC) = %lld\n",clz64(FXULONG(0x0000123456789ABC)));    // 19
  fxmessage("clz64(0x00000123456789AB) = %lld\n",clz64(FXULONG(0x00000123456789AB)));    // 23
  fxmessage("clz64(0x000000123456789A) = %lld\n",clz64(FXULONG(0x000000123456789A)));    // 27
  fxmessage("clz64(0x0000000123456789) = %lld\n",clz64(FXULONG(0x0000000123456789)));    // 31
  fxmessage("clz64(0x0000000012345678) = %lld\n",clz64(FXULONG(0x0000000012345678)));    // 35
  fxmessage("clz64(0x0000000001234567) = %lld\n",clz64(FXULONG(0x0000000001234567)));    // 39
  fxmessage("clz64(0x0000000000123456) = %lld\n",clz64(FXULONG(0x0000000000123456)));    // 43
  fxmessage("clz64(0x0000000000012345) = %lld\n",clz64(FXULONG(0x0000000000012345)));    // 47
  fxmessage("clz64(0x0000000000001234) = %lld\n",clz64(FXULONG(0x0000000000001234)));    // 51
  fxmessage("clz64(0x0000000000000123) = %lld\n",clz64(FXULONG(0x0000000000000123)));    // 55
  fxmessage("clz64(0x0000000000000012) = %lld\n",clz64(FXULONG(0x0000000000000012)));    // 59
  fxmessage("clz64(0x0000000000000001) = %lld\n",clz64(FXULONG(0x0000000000000001)));    // 63
  fxmessage("\n");

  fxmessage("ctz32(0x87654321) = %d\n",ctz32(0x87654321));      // 0
  fxmessage("ctz32(0x87654320) = %d\n",ctz32(0x87654320));      // 5
  fxmessage("ctz32(0x87654300) = %d\n",ctz32(0x87654300));      // 8
  fxmessage("ctz32(0x87654000) = %d\n",ctz32(0x87654000));      // 14
  fxmessage("ctz32(0x87650000) = %d\n",ctz32(0x87650000));      // 16
  fxmessage("ctz32(0x87600000) = %d\n",ctz32(0x87600000));      // 21
  fxmessage("ctz32(0x87000000) = %d\n",ctz32(0x87000000));      // 24
  fxmessage("ctz32(0x80000000) = %d\n",ctz32(0x80000000));      // 31
  fxmessage("\n");

  fxmessage("ctz64(0x8FEDCBA987654321) = %lld\n",ctz64(FXULONG(0x8FEDCBA987654321)));      // 0
  fxmessage("ctz64(0x8FEDCBA987654320) = %lld\n",ctz64(FXULONG(0x8FEDCBA987654320)));      // 5
  fxmessage("ctz64(0x8FEDCBA987654300) = %lld\n",ctz64(FXULONG(0x8FEDCBA987654300)));      // 8
  fxmessage("ctz64(0x8FEDCBA987654000) = %lld\n",ctz64(FXULONG(0x8FEDCBA987654000)));      // 14
  fxmessage("ctz64(0x8FEDCBA987650000) = %lld\n",ctz64(FXULONG(0x8FEDCBA987650000)));      // 16
  fxmessage("ctz64(0x8FEDCBA987600000) = %lld\n",ctz64(FXULONG(0x8FEDCBA987600000)));      // 21
  fxmessage("ctz64(0x8FEDCBA987000000) = %lld\n",ctz64(FXULONG(0x8FEDCBA987000000)));      // 24
  fxmessage("ctz64(0x8FEDCBA980000000) = %lld\n",ctz64(FXULONG(0x8FEDCBA980000000)));      // 31
  fxmessage("ctz64(0x8FEDCBA900000000) = %lld\n",ctz64(FXULONG(0x8FEDCBA900000000)));      // 32
  fxmessage("ctz64(0x8FEDCBA000000000) = %lld\n",ctz64(FXULONG(0x8FEDCBA000000000)));      // 37
  fxmessage("ctz64(0x8FEDCB0000000000) = %lld\n",ctz64(FXULONG(0x8FEDCB0000000000)));      // 40
  fxmessage("ctz64(0x8FEDC00000000000) = %lld\n",ctz64(FXULONG(0x8FEDC00000000000)));      // 46
  fxmessage("ctz64(0x8FED000000000000) = %lld\n",ctz64(FXULONG(0x8FED000000000000)));      // 48
  fxmessage("ctz64(0x8FE0000000000000) = %lld\n",ctz64(FXULONG(0x8FE0000000000000)));      // 53
  fxmessage("ctz64(0x8F00000000000000) = %lld\n",ctz64(FXULONG(0x8F00000000000000)));      // 56
  fxmessage("ctz64(0x8000000000000000) = %lld\n",ctz64(FXULONG(0x8000000000000000)));      // 63
  fxmessage("\n");

  fxmessage("pop32(0x12345678) = %d\n", pop32(0x12345678));                // 13
  fxmessage("ctz64(0x123456789ABCDEF0) = %lld\n", pop64(FXULONG(0x123456789ABCDEF0)));      // 32
  fxmessage("\n");

  //setlocale(LC_ALL,"");
  if(3<=argc){
    const FXchar *num=argv[1];
    const FXchar *fmt=argv[2];
    if(strchr(num,'.') || strchr(num,'e') || strchr(num,'E')){
      FXdouble val=strtod(num,nullptr);
      snprintf(buffer,sizeof(buffer),fmt,val);
      fprintf(stdout,"native: %s\n",buffer);
      __snprintf(buffer,sizeof(buffer),fmt,val);
      fprintf(stdout,"ours  : %s\n",buffer);
      }
    else{
      FXlong val=strtoll(num,nullptr,10);
      snprintf(buffer,sizeof(buffer),fmt,val);
      fprintf(stdout,"native: %s\n",buffer);
      __snprintf(buffer,sizeof(buffer),fmt,val);
      fprintf(stdout,"ours  : %s\n",buffer);
      }
    return 0;
    }

  // Testing int formats
  for(x=0; x<ARRAYNUMBER(intformat); x++){
    for(y=0; y<ARRAYNUMBER(intnumbers); y++){
      __snprintf(buffer,sizeof(buffer),intformat[x],intnumbers[y]);
      fprintf(stdout,"format=\"%s\" output=\"%s\"\n",intformat[x],buffer);
      }
    fprintf(stdout,"\n");
    }

  fprintf(stdout,"\n");

  // 33 is the Magic Number
  __snprintf(buffer,sizeof(buffer),"%0*d",6,33);
  fprintf(stdout,"format=\"%s\" output=\"%s\"\n","%0*d",buffer);

  // Testing double formats
  for(x=0; x<ARRAYNUMBER(floatformat); x++){
    for(y=0; y<ARRAYNUMBER(floatnumbers); y++){
      __snprintf(buffer,sizeof(buffer),floatformat[x],floatnumbers[y]);
      fprintf(stdout,"format=\"%s\" output=\"%s\"\n",floatformat[x],buffer);
      }
    fprintf(stdout,"\n");
    }

  for(y=0; y<ARRAYNUMBER(floatnumbers); y++){
    __snprintf(buffer,sizeof(buffer),"0x%016llx",Math::fpBits(floatnumbers[y]));
    fprintf(stdout,"format=\"%s\" output=\"%s\"\n","0x%016llx",buffer);
    }

  fprintf(stdout,"\n");

  // Testing Inf's
  for(x=0; x<ARRAYNUMBER(floatformat); x++){
    __snprintf(buffer,sizeof(buffer),floatformat[x],doubleinf.f);
    fprintf(stdout,"format=\"%s\" output=\"%s\"\n",floatformat[x],buffer);
    }

  fprintf(stdout,"\n");

  // Testing NaN's
  for(x=0; x<ARRAYNUMBER(floatformat); x++){
    __snprintf(buffer,sizeof(buffer),floatformat[x],doublenan.f);
    fprintf(stdout,"format=\"%s\" output=\"%s\"\n",floatformat[x],buffer);
    }

  fprintf(stdout,"\n");

  // Testing positional formats
  for(x=0; x<ARRAYNUMBER(positionalformat); x++){
    __snprintf(buffer,sizeof(buffer),positionalformat[x],10,20,30);
    fprintf(stdout,"format=\"%s\" output=\"%s\"\n",positionalformat[x],buffer);
    }

  fprintf(stdout,"\n");

  __snprintf(buffer,sizeof(buffer),positionalformat2,3.14159265358979,20,10);
  fprintf(stdout,"format=\"%s\" output=\"%s\"\n",positionalformat2,buffer);

  __snprintf(buffer,sizeof(buffer),positionalformat3,10,20,30);
  fprintf(stdout,"format=\"%s\" output=\"%s\"\n",positionalformat3,buffer);

  fprintf(stdout,"\n");

  // Special cases float formatting
  specialcases("%'.5e");
  specialcases("%'+.5e");
  specialcases("%'+ .5e");
  specialcases("%' .5e");
  specialcases("%'.5f");
  specialcases("%'.5g");

  __snprintf(buffer,sizeof(buffer),"%#.3g",0.0);
  fprintf(stdout,"format=\"%s\" output=\"%s\"\n","%#.3g",buffer);

  fprintf(stdout,"\n");

  // Small dernormalized float, regular notation
  __snprintf(buffer,sizeof(buffer),"%.18le",5e-320);
  fprintf(stdout,"format=\"%s\" output=\"%s\"\n","%.18le",buffer);
  __snprintf(buffer,sizeof(buffer),"%a",5e-320);
  fprintf(stdout,"format=\"%s\" output=\"%s\"\n","%a",buffer);
  fprintf(stdout,"\n");

  // Small dernormalized float, passed as hex (types may be flagged as wrong)
  __snprintf(buffer,sizeof(buffer),"%.18le",doublesmall.f);
  fprintf(stdout,"small format=\"%s\" output=\"%s\"\n","%.18le",buffer);
  __snprintf(buffer,sizeof(buffer),"%a",doublesmall.f);
  fprintf(stdout,"small format=\"%s\" output=\"%s\"\n","%a",buffer);
  fprintf(stdout,"\n");

  // Small dernormalized float, passed as floating point hex syntax
#if defined(__GNUC__) && (__GNUC__ >= 14)
  __snprintf(buffer,sizeof(buffer),"%.18le",0x0.0000000002788p-1023);
  fprintf(stdout,"hex format=\"%s\" output=\"%s\"\n","%.18le",buffer);
  __snprintf(buffer,sizeof(buffer),"%a",0x0.0000000002788p-1023);
  fprintf(stdout,"hex format=\"%s\" output=\"%s\"\n","%a",buffer);
  fprintf(stdout,"\n");

  __snprintf(buffer,sizeof(buffer),"%.18le",0x1.0p-53);
  fprintf(stdout,"hex format=\"%s\" output=\"%s\"\n","%.18le",buffer);
  __snprintf(buffer,sizeof(buffer),"%a",0x1.0p-53);
  fprintf(stdout,"hex format=\"%s\" output=\"%s\"\n","%a",buffer);
  __snprintf(buffer,sizeof(buffer),"%a",1.110223024625156480e-16);
  fprintf(stdout,"hex format=\"%s\" output=\"%s\"\n","%a",buffer);
  __snprintf(buffer,sizeof(buffer),"%a",1.0/(FXLONG(1)<<53));
  fprintf(stdout,"hex format=\"%s\" output=\"%s\"\n","%a",buffer);
  fprintf(stdout,"\n");

  __snprintf(buffer,sizeof(buffer),"%.18le",0x1.0p-24);
  fprintf(stdout,"hex format=\"%s\" output=\"%s\"\n","%.18le",buffer);
  __snprintf(buffer,sizeof(buffer),"%a",0x1.0p-24);
  fprintf(stdout,"hex format=\"%s\" output=\"%s\"\n","%a",buffer);
  __snprintf(buffer,sizeof(buffer),"%a",(FXdouble)(5.960464477539062784e-08f));
  fprintf(stdout,"hex format=\"%s\" output=\"%s\"\n","%a",buffer);
  __snprintf(buffer,sizeof(buffer),"%a",(FXdouble)(1.0f/(FXLONG(1)<<24)));
  fprintf(stdout,"hex format=\"%s\" output=\"%s\"\n","%a",buffer);
  fprintf(stdout,"\n");

  // Scan ourselves
  __sscanf("0x0.0000000002788p-1023","%la",&trouble);

  __snprintf(buffer,sizeof(buffer),"%.18le",trouble);
  fprintf(stdout,"scanned hex format=\"%s\" output=\"%s\"\n","%.18le",buffer);

  __snprintf(buffer,sizeof(buffer),"%a",trouble);
  fprintf(stdout,"scanned hex format=\"%s\" output=\"%s\"\n","%a",buffer);

  fprintf(stdout,"\n");
#endif

  __snprintf(buffer,sizeof(buffer),"%.20le",6.4969530541989433e-17);
  fprintf(stdout,"format=\"%s\" output=\"%s\"\n","%.20le",buffer);

  return 0;
  }

