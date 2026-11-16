/********************************************************************************
*                                                                               *
*                   V a r i a n t   a n d   J S O N   T e s t s                 *
*                                                                               *
*********************************************************************************
* Copyright (C) 2013,2026 by Jeroen van der Zijp.   All Rights Reserved.        *
********************************************************************************/
#include <limits.h>
#include <stdarg.h>
#include "fx.h"
#include "testargs.h"


// Print options
void printusage(const char* prog){
  fxmessage("%s options:\n",prog);
  fxmessage("  --test                   Run self-checks.\n");
  fxmessage("  --load-json <file>       Load json file.\n");
  fxmessage("  --save-json <file>       Save json file.\n");
  fxmessage("  --load-ini <file>        Load ini file.\n");
  fxmessage("  --save-ini <file>        Save ini file.\n");
  fxmessage("  --tracelevel <level>     Set trace level.\n");
  fxmessage("  --precision <prec>       Set numeric precision for floating point.\n");
  fxmessage("  --format <format>        Set exponent format for floating point (0=Never,1=Always,2=As-Needed).\n");
  fxmessage("  --flow <flow>            Set output flow format (0=Stream,1=Compact,2=Pretty).\n");
  fxmessage("  --dent <dent>            Set indentation amount.\n");
  fxmessage("  --wrap <columns>         Set line wrap columns.\n");
  fxmessage("  --esc <mode>             Set unicode escape mode (0=OFF,1=\\xHH,2=\\uHHHH).\n");
  fxmessage("  --ver 5                  Set JSON version (currently, 1...5).\n");
  fxmessage("  --single-quotes          Set single quotes mode (JSON5 only).\n");
  fxmessage("  --double-quotes          Set double quotes mode.\n");
  fxmessage("  -h, --help               Print help.\n");
  }


/*******************************************************************************/

// Failures found
static FXint failures=0;

// Report failure
static void bad(const char* fmt,...){
  va_list ap;
  failures++;
  if(failures<=40){
    va_start(ap,fmt);
    printf("BAD: ");
    vprintf(fmt,ap);
    printf("\n");
    va_end(ap);
    }
  }

// Expect boolean
static void expectbool(const char* what,FXbool got,FXbool want){
  if(!!got!=!!want) bad("%s: got %d expected %d",what,!!got,!!want);
  }

// Expect signed integer
static void expectlong(const char* what,FXlong got,FXlong want){
  if(got!=want) bad("%s: got %lld expected %lld",what,(long long)got,(long long)want);
  }

// Expect floating point number, optionally within relative tolerance
static void expectdbl(const char* what,FXdouble got,FXdouble want,FXdouble tol=0.0){
  if(got!=got && want!=want) return;
  if(got==want) return;
  if(tol>0.0 && fabs(got-want)<=tol*fabs(want)) return;
  bad("%s: got %.17g expected %.17g",what,got,want);
  }

// Expect string
static void expectstr(const char* what,const FXString& got,const FXString& want){
  if(got!=want) bad("%s: got \"%s\" expected \"%s\"",what,got.text(),want.text());
  }

// Check type and all type predicates
static void checktype(const char* what,const FXVariant& v,FXVariant::Type t){
  if(v.getType()!=t) bad("%s: type %d expected %d",what,(FXint)v.getType(),(FXint)t);
  if(v.isNull()!=(t==FXVariant::NullType)) bad("%s: isNull wrong",what);
  if(v.isBool()!=(t==FXVariant::BoolType)) bad("%s: isBool wrong",what);
  if(v.isChar()!=(t==FXVariant::CharType)) bad("%s: isChar wrong",what);
  if(v.isInt()!=(t==FXVariant::IntType)) bad("%s: isInt wrong",what);
  if(v.isUInt()!=(t==FXVariant::UIntType)) bad("%s: isUInt wrong",what);
  if(v.isLong()!=(t==FXVariant::LongType)) bad("%s: isLong wrong",what);
  if(v.isULong()!=(t==FXVariant::ULongType)) bad("%s: isULong wrong",what);
  if(v.isFloat()!=(t==FXVariant::FloatType)) bad("%s: isFloat wrong",what);
  if(v.isDouble()!=(t==FXVariant::DoubleType)) bad("%s: isDouble wrong",what);
  if(v.isPtr()!=(t==FXVariant::PointerType)) bad("%s: isPtr wrong",what);
  if(v.isString()!=(t==FXVariant::StringType)) bad("%s: isString wrong",what);
  if(v.isArray()!=(t==FXVariant::ArrayType)) bad("%s: isArray wrong",what);
  if(v.isMap()!=(t==FXVariant::MapType)) bad("%s: isMap wrong",what);
  if(v.isInteger()!=(FXVariant::BoolType<=t && t<=FXVariant::ULongType)) bad("%s: isInteger wrong",what);
  if(v.isReal()!=(t==FXVariant::FloatType || t==FXVariant::DoubleType)) bad("%s: isReal wrong",what);
  if(v.isNumber()!=(FXVariant::BoolType<=t && t<=FXVariant::DoubleType)) bad("%s: isNumber wrong",what);
  }

// Check conversion to int
static void chkint(const char* what,const FXVariant& v,FXint want,FXbool wantok){
  FXbool ok=!wantok;
  FXint got=v.toInt(&ok);
  if(got!=want || ok!=wantok) bad("%s: toInt got %d ok=%d expected %d ok=%d",what,got,ok,want,wantok);
  }

// Check conversion to unsigned int
static void chkuint(const char* what,const FXVariant& v,FXuint want,FXbool wantok){
  FXbool ok=!wantok;
  FXuint got=v.toUInt(&ok);
  if(got!=want || ok!=wantok) bad("%s: toUInt got %u ok=%d expected %u ok=%d",what,got,ok,want,wantok);
  }

// Check conversion to long
static void chklong(const char* what,const FXVariant& v,FXlong want,FXbool wantok){
  FXbool ok=!wantok;
  FXlong got=v.toLong(&ok);
  if(got!=want || ok!=wantok) bad("%s: toLong got %lld ok=%d expected %lld ok=%d",what,(long long)got,ok,(long long)want,wantok);
  }

// Check conversion to unsigned long
static void chkulong(const char* what,const FXVariant& v,FXulong want,FXbool wantok){
  FXbool ok=!wantok;
  FXulong got=v.toULong(&ok);
  if(got!=want || ok!=wantok) bad("%s: toULong got %llu ok=%d expected %llu ok=%d",what,(unsigned long long)got,ok,(unsigned long long)want,wantok);
  }

// Check conversion to double
static void chkdbl(const char* what,const FXVariant& v,FXdouble want,FXbool wantok){
  FXbool ok=!wantok;
  FXdouble got=v.toDouble(&ok);
  if(got!=want || ok!=wantok) bad("%s: toDouble got %.17g ok=%d expected %.17g ok=%d",what,got,ok,want,wantok);
  }

// Check conversion to float
static void chkflt(const char* what,const FXVariant& v,FXfloat want,FXbool wantok){
  FXbool ok=!wantok;
  FXfloat got=v.toFloat(&ok);
  if(got!=want || ok!=wantok) bad("%s: toFloat got %.9g ok=%d expected %.9g ok=%d",what,(double)got,ok,(double)want,wantok);
  }

// Check conversion to string
static void chkstr(const char* what,const FXVariant& v,const char* want,FXbool wantok){
  FXbool ok=!wantok;
  FXString got=v.toString(&ok);
  if(got!=want || ok!=wantok) bad("%s: toString got \"%s\" ok=%d expected \"%s\" ok=%d",what,got.text(),ok,want,wantok);
  }

/*******************************************************************************/

// Types, constructors, assignment, copying
static void testbasics(){
  FXVariant v;
  FXint x=0;
  checktype("default",v,FXVariant::NullType);
  checktype("bool",FXVariant((FXbool)true),FXVariant::BoolType);
  checktype("char",FXVariant((FXchar)'A'),FXVariant::CharType);
  checktype("int",FXVariant((FXint)5),FXVariant::IntType);
  checktype("uint",FXVariant((FXuint)5),FXVariant::UIntType);
  checktype("long",FXVariant((FXlong)5),FXVariant::LongType);
  checktype("ulong",FXVariant((FXulong)5),FXVariant::ULongType);
  checktype("float",FXVariant((FXfloat)5.0f),FXVariant::FloatType);
  checktype("double",FXVariant((FXdouble)5.0),FXVariant::DoubleType);
  checktype("pointer",FXVariant((FXptr)&x),FXVariant::PointerType);
  checktype("chars",FXVariant("abc"),FXVariant::StringType);
  checktype("string",FXVariant(FXString("abc")),FXVariant::StringType);
  v=true;       checktype("=bool",v,FXVariant::BoolType);
  v='c';        checktype("=char",v,FXVariant::CharType);
  v=1;          checktype("=int",v,FXVariant::IntType);
  v=1u;         checktype("=uint",v,FXVariant::UIntType);
  v=(FXlong)1;  checktype("=long",v,FXVariant::LongType);
  v=(FXulong)1; checktype("=ulong",v,FXVariant::ULongType);
  v=1.0f;       checktype("=float",v,FXVariant::FloatType);
  v=1.0;        checktype("=double",v,FXVariant::DoubleType);
  v=(FXptr)&x;  checktype("=pointer",v,FXVariant::PointerType);
  v="text";     checktype("=chars",v,FXVariant::StringType);
  v=FXString("text"); checktype("=string",v,FXVariant::StringType);
  v.clear();    checktype("cleared",v,FXVariant::NullType);

  // setType gives a default value of that type
  v.setType(FXVariant::IntType);    checktype("setType int",v,FXVariant::IntType);    chklong("setType int value",v,0,true);
  v.setType(FXVariant::StringType); checktype("setType string",v,FXVariant::StringType); expectstr("setType string value",v.asString(),"");
  v.setType(FXVariant::ArrayType);  checktype("setType array",v,FXVariant::ArrayType); expectlong("setType array no",v.no(),0);
  v.setType(FXVariant::MapType);    checktype("setType map",v,FXVariant::MapType);    expectlong("setType map used",v.asMap().used(),0);
  v.setType(FXVariant::NullType);   checktype("setType null",v,FXVariant::NullType);

  // Copies are deep
  FXVariant a;
  a["name"]="fox";
  a["list"][2]=7;
  a["sub"]["k"]=1.5;
  FXVariant b(a);
  FXVariant c;
  c=a;
  b["name"]="changed";
  b["list"][2]=8;
  b["sub"]["k"]=2.5;
  expectstr("copy source name",a["name"].toString(),"fox");
  chkint("copy source list",a["list"][2],7,true);
  chkdbl("copy source sub",a["sub"]["k"],1.5,true);
  expectstr("copy name",c["name"].toString(),"fox");
  expectstr("modified name",b["name"].toString(),"changed");
  chkint("modified list",b["list"][2],8,true);
  chkdbl("modified sub",b["sub"]["k"],2.5,true);

  // Self assignment
  FXVariant& r=a;
  a=r;
  expectstr("self assign",a["name"].toString(),"fox");

  // Assigning part of itself to itself
  FXVariant n;
  n["a"]["b"]=1;
  n["a"]["s"]="str";
  n=n["a"];
  checktype("subtree assign type",n,FXVariant::MapType);
  chkint("subtree assign b",n["b"],1,true);
  expectstr("subtree assign s",n["s"].toString(),"str");
  n=n["s"];
  checktype("subtree string type",n,FXVariant::StringType);
  expectstr("subtree string value",n.toString(),"str");
  FXVariant m;
  m["x"]="abc";
  m["y"]=0;
  m["y"]=m["x"].asString();
  expectstr("string from itself",m["y"].toString(),"abc");
  FXVariant t;
  t="hello";
  t=t.asChars()+1;
  expectstr("chars from itself",t.toString(),"ello");

  // Adopt moves contents
  FXVariant s;
  FXVariant d;
  s["k"]=1;
  d=5;
  d.adopt(s);
  checktype("adopt source",s,FXVariant::NullType);
  checktype("adopt target",d,FXVariant::MapType);
  chkint("adopt value",d["k"],1,true);
  d.adopt(d);
  checktype("self adopt",d,FXVariant::MapType);
  }

// Conversions, with overflow flags and clamping
static void testconvert(){
  FXint x=0;
  FXVariant nul;
  FXVariant fls((FXbool)false);
  FXVariant tru((FXbool)true);
  FXVariant chr((FXchar)'A');
  FXVariant neg((FXint)-5);
  FXVariant uzero((FXuint)0);
  FXVariant umax((FXuint)0xFFFFFFFFu);
  FXVariant lmax((FXlong)LLONG_MAX);
  FXVariant lmin((FXlong)LLONG_MIN);
  FXVariant umaxl((FXulong)ULLONG_MAX);
  FXVariant ubig((FXulong)FXULONG(10000000000000000000));
  FXVariant ptr((FXptr)&x);
  FXVariant arr;
  FXVariant map;
  arr[0]=1;
  map["a"]=1;

  // Bool
  chkint("false int",fls,0,true);
  chkint("true int",tru,1,true);
  chkuint("false uint",fls,0,true);
  chkulong("true ulong",tru,1,true);
  chkdbl("true dbl",tru,1.0,true);
  chkflt("true flt",tru,1.0f,true);

  // Char
  chkint("char int",chr,65,true);
  chkuint("char uint",chr,65,true);
  chklong("char long",chr,65,true);
  chkulong("char ulong",chr,65,true);
  chkdbl("char dbl",chr,65.0,true);
  chkflt("char flt",chr,65.0f,true);
  if((FXchar)-1<0){
    FXVariant nc((FXchar)-1);
    chkint("neg char int",nc,-1,true);
    chkdbl("neg char dbl",nc,-1.0,true);
    chkflt("neg char flt",nc,-1.0f,true);
    chkuint("neg char uint",nc,0,false);
    chkulong("neg char ulong",nc,0,false);
    }

  // Int
  chkint("neg int",neg,-5,true);
  chkuint("neg uint",neg,0,false);
  chklong("neg long",neg,-5,true);
  chkulong("neg ulong",neg,0,false);
  chkdbl("neg dbl",neg,-5.0,true);
  chkflt("neg flt",neg,-5.0f,true);

  // UInt
  chkuint("uzero uint",uzero,0,true);
  chkint("uzero int",uzero,0,true);
  chkulong("uzero ulong",uzero,0,true);
  chkint("umax int",umax,INT_MAX,false);
  chkuint("umax uint",umax,UINT_MAX,true);
  chklong("umax long",umax,4294967295LL,true);
  chkulong("umax ulong",umax,4294967295ULL,true);
  chkdbl("umax dbl",umax,4294967295.0,true);

  // Long
  chkint("lmax int",lmax,INT_MAX,false);
  chkuint("lmax uint",lmax,UINT_MAX,false);
  chklong("lmax long",lmax,LLONG_MAX,true);
  chkulong("lmax ulong",lmax,(FXulong)LLONG_MAX,true);
  chkint("lmin int",lmin,INT_MIN,false);
  chkuint("lmin uint",lmin,0,false);
  chklong("lmin long",lmin,LLONG_MIN,true);
  chkulong("lmin ulong",lmin,0,false);
  chkdbl("lmin dbl",lmin,-9223372036854775808.0,true);
  chkint("long in range",FXVariant((FXlong)-123456),-123456,true);
  chkint("long int max",FXVariant((FXlong)INT_MAX),INT_MAX,true);
  chkint("long int min",FXVariant((FXlong)INT_MIN),INT_MIN,true);
  chkint("long over int max",FXVariant((FXlong)INT_MAX+1),INT_MAX,false);
  chkint("long under int min",FXVariant((FXlong)INT_MIN-1),INT_MIN,false);
  chkuint("long over uint max",FXVariant((FXlong)UINT_MAX+1),UINT_MAX,false);
  chkuint("long uint max",FXVariant((FXlong)UINT_MAX),UINT_MAX,true);

  // ULong
  chkint("umaxl int",umaxl,INT_MAX,false);
  chkuint("umaxl uint",umaxl,UINT_MAX,false);
  chklong("umaxl long",umaxl,LLONG_MAX,false);
  chkulong("umaxl ulong",umaxl,ULLONG_MAX,true);
  chkint("ubig int",ubig,INT_MAX,false);
  chkuint("ubig uint",ubig,UINT_MAX,false);
  chklong("ubig long",ubig,LLONG_MAX,false);
  chkulong("ubig ulong",ubig,FXULONG(10000000000000000000),true);
  chkdbl("ubig dbl",ubig,1e19,true);
  chklong("ulong small long",FXVariant((FXulong)7),7,true);

  // Floating point
  chkint("dbl 2.4",FXVariant(2.4),2,true);
  chkint("dbl 2.6",FXVariant(2.6),3,true);
  chkint("dbl -2.6",FXVariant(-2.6),-3,true);
  chkint("dbl big int",FXVariant(3e9),INT_MAX,false);
  chkint("dbl small int",FXVariant(-3e9),INT_MIN,false);
  chkuint("dbl uint",FXVariant(4.2e9),4200000000u,true);
  chkuint("dbl big uint",FXVariant(5e9),UINT_MAX,false);
  chkuint("dbl neg uint",FXVariant(-1.0),0,false);
  chklong("dbl long",FXVariant(-1e12),-1000000000000LL,true);
  chklong("dbl big long",FXVariant(1e19),LLONG_MAX,false);
  chklong("dbl small long",FXVariant(-1e19),LLONG_MIN,false);
  chkulong("dbl ulong",FXVariant(1e19),FXULONG(10000000000000000000),true);
  chkulong("dbl big ulong",FXVariant(2e19),ULLONG_MAX,false);
  chkulong("dbl neg ulong",FXVariant(-1.0),0,false);
  chkdbl("dbl dbl",FXVariant(0.25),0.25,true);
  chkflt("dbl flt",FXVariant(0.25),0.25f,true);
  chkdbl("flt dbl",FXVariant(0.5f),0.5,true);
  chkint("flt int",FXVariant(7.0f),7,true);

  // Strings
  chkint("str int",FXVariant("123"),123,true);
  chkint("str neg int",FXVariant("-45"),-45,true);
  chkint("bad str int",FXVariant("abc"),0,false);
  chkint("empty str int",FXVariant(""),0,false);
  chkuint("str uint",FXVariant("4000000000"),4000000000u,true);
  chklong("str long",FXVariant("-5000000000"),-5000000000LL,true);
  chkulong("str ulong",FXVariant("18446744073709551615"),ULLONG_MAX,true);
  chkdbl("str dbl",FXVariant("3.5"),3.5,true);
  chkdbl("bad str dbl",FXVariant("x"),0.0,false);
  chkflt("str flt",FXVariant("0.5"),0.5f,true);

  // No conversion for null, pointers, arrays and maps
  const FXVariant* others[4]={&nul,&ptr,&arr,&map};
  const char* names[4]={"null","pointer","array","map"};
  for(FXint i=0; i<4; ++i){
    chkint(names[i],*others[i],0,false);
    chkuint(names[i],*others[i],0,false);
    chklong(names[i],*others[i],0,false);
    chkulong(names[i],*others[i],0,false);
    chkdbl(names[i],*others[i],0.0,false);
    chkflt(names[i],*others[i],0.0f,false);
    chkstr(names[i],*others[i],"",false);
    }

  // Pointers
  if(ptr.toPtr()!=(FXptr)&x) bad("pointer toPtr");
  if(tru.toPtr()!=nullptr) bad("bool toPtr");
  if(FXVariant("s").toPtr()!=nullptr) bad("string toPtr");

  // Truth values
  expectbool("null truth",nul.toBool(),false);
  expectbool("false truth",fls.toBool(),false);
  expectbool("true truth",tru.toBool(),true);
  expectbool("zero truth",FXVariant((FXint)0).toBool(),false);
  expectbool("five truth",FXVariant((FXint)5).toBool(),true);
  expectbool("neg truth",neg.toBool(),true);
  expectbool("ulong truth",umaxl.toBool(),true);
  expectbool("0.0 truth",FXVariant(0.0).toBool(),false);
  expectbool("0.5 truth",FXVariant(0.5).toBool(),true);
  expectbool("pointer truth",ptr.toBool(),true);
  expectbool("null pointer truth",FXVariant((FXptr)nullptr).toBool(),false);
  expectbool("empty string truth",FXVariant("").toBool(),false);
  expectbool("string truth",FXVariant("x").toBool(),true);
  expectbool("empty array truth",FXVariant().toBool(),false);
  expectbool("array truth",arr.toBool(),true);
  expectbool("map truth",map.toBool(),true);
  FXVariant emptyarray;
  emptyarray.setType(FXVariant::ArrayType);
  expectbool("new empty array truth",emptyarray.toBool(),false);
  FXVariant emptymap;
  emptymap.setType(FXVariant::MapType);
  expectbool("new empty map truth",emptymap.toBool(),false);
  emptymap["a"]=1;
  emptymap["b"]=2;
  emptymap.remove("a");
  emptymap.remove("b");
  expectbool("emptied map truth",emptymap.toBool(),false);

  // Strings of values
  chkstr("false string",fls,"false",true);
  chkstr("true string",tru,"true",true);
  chkstr("char string",chr,"A",true);
  chkstr("int string",neg,"-5",true);
  chkstr("uint string",umax,"4294967295",true);
  chkstr("long string",lmin,"-9223372036854775808",true);
  chkstr("ulong string",umaxl,"18446744073709551615",true);
  chkstr("dbl string",FXVariant(0.5),"0.5",true);
  chkstr("flt string",FXVariant(0.25f),"0.25",true);
  chkstr("str string",FXVariant("abc"),"abc",true);
  expectstr("chars of string",FXVariant("abc").toChars(),"abc");
  expectstr("chars of int",FXVariant(5).toChars(),"");

  // Implicit conversions
  FXVariant iv((FXint)42);
  FXint ii=iv;
  FXdouble idd=iv;
  FXString is=FXVariant("zz");
  expectlong("implicit int",ii,42);
  expectdbl("implicit double",idd,42.0);
  expectstr("implicit string",is,"zz");
  }

// Arrays and maps
static void testcontainers(){
  FXVariant m;
  FXVariant a;
  FXint i;

  // Map
  m["a"]=1;
  m["b"]="x";
  m["c"]=2.5;
  checktype("map type",m,FXVariant::MapType);
  expectlong("map used",m.asMap().used(),3);
  expectbool("has a",m.has("a"),true);
  expectbool("has FXString",m.has(FXString("b")),true);
  expectbool("has z",m.has("z"),false);
  expectbool("remove a",m.remove("a"),true);
  expectbool("removed a",m.has("a"),false);
  expectbool("remove a again",m.remove("a"),false);
  expectbool("remove z",m.remove("z"),false);
  expectlong("map used after remove",m.asMap().used(),2);
  chkdbl("map c",m["c"],2.5,true);
  expectstr("map FXString key",m[FXString("b")].toString(),"x");

  // Const lookups do not create entries
  const FXVariant& cm=m;
  expectbool("const missing is null",cm["missing"].isNull(),true);
  expectbool("const lookup not creating",m.has("missing"),false);
  expectlong("const lookup used",m.asMap().used(),2);
  const FXVariant none;
  expectbool("const null key",none["x"].isNull(),true);
  expectbool("const null index",none[3].isNull(),true);
  checktype("const null stays null",none,FXVariant::NullType);
  FXVariant five(5);
  expectbool("has on non-map",five.has("x"),false);
  expectbool("remove on non-map",five.remove("x"),false);
  expectbool("erase on non-array",five.erase(0),false);
  expectlong("no on non-array",five.no(),0);

  // Many keys
  FXVariant big;
  for(i=0; i<1000; ++i) big[FXString::value(i)]=i;
  expectlong("big used",big.asMap().used(),1000);
  for(i=0; i<1000; ++i){
    if(!big.has(FXString::value(i)) || big[FXString::value(i)].toInt()!=i){ bad("big lookup %d",i); break; }
    }
  for(i=0; i<1000; i+=2) if(!big.remove(FXString::value(i))){ bad("big remove %d",i); break; }
  expectlong("big used after remove",big.asMap().used(),500);
  for(i=0; i<1000; ++i){
    if(big.has(FXString::value(i))!=(i&1)){ bad("big has after remove %d",i); break; }
    }
  for(i=0; i<1000; i+=2) big[FXString::value(i)]=-i;
  expectlong("big used after readd",big.asMap().used(),1000);
  for(i=0; i<1000; ++i){
    if(big[FXString::value(i)].toInt()!=((i&1)?i:-i)){ bad("big value %d",i); break; }
    }

  // Array
  a[3]=7;
  checktype("array type",a,FXVariant::ArrayType);
  expectlong("array no",a.no(),4);
  expectbool("array hole is null",a[0].isNull(),true);
  chkint("array element",a[3],7,true);
  const FXVariant& ca=a;
  expectbool("const beyond end",ca[10].isNull(),true);
  expectlong("const beyond end keeps size",a.no(),4);
  a[5]="s";
  expectlong("array grown",a.no(),6);
  a.no(2);
  expectlong("array shrunk",a.no(),2);
  expectbool("erase 0",a.erase(0),true);
  expectlong("array after erase",a.no(),1);
  expectbool("erase beyond",a.erase(5),false);
  expectbool("erase negative",a.erase(-1),false);
  a.no(0);
  expectlong("array emptied",a.no(),0);
  for(i=0; i<500; ++i) a[i]=i;
  expectlong("array 500",a.no(),500);
  expectlong("array 500 last",a[499].toInt(),499);

  // Negative indexes throw
  FXbool thrown=false;
  try{ a[-1]=1; } catch(const FXRangeException&){ thrown=true; }
  expectbool("negative index throws",thrown,true);
  thrown=false;
  try{ FXint v=ca[-1]; (void)v; } catch(const FXRangeException&){ thrown=true; }
  expectbool("negative const index throws",thrown,true);

  // Indexing changes the type
  FXVariant t(5);
  t["k"]=1;
  checktype("int to map",t,FXVariant::MapType);
  t[2]=1;
  checktype("map to array",t,FXVariant::ArrayType);
  expectlong("map to array no",t.no(),3);
  t.no(1);
  checktype("no keeps array",t,FXVariant::ArrayType);
  FXVariant u("str");
  u.no(2);
  checktype("no changes to array",u,FXVariant::ArrayType);
  expectlong("no changes to array size",u.no(),2);
  }

/*******************************************************************************/

// JSON writer with access to its buffer
class JSONMem : public FXJSON {
public:
  FXString output() const { return FXString(begptr,(FXint)(wptr-begptr)); }
  };

// INI writer with access to its buffer
class INIMem : public FXINI {
public:
  FXString output() const { return FXString(begptr,(FXint)(wptr-begptr)); }
  };

// Save variant as JSON text
static FXString tojson(const FXVariant& v,FXint flow=FXJSON::Stream,FXint ver=4,FXint esc=0,FXchar quote='"',FXint prec=15,FXint fmt=2,FXint dent=2){
  FXchar buffer[65536];
  JSONMem json;
  FXString result;
  if(json.open(buffer,sizeof(buffer),FXParseBuffer::Save)){
    json.setOutputFlow((FXJSON::Flow)flow);
    json.setVersion(ver);
    json.setEscapeMode(esc);
    json.setQuote(quote);
    json.setNumericPrecision(prec);
    json.setNumericFormat(fmt);
    json.setIndentation(dent);
    if(json.save(v)==FXJSON::ErrOK) result=json.output();
    else bad("tojson: save failed");
    json.close();
    }
  else{
    bad("tojson: open failed");
    }
  return result;
  }

// Load variant from JSON text
static FXJSON::Error fromjson(FXVariant& v,const FXString& text,FXint* line=nullptr){
  FXString copy(text);
  JSONMem json;
  FXJSON::Error err=FXJSON::ErrLoad;
  if(json.open(const_cast<FXchar*>(copy.text()),copy.length(),FXParseBuffer::Load)){
    err=json.load(v);
    if(line) *line=json.getLine();
    json.close();
    }
  return err;
  }

// Compare variants in the way JSON can preserve them
static FXbool same(const FXVariant& a,const FXVariant& b,FXdouble tol,FXString path,FXint& reported){
  FXbool ok=true;
  if(a.isNull() || b.isNull()){
    ok=(a.isNull() && b.isNull());
    }
  else if(a.isBool() || b.isBool()){
    ok=(a.isBool() && b.isBool() && a.toBool()==b.toBool());
    }
  else if(a.isString() || b.isString()){
    ok=(a.isString() && b.isString() && a.asString()==b.asString());
    }
  else if(a.isNumber() && b.isNumber()){
    if(a.isInteger() && b.isInteger()){
      ok=(a.toLong()==b.toLong() && a.toULong()==b.toULong());
      }
    else{
      FXdouble x=a.toDouble();
      FXdouble y=b.toDouble();
      ok=(x==y) || (x!=x && y!=y) || (fabs(x-y)<=tol*fabs(y));
      }
    }
  else if(a.isArray() && b.isArray()){
    if(a.no()!=b.no()){
      ok=false;
      }
    else{
      for(FXival i=0; i<a.no(); ++i){
        if(!same(a[i],b[i],tol,path+"["+FXString::value(i)+"]",reported)) ok=false;
        }
      }
    }
  else if(a.isMap() && b.isMap()){
    if(a.asMap().used()!=b.asMap().used()){
      ok=false;
      }
    else{
      for(FXival i=0; i<a.asMap().no(); ++i){
        if(a.asMap().key(i).empty()) continue;
        if(!b.has(a.asMap().key(i))){ ok=false; continue; }
        if(!same(a.asMap().data(i),b[a.asMap().key(i)],tol,path+"."+a.asMap().key(i),reported)) ok=false;
        }
      }
    }
  else{
    ok=false;
    }
  if(!ok && !reported){ reported=1; bad("mismatch at %s",path.text()); }
  return ok;
  }

// Variant with every kind of data JSON can hold
static void makedata(FXVariant& v){
  v.clear();
  v["null"]=FXVariant::null;
  v["yes"]=true;
  v["no"]=false;
  v["zero"]=0;
  v["int"]=42;
  v["neg"]=-7;
  v["long"]=(FXlong)LLONG_MAX;
  v["pi"]=3.14159265358979;
  v["tiny"]=1e-100;
  v["huge"]=1e100;
  v["frac"]=123456789.125;
  v["inf"]=HUGE_VAL;
  v["ninf"]=-HUGE_VAL;
  v["nan"]=FXdouble(NAN);
  v["empty"]="";
  v["plain"]="plain text";
  v["special"]="quote\" back\\ slash/ tab\t nl\n cr\r bell\a esc\x1b" "end\x01" "ctl\x7f" "del";
  v["quotes"]="'single' \"double\"";
  v["unicode"]="caf\xc3\xa9 \xe2\x82\xac \xf0\x9f\x98\x80 \xc3\xbc\xc3\xb6";
  v["gr\xc3\xb6\xc3\x9f" "e"]=1;
  v["emptymap"].setType(FXVariant::MapType);
  v["emptyarray"].setType(FXVariant::ArrayType);
  v["nested"][0][0]=1;
  v["nested"][0][1]=2;
  v["nested"][1][0]=3;
  v["nested"][1][1][0]=4;
  v["nested"][1][1][1]=5;
  v["nested"][2].setType(FXVariant::ArrayType);
  v["deep"]["a"]["b"]["c"]["d"]="deep";
  v["mixed"][0]=1;
  v["mixed"][1]["k"]="v";
  v["mixed"][2][0]=true;
  v["mixed"][3]=FXVariant::null;
  for(FXint i=0; i<100; ++i) v["sines"][i]=sin(i*0.1);
  }

// Check exact JSON text
static void chkjson(const char* what,const FXVariant& v,const char* want,FXint flow=FXJSON::Stream,FXint ver=4,FXint esc=0,FXchar quote='"',FXint prec=15,FXint fmt=2){
  FXString got=tojson(v,flow,ver,esc,quote,prec,fmt);
  got.substitute("\r","");
  if(got!=want) bad("%s: got <%s> expected <%s>",what,got.text(),want);
  }

// Write JSON and check the text
static void testjsonwrite(){
  FXVariant v;
  chkjson("null",v,"null");
  v=true;                       chkjson("true",v,"true");
  v=false;                      chkjson("false",v,"false");
  v=0;                          chkjson("zero",v,"0");
  v=42;                         chkjson("int",v,"42");
  v=-7;                         chkjson("neg",v,"-7");
  v=(FXuint)4000000000u;        chkjson("uint",v,"4000000000");
  v=(FXlong)LLONG_MIN;          chkjson("long min",v,"-9223372036854775808");
  v=(FXlong)LLONG_MAX;          chkjson("long max",v,"9223372036854775807");
  v=(FXulong)ULLONG_MAX;        chkjson("ulong max",v,"18446744073709551615");
  v=(FXchar)'A';                chkjson("char",v,"65");
  v=1.5;                        chkjson("1.5",v,"1.5");
  v=-0.25;                      chkjson("-0.25",v,"-0.25");
  v=0.1;                        chkjson("0.1",v,"0.1");
  v=100.0;                      chkjson("100.0",v,"100");
  v=3.14159265358979;           chkjson("pi",v,"3.14159265358979");
  v=3.14159265358979;           chkjson("pi prec 5",v,"3.1416",FXJSON::Stream,4,0,'"',5);
  v=1e100;                      chkjson("1e100",v,"1E+100");
  v=1e-100;                     chkjson("1e-100",v,"1E-100");
  v=HUGE_VAL;                   chkjson("inf",v,"INF");
  v=-HUGE_VAL;                  chkjson("-inf",v,"-INF");
  v=FXdouble(NAN);              chkjson("nan",v,"NAN");
  v="abc";                      chkjson("string",v,"\"abc\"");
  v="";                         chkjson("empty string",v,"\"\"");
  v="a\"b\\c/d";               chkjson("string escapes",v,"\"a\\\"b\\\\c/d\"");
  v="a\nb\tc\rd";               chkjson("control escapes",v,"\"a\\nb\\tc\\rd\"");
  v="it's";                     chkjson("apostrophe",v,"\"it's\"");
  v="it's";                     chkjson("single quote",v,"'it\\'s'",FXJSON::Stream,5,0,'\'');
  v="say \"hi\"";               chkjson("quote in single quotes",v,"'say \"hi\"'",FXJSON::Stream,5,0,'\'');
  v="\x01";                     chkjson("ctl char",v,"\"\\x01\"");
  v="caf\xc3\xa9";              chkjson("utf8 esc0",v,"\"caf\xc3\xa9\"",FXJSON::Stream,4,0);
  v="caf\xc3\xa9";              chkjson("utf8 esc1",v,"\"caf\\xC3\\xA9\"",FXJSON::Stream,4,1);
  v="caf\xc3\xa9";              chkjson("utf8 esc2",v,"\"caf\\u00E9\"",FXJSON::Stream,4,2);
  v="\xe2\x82\xac";             chkjson("euro esc2",v,"\"\\u20AC\"",FXJSON::Stream,4,2);
  v="\xf0\x9f\x98\x80";         chkjson("emoji esc2",v,"\"\\uD83D\\uDE00\"",FXJSON::Stream,4,2);

  // Containers
  v.setType(FXVariant::ArrayType);   chkjson("empty array",v,"[]");
  v.setType(FXVariant::MapType);     chkjson("empty map",v,"{}");
  v.clear(); v[2]=3;                 chkjson("array with holes",v,"[null,null,3]");
  v.clear(); v[0]=1; v[1]=2; v[2]=3;
  chkjson("stream array",v,"[1,2,3]",FXJSON::Stream);
  chkjson("compact array",v,"[1, 2, 3]",FXJSON::Compact);
  chkjson("pretty array",v,"[\n  1,\n  2,\n  3\n]",FXJSON::Pretty);
  v.clear(); v["a"]=1;
  chkjson("stream map",v,"{\"a\":1}",FXJSON::Stream);
  chkjson("compact map",v,"{\"a\" : 1}",FXJSON::Compact);
  chkjson("pretty map",v,"{\n  \"a\" : 1\n}",FXJSON::Pretty);
  chkjson("json5 map",v,"{a:1}",FXJSON::Stream,5);
  chkjson("json5 pretty map",v,"{\n  a : 1\n}",FXJSON::Pretty,5);
  chkjson("single quoted key",v,"{'a':1}",FXJSON::Stream,4,0,'\'');
  v.clear(); v["a"][0]=1; v["a"][1]["b"]=FXVariant::null;
  chkjson("stream nested",v,"{\"a\":[1,{\"b\":null}]}",FXJSON::Stream);
  chkjson("pretty nested",v,"{\n  \"a\" : [\n    1,\n    {\n      \"b\" : null\n    }\n  ]\n}",FXJSON::Pretty);
  chkjson("pretty indent 2",v,"{\n  \"a\" : [\n    1,\n    {\n      \"b\" : null\n    }\n  ]\n}",FXJSON::Pretty);

  // Output into a buffer that is too small fails
  FXchar tiny[4];
  JSONMem json;
  FXVariant w("a long string");
  if(json.open(tiny,sizeof(tiny),FXParseBuffer::Save)){
    if(json.save(w)==FXJSON::ErrOK) bad("save into tiny buffer succeeded");
    json.close();
    }
  FXJSON::Error err=json.save(w);
  if(err!=FXJSON::ErrSave) bad("save on closed json gave %d",(FXint)err);
  err=json.load(w);
  if(err!=FXJSON::ErrLoad) bad("load on closed json gave %d",(FXint)err);
  }

// Check that text parses and writes back as the canonical text
struct GoodJSON { const char* text; const char* want; };

static const GoodJSON goodjson[]={
  {"null","null"},
  {" true ","true"},
  {"\tfalse\n","false"},
  {"0","0"},
  {"-0","0"},
  {"007","7"},
  {"+5","5"},
  {"0x1F","31"},
  {"0XfF","255"},
  {"-0x10","-16"},
  {"+0xff","255"},
  {".5","0.5"},
  {"-.5","-0.5"},
  {"5.","5"},
  {"1e3","1000"},
  {"1E+3","1000"},
  {"2.5e-1","0.25"},
  {"123456789012","123456789012"},
  {"\"abc\"","\"abc\""},
  {"'abc'","\"abc\""},
  {"\"\"","\"\""},
  {"''","\"\""},
  {"\"a\\u00e9b\"","\"a\xc3\xa9" "b\""},
  {"\"\\x41\\102\"","\"AB\""},
  {"\"tab\\there\"","\"tab\\there\""},
  {"\"slash\\/\"","\"slash/\""},
  {"\"line\\\ncont\"","\"linecont\""},
  {"\"line\\\r\ncont\"","\"linecont\""},
  {"'it\\'s'","\"it's\""},
  {"\"say \\\"hi\\\"\"","\"say \\\"hi\\\"\""},
  {"\"\xe2\x82\xac\"","\"\xe2\x82\xac\""},
  {"\"\\uD83D\\uDE00\"","\"\xf0\x9f\x98\x80\""},
  {"[]","[]"},
  {"{}","{}"},
  {"[ ]","[]"},
  {"{ }","{}"},
  {"[1,2,3]","[1,2,3]"},
  {"[1,2,]","[1,2]"},
  {"[1 , 2 , 3 ]","[1,2,3]"},
  {"[[1],[]]","[[1],[]]"},
  {"[ [1] , [ ] ]","[[1],[]]"},
  {"[null,true,\"x\",1.5]","[null,true,\"x\",1.5]"},
  {"{\"a\":1}","{\"a\":1}"},
  {"{a:1}","{\"a\":1}"},
  {"{ 'a' : 1 , }","{\"a\":1}"},
  {"{null:1}","{\"null\":1}"},
  {"{true:[]}","{\"true\":[]}"},
  {"{$k_1:2}","{\"$k_1\":2}"},
  {"{\"a\\u0062\":0}","{\"ab\":0}"},
  {"{\xc3\xbc:1}","{\"\xc3\xbc\":1}"},
  {"{\"a\":{\"b\":[1,{\"c\":null}]}}","{\"a\":{\"b\":[1,{\"c\":null}]}}"},
  {"/* c */ [1] // t\n","[1]"},
  {"/* a /* nested */ b */ 7","7"},
  {"// only\n 8","8"},
  {"[1, // x\n 2]","[1,2]"},
  {"[1, /* x */ 2]","[1,2]"},
  {"{\"a\" /* c */ : /* d */ 1}","{\"a\":1}"},
  {"\xef\xbb\xbf[1]","[1]"},
  {"\xc2\xa0[1]","[1]"},
  {"\x0b\x0c[1]","[1]"},
  {"\r\n\r\n[1]\r\n","[1]"},
  {nullptr,nullptr}
  };

// Text that must fail to parse
struct BadJSON { const char* text; FXJSON::Error err; };

static const BadJSON badjson[]={
  {"   ",FXJSON::ErrEnd},
  {"[1,2",FXJSON::ErrEnd},
  {"[",FXJSON::ErrEnd},
  {"{",FXJSON::ErrEnd},
  {"{\"a\"",FXJSON::ErrEnd},
  {"{\"a\":",FXJSON::ErrEnd},
  {"{\"a\":1",FXJSON::ErrEnd},
  {"{\"a\":1,",FXJSON::ErrEnd},
  {"\"abc",FXJSON::ErrEnd},
  {"/* open",FXJSON::ErrEnd},
  {"-",FXJSON::ErrEnd},
  {"+",FXJSON::ErrEnd},
  {"[1 2]",FXJSON::ErrBracket},
  {"[,]",FXJSON::ErrBracket},
  {"[1,,2]",FXJSON::ErrBracket},
  {"[1}",FXJSON::ErrBracket},
  {"{\"a\" 1}",FXJSON::ErrColon},
  {"{\"a\":1,\"b\"}",FXJSON::ErrColon},
  {"{\"a\":1 \"b\":2}",FXJSON::ErrBrace},
  {"{1:2}",FXJSON::ErrBrace},
  {"{\"a\":1]",FXJSON::ErrBrace},
  {"{\"a\":1,\"a\":2}",FXJSON::ErrDuplicate},
  {"{a:1,\"a\":2}",FXJSON::ErrDuplicate},
  {"\"abc\n\"",FXJSON::ErrToken},
  {"@",FXJSON::ErrToken},
  {"]",FXJSON::ErrToken},
  {"}",FXJSON::ErrToken},
  {",",FXJSON::ErrToken},
  {":",FXJSON::ErrToken},
  {"{\"a\":}",FXJSON::ErrToken},
  {"1.5e",FXJSON::ErrToken},
  {"0x",FXJSON::ErrToken},
  {"- x",FXJSON::ErrToken},
  {"+\"a\"",FXJSON::ErrToken},
  {"abc",FXJSON::ErrIdent},
  {"[abc]",FXJSON::ErrBracket},
  {nullptr,FXJSON::ErrOK}
  };

// Parse JSON text
static void testjsonread(){
  FXVariant v;
  FXint i;

  // Good text
  for(i=0; goodjson[i].text; ++i){
    v.clear();
    FXJSON::Error err=fromjson(v,goodjson[i].text);
    if(err!=FXJSON::ErrOK){
      bad("good json %d <%s>: error %d",i,goodjson[i].text,(FXint)err);
      continue;
      }
    FXString got=tojson(v);
    if(got!=goodjson[i].want) bad("good json %d <%s>: got <%s> expected <%s>",i,goodjson[i].text,got.text(),goodjson[i].want);
    }

  // Bad text
  for(i=0; badjson[i].text; ++i){
    v.clear();
    FXJSON::Error err=fromjson(v,badjson[i].text);
    if(err!=badjson[i].err) bad("bad json %d <%s>: error %d expected %d",i,badjson[i].text,(FXint)err,(FXint)badjson[i].err);
    }

  // Types of loaded numbers
  fromjson(v,"42");          checktype("loaded int",v,FXVariant::LongType);   chklong("loaded int value",v,42,true);
  fromjson(v,"1.5");         checktype("loaded real",v,FXVariant::DoubleType); chkdbl("loaded real value",v,1.5,true);
  fromjson(v,"0x10");        checktype("loaded hex",v,FXVariant::LongType);   chklong("loaded hex value",v,16,true);
  fromjson(v,"true");        checktype("loaded true",v,FXVariant::BoolType);  expectbool("loaded true value",v.toBool(),true);
  fromjson(v,"null");        checktype("loaded null",v,FXVariant::NullType);
  fromjson(v,"\"s\"");       checktype("loaded string",v,FXVariant::StringType);
  fromjson(v,"[1]");         checktype("loaded array",v,FXVariant::ArrayType);
  fromjson(v,"{}");          checktype("loaded map",v,FXVariant::MapType);

  // Large integers
  fromjson(v,"9223372036854775807");
  chklong("long max",v,LLONG_MAX,true);
  fromjson(v,"-9223372036854775808");
  chklong("long min",v,LLONG_MIN,true);
  fromjson(v,"18446744073709551615");
  chkulong("ulong max",v,ULLONG_MAX,true);
  fromjson(v,"9223372036854775808");
  chkulong("above long max",v,FXULONG(9223372036854775808),true);

  // Special numbers
  fromjson(v,"Infinity");    chkdbl("Infinity",v,HUGE_VAL,true);
  fromjson(v,"-Infinity");   chkdbl("-Infinity",v,-HUGE_VAL,true);
  fromjson(v,"+Infinity");   chkdbl("+Infinity",v,HUGE_VAL,true);
  fromjson(v,"inf");         chkdbl("inf",v,HUGE_VAL,true);
  fromjson(v,"INF");         chkdbl("INF",v,HUGE_VAL,true);
  fromjson(v,"-Inf");        chkdbl("-Inf",v,-HUGE_VAL,true);
  fromjson(v,"NaN");         if(!v.isReal() || v.toDouble()==v.toDouble()) bad("NaN not loaded as nan");
  fromjson(v,"nan");         if(!v.isReal() || v.toDouble()==v.toDouble()) bad("nan not loaded as nan");
  fromjson(v,"[NaN,Infinity]");
  if(v.no()!=2 || !v[1].isReal() || v[1].toDouble()!=HUGE_VAL) bad("special values in array");

  // Error position
  FXint line=0;
  FXJSON::Error err=fromjson(v,"[1,\n2\n3]",&line);
  if(err!=FXJSON::ErrBracket || line!=3) bad("error position: error %d line %d",(FXint)err,line);
  err=fromjson(v,"{\"a\":1,\n\"a\":2}",&line);
  if(err!=FXJSON::ErrDuplicate || line!=2) bad("duplicate position: error %d line %d",(FXint)err,line);

  // Loading into a populated variant replaces its contents
  v.clear();
  v["a"]=0;
  v["b"]=2;
  v["sub"]["x"]=1;
  v["sub"]["y"]=2;
  err=fromjson(v,"{\"a\":1,\"sub\":{\"x\":10}}");
  if(err!=FXJSON::ErrOK) bad("reload error %d",(FXint)err);
  chklong("reload a",v["a"],1,true);
  expectbool("reload b",v.has("b"),false);
  chklong("reload sub.x",v["sub"]["x"],10,true);
  expectbool("reload sub.y",v["sub"].has("y"),false);
  }

// Save, load, and compare under every output setting
static void testjsonroundtrip(){
  FXVariant data;
  makedata(data);
  FXint flow,ver,esc,q,rep;
  const FXchar quotes[2]={'"','\''};
  for(flow=FXJSON::Stream; flow<=FXJSON::Pretty; ++flow){
    for(ver=1; ver<=5; ++ver){
      for(esc=0; esc<=2; ++esc){
        for(q=0; q<2; ++q){
          FXString text=tojson(data,flow,ver,esc,quotes[q]);
          FXVariant loaded;
          FXJSON::Error err=fromjson(loaded,text);
          if(err!=FXJSON::ErrOK){
            bad("round trip flow=%d ver=%d esc=%d quote=%c: load error %d",flow,ver,esc,quotes[q],(FXint)err);
            continue;
            }
          rep=0;
          if(!same(data,loaded,1e-14,"",rep)) bad("round trip flow=%d ver=%d esc=%d quote=%c differs",flow,ver,esc,quotes[q]);
          if(esc>0 && ver<5){
            for(FXint i=0; i<text.length(); ++i){
              if(text[i]&0x80){ bad("round trip esc=%d left a non-ASCII byte",esc); break; }
              }
            }
          }
        }
      }
    }

  // Output is stable once loaded
  FXString first=tojson(data,FXJSON::Pretty,4,0);
  FXVariant loaded;
  fromjson(loaded,first);
  FXString second=tojson(loaded,FXJSON::Pretty,4,0);
  FXVariant again;
  fromjson(again,second);
  rep=0;
  if(!same(loaded,again,0.0,"",rep)) bad("second round trip differs");

  // High precision keeps all digits
  FXVariant d;
  FXdouble values[]={0.1,1.0/3.0,3.14159265358979323846,2.718281828459045,1e-300,1.7e308,123456.789e-7,-0.30000000000000004};
  for(FXint i=0; i<(FXint)(sizeof(values)/sizeof(values[0])); ++i){
    d=values[i];
    FXVariant back;
    fromjson(back,tojson(d,FXJSON::Stream,4,0,'"',17));
    expectdbl("precision 17",back.toDouble(),values[i],1e-15);
    fromjson(back,tojson(d,FXJSON::Stream,4,0,'"',15));
    expectdbl("precision 15",back.toDouble(),values[i],1e-14);
    }

  // Exponent formats
  d=12345.0;
  chkjson("format never",d,"12345.000000000000000",FXJSON::Stream,4,0,'"',15,0);
  d=1.5;
  chkjson("format always",d,"1.500000000000000E+00",FXJSON::Stream,4,0,'"',15,1);

  // Line wrapping does not change what loads back
  FXVariant wide;
  for(FXint i=0; i<60; ++i){ wide[i]=i*1000; wide[60+i]=FXString::value(i); }
  for(FXint wrap=1; wrap<=120; wrap+=17){
    FXchar buffer[16384];
    JSONMem json;
    FXString text;
    if(json.open(buffer,sizeof(buffer),FXParseBuffer::Save)){
      json.setOutputFlow(FXJSON::Compact);
      json.setLineWrap(wrap);
      json.save(wide);
      text=json.output();
      json.close();
      }
    FXVariant back;
    rep=0;
    if(fromjson(back,text)!=FXJSON::ErrOK || !same(wide,back,0.0,"",rep)) bad("wrap %d round trip",wrap);
    }
  }

/*******************************************************************************/

// Save variant as INI text
static FXString toini(const FXVariant& v,FXint esc=0){
  FXchar buffer[65536];
  INIMem ini;
  FXString result;
  if(ini.open(buffer,sizeof(buffer),FXParseBuffer::Save)){
    ini.setEscapeMode(esc);
    if(ini.save(v)==FXINI::ErrOK) result=ini.output();
    else bad("toini: save failed");
    ini.close();
    }
  else{
    bad("toini: open failed");
    }
  return result;
  }

// Load variant from INI text
static FXINI::Error fromini(FXVariant& v,const FXString& text){
  FXString copy(text);
  INIMem ini;
  FXINI::Error err=FXINI::ErrLoad;
  if(ini.open(const_cast<FXchar*>(copy.text()),copy.length(),FXParseBuffer::Load)){
    err=ini.load(v);
    ini.close();
    }
  return err;
  }

// INI files
static void testini(){
  FXVariant v;
  FXString text;
  FXINI::Error err;

  // Exact output
  v["Sec"]["key"]="value";
  text=toini(v);
  text.substitute("\r","");
  expectstr("ini one entry",text,"[Sec]\nkey=value\n\n");
  v.clear();
  v["Sec"]["n"]=5;
  text=toini(v);
  text.substitute("\r","");
  expectstr("ini int entry",text,"[Sec]\nn=5\n\n");
  v.clear();
  v["Sec"]["f"]=1.5;
  text=toini(v);
  text.substitute("\r","");
  expectstr("ini real entry",text,"[Sec]\nf=1.5\n\n");
  v.clear();
  v["Sec"]["b"]=true;
  text=toini(v);
  text.substitute("\r","");
  expectstr("ini bool entry",text,"[Sec]\nb=true\n\n");
  v.clear();
  v["Sec"]["z"]=FXVariant::null;
  text=toini(v);
  text.substitute("\r","");
  expectstr("ini null entry",text,"[Sec]\nz=\n\n");
  v.clear();
  v["Empty"].setType(FXVariant::MapType);
  expectstr("ini empty section",toini(v),"");
  v.clear();
  v=5;
  FXchar buffer[256];
  INIMem ini;
  if(ini.open(buffer,sizeof(buffer),FXParseBuffer::Save)){
    if(ini.save(v)!=FXINI::ErrSave) bad("ini saved a non-map");
    ini.close();
    }

  // Parse
  v.clear();
  err=fromini(v,"; comment\n[One]\nkey=value\n# another\nn=5\n\n[Two]\nx=\"quoted \\\"string\\\"\"\nempty=\n");
  if(err!=FXINI::ErrOK) bad("ini parse error %d",(FXint)err);
  expectlong("ini sections",v.asMap().used(),2);
  expectstr("ini key",v["One"]["key"].toString(),"value");
  expectstr("ini number",v["One"]["n"].toString(),"5");
  expectstr("ini quoted",v["Two"]["x"].toString(),"quoted \"string\"");
  expectbool("ini empty exists",v["Two"].has("empty"),true);
  expectstr("ini empty",v["Two"]["empty"].toString(),"");
  chkint("ini string to int",v["One"]["n"],5,true);

  // Line endings
  v.clear();
  err=fromini(v,"[A]\r\nk=1\r\nl=2\r\n[B]\rm=3\r");
  if(err!=FXINI::ErrOK) bad("ini line ending error %d",(FXint)err);
  expectstr("ini crlf",v["A"]["l"].toString(),"2");
  expectstr("ini cr",v["B"]["m"].toString(),"3");

  // Last line without newline
  v.clear();
  err=fromini(v,"[A]\nk=last");
  if(err!=FXINI::ErrOK) bad("ini no newline error %d",(FXint)err);
  expectstr("ini no newline",v["A"]["k"].toString(),"last");

  // Spaces
  v.clear();
  err=fromini(v,"[A]\n  key  =  spaced value  \n");
  if(err!=FXINI::ErrOK) bad("ini spaces error %d",(FXint)err);
  expectbool("ini key spaces",v["A"].has("key"),true);
  expectstr("ini value spaces",v["A"]["key"].toString(),"spaced value");

  // Errors
  const char* badini[]={"k=v\n","[A]\nk\n","[]\nk=v\n","[A]\n=v\n","[A\nk=v\n","A]\nk=v\n",nullptr};
  for(FXint i=0; badini[i]; ++i){
    v.clear();
    err=fromini(v,badini[i]);
    if(err==FXINI::ErrOK) bad("bad ini %d <%s> accepted",i,badini[i]);
    }

  // Equals sign inside a value is part of the value
  v.clear();
  err=fromini(v,"[A]\nk=v=w\n");
  if(err!=FXINI::ErrOK) bad("ini equals in value error %d",(FXint)err);
  expectstr("ini equals in value",v["A"]["k"].toString(),"v=w");

  // Equals sign inside a value is part of the value
  v.clear();
  err=fromini(v,"[A]\nk=v=w\n");
  if(err!=FXINI::ErrOK) bad("ini equals in value error %d",(FXint)err);
  expectstr("ini equals in value",v["A"]["k"].toString(),"v=w");

  // Round trip of every kind of string, under every escape mode
  const char* strings[]={"plain","with space","  leading","trailing  ","quote \" inside","back\\slash","line\nbreak","tab\there","; semicolon","# hash","=equals","[brackets]","caf\xc3\xa9 \xe2\x82\xac","\xf0\x9f\x98\x80","ctl\x01" "x","percent %s",nullptr};
  for(FXint esc=0; esc<=2; ++esc){
    for(FXint i=0; strings[i]; ++i){
      v.clear();
      v["Sec"]["key"]=strings[i];
      text=toini(v,esc);
      FXVariant back;
      err=fromini(back,text);
      if(err!=FXINI::ErrOK){ bad("ini string %d esc=%d: error %d text <%s>",i,esc,(FXint)err,text.text()); continue; }
      if(back["Sec"]["key"].toString()!=strings[i]) bad("ini string %d esc=%d: got <%s> text <%s>",i,esc,back["Sec"]["key"].toString().text(),text.text());
      }
    }

  // Round trip of several sections
  v.clear();
  for(FXint s=0; s<10; ++s){
    for(FXint k=0; k<10; ++k){
      v[FXString::value("Section %d",s)][FXString::value("key%d",k)]=s*100+k;
      }
    }
  text=toini(v);
  FXVariant back;
  err=fromini(back,text);
  if(err!=FXINI::ErrOK) bad("ini sections round trip error %d",(FXint)err);
  expectlong("ini sections count",back.asMap().used(),10);
  for(FXint s=0; s<10; ++s){
    for(FXint k=0; k<10; ++k){
      if(back[FXString::value("Section %d",s)][FXString::value("key%d",k)].toInt()!=s*100+k){ bad("ini value %d/%d",s,k); s=10; break; }
      }
    }
  }

/*******************************************************************************/

// Run all checks
static int selftest(){
  testbasics();
  testconvert();
  testcontainers();
  testjsonwrite();
  testjsonread();
  testjsonroundtrip();
  testini();
  if(failures){
    printf("FAILED: %d check(s)\n",failures);
    return 1;
    }
  printf("all checks passed\n");
  return 0;
  }

// Test JSON I/O
int main(int argc,char *argv[]){
  fxtestargs(argc,argv);
  if(2<=argc && strcmp(argv[1],"--test")==0) return selftest();
  FXString savefile="test.json";
  FXString loadfile;
  FXbool loadjson=true;
  FXbool savejson=true;
  FXint precision;
  FXint format;
  FXint flow;
  FXint dent;
  FXint wrap;
  FXint quote;
  FXint esc;
  FXint ver;
  FXbool ok=true;

  // JSON I/O
  FXJSONFile json;
  FXINIFile  ini;

  // Variant to save
  FXVariant var;

  // Assume defaults for starters
  precision=json.getNumericPrecision();
  format=json.getNumericFormat();
  flow=json.getOutputFlow();
  dent=json.getIndentation();
  wrap=json.getLineWrap();
  quote=json.getQuote();
  esc=json.getEscapeMode();
  ver=json.getVersion();

  fxmessage("sizeof(FXVariant) = %ld\n",sizeof(FXVariant));
  fxmessage("sizeof(FXVariantMap) = %ld\n",sizeof(FXVariantMap));
  fxmessage("sizeof(FXVariantArray) = %ld\n",sizeof(FXVariantArray));
  fxmessage("sizeof(FXString) = %ld\n",sizeof(FXString));
  fxmessage("sizeof(FXHash) = %ld\n",sizeof(FXHash));
  fxmessage("sizeof(FXStringDictionary) = %ld\n",sizeof(FXStringDictionary));
  fxmessage("sizeof(FXDictionary) = %ld\n",sizeof(FXDictionary));
  fxmessage("sizeof(FXReverseDictionary) = %ld\n",sizeof(FXReverseDictionary));

  // Grab a few arguments
  for(FXint arg=1; arg<argc; ++arg){
    if(strcmp(argv[arg],"-h")==0 || strcmp(argv[arg],"--help")==0){
      printusage(argv[0]);
      exit(0);
      }
    else if(strcmp(argv[arg],"--load-json")==0){
      if(++arg>=argc){ fxmessage("Missing filename argument.\n"); exit(1); }
      loadfile=argv[arg];
      loadjson=true;
      }
    else if(strcmp(argv[arg],"--save-json")==0){
      if(++arg>=argc){ fxmessage("Missing filename argument.\n"); exit(1); }
      savefile=argv[arg];
      savejson=true;
      }
    else if(strcmp(argv[arg],"--load-ini")==0){
      if(++arg>=argc){ fxmessage("Missing filename argument.\n"); return 1; }
      loadfile=argv[arg];
      loadjson=false;
      }
    else if(strcmp(argv[arg],"--save-ini")==0){
      if(++arg>=argc){ fxmessage("Missing filename argument.\n"); return 1; }
      savefile=argv[arg];
      savejson=false;
      }
    else if(strcmp(argv[arg],"--tracelevel")==0){
      if(++arg>=argc){ fxmessage("Missing tracelevel number argument.\n"); exit(1); }
      setTraceLevel(strtoul(argv[arg],nullptr,0));
      }
    else if(strcmp(argv[arg],"--tracetopics")==0){
      if(++arg>=argc){ fxmessage("Missing tracelevel number argument.\n"); exit(1); }
      setTraceTopics(argv[arg],true);
      }
    else if(strcmp(argv[arg],"--precision")==0){
      if(++arg>=argc){ fxmessage("Missing precision number argument.\n"); exit(1); }
      precision=strtoul(argv[arg],nullptr,0);
      }
    else if(strcmp(argv[arg],"--format")==0){
      if(++arg>=argc){ fxmessage("Missing format number argument.\n"); exit(1); }
      format=strtoul(argv[arg],nullptr,0);
      }
    else if(strcmp(argv[arg],"--flow")==0){
      if(++arg>=argc){ fxmessage("Missing flow number argument.\n"); exit(1); }
      flow=strtoul(argv[arg],nullptr,0);
      }
    else if(strcmp(argv[arg],"--dent")==0){
      if(++arg>=argc){ fxmessage("Missing indentation amount number argument.\n"); exit(1); }
      dent=strtoul(argv[arg],nullptr,0);
      }
    else if(strcmp(argv[arg],"--wrap")==0){
      if(++arg>=argc){ fxmessage("Missing line wrap columns argument.\n"); exit(1); }
      wrap=strtoul(argv[arg],nullptr,0);
      }
    else if(strcmp(argv[arg],"--esc")==0){
      if(++arg>=argc){ fxmessage("Missing escape mode argument.\n"); exit(1); }
      esc=strtoul(argv[arg],nullptr,0);
      }
    else if(strcmp(argv[arg],"--ver")==0){
      if(++arg>=argc){ fxmessage("Missing version argument.\n"); exit(1); }
      ver=strtoul(argv[arg],nullptr,0);
      }
    else if(strcmp(argv[arg],"--single-quotes")==0){
      quote='\'';
      }
    else if(strcmp(argv[arg],"--double-quotes")==0){
      quote='"';
      }
    else{
      fxmessage("Bad argument.\n");
      printusage(argv[0]);
      exit(1);
      }
    }

  // If not loading, make up some data
  if(loadfile.empty()){

    // Simple values
    var["real"]=PI;
    var["bool"]=true;
    var["int"]=87654321;
    var["string"]="variant";
    var["char"]='X';
    var["null"]=FXVariant::null;
    var["inf"]=33.0;
    var["nan"]=22.0;
    var["array"][5]=10.0;
    var["array"][3][2]=10.0;
    var["array"][3][1]='x';
    var["array"][2]="variant";
    var["array"][4]=2.0;
    var["array"][0]=var["array"][5];
    var["array"][1]='c';
    var["map"]["key"]="value";
    var["map"]["more"]["pi"]=3.141592653;
    var["map"]["more"]["e"]=2.718281828;
    var["map"]["more"]["c"]=299792458.0;
    var["map"]["more"]["answer"]=42.0;
    var["map"]["more"]["letter"]="Unicode: \xC3\xBC Hex:\377Q\xFF\x41\x42\x43\x44\x45\x46\"GH\xC2\xB0K\a\b\t\n\v\f\r\\Esc: \e";
    var["emptymap"].setType(FXVariant::MapType);
    var["emptyarray"].setType(FXVariant::ArrayType);
    for(FXival i=0; i<100; ++i){
      var["bigarray"][i]=Math::sin(0.005*i/PI);
      }
    }

  // Load some data
  if(!loadfile.empty()){

    // Load as json
    if(loadjson){
      if(json.open(loadfile,FXJSON::Load)){
        fxmessage("Start load from: %s\n",loadfile.text());
        FXJSON::Error loaderr=json.load(var);
        fxmessage("Loaded %lld bytes, %d lines\n",json.getOffset(),json.getLine());
        if(loaderr!=FXJSON::ErrOK){
          fxmessage("Error: %s:%d:%d: %s\n",loadfile.text(),json.getLine(),json.getColumn(),FXJSON::getError(loaderr));
          }
        else{
          fxmessage("OK\n");
          }
        json.close();
        }
      else{
        fxwarning("Error: unable to open: \"%s\" for reading.\n",loadfile.text());
        }
      }

    // Load as ini
    else{
      if(ini.open(loadfile,FXINI::Load)){
        fxmessage("Start load from: %s\n",loadfile.text());
        FXINI::Error loaderr=ini.load(var);
        fxmessage("Loaded %lld bytes\n",ini.getOffset());
        if(loaderr!=FXINI::ErrOK){
          fxwarning("Error: %s:%d:%d: %s\n",loadfile.text(),ini.getLine(),ini.getColumn(),FXINIFile::getError(loaderr));
          }
        else{
          fxwarning("OK\n");
          }
        ini.close();
        }
      else{
        fxwarning("Error: unable to open: \"%s\" for reading.\n",loadfile.text());
        }
      }
    }

  // Save some data
  if(!savefile.empty()){

    // Save as json
    if(savejson){

      // Set precision and format
      json.setNumericPrecision(precision);
      json.setNumericFormat(format);
      json.setIndentation(dent);
      json.setOutputFlow(flow);
      json.setLineWrap(wrap);
      json.setEscapeMode(esc);
      json.setQuote(quote);
      json.setVersion(ver);

      // Report float precision used to save
      fxmessage("Precision: %d format: %d flow: %d dent: %d wrap: %d\n",precision,format,flow,dent,wrap);

      // Save test
      if(json.open(savefile,FXJSON::Save)){
        fxmessage("Start save to: %s\n",savefile.text());
        FXJSON::Error saveerr=json.save(var);
        fxmessage("Stored %lld bytes, %d lines\n",json.getOffset(),json.getLine());
        if(saveerr!=FXJSON::ErrOK){
          fxmessage("Error: %s:%d:%d: %s\n",savefile.text(),json.getLine(),json.getColumn(),FXJSON::getError(saveerr));
          }
        else{
          fxmessage("OK\n");
          }
        json.close();
        }
      else{
        fxwarning("Error: unable to open: \"%s\" for writing.\n",savefile.text());
        }
      }

    // Save as ini
    else{
      if(ini.open(savefile,FXINI::Save)){
        fxmessage("Start save to: %s\n",savefile.text());
        FXINI::Error saveerr=ini.save(var);
        fxmessage("Stored %lld bytes, %d lines\n",ini.getOffset(),ini.getLine());
        if(saveerr!=FXINI::ErrOK){
          fxmessage("Error: %s:%d:%d: %s\n",savefile.text(),ini.getLine(),ini.getColumn(),FXINIFile::getError(saveerr));
          }
        else{
          fxmessage("OK\n");
          }
        json.close();
        }
      else{
        fxwarning("Error: unable to open: \"%s\" for writing.\n",savefile.text());
        }
      }
    }

  return 0;
  }


