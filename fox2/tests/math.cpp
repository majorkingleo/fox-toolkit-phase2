/********************************************************************************
*                                                                               *
*                              M a t h   T e s t                                *
*                                                                               *
*********************************************************************************
* Copyright (C) 2007,2026 by Jeroen van der Zijp.   All Rights Reserved.        *
********************************************************************************/
#include "fx.h"
#include "testargs.h"


/*
  Notes:
  - Test battery for FX::Math functions.

  - With --test (and optionally --verbose), a bounded self-check runs: known values,
    edge cases (zero signs, infinity, NaN, denormals, overflow, underflow, results
    which need care in the intermediate steps) and a fixed-seed sample compared with
    the C library within a number of ulps.  The exit code is 0 only if all hold.

  - Without arguments the program tests every float and 10^9 doubles per function,
    which takes hours; use  math <function> d|f <value>  to look at a single value.
*/

/*******************************************************************************/

// To pick apart a IEEE754 double
union DP {
  FXdouble f;
  FXulong  u;
  };


// To pick apart a IEEE754 float
union SP {
  FXfloat f;
  FXuint  u;
  };


// Functions
typedef FXdouble (*DBLFUN1)(FXdouble);
typedef FXfloat  (*FLTFUN1)(FXfloat);


// A pair of float funs
struct FLTFUNS {
  FLTFUN1 tf;
  FLTFUN1 rf;
  };


// And pair of double funs
struct DBLFUNS {
  DBLFUN1 tf;
  DBLFUN1 rf;
  };


// IEEE754 number patterns
enum {
  FORBID_NEG =  1,      // Forbid negative values
  FORBID_INF =  2,      // Forbid infinity
  FORBID_NAN =  4,      // Forbid NaN
  FORBID_DEN =  8,      // Forbid denormals
  FORCE_NEG  = 16,      // Force negative values
  FORCE_NAN  = 32,      // Force inifinity or NaN
  FORCE_DEN  = 64       // Force denormals
  };


// Keeping track of accuracy
struct DPAccuracy {
  FXdouble worstabserr;         // Worst absolute error
  FXdouble worstabsarg;         // Argument passed to function
  FXdouble worstabsres;         // Desired result
  FXdouble worstabsact;         // Actual value returned
  FXdouble worstrelerr;         // Worst relative error
  FXdouble worstrelarg;         // Argument passed to function
  FXdouble worstrelres;         // Desired result
  FXdouble worstrelact;         // Actual value returned
  FXulong  errcount;            // Total number of errors
  };


// Keeping track of accuracy
struct SPAccuracy {
  FXfloat worstabserr;          // Worst absolute error
  FXfloat worstabsarg;          // Argument passed to function
  FXfloat worstabsres;          // Desired result
  FXfloat worstabsact;          // Actual value returned
  FXfloat worstrelerr;          // Worst relative error
  FXfloat worstrelarg;          // Argument passed to function
  FXfloat worstrelres;          // Desired result
  FXfloat worstrelact;          // Actual value returned
  FXulong errcount;             // Total number of errors
  };


// Reference functions; log1p(), expm1() and log2() come from the C library, which has
// them even where the library has to use its own versions, exp2() and exp10() are
// missing from some C libraries

// exp2()
#if defined(NO_EXP2)
FXdouble exp2(FXdouble x){ return ::pow(2.0,x); }
#endif

// exp2f()
#if defined(NO_EXP2F)
FXfloat exp2f(FXfloat x){ return ::powf(2.0f,x); }
#endif


// exp10()
#if defined(NO_EXP10)
FXdouble exp10(FXdouble x){ return ::pow(10.0,x); }
#endif

// exp10f()
#if defined(NO_EXP10F)
FXfloat exp10f(FXfloat x){ return ::powf(10.0f,x); }
#endif


/// Fast single precision error function ~3.0E-7
FXfloat erfast(FXfloat x){
  const FXfloat a6=0.0000430638f;
  const FXfloat a5=0.0002765672f;
  const FXfloat a4=0.0001520143f;
  const FXfloat a3=0.0092705272f;
  const FXfloat a2=0.0422820123f;
  const FXfloat a1=0.0705230784f;
  const FXfloat a0=1.0000000000f;
  FXfloat t=Math::fabs(x);
  FXfloat y=(((((a6*t+a5)*t+a4)*t+a3)*t+a2)*t+a1)*t+a0;
  y=y*y;
  y=y*y;
  y=y*y;
  y=y*y;
  y=a0-a0/y;
  return Math::copysign(y,x);
  }



// Testing erf():
FXdouble my_erf(FXdouble a){
  FXdouble t=Math::fabs(a);
  FXdouble s=a*a;
  FXdouble r,u;

  if(t>=1.0){
    // max ulp error = 0.97749 (USE_EXPM1 = 1); 1.05364 (USE_EXPM1 = 0)
    r=fma(-5.6271698391213282e-18,t, 4.8565951797366214e-16);   // -0x1.9f363ba3b515dp-58, 0x1.17f6b1d68f44bp-51
    u=fma(-1.9912968283386570e-14,t, 5.1614612434698227e-13);   // -0x1.66b85b7fbd01ap-46, 0x1.22907eebc22e0p-41
    r=fma(r,s,u);
    u=fma(-9.4934693745934645e-12,t, 1.3183034417605052e-10);   // -0x1.4e0591fd97592p-37, 0x1.21e5e2d8544d1p-33
    r=fma(r,s,u);
    u=fma(-1.4354030030292210e-09,t, 1.2558925114413972e-08);   // -0x1.8a8f81b7e0e84p-30, 0x1.af85793b93d2fp-27
    r=fma(r,s,u);
    u=fma(-8.9719702096303798e-08,t, 5.2832013824348913e-07);   // -0x1.8157db0edbfa8p-24, 0x1.1ba3c453738fdp-21
    r=fma(r,s,u);
    u=fma(-2.5730580226082933e-06,t, 1.0322052949676148e-05);   // -0x1.595999b7e922dp-19, 0x1.5a59c27b3b856p-17
    r=fma(r,s,u);
    u=fma(-3.3555264836700767e-05,t, 8.4667486930266041e-05);   // -0x1.197b61ee37123p-15, 0x1.631f0597f62b8p-14
    r=fma(r,s,u);
    u=fma(-1.4570926486271945e-04,t, 7.1877160107954648e-05);   // -0x1.319310dfb8583p-13, 0x1.2d798353da894p-14
    r=fma(r,s,u);
    u=fma( 4.9486959714661590e-04,t,-1.6221099717135270e-03);   //  0x1.037445e25d3e5p-11,-0x1.a939f51db8c06p-10
    r=fma(r,s,u);
    u=fma( 1.6425707149019379e-04,t, 1.9148914196620660e-02);   //  0x1.5878d80188695p-13, 0x1.39bc5e0e9e09ap-6
    r=fma(r,s,u);
    r=fma(r,t,-1.0277918343487560e-1);                          // -0x1.a4fbc8f8ff7dap-4
    r=fma(r,t,-6.3661844223699315e-1);                          // -0x1.45f2da3ae06f8p-1
    r=fma(r,t,-1.2837929411398119e-1);                          // -0x1.06ebb92d9ffa8p-3
    r=fma(r,t,-t);
    r=Math::expm1(r);
    r=Math::copysign(r,a);
    }
  else{
    // max ulp error = 1.01912
    r=        -7.7794684889591997e-10;                          // -0x1.abae491c44131p-31
    r=fma(r,s, 1.3710980398024347e-8);                          //  0x1.d71b0f1b10071p-27
    r=fma(r,s,-1.6206313758492398e-7);                          // -0x1.5c0726f04dbc7p-23
    r=fma(r,s, 1.6447131571278227e-6);                          //  0x1.b97fd3d9927cap-20
    r=fma(r,s,-1.4924712302009488e-5);                          // -0x1.f4ca4d6f3e232p-17
    r=fma(r,s, 1.2055293576900605e-4);                          //  0x1.f9a2baa8fedc2p-14
    r=fma(r,s,-8.5483259293144627e-4);                          // -0x1.c02db03dd71bbp-11
    r=fma(r,s, 5.2239776061185055e-3);                          //  0x1.565bccf92b31ep-8
    r=fma(r,s,-2.6866170643111514e-2);                          // -0x1.b82ce311fa94bp-6
    r=fma(r,s, 1.1283791670944182e-1);                          //  0x1.ce2f21a040d14p-4
    r=fma(r,s,-3.7612638903183515e-1);                          // -0x1.812746b0379bcp-2
    r=fma(r,s, 1.2837916709551256e-1);                          //  0x1.06eba8214db68p-3
    r=fma(r,a,a);
    }
  return r;
  }



// Function names
static const char *const names[]={
  "fabs",
  "ceil",
  "floor",
  "round",
  "trunc",
  "nearbyint",
  "rint",
  "sinh",
  "cosh",
  "tanh",
  "asinh",
  "acosh",
  "atanh",
  "expm1",
  "exp2",
  "exp10",
  "log1p",
  "log2",
  "erf",
  };


// Double function list
static const DBLFUNS dblfuns[]={
  {Math::fabs,      fabs},
  {Math::ceil,      ceil},
  {Math::floor,     floor},
  {Math::round,     round},
  {Math::trunc,     trunc},
  {Math::nearbyint, nearbyint},
  {Math::rint,      rint},
  {Math::sinh,      sinh},
  {Math::cosh,      cosh},
  {Math::tanh,      tanh},
  {Math::asinh,     asinh},
  {Math::acosh,     acosh},
  {Math::atanh,     atanh},
  {Math::expm1,     expm1},
  {Math::exp2,      exp2},
  {Math::exp10,     exp10},
  {Math::log1p,     log1p},
  {Math::log2,      log2},
  {Math::erf,       my_erf},
  };


// Float function list
static const FLTFUNS fltfuns[]={
  {Math::fabs,      fabsf},
  {Math::ceil,      ceilf},
  {Math::floor,     floorf},
  {Math::round,     roundf},
  {Math::trunc,     truncf},
  {Math::nearbyint, nearbyintf},
  {Math::rint,      rintf},
  {Math::sinh,      sinhf},
  {Math::cosh,      coshf},
  {Math::tanh,      tanhf},
  {Math::asinh,     asinhf},
  {Math::acosh,     acoshf},
  {Math::atanh,     atanhf},
  {Math::expm1,     expm1f},
  {Math::exp2,      exp2f},
  {Math::exp10,     exp10f},
  {Math::log1p,     log1pf},
  {Math::log2,      log2f},
  {Math::erf,       erfast},
  };


// Random source
FXRandom rng;

/*******************************************************************************/

// Find function with name
FXint findFunc(const char* name){
  for(FXuint i=0; i<ARRAYNUMBER(names); ++i){
    if(strcmp(name,names[i])==0) return i;
    }
  return -1;
  }


// Record result of res=f(arg), which should have been act
void record(DPAccuracy& acc,FXdouble arg,FXdouble act,FXdouble res){
  FXdouble err,mag,abserr,relerr;
  err=(act-res);
  if(err!=0.0){
    abserr=(err<0.0)?-err:err;
    if(acc.worstabserr<abserr){         // Record worst absolute error in results
      acc.worstabserr=abserr;
      acc.worstabsarg=arg;
      acc.worstabsres=res;
      acc.worstabsact=act;
      }
    mag=(res<0.0)?-res:res;
    if(0.0<mag){
      relerr=abserr/mag;
      if(acc.worstrelerr<relerr){       // Record worst relative error in results
        acc.worstrelerr=relerr;
        acc.worstrelarg=arg;
        acc.worstrelres=res;
        acc.worstrelact=act;
        }
      }
    acc.errcount++;
    }
  }


// Record result of res=f(arg), which should have been act
void record(SPAccuracy& acc,FXfloat arg,FXfloat act,FXfloat res){
  FXfloat err,mag,abserr,relerr;
  err=(act-res);
  if(err!=0.0f){
    abserr=(err<0.0f)?-err:err;
    if(acc.worstabserr<abserr){         // Record worst absolute error in results
      acc.worstabserr=abserr;
      acc.worstabsarg=arg;
      acc.worstabsres=res;
      acc.worstabsact=act;
      }
    mag=(res<0.0f)?-res:res;
    if(0.0f<mag){
      relerr=abserr/mag;
      if(acc.worstrelerr<relerr){       // Record worst relative error in results
        acc.worstrelerr=relerr;
        acc.worstrelarg=arg;
        acc.worstrelres=res;
        acc.worstrelact=act;
        }
      }
    acc.errcount++;
    }
  }

static const char sign[2]={'+','-'};

// Print a double, disassembled
void printDP(FXdouble x){
  fprintf(stderr,"0x%016lx (%c)(% 4ld)(0x%014lx) -> %+.17lg\n",Math::fpBits(x),sign[Math::fpSign(x)],Math::fpExponent(x),Math::fpMantissa(x),x);
  }


// Print a double, disassembled
void printSP(FXfloat x){
  fprintf(stderr,"0x%08x (%c)(% 3d)(0x%06x) -> %+.17lg\n",Math::fpBits(x),sign[Math::fpSign(x)],Math::fpExponent(x),Math::fpMantissa(x),(FXdouble)x);
  }


// Report errors found during tests
void report(const DPAccuracy& acc,const char* name){
  if(!acc.errcount) return;
  fprintf(stderr,"Worst Absolute: %s(%- 23.17lg): %- 23.17lg expected: %- 23.17lg err=%- 23.17lg\n",name,acc.worstabsarg,acc.worstabsact,acc.worstabsres,acc.worstabserr);
  fprintf(stderr,"arg: "); printDP(acc.worstabsarg);
  fprintf(stderr,"act: "); printDP(acc.worstabsact);
  fprintf(stderr,"res: "); printDP(acc.worstabsres);
  fprintf(stderr,"Worst Relative: %s(%- 23.17lg): %- 23.17lg expected: %- 23.17lg err=%- 23.17lg\n",name,acc.worstrelarg,acc.worstrelact,acc.worstrelres,acc.worstrelerr);
  fprintf(stderr,"arg: "); printDP(acc.worstrelarg);
  fprintf(stderr,"act: "); printDP(acc.worstrelact);
  fprintf(stderr,"res: "); printDP(acc.worstrelres);
  fprintf(stderr,"total: %ld\n",acc.errcount);
  }


// Report errors found during tests
void report(const SPAccuracy& acc,const char* name){
  if(!acc.errcount) return;
  fprintf(stderr,"Worst Absolute: %s(%- 23.17lg): %- 23.17lg expected: %- 23.17lg err=%- 23.17lg\n",name,(FXdouble)acc.worstabsarg,(FXdouble)acc.worstabsact,(FXdouble)acc.worstabsres,(FXdouble)acc.worstabserr);
  fprintf(stderr,"arg: "); printSP(acc.worstabsarg);
  fprintf(stderr,"act: "); printSP(acc.worstabsact);
  fprintf(stderr,"res: "); printSP(acc.worstabsres);
  fprintf(stderr,"Worst Relative: %s(%- 23.17lg): %- 23.17lg expected: %- 23.17lg err=%- 23.17lg\n",name,(FXdouble)acc.worstrelarg,(FXdouble)acc.worstrelact,(FXdouble)acc.worstrelres,(FXdouble)acc.worstrelerr);
  fprintf(stderr,"arg: "); printSP(acc.worstrelarg);
  fprintf(stderr,"act: "); printSP(acc.worstrelact);
  fprintf(stderr,"res: "); printSP(acc.worstrelres);
  fprintf(stderr,"total: %ld\n",acc.errcount);
  }


// FIXME instead, random draws + or- and and-mask.
// FIXME will allow for testing special patterns easily.

// FIXME this is wrong; UPDATE THIS!

// Make a double number given some constraints
FXdouble makeDouble(FXuint flags){
  DP z;
  z.u=(FXulong)rng.randLong();
  if(flags){
    FXulong sx=z.u&FXULONG(0x8000000000000000);
    FXulong mx=z.u&FXULONG(0x7fffffffffffffff);
    if(flags&FORCE_NEG){sx=FXULONG(0x8000000000000000);}                                        // Force negative
    if(flags&FORCE_NAN){mx|=FXULONG(0x7ff0000000000000);}                                       // Force NaN
    if(flags&FORCE_DEN){mx=(mx&FXULONG(0x000fffffffffffff))|FXULONG(0x0010000000000000);}       // Force denormal
    if(flags&FORBID_NEG){sx=0;}                                                                 // No negative
    if((flags&FORBID_DEN)&&(mx<FXULONG(0x0010000000000000))){mx=0;}                             // No denormal
    if((flags&FORBID_NAN)&&(mx>FXULONG(0x7ff0000000000000))){mx&=FXULONG(0x7ff0000000000000);}  // No NaN
    if((flags&FORBID_INF)&&(mx==FXULONG(0x7ff0000000000000))){mx-=1;}                           // No infinite
    z.u=sx|mx;
    }
  return z.f;
  }


// Test one: zero
void testDoubleZero(DPAccuracy& acc,DBLFUN1 tf,DBLFUN1 rf){
  FXdouble res,act;
  act=tf(0.0);
  res=rf(0.0);
  record(acc,0.0,act,res);
  act=tf(-0.0);
  res=rf(-0.0);
  record(acc,-0.0,act,res);
  }


// Test two: Infinity
void testDoubleInfinity(DPAccuracy& acc,DBLFUN1 tf,DBLFUN1 rf){
  FXdouble res,act;
  DP neginf,posinf;
  neginf.u=FXULONG(0xfff0000000000000);
  posinf.u=FXULONG(0x7ff0000000000000);
  act=tf(neginf.f);
  res=rf(neginf.f);
  record(acc,neginf.f,act,res);
  act=tf(posinf.f);
  res=rf(posinf.f);
  record(acc,posinf.f,act,res);
  }


// Test tree: NaN
void testDoubleNaN(DPAccuracy& acc,DBLFUN1 tf,DBLFUN1 rf,FXulong cov){
  FXdouble arg,res,act;
  while(cov-->0){
    arg=makeDouble(FORCE_NAN);
    act=tf(arg);
    res=rf(arg);
    record(acc,arg,act,res);
    }
  }


// Test tree: denormals
void testDoubleDenorm(DPAccuracy& acc,DBLFUN1 tf,DBLFUN1 rf,FXulong cov){
  FXdouble arg,res,act;
  while(cov-->0){
    arg=makeDouble(FORCE_DEN);
    act=tf(arg);
    res=rf(arg);
    record(acc,arg,act,res);
    }
  }


// Test tree: denormals
void testFloatDenorm(SPAccuracy& acc,FLTFUN1 tf,FLTFUN1 rf,FXulong){
  FXfloat arg,res,act;
  SP z;
  for(z.u=0x00000001; z.u<0x00800000; z.u++){
    arg=z.f;
    act=tf(arg);
    res=rf(arg);
    record(acc,arg,act,res);
    z.u|=0x80000000;
    arg=z.f;
    act=tf(arg);
    res=rf(arg);
    record(acc,arg,act,res);
    z.u&=0x7fffffff;
    }
  }


// Test four: any
void testDoubleFinite(DPAccuracy& acc,DBLFUN1 tf,DBLFUN1 rf,FXulong cov){
  FXdouble arg,res,act;
  while(cov-->0){
    arg=makeDouble(FORBID_NAN|FORBID_INF);
    act=tf(arg);
    res=rf(arg);
    record(acc,arg,act,res);
    }
  }


// Test all for float
void testFloatAll(SPAccuracy& acc,FLTFUN1 tf,FLTFUN1 rf){
  FXfloat arg,res,act;
  SP z;
  for(z.u=0x00000000; z.u<=0x7fffffff; z.u++){
    arg=z.f;
    act=tf(arg);
    res=rf(arg);
    record(acc,arg,act,res);
    z.u|=0x80000000;
    arg=z.f;
    act=tf(arg);
    res=rf(arg);
    record(acc,arg,act,res);
    z.u&=0x7fffffff;
    }
  }


// Test all
void testDouble(DBLFUN1 tf,DBLFUN1 rf,const char* name){
  DPAccuracy acc;
  fprintf(stderr,"Testing %s():\n",name);
  acc.worstabserr=0.0;
  acc.worstabsarg=0.0;
  acc.worstabsres=0.0;
  acc.worstabsact=0.0;
  acc.worstrelerr=0.0;
  acc.worstrelarg=0.0;
  acc.worstrelres=0.0;
  acc.worstrelact=0.0;
  acc.errcount=0;
  testDoubleZero(acc,tf,rf);
  testDoubleInfinity(acc,tf,rf);
  testDoubleNaN(acc,tf,rf,1000000000);
  testDoubleFinite(acc,tf,rf,1000000000);
  report(acc,name);
  fprintf(stderr,"\n");
  }


// Test all
void testFloat(FLTFUN1 tf,FLTFUN1 rf,const char* name){
  SPAccuracy acc;
  fprintf(stderr,"Testing %s():\n",name);
  acc.worstabserr=0.0f;
  acc.worstabsarg=0.0f;
  acc.worstabsres=0.0f;
  acc.worstabsact=0.0f;
  acc.worstrelerr=0.0f;
  acc.worstrelarg=0.0f;
  acc.worstrelres=0.0f;
  acc.worstrelact=0.0f;
  acc.errcount=0;
  testFloatAll(acc,tf,rf);
  report(acc,name);
  fprintf(stderr,"\n");
  }


/*******************************************************************************/

// Self-check, started with --test: known values, edge cases and a bounded
// comparison against the C library; no long run, no arguments.

static FXint failures=0;
static FXbool verbose=false;
static const FXchar* kind="double";


// IEEE754 specials for either precision
template<class T> struct Num;

template<> struct Num<FXdouble> {
  static FXdouble inf(){ return Math::fpFlt(FXULONG(0x7ff0000000000000)); }
  static FXdouble nan(){ return Math::fpFlt(FXULONG(0x7ff8000000000000)); }
  static FXdouble big(){ return Math::fpFlt(FXULONG(0x7fefffffffffffff)); }
  };

template<> struct Num<FXfloat> {
  static FXfloat inf(){ return Math::fpFlt((FXuint)0x7f800000); }
  static FXfloat nan(){ return Math::fpFlt((FXuint)0x7fc00000); }
  static FXfloat big(){ return Math::fpFlt((FXuint)0x7f7fffff); }
  };


// Equal within relative tolerance (absolute if expected is zero); NaN only equals NaN
template<class T> static FXbool nearly(T got,T want,T tol){
  if(Math::fpNan(got) || Math::fpNan(want)) return Math::fpNan(got) && Math::fpNan(want);
  if(Math::fpInfinite(got) || Math::fpInfinite(want)) return got==want;
  if(got==want) return true;
  T d=Math::fabs(got-want);
  return d<=tol*Math::fabs(want) || (want==T(0) && d<=tol);
  }


// Check a result
template<class T> static void expect(const FXchar* what,T got,T want,T tol){
  if(!nearly(got,want,tol)){
    printf("BAD: [%s] %s = %.17g, expected %.17g\n",kind,what,(FXdouble)got,(FXdouble)want);
    failures++;
    }
  }


// Check a result is a zero with the given sign
template<class T> static void expectzero(const FXchar* what,T got,FXint negative){
  if(got!=T(0) || (FXint)Math::fpSign(got)!=negative){
    printf("BAD: [%s] %s = %.17g, expected %s zero\n",kind,what,(FXdouble)got,negative?"negative":"positive");
    failures++;
    }
  }


// Check an integer result
static void expectint(const FXchar* what,FXlong got,FXlong want){
  if(got!=want){
    printf("BAD: %s = %ld, expected %ld\n",what,(long)got,(long)want);
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


// Values which hold in both precisions
template<class T> static void testcommon(T tol){
  const T inf=Num<T>::inf();
  const T nan=Num<T>::nan();
  const T big=Num<T>::big();
  const T zero=T(0);
  const T one=T(1);
  const T pi=T(PI);
  T s,c;

  // Absolute value
  expect("fabs(-2.5)",Math::fabs(T(-2.5)),T(2.5),zero);
  expectzero("fabs(-0)",Math::fabs(-zero),0);
  expect("fabs(-inf)",Math::fabs(-inf),inf,zero);
  expect("fabs(nan)",Math::fabs(nan),nan,zero);

  // Rounding
  expect("ceil(1.2)",Math::ceil(T(1.2)),T(2),zero);
  expect("ceil(-1.2)",Math::ceil(T(-1.2)),T(-1),zero);
  expectzero("ceil(-0.5)",Math::ceil(T(-0.5)),1);
  expect("ceil(7)",Math::ceil(T(7)),T(7),zero);
  expect("ceil(inf)",Math::ceil(inf),inf,zero);
  expect("ceil(nan)",Math::ceil(nan),nan,zero);
  expect("floor(1.8)",Math::floor(T(1.8)),T(1),zero);
  expect("floor(-1.2)",Math::floor(T(-1.2)),T(-2),zero);
  expect("floor(-7)",Math::floor(T(-7)),T(-7),zero);
  expect("floor(-inf)",Math::floor(-inf),-inf,zero);
  expect("floor(nan)",Math::floor(nan),nan,zero);
  expect("round(2.5)",Math::round(T(2.5)),T(3),zero);
  expect("round(-2.5)",Math::round(T(-2.5)),T(-3),zero);
  expect("round(0.5)",Math::round(T(0.5)),T(1),zero);
  expect("round(2.4)",Math::round(T(2.4)),T(2),zero);
  expect("round(-2.6)",Math::round(T(-2.6)),T(-3),zero);
  expectzero("round(-0.4)",Math::round(T(-0.4)),1);
  expect("round(inf)",Math::round(inf),inf,zero);
  expect("round(nan)",Math::round(nan),nan,zero);
  expect("trunc(2.7)",Math::trunc(T(2.7)),T(2),zero);
  expect("trunc(-2.7)",Math::trunc(T(-2.7)),T(-2),zero);
  expectzero("trunc(-0.7)",Math::trunc(T(-0.7)),1);
  expect("trunc(-inf)",Math::trunc(-inf),-inf,zero);
  expect("trunc(nan)",Math::trunc(nan),nan,zero);
  expect("nearbyint(2.5)",Math::nearbyint(T(2.5)),T(2),zero);
  expect("nearbyint(3.5)",Math::nearbyint(T(3.5)),T(4),zero);
  expect("nearbyint(-2.5)",Math::nearbyint(T(-2.5)),T(-2),zero);
  expect("nearbyint(2.6)",Math::nearbyint(T(2.6)),T(3),zero);
  expect("nearbyint(inf)",Math::nearbyint(inf),inf,zero);
  expect("rint(2.5)",Math::rint(T(2.5)),T(2),zero);
  expect("rint(3.5)",Math::rint(T(3.5)),T(4),zero);
  expect("rint(-3.5)",Math::rint(T(-3.5)),T(-4),zero);
  expect("rint(nan)",Math::rint(nan),nan,zero);
  expectint("lrint(2.4)",Math::lrint(T(2.4)),2);
  expectint("lrint(-2.6)",Math::lrint(T(-2.6)),-3);
  expectint("lrint(0)",Math::lrint(zero),0);

  // Roots and powers of two and three
  expect("sqrt(0)",Math::sqrt(zero),zero,zero);
  expect("sqrt(4)",Math::sqrt(T(4)),T(2),zero);
  expect("sqrt(2)",Math::sqrt(T(2)),T(1.4142135623730951),tol);
  expectzero("sqrt(-0)",Math::sqrt(-zero),1);
  expect("sqrt(-1)",Math::sqrt(T(-1)),nan,zero);
  expect("sqrt(inf)",Math::sqrt(inf),inf,zero);
  expect("safesqrt(-4)",Math::safesqrt(T(-4)),zero,zero);
  expect("safesqrt(9)",Math::safesqrt(T(9)),T(3),zero);
  expect("cbrt(27)",Math::cbrt(T(27)),T(3),tol);
  expect("cbrt(-8)",Math::cbrt(T(-8)),T(-2),tol);
  expect("cbrt(0)",Math::cbrt(zero),zero,zero);
  expect("cbrt(inf)",Math::cbrt(inf),inf,zero);
  expect("cbrt(-inf)",Math::cbrt(-inf),-inf,zero);
  expect("cbrt(nan)",Math::cbrt(nan),nan,zero);
  expect("sqr(-3)",Math::sqr(T(-3)),T(9),zero);
  expect("cub(-2)",Math::cub(T(-2)),T(-8),zero);
  expect("rsqrt(4)",Math::rsqrt(T(4)),T(0.5),tol);
  expect("rsqrt(0)",Math::rsqrt(zero),inf,zero);
  expect("recip(4)",Math::recip(T(4)),T(0.25),zero);
  expect("recip(0)",Math::recip(zero),inf,zero);
  expect("recip(-0)",Math::recip(-zero),-inf,zero);
  expect("hypot(3,4)",Math::hypot(T(3),T(4)),T(5),tol);
  expect("hypot(-3,4)",Math::hypot(T(-3),T(4)),T(5),tol);
  expect("hypot(0,0)",Math::hypot(zero,zero),zero,zero);
  expect("hypot(inf,nan)",Math::hypot(inf,nan),inf,zero);

  // Trigonometry
  expect("sin(0)",Math::sin(zero),zero,zero);
  expectzero("sin(-0)",Math::sin(-zero),1);
  expect("sin(PI/2)",Math::sin(pi/T(2)),one,tol);
  expect("sin(PI)",Math::sin(pi),zero,tol);
  expect("sin(-PI/2)",Math::sin(-pi/T(2)),-one,tol);
  expect("sin(inf)",Math::sin(inf),nan,zero);
  expect("cos(0)",Math::cos(zero),one,zero);
  expect("cos(PI)",Math::cos(pi),-one,tol);
  expect("cos(PI/3)",Math::cos(pi/T(3)),T(0.5),tol);
  expect("cos(inf)",Math::cos(inf),nan,zero);
  expect("tan(0)",Math::tan(zero),zero,zero);
  expect("tan(PI/4)",Math::tan(pi/T(4)),one,tol);
  expect("tan(inf)",Math::tan(inf),nan,zero);
  expect("asin(1)",Math::asin(one),pi/T(2),tol);
  expect("asin(-1)",Math::asin(-one),-pi/T(2),tol);
  expect("asin(0.5)",Math::asin(T(0.5)),pi/T(6),tol);
  expect("asin(2)",Math::asin(T(2)),nan,zero);
  expect("acos(1)",Math::acos(one),zero,tol);
  expect("acos(-1)",Math::acos(-one),pi,tol);
  expect("acos(0)",Math::acos(zero),pi/T(2),tol);
  expect("acos(2)",Math::acos(T(2)),nan,zero);
  expect("atan(0)",Math::atan(zero),zero,zero);
  expect("atan(1)",Math::atan(one),pi/T(4),tol);
  expect("atan(inf)",Math::atan(inf),pi/T(2),tol);
  expect("atan(-inf)",Math::atan(-inf),-pi/T(2),tol);
  expect("atan2(1,1)",Math::atan2(one,one),pi/T(4),tol);
  expect("atan2(-1,-1)",Math::atan2(-one,-one),T(-3)*pi/T(4),tol);
  expect("atan2(1,0)",Math::atan2(one,zero),pi/T(2),tol);
  expect("atan2(0,-1)",Math::atan2(zero,-one),pi,tol);
  expect("atan2(-0,-1)",Math::atan2(-zero,-one),-pi,tol);
  expect("atan2(0,0)",Math::atan2(zero,zero),zero,zero);
  expect("atan2(nan,1)",Math::atan2(nan,one),nan,zero);
  Math::sincos(pi/T(6),s,c);
  expect("sincos(PI/6) sine",s,T(0.5),tol);
  expect("sincos(PI/6) cosine",c,T(0.86602540378443865),tol);

  // Hyperbolic functions
  expect("sinh(0)",Math::sinh(zero),zero,zero);
  expect("sinh(1)",Math::sinh(one),T(1.1752011936438014),tol);
  expect("sinh(-1)",Math::sinh(-one),T(-1.1752011936438014),tol);
  expect("sinh(1e-10)",Math::sinh(T(1e-10)),T(1e-10),tol);
  expect("sinh(inf)",Math::sinh(inf),inf,zero);
  expect("sinh(-inf)",Math::sinh(-inf),-inf,zero);
  expect("sinh(nan)",Math::sinh(nan),nan,zero);
  expect("cosh(0)",Math::cosh(zero),one,zero);
  expect("cosh(1)",Math::cosh(one),T(1.5430806348152437),tol);
  expect("cosh(-1)",Math::cosh(-one),T(1.5430806348152437),tol);
  expect("cosh(1e-10)",Math::cosh(T(1e-10)),one,tol);
  expect("cosh(-inf)",Math::cosh(-inf),inf,zero);
  expect("cosh(nan)",Math::cosh(nan),nan,zero);
  expect("tanh(0)",Math::tanh(zero),zero,zero);
  expect("tanh(1)",Math::tanh(one),T(0.76159415595576489),tol);
  expect("tanh(-1)",Math::tanh(-one),T(-0.76159415595576489),tol);
  expect("tanh(1e-10)",Math::tanh(T(1e-10)),T(1e-10),tol);
  expect("tanh(20)",Math::tanh(T(20)),one,tol);
  expect("tanh(inf)",Math::tanh(inf),one,zero);
  expect("tanh(-inf)",Math::tanh(-inf),-one,zero);
  expect("tanh(nan)",Math::tanh(nan),nan,zero);
  expect("asinh(0)",Math::asinh(zero),zero,zero);
  expect("asinh(1)",Math::asinh(one),T(0.88137358701954303),tol);
  expect("asinh(-1)",Math::asinh(-one),T(-0.88137358701954303),tol);
  expect("asinh(1e-10)",Math::asinh(T(1e-10)),T(1e-10),tol);
  expect("asinh(inf)",Math::asinh(inf),inf,zero);
  expect("asinh(-inf)",Math::asinh(-inf),-inf,zero);
  expect("asinh(nan)",Math::asinh(nan),nan,zero);
  expect("acosh(1)",Math::acosh(one),zero,zero);
  expect("acosh(2)",Math::acosh(T(2)),T(1.3169578969248166),tol);
  expect("acosh(0.5)",Math::acosh(T(0.5)),nan,zero);
  expect("acosh(-1)",Math::acosh(-one),nan,zero);
  expect("acosh(inf)",Math::acosh(inf),inf,zero);
  expect("acosh(nan)",Math::acosh(nan),nan,zero);
  expect("atanh(0)",Math::atanh(zero),zero,zero);
  expect("atanh(0.5)",Math::atanh(T(0.5)),T(0.54930614433405485),tol);
  expect("atanh(-0.5)",Math::atanh(T(-0.5)),T(-0.54930614433405485),tol);
  expect("atanh(1e-10)",Math::atanh(T(1e-10)),T(1e-10),tol);
  expect("atanh(1)",Math::atanh(one),inf,zero);
  expect("atanh(-1)",Math::atanh(-one),-inf,zero);
  expect("atanh(2)",Math::atanh(T(2)),nan,zero);
  expect("atanh(nan)",Math::atanh(nan),nan,zero);

  // Exponentials
  expect("exp(0)",Math::exp(zero),one,zero);
  expect("exp(1)",Math::exp(one),T(2.7182818284590452),tol);
  expect("exp(-1)",Math::exp(-one),T(0.36787944117144233),tol);
  expect("exp(inf)",Math::exp(inf),inf,zero);
  expect("exp(-inf)",Math::exp(-inf),zero,zero);
  expect("exp(nan)",Math::exp(nan),nan,zero);
  expect("expm1(0)",Math::expm1(zero),zero,zero);
  expect("expm1(1)",Math::expm1(one),T(1.7182818284590452),tol);
  expect("expm1(-1)",Math::expm1(-one),T(-0.63212055882855768),tol);
  expect("expm1(1e-10)",Math::expm1(T(1e-10)),T(1.00000000005e-10),tol);
  expect("expm1(inf)",Math::expm1(inf),inf,zero);
  expect("expm1(-inf)",Math::expm1(-inf),-one,zero);
  expect("expm1(nan)",Math::expm1(nan),nan,zero);
  expect("exp2(0)",Math::exp2(zero),one,zero);
  expect("exp2(10)",Math::exp2(T(10)),T(1024),tol);
  expect("exp2(-1)",Math::exp2(-one),T(0.5),tol);
  expect("exp2(0.5)",Math::exp2(T(0.5)),T(1.4142135623730951),tol);
  expect("exp2(inf)",Math::exp2(inf),inf,zero);
  expect("exp2(-inf)",Math::exp2(-inf),zero,zero);
  expect("exp2(nan)",Math::exp2(nan),nan,zero);
  expect("exp10(0)",Math::exp10(zero),one,zero);
  expect("exp10(3)",Math::exp10(T(3)),T(1000),tol);
  expect("exp10(-2)",Math::exp10(T(-2)),T(0.01),tol);
  expect("exp10(0.5)",Math::exp10(T(0.5)),T(3.1622776601683795),tol);
  expect("exp10(inf)",Math::exp10(inf),inf,zero);
  expect("exp10(-inf)",Math::exp10(-inf),zero,zero);
  expect("exp10(nan)",Math::exp10(nan),nan,zero);
  expect("pow10(2)",Math::pow10(T(2)),T(100),tol);
  expect("pow(2,10)",Math::pow(T(2),T(10)),T(1024),tol);
  expect("pow(2,-2)",Math::pow(T(2),T(-2)),T(0.25),tol);
  expect("pow(9,0.5)",Math::pow(T(9),T(0.5)),T(3),tol);
  expect("pow(-2,3)",Math::pow(T(-2),T(3)),T(-8),tol);
  expect("pow(0,0)",Math::pow(zero,zero),one,zero);
  expect("pow(nan,0)",Math::pow(nan,zero),one,zero);
  expect("pow(1,nan)",Math::pow(one,nan),one,zero);
  expect("pow(-8,1/3)",Math::pow(T(-8),one/T(3)),nan,zero);
  expect("pow(0,-1)",Math::pow(zero,-one),inf,zero);
  expect("pow(2,inf)",Math::pow(T(2),inf),inf,zero);
  expect("pow(0.5,inf)",Math::pow(T(0.5),inf),zero,zero);
  expect("powi(2,10)",Math::powi(T(2),10),T(1024),tol);
  expect("powi(2,-2)",Math::powi(T(2),-2),T(0.25),tol);
  expect("powi(-2,3)",Math::powi(T(-2),3),T(-8),tol);
  expect("powi(-2,2)",Math::powi(T(-2),2),T(4),tol);
  expect("powi(3,0)",Math::powi(T(3),0),one,zero);
  expect("powi(10,-3)",Math::powi(T(10),-3),T(0.001),tol);

  // Logarithms
  expect("log(1)",Math::log(one),zero,zero);
  expect("log(E)",Math::log(T(EULER)),one,tol);
  expect("log(0.5)",Math::log(T(0.5)),T(-0.69314718055994531),tol);
  expect("log(0)",Math::log(zero),-inf,zero);
  expect("log(-1)",Math::log(-one),nan,zero);
  expect("log(inf)",Math::log(inf),inf,zero);
  expect("log(nan)",Math::log(nan),nan,zero);
  expect("log1p(0)",Math::log1p(zero),zero,zero);
  expect("log1p(1)",Math::log1p(one),T(0.69314718055994531),tol);
  expect("log1p(1e-10)",Math::log1p(T(1e-10)),T(9.9999999995e-11),tol);
  expect("log1p(-0.5)",Math::log1p(T(-0.5)),T(-0.69314718055994531),tol);
  expect("log1p(-1)",Math::log1p(-one),-inf,zero);
  expect("log1p(-2)",Math::log1p(T(-2)),nan,zero);
  expect("log1p(inf)",Math::log1p(inf),inf,zero);
  expect("log2(1)",Math::log2(one),zero,zero);
  expect("log2(8)",Math::log2(T(8)),T(3),tol);
  expect("log2(0.5)",Math::log2(T(0.5)),-one,tol);
  expect("log2(1024)",Math::log2(T(1024)),T(10),tol);
  expect("log2(10)",Math::log2(T(10)),T(3.3219280948873623),tol);
  expect("log2(0)",Math::log2(zero),-inf,zero);
  expect("log2(-1)",Math::log2(-one),nan,zero);
  expect("log2(inf)",Math::log2(inf),inf,zero);
  for(FXint k=-100; k<=100; k++){
    FXchar name[32];
    snprintf(name,sizeof(name),"log2(2^%d)",k);
    expect(name,Math::log2(Math::powi(T(2),k)),T(k),zero);
    }
  expect("log10(1)",Math::log10(one),zero,zero);
  expect("log10(1000)",Math::log10(T(1000)),T(3),tol);
  expect("log10(1e-5)",Math::log10(T(1e-5)),T(-5),tol);
  expect("log10(2)",Math::log10(T(2)),T(0.3010299956639812),tol);
  expect("log10(0)",Math::log10(zero),-inf,zero);
  expect("log10(-1)",Math::log10(-one),nan,zero);
  expect("log10(inf)",Math::log10(inf),inf,zero);

  // Error function
  expect("erf(0)",Math::erf(zero),zero,zero);
  expect("erf(1)",Math::erf(one),T(0.84270079294971487),tol);
  expect("erf(-1)",Math::erf(-one),T(-0.84270079294971487),tol);
  expect("erf(0.1)",Math::erf(T(0.1)),T(0.1124629160182849),tol);
  expect("erf(1e-10)",Math::erf(T(1e-10)),T(1.1283791670955126e-10),tol);
  expect("erf(6)",Math::erf(T(6)),one,tol);
  expect("erf(inf)",Math::erf(inf),one,zero);
  expect("erf(-inf)",Math::erf(-inf),-one,zero);
  expect("erf(nan)",Math::erf(nan),nan,zero);
  expect("erfc(0)",Math::erfc(zero),one,zero);
  expect("erfc(1)",Math::erfc(one),T(0.15729920705028513),tol);
  expect("erfc(-1)",Math::erfc(-one),T(1.8427007929497149),tol);
  expect("erfc(inf)",Math::erfc(inf),zero,zero);
  expect("erfc(-inf)",Math::erfc(-inf),T(2),zero);
  expect("erfc(nan)",Math::erfc(nan),nan,zero);
  expect("inverf(0)",Math::inverf(zero),zero,tol);
  expect("inverf(0.5)",Math::inverf(T(0.5)),T(0.47693627620446987),tol);
  expect("inverf(-0.5)",Math::inverf(T(-0.5)),T(-0.47693627620446987),tol);
  expect("inverf(erf(1))",Math::inverf(Math::erf(one)),one,T(10)*tol);
  expect("inverf(1)",Math::inverf(one),big,zero);  // Clamped to largest finite value
  expect("inverf(-1)",Math::inverf(-one),-big,zero);
  expect("inverf(nan)",Math::inverf(nan),nan,zero);
  expect("inverfc(1)",Math::inverfc(one),zero,tol);
  expect("inverfc(0.5)",Math::inverfc(T(0.5)),T(0.47693627620446987),tol);
  expect("inverfc(0)",Math::inverfc(zero),big,zero);
  expect("inverfc(2)",Math::inverfc(T(2)),-big,zero);
  expect("inverfc(1e-3)",Math::inverfc(T(1e-3)),T(2.3267537655135246),T(100)*tol);

  // Minimum, maximum, clamping and the like
  expect("fmin(2,3)",Math::fmin(T(2),T(3)),T(2),zero);
  expect("fmax(2,3)",Math::fmax(T(2),T(3)),T(3),zero);
  expect("fmin(nan,1)",Math::fmin(nan,one),one,zero);
  expect("fmax(1,nan)",Math::fmax(one,nan),one,zero);
  expect("fmin(1,nan)",Math::fmin(one,nan),one,zero);
  expect("fmax(nan,1)",Math::fmax(nan,one),one,zero);
  expect("fmin(nan,nan)",Math::fmin(nan,nan),nan,zero);
  expect("fmax(nan,nan)",Math::fmax(nan,nan),nan,zero);
  expect("fclamp(0,5,1)",Math::fclamp(zero,T(5),one),one,zero);
  expect("fclamp(0,-5,1)",Math::fclamp(zero,T(-5),one),zero,zero);
  expect("fclamp(0,0.5,1)",Math::fclamp(zero,T(0.5),one),T(0.5),zero);
  expect("fclamp(5,2)",Math::fclamp(T(5),T(2)),T(2),zero);
  expect("fclamp(-5,2)",Math::fclamp(T(-5),T(2)),T(-2),zero);
  expect("fdim(5,3)",Math::fdim(T(5),T(3)),T(2),zero);
  expect("fdim(3,5)",Math::fdim(T(3),T(5)),zero,zero);
  expect("fmod(5.5,2)",Math::fmod(T(5.5),T(2)),T(1.5),zero);
  expect("fmod(-5.5,2)",Math::fmod(T(-5.5),T(2)),T(-1.5),zero);
  expect("fmod(1,0)",Math::fmod(one,zero),nan,zero);
  expect("copysign(3,-0)",Math::copysign(T(3),-zero),T(-3),zero);
  expect("copysign(-3,1)",Math::copysign(T(-3),one),T(3),zero);
  expect("copysign(inf,-1)",Math::copysign(inf,-one),-inf,zero);
  expectzero("copysign(0,-1)",Math::copysign(zero,-one),1);
  expect("fblend(1,2,10,20)",Math::fblend(one,T(2),T(10),T(20)),T(10),zero);
  expect("fblend(2,1,10,20)",Math::fblend(T(2),one,T(10),T(20)),T(20),zero);
  expect("lerp(2,4,0)",Math::lerp(T(2),T(4),zero),T(2),zero);
  expect("lerp(2,4,0.5)",Math::lerp(T(2),T(4),T(0.5)),T(3),zero);
  expect("lerp(2,4,1)",Math::lerp(T(2),T(4),one),T(4),zero);
  expect("smoothstep(0)",Math::smoothstep(zero),zero,zero);
  expect("smoothstep(0.25)",Math::smoothstep(T(0.25)),T(0.15625),zero);
  expect("smoothstep(0.5)",Math::smoothstep(T(0.5)),T(0.5),zero);
  expect("smoothstep(1)",Math::smoothstep(one),one,zero);

  // Phase wrapping, stepping and periodic functions
  expect("wrap(0)",Math::wrap(zero),zero,zero);
  expect("wrap(7)",Math::wrap(T(7)),T(0.71681469282041353),T(10)*tol);
  expect("wrap(-7)",Math::wrap(T(-7)),T(-0.71681469282041353),T(10)*tol);
  expect("wrap(1)",Math::wrap(one),one,zero);
  expect("wrap4(0)",Math::wrap4(zero),zero,zero);
  expect("wrap4(-1)",Math::wrap4(-one),T(5.2831853071795865),tol);
  expect("wrap4(7)",Math::wrap4(T(7)),T(0.71681469282041353),T(10)*tol);
  expect("stepify(1.3,0.5)",Math::stepify(T(1.3),T(0.5)),T(1.5),zero);
  expect("stepify(1.2,0.5)",Math::stepify(T(1.2),T(0.5)),one,zero);
  expect("stepify(-1.3,0.5)",Math::stepify(T(-1.3),T(0.5)),T(-1.5),zero);
  expect("zigzag(0)",Math::zigzag(zero),zero,zero);
  expect("zigzag(0.25)",Math::zigzag(T(0.25)),T(0.5),zero);
  expect("zigzag(0.75)",Math::zigzag(T(0.75)),T(0.5),zero);
  expect("zigzag(1)",Math::zigzag(one),zero,zero);
  expect("sawtooth(3)",Math::sawtooth(T(3)),zero,zero);
  expect("sawtooth(1.25)",Math::sawtooth(T(1.25)),T(0.25),zero);
  expect("sawtooth(-0.25)",Math::sawtooth(T(-0.25)),T(0.75),zero);
  expect("rsawtooth(3)",Math::rsawtooth(T(3)),zero,zero);
  expect("rsawtooth(1.25)",Math::rsawtooth(T(1.25)),T(0.75),zero);
  expect("rsawtooth(-0.25)",Math::rsawtooth(T(-0.25)),T(0.25),zero);

  // Classification
  expecttrue("fpFinite(1)",Math::fpFinite(one));
  expecttrue("!fpFinite(inf)",!Math::fpFinite(inf));
  expecttrue("!fpFinite(nan)",!Math::fpFinite(nan));
  expecttrue("fpInfinite(-inf)",Math::fpInfinite(-inf));
  expecttrue("!fpInfinite(nan)",!Math::fpInfinite(nan));
  expecttrue("!fpInfinite(1)",!Math::fpInfinite(one));
  expecttrue("fpNan(nan)",Math::fpNan(nan));
  expecttrue("!fpNan(inf)",!Math::fpNan(inf));
  expecttrue("!fpNan(1)",!Math::fpNan(one));
  expecttrue("fpNormal(1)",Math::fpNormal(one));
  expecttrue("fpNormal(0)",Math::fpNormal(zero));
  expecttrue("!fpNormal(nan)",!Math::fpNormal(nan));
  expectint("fpSign(1)",Math::fpSign(one),0);
  expectint("fpSign(-1)",Math::fpSign(-one),1);
  expectint("fpSign(-0)",Math::fpSign(-zero),1);
  expectint("fpExponent(1)",Math::fpExponent(one),0);
  expectint("fpExponent(2)",Math::fpExponent(T(2)),1);
  expectint("fpExponent(0.5)",Math::fpExponent(T(0.5)),-1);
  expectint("fpExponent(0.75)",Math::fpExponent(T(0.75)),-1);
  }


// Values which only hold in double precision
static void testdouble(){
  const FXdouble inf=Num<FXdouble>::inf();
  const FXdouble nan=Num<FXdouble>::nan();
  const FXdouble tol=1E-14;
  const FXdouble tiny=Math::fpMake((FXulong)1,(FXulong)0,(FXulong)0);                 // Smallest denormal
  const FXdouble least=Math::fpMake((FXulong)0,(FXulong)1,(FXulong)0);                // Smallest normal
  const FXdouble two52=4503599627370496.0;
  const FXdouble big=1.7976931348623157e308;
  kind="double";
  testcommon<FXdouble>(tol);

  // Bit patterns
  expecttrue("fpBits(1)",Math::fpBits(1.0)==FXULONG(0x3ff0000000000000));
  expecttrue("fpBits(-2)",Math::fpBits(-2.0)==FXULONG(0xc000000000000000));
  expecttrue("fpFlt(bits of 1.5)",Math::fpFlt(FXULONG(0x3ff8000000000000))==1.5);
  expect("fpMake(0)",Math::fpMake((FXulong)0),1.0,0.0);
  expect("fpMake(1<<51)",Math::fpMake((FXulong)1<<51),1.5,0.0);
  expect("fpMake(0,1024,1)",Math::fpMake((FXulong)0,(FXulong)1024,(FXulong)1),-2.0,0.0);
  expectint("fpMantissa(1)",Math::fpMantissa(1.0),FXLONG(0x0010000000000000));
  expectint("fpExponent(denormal)",Math::fpExponent(tiny),-1022);
  expectint("fpMantissa(denormal)",Math::fpMantissa(tiny),1);
  expecttrue("!fpNormal(denormal)",!Math::fpNormal(tiny));
  expecttrue("fpNormal(least)",Math::fpNormal(least));
  expecttrue("fpFinite(big)",Math::fpFinite(big));

  // Integers too large to have a fraction
  expect("round(2^52+1)",Math::round(two52+1.0),two52+1.0,0.0);
  expect("round(-2^52-1)",Math::round(-two52-1.0),-two52-1.0,0.0);
  expect("ceil(2^52+1)",Math::ceil(two52+1.0),two52+1.0,0.0);
  expect("floor(2^52+1)",Math::floor(two52+1.0),two52+1.0,0.0);
  expect("trunc(2^52+1)",Math::trunc(two52+1.0),two52+1.0,0.0);
  expect("nearbyint(2^52+1)",Math::nearbyint(two52+1.0),two52+1.0,0.0);
  expect("rint(2^52+1)",Math::rint(two52+1.0),two52+1.0,0.0);
  expect("round(1e300)",Math::round(1e300),1e300,0.0);
  expect("floor(-1e300)",Math::floor(-1e300),-1e300,0.0);
  expect("ceil(1e300)",Math::ceil(1e300),1e300,0.0);
  expect("trunc(1e300)",Math::trunc(1e300),1e300,0.0);

  // Largest number below one half must not round up
  expect("round(0.49999999999999994)",Math::round(0.49999999999999994),0.0,0.0);
  expect("round(-0.49999999999999994)",Math::round(-0.49999999999999994),0.0,0.0);
  expect("round(0.5-ulp)+1",Math::round(1.4999999999999998),1.0,0.0);

  // Denormals
  expect("fabs(denormal)",Math::fabs(-tiny),tiny,0.0);
  expect("sqrt(denormal)",Math::sqrt(tiny),2.2227587494850775e-162,tol);
  expect("sinh(denormal)",Math::sinh(tiny),tiny,0.0);
  expect("tanh(denormal)",Math::tanh(tiny),tiny,0.0);
  expect("asinh(denormal)",Math::asinh(tiny),tiny,0.0);
  expect("atanh(denormal)",Math::atanh(tiny),tiny,0.0);
  expect("expm1(denormal)",Math::expm1(tiny),tiny,0.0);
  expect("log1p(denormal)",Math::log1p(tiny),tiny,0.0);
  expect("erf(smallest normal)",Math::erf(least),least*1.1283791670955126,1E-12);
  expect("exp2(-1074)",Math::exp2(-1074.0),tiny,0.0);
  expect("exp2(-1080)",Math::exp2(-1080.0),0.0,0.0);
  expect("log2(denormal)",Math::log2(tiny),-1074.0,tol);
  expect("log(denormal)",Math::log(tiny),-1074.0*0.69314718055994531,tol);
  expect("log10(denormal)",Math::log10(tiny),-1074.0*0.30102999566398120,tol);
  expect("log10(smallest normal)",Math::log10(least),-1022.0*0.30102999566398120,tol);
  expect("cos(denormal)",Math::cos(tiny),1.0,0.0);
  expect("hypot(denormal,0)",Math::hypot(tiny,0.0),tiny,0.0);

  // Overflow and underflow
  expect("exp(709)",Math::exp(709.0),8.2184074615549724e307,tol);
  expect("exp(710)",Math::exp(710.0),inf,0.0);
  expect("exp(-800)",Math::exp(-800.0),0.0,0.0);
  expect("expm1(710)",Math::expm1(710.0),inf,0.0);
  expect("expm1(-800)",Math::expm1(-800.0),-1.0,0.0);
  expect("exp2(1023)",Math::exp2(1023.0),8.98846567431158e307,tol);
  expect("exp2(1024)",Math::exp2(1024.0),inf,0.0);
  expect("exp2(-2000)",Math::exp2(-2000.0),0.0,0.0);
  expect("exp10(308)",Math::exp10(308.0),1e308,tol);
  expect("exp10(309)",Math::exp10(309.0),inf,0.0);
  expect("exp10(-400)",Math::exp10(-400.0),0.0,0.0);
  expect("sinh(710)",Math::sinh(710.0),1.1169973830808557e308,tol);
  expect("sinh(711)",Math::sinh(711.0),inf,0.0);
  expect("sinh(-711)",Math::sinh(-711.0),-inf,0.0);
  expect("cosh(710)",Math::cosh(710.0),1.1169973830808557e308,tol);
  expect("cosh(-711)",Math::cosh(-711.0),inf,0.0);
  expect("tanh(710)",Math::tanh(710.0),1.0,0.0);
  expect("tanh(-710)",Math::tanh(-710.0),-1.0,0.0);
  expect("pow(2,1024)",Math::pow(2.0,1024.0),inf,0.0);
  expect("pow(10,308)",Math::pow(10.0,308.0),1e308,tol);
  expect("pow(10,-324)",Math::pow(10.0,-324.0),0.0,0.0);
  expect("powi(10,308)",Math::powi(10.0,308),1e308,tol);
  expect("powi(10,309)",Math::powi(10.0,309),inf,0.0);
  expect("powi(10,-324)",Math::powi(10.0,-324),0.0,0.0);
  expect("pow10i(0)",Math::pow10i(0),1.0,0.0);
  expect("pow10i(3)",Math::pow10i(3),1000.0,0.0);
  expect("pow10i(-3)",Math::pow10i(-3),0.001,tol);
  expect("pow10i(22)",Math::pow10i(22),1e22,0.0);
  expect("pow10i(308)",Math::pow10i(308),1e308,tol);
  expect("sqr(1e200)",Math::sqr(1e200),inf,0.0);
  expect("cbrt(1e300)",Math::cbrt(1e300),1e100,tol);
  expect("cbrt(-1e300)",Math::cbrt(-1e300),-1e100,tol);
  expect("log(max)",Math::log(big),709.78271289338397,tol);
  expect("log2(max)",Math::log2(big),1024.0,tol);
  expect("log10(max)",Math::log10(big),308.25471555991675,tol);
  expect("asinh(1e300)",Math::asinh(1e300),691.46867507877370,tol);
  expect("asinh(-1e300)",Math::asinh(-1e300),-691.46867507877370,tol);
  expect("acosh(1e300)",Math::acosh(1e300),691.46867507877370,tol);
  expect("atanh(1-2^-53)",Math::atanh(1.0-1.1102230246251565e-16),18.714973875118524,tol);

  // Intermediate results which overflow or underflow
  expect("hypot(1e200,1e200)",Math::hypot(1e200,1e200),1.4142135623730951e200,tol);
  expect("hypot(1e-200,1e-200)",Math::hypot(1e-200,1e-200),1.4142135623730951e-200,tol);
  expect("hypot(max,max)",Math::hypot(big,big),inf,0.0);
  expect("hypot(nan,1)",Math::hypot(nan,1.0),nan,0.0);
  expecttrue("wrap(1e10) is within -PI..PI",Math::fabs(Math::wrap(1e10))<=PI);
  expecttrue("wrap4(1e10) is within 0..2*PI",Math::wrap4(1e10)>=0.0 && Math::wrap4(1e10)<=2.0*PI);
  }


// Values which only hold in single precision
static void testfloat(){
  const FXfloat inf=Num<FXfloat>::inf();
  const FXfloat nan=Num<FXfloat>::nan();
  const FXfloat tol=5E-6f;
  const FXfloat tiny=Math::fpMake((FXuint)1,(FXuint)0,(FXuint)0);                     // Smallest denormal
  const FXfloat least=Math::fpMake((FXuint)0,(FXuint)1,(FXuint)0);                    // Smallest normal
  const FXfloat two23=8388608.0f;
  const FXfloat big=3.4028234663852886e38f;
  kind="float";
  testcommon<FXfloat>(tol);

  // Bit patterns
  expecttrue("fpBits(1)",Math::fpBits(1.0f)==0x3f800000u);
  expecttrue("fpBits(-2)",Math::fpBits(-2.0f)==0xc0000000u);
  expecttrue("fpFlt(bits of 1.5)",Math::fpFlt((FXuint)0x3fc00000)==1.5f);
  expect("fpMake(0)",Math::fpMake((FXuint)0),1.0f,0.0f);
  expect("fpMake(1<<22)",Math::fpMake((FXuint)1<<22),1.5f,0.0f);
  expect("fpMake(0,128,1)",Math::fpMake((FXuint)0,(FXuint)128,(FXuint)1),-2.0f,0.0f);
  expectint("fpMantissa(1)",Math::fpMantissa(1.0f),0x00800000);
  expectint("fpExponent(denormal)",Math::fpExponent(tiny),-126);
  expectint("fpMantissa(denormal)",Math::fpMantissa(tiny),1);
  expecttrue("!fpNormal(denormal)",!Math::fpNormal(tiny));
  expecttrue("fpNormal(least)",Math::fpNormal(least));
  expecttrue("fpFinite(big)",Math::fpFinite(big));

  // Integers too large to have a fraction
  expect("round(2^23+1)",Math::round(two23+1.0f),two23+1.0f,0.0f);
  expect("round(-2^23-1)",Math::round(-two23-1.0f),-two23-1.0f,0.0f);
  expect("ceil(2^23+1)",Math::ceil(two23+1.0f),two23+1.0f,0.0f);
  expect("floor(2^23+1)",Math::floor(two23+1.0f),two23+1.0f,0.0f);
  expect("trunc(2^23+1)",Math::trunc(two23+1.0f),two23+1.0f,0.0f);
  expect("nearbyint(2^23+1)",Math::nearbyint(two23+1.0f),two23+1.0f,0.0f);
  expect("rint(2^23+1)",Math::rint(two23+1.0f),two23+1.0f,0.0f);
  expect("round(1e30)",Math::round(1e30f),1e30f,0.0f);
  expect("floor(-1e30)",Math::floor(-1e30f),-1e30f,0.0f);

  // Largest number below one half must not round up
  expect("round(0.49999997)",Math::round(0.49999997f),0.0f,0.0f);
  expect("round(-0.49999997)",Math::round(-0.49999997f),0.0f,0.0f);

  // Denormals
  expect("fabs(denormal)",Math::fabs(-tiny),tiny,0.0f);
  expect("sinh(denormal)",Math::sinh(tiny),tiny,0.0f);
  expect("tanh(denormal)",Math::tanh(tiny),tiny,0.0f);
  expect("asinh(denormal)",Math::asinh(tiny),tiny,0.0f);
  expect("atanh(denormal)",Math::atanh(tiny),tiny,0.0f);
  expect("expm1(denormal)",Math::expm1(tiny),tiny,0.0f);
  expect("log1p(denormal)",Math::log1p(tiny),tiny,0.0f);
  expect("exp2(-149)",Math::exp2(-149.0f),tiny,0.0f);
  expect("exp2(-160)",Math::exp2(-160.0f),0.0f,0.0f);
  expect("log2(denormal)",Math::log2(tiny),-149.0f,tol);
  expect("log10(smallest normal)",Math::log10(least),-126.0f*0.30102999566398120f,tol);

  // Overflow and underflow
  expect("exp(88)",Math::exp(88.0f),1.6516362549940018e38f,tol);
  expect("exp(89)",Math::exp(89.0f),inf,0.0f);
  expect("exp(-200)",Math::exp(-200.0f),0.0f,0.0f);
  expect("expm1(89)",Math::expm1(89.0f),inf,0.0f);
  expect("expm1(-200)",Math::expm1(-200.0f),-1.0f,0.0f);
  expect("exp2(127)",Math::exp2(127.0f),1.7014118346046923e38f,tol);
  expect("exp2(128)",Math::exp2(128.0f),inf,0.0f);
  expect("exp2(-300)",Math::exp2(-300.0f),0.0f,0.0f);
  expect("exp10(38)",Math::exp10(38.0f),1e38f,tol);
  expect("exp10(39)",Math::exp10(39.0f),inf,0.0f);
  expect("exp10(-60)",Math::exp10(-60.0f),0.0f,0.0f);
  expect("sinh(89)",Math::sinh(89.0f),2.2448064e38f,tol);
  expect("sinh(90)",Math::sinh(90.0f),inf,0.0f);
  expect("sinh(-90)",Math::sinh(-90.0f),-inf,0.0f);
  expect("cosh(89)",Math::cosh(89.0f),2.2448064e38f,tol);
  expect("cosh(-90)",Math::cosh(-90.0f),inf,0.0f);
  expect("tanh(90)",Math::tanh(90.0f),1.0f,0.0f);
  expect("tanh(-90)",Math::tanh(-90.0f),-1.0f,0.0f);
  expect("pow(2,128)",Math::pow(2.0f,128.0f),inf,0.0f);
  expect("pow(10,38)",Math::pow(10.0f,38.0f),1e38f,tol);
  expect("pow(10,-50)",Math::pow(10.0f,-50.0f),0.0f,0.0f);
  expect("powi(10,38)",Math::powi(10.0f,38),1e38f,tol);
  expect("powi(10,39)",Math::powi(10.0f,39),inf,0.0f);
  expect("sqr(1e30)",Math::sqr(1e30f),inf,0.0f);
  expect("cbrt(1e30)",Math::cbrt(1e30f),1e10f,tol);
  expect("log(max)",Math::log(big),88.722839052068f,tol);
  expect("log2(max)",Math::log2(big),128.0f,tol);
  expect("log10(max)",Math::log10(big),38.531839444f,tol);
  expect("asinh(1e30)",Math::asinh(1e30f),69.770699970f,tol);
  expect("asinh(-1e30)",Math::asinh(-1e30f),-69.770699970f,tol);
  expect("acosh(1e30)",Math::acosh(1e30f),69.770699970f,tol);

  // Intermediate results which overflow or underflow
  expect("hypot(1e30,1e30)",Math::hypot(1e30f,1e30f),1.4142135e30f,tol);
  expect("hypot(1e-30,1e-30)",Math::hypot(1e-30f,1e-30f),1.4142135e-30f,tol);
  expect("hypot(max,max)",Math::hypot(big,big),inf,0.0f);
  expect("hypot(nan,1)",Math::hypot(nan,1.0f),nan,0.0f);
  }


// Integer helpers
static void testinteger(){
  expectint("imin(3,-2)",Math::imin(3,-2),-2);
  expectint("imax(3,-2)",Math::imax(3,-2),3);
  expectint("imin(3u,2u)",Math::imin(3u,2u),2);
  expectint("imax(3u,4000000000u)",Math::imax(3u,4000000000u),4000000000LL);
  expectint("imin(long)",Math::imin((FXlong)-5,(FXlong)7),-5);
  expectint("imax(long)",Math::imax((FXlong)-5,(FXlong)7),7);
  expectint("iabs(-7)",Math::iabs(-7),7);
  expectint("iabs(7)",Math::iabs(7),7);
  expectint("iabs(0)",Math::iabs(0),0);
  expectint("iabs(short)",Math::iabs((FXshort)-300),300);
  expectint("iabs(long)",Math::iabs((FXlong)-5000000000LL),5000000000LL);
  expectint("iclamp(0,5,3)",Math::iclamp(0,5,3),3);
  expectint("iclamp(0,-5,3)",Math::iclamp(0,-5,3),0);
  expectint("iclamp(0,2,3)",Math::iclamp(0,2,3),2);
  expectint("iclamp(-9,4)",Math::iclamp(-9,4),-4);
  expectint("iclamp(9,4)",Math::iclamp(9,4),4);
  expectint("iclamp(3,4)",Math::iclamp(3,4),3);
  expectint("iclamp(long)",Math::iclamp((FXlong)0,(FXlong)5000000000LL,(FXlong)7),7);
  expectint("isign(-9)",Math::isign(-9),-1);
  expectint("isign(0)",Math::isign(0),0);
  expectint("isign(9)",Math::isign(9),1);
  expectint("isign(long)",Math::isign((FXlong)-9000000000LL),-1);
  expectint("iblend(1,2,10,20)",Math::iblend(1,2,10,20),10);
  expectint("iblend(2,1,10,20)",Math::iblend(2,1,10,20),20);
  expectint("iblend(2,2,10,20)",Math::iblend(2,2,10,20),20);
  }


// Position of the number on the line of all numbers
static FXulong ordered(FXdouble f){
  FXulong u=Math::fpBits(f);
  return (u&FXULONG(0x8000000000000000)) ? FXULONG(0x8000000000000000)-(u&FXULONG(0x7fffffffffffffff)) : FXULONG(0x8000000000000000)+u;
  }

static FXulong ordered(FXfloat f){
  FXulong u=Math::fpBits(f);
  return (u&FXULONG(0x80000000)) ? FXULONG(0x80000000)-(u&FXULONG(0x7fffffff)) : FXULONG(0x80000000)+u;
  }


// Number of representable values between a and b; NaN equals NaN, infinity only itself
template<class T> static FXulong ulpdist(T a,T b){
  FXulong x,y;
  if(Math::fpNan(a) || Math::fpNan(b)) return (Math::fpNan(a) && Math::fpNan(b)) ? 0 : FXULONG(0xffffffffffffffff);
  x=ordered(a);
  y=ordered(b);
  return (x>y) ? x-y : y-x;
  }


// Domain for the sampled comparison, and number of values allowed between result and reference
struct Domain {
  FXdouble lo;
  FXdouble hi;
  FXulong  dblulps;
  FXulong  fltulps;
  };


// Same order as the names and function tables above
static const Domain domains[]={
  {-1E6,  1E6,   0, 0},         // fabs
  {-1E6,  1E6,   0, 0},         // ceil
  {-1E6,  1E6,   0, 0},         // floor
  {-1E6,  1E6,   0, 0},         // round
  {-1E6,  1E6,   0, 0},         // trunc
  {-1E6,  1E6,   0, 0},         // nearbyint
  {-1E6,  1E6,   0, 0},         // rint
  {-20.0, 20.0,  4, 4},         // sinh
  {-20.0, 20.0,  4, 4},         // cosh
  {-10.0, 10.0,  4, 4},         // tanh
  {-1E3,  1E3,   4, 4},         // asinh
  {1.0,   1E3,   4, 4},         // acosh
  {-0.99, 0.99,  4, 4},         // atanh
  {-5.0,  5.0,   4, 4},         // expm1
  {-20.0, 20.0,  4, 4},         // exp2
  {-10.0, 10.0,  4, 4},         // exp10
  {-0.9,  10.0,  4, 4},         // log1p
  {1E-6,  1E3,   4, 4},         // log2
  {-4.0,  4.0,   4, 0}          // erf, single precision reference is only accurate to 2E-6
  };


// Compare one function with its reference over a bounded sample in double precision
static void sampledouble(FXuint func){
  const Domain& dom=domains[func];
  FXulong worst=0,count=0;
  FXdouble worstarg=0.0,worstact=0.0,worstref=0.0;
  for(FXint i=0; i<30000; i++){
    FXdouble arg;
    if(i<20000) arg=dom.lo+(dom.hi-dom.lo)*rng.randDouble();            // Inside the usual domain
    else if(i<28000) arg=makeDouble(0);                                 // Any bit pattern: huge, tiny, infinity, NaN
    else arg=makeDouble(FORCE_DEN);                                     // Denormals
    FXdouble act=dblfuns[func].tf(arg);
    FXdouble ref=dblfuns[func].rf(arg);
    FXulong d=ulpdist(act,ref);
    if(worst<d){ worst=d; worstarg=arg; worstact=act; worstref=ref; }
    if(dom.dblulps<d) count++;
    }
  if(verbose) printf("%-10s double: worst %lu ulps\n",names[func],(unsigned long)worst);
  if(count){
    printf("BAD: [double] %s(%.17g) = %.17g, reference %.17g, %lu ulps apart (allowed %lu), %lu values too far\n",names[func],worstarg,worstact,worstref,(unsigned long)worst,(unsigned long)dom.dblulps,(unsigned long)count);
    failures++;
    }
  }


// Compare one function with its reference over a bounded sample in single precision
static void samplefloat(FXuint func){
  const Domain& dom=domains[func];
  FXulong worst=0,count=0;
  FXfloat worstarg=0.0f,worstact=0.0f,worstref=0.0f;
  FXuint pattern=1;
  for(FXint i=0; i<80000; i++){
    FXfloat arg;
    if(i<20000) arg=(FXfloat)(dom.lo+(dom.hi-dom.lo)*rng.randDouble()); // Inside the usual domain
    else{ arg=Math::fpFlt(pattern); pattern+=65537; }                   // Bit patterns over the entire range of floats
    FXfloat act=fltfuns[func].tf(arg);
    FXfloat ref=fltfuns[func].rf(arg);
    FXbool ok;
    FXulong d=ulpdist(act,ref);
    if(dom.fltulps==0 && func==18){                                     // Error function, compare absolutely
      ok=(Math::fpNan(act)&&Math::fpNan(ref)) || Math::fabs(act-ref)<=2E-6f;
      d=ok?0:1;
      }
    else ok=(d<=dom.fltulps);
    if(worst<d){ worst=d; worstarg=arg; worstact=act; worstref=ref; }
    if(!ok) count++;
    }
  if(verbose) printf("%-10s float:  worst %lu ulps\n",names[func],(unsigned long)worst);
  if(count){
    printf("BAD: [float] %s(%.9g) = %.9g, reference %.9g, %lu ulps apart (allowed %lu), %lu values too far\n",names[func],(FXdouble)worstarg,(FXdouble)worstact,(FXdouble)worstref,(unsigned long)worst,(unsigned long)dom.fltulps,(unsigned long)count);
    failures++;
    }
  }


// Run the self-check
static FXint selftest(){
  rng.seed(20260101);
  testdouble();
  testfloat();
  testinteger();
  for(FXuint func=0; func<ARRAYNUMBER(dblfuns); func++){
    sampledouble(func);
    samplefloat(func);
    }
  return failures;
  }


// Start
int main(int argc,char *argv[]){
  fxtestargs(argc,argv);
  FXdouble dbl_arg,dbl_res,dbl_act;
  FXfloat  flt_arg,flt_res,flt_act;
  FXint    func;

  // Bounded self-check
  if(1<argc && strcmp(argv[1],"--test")==0){
    verbose=(2<argc && strcmp(argv[2],"--verbose")==0);
    if(selftest()){
      printf("FAILED: %d check(s)\n",failures);
      return 1;
      }
    printf("all checks passed\n");
    return 0;
    }
  rng.seed(FXThread::time());

  if(3<argc){
    func=findFunc(argv[1]);
    if(func<0){
      fprintf(stderr,"Unknown function: %s\n",argv[1]);
      exit(1);
      }
    if(argv[2][0]=='d'){
      dbl_arg=strtod(argv[3],nullptr);
      dbl_act=dblfuns[func].tf(dbl_arg);
      dbl_res=dblfuns[func].rf(dbl_arg);
      fprintf(stderr,"%s()\n",argv[1]);
      fprintf(stderr,"arg: ");
      printDP(dbl_arg);
      fprintf(stderr,"act: ");
      printDP(dbl_act);
      fprintf(stderr,"res: ");
      printDP(dbl_res);
      }
    else{
      flt_arg=strtof(argv[3],nullptr);
      flt_act=fltfuns[func].tf(flt_arg);
      flt_res=fltfuns[func].rf(flt_arg);
      fprintf(stderr,"%s()\n",argv[1]);
      fprintf(stderr,"arg: ");
      printSP(flt_arg);
      fprintf(stderr,"act: ");
      printSP(flt_act);
      fprintf(stderr,"res: ");
      printSP(flt_res);
      }
    return 0;
    }

  // Error function
  testFloat(Math::erf,erfast,"erff");
  testDouble(Math::erf,my_erf,"erf");

  // Hyperbolic sine, cosine, tangent
  testDouble(Math::sinh,sinh,"sinh");
  testFloat(Math::sinh,sinhf,"sinhf");
  testDouble(Math::cosh,cosh,"cosh");
  testFloat(Math::cosh,coshf,"coshf");
  testDouble(Math::tanh,tanh,"tanh");
  testFloat(Math::tanh,tanhf,"tanhf");

  // Exp
  testDouble(Math::expm1,expm1,"expm1");
  testFloat(Math::expm1,expm1f,"expm1f");
  testDouble(Math::exp2,exp2,"exp2");
  testFloat(Math::exp2,exp2f,"exp2f");
  testDouble(Math::exp10,exp10,"exp10");
  testFloat(Math::exp10,exp10f,"exp10f");

  // Inverse hyperbolic sine, cosine, tangent
  testDouble(Math::asinh,asinh,"asinh");
  testFloat(Math::asinh,asinhf,"asinhf");
  testDouble(Math::acosh,acosh,"acosh");
  testFloat(Math::acosh,acoshf,"acoshf");
  testDouble(Math::atanh,atanh,"atanh");
  testFloat(Math::atanh,atanhf,"atanhf");

  // Ceil and floor
  testDouble(Math::ceil,ceil,"ceil");
  testFloat(Math::ceil,ceilf,"ceilf");
  testDouble(Math::floor,floor,"floor");
  testFloat(Math::floor,floorf,"floorf");

  // Round and trunc
  testDouble(Math::round,round,"round");
  testFloat(Math::round,roundf,"roundf");
  testDouble(Math::trunc,trunc,"trunc");
  testFloat(Math::trunc,truncf,"truncf");

  // Round to nearby int
  testDouble(Math::nearbyint,nearbyint,"nearbyint");
  testFloat(Math::nearbyint,nearbyintf,"nearbyintf");
  testDouble(Math::rint,rint,"rint");
  testFloat(Math::rint,rintf,"rintf");

  // Log
  testDouble(Math::log1p,log1p,"log1p");
  testFloat(Math::log1p,log1pf,"log1pf");
  testDouble(Math::log2,log2,"log2");
  testFloat(Math::log2,log2f,"log2f");
  testDouble(Math::log10,log10,"log10");
  testFloat(Math::log10,log10f,"log10f");
/*
*/
  return 0;
  }

