/********************************************************************************
*                                                                               *
*                             Half Float Support Test                           *
*                                                                               *
*********************************************************************************
* Copyright (C) 2008,2026 by Jeroen van der Zijp.   All Rights Reserved.        *
********************************************************************************/
#include "fx.h"
#include "testargs.h"

/*
  Notes:
  - Tests for FXhalf.
*/

#define HALFMINDEN 0x0001       // Smallest denormalized
#define HALFMAXDEN 0x03FF       // Largest denormalized
#define HALFMIN    0x0400       // Minimum normalized
#define HALFMAX    0x7BFF       // Maximum normalized
#define HALFEXP    0x7C00       // Mask exponent
#define HALFMNT    0x03FF       // Mask mantissa
#define HALFSGN    0x8000       // Mask sign
#define HALFABS    0x7FFF       // Mask exponent and mantissa
#define HALFINF    0x7C00       // Infinite

/*******************************************************************************/

union CNV {
  FXuint  u;
  FXfloat f;
  };

// Initial half data
FXhalf  initial[65536];

// Converted from half
FXfloat converted[65536];

// Converted back from float
FXhalf  convertedback[65536];


// Failures found
static FXint failures=0;

// Raw bits of half
static FXushort bits(FXhalf h){ FXushort v; memcpy(&v,&h,sizeof(v)); return v; }

// Half from raw bits
static FXhalf frombits(FXushort v){ FXhalf h; memcpy(&h,&v,sizeof(h)); return h; }

// Raw bits of float
static FXuint fbits(FXfloat f){ CNV c; c.f=f; return c.u; }

// Float from raw bits
static FXfloat ffrombits(FXuint u){ CNV c; c.u=u; return c.f; }

// Report failure
static void bad(const char* what,FXuint a,FXuint b,FXuint c){
  failures++;
  if(failures<=25) printf("BAD: %s [0x%08x] expected 0x%08x got 0x%08x\n",what,a,b,c);
  }

// Exact float value of half bits, computed with ldexp
static FXdouble reference(FXushort v){
  FXint e=(v>>10)&31;
  FXint m=v&0x3ff;
  FXdouble r;
  if(e==0) r=ldexp((FXdouble)m,-24);
  else r=ldexp((FXdouble)(m+1024),e-25);
  return (v&HALFSGN)?-r:r;
  }

// Check conversion half to float for every half, and back again
static void testexhaustive(){
  FXint i;
  for(i=0; i<65536; ++i){
    FXhalf h=frombits((FXushort)i);
    FXfloat f=h;
    FXushort back=bits(FXhalf(f));
    FXint e=(i>>10)&31;
    FXint m=i&HALFMNT;
    if(e==31){
      if(m==0){                                         // Infinity
        if(fbits(f)!=(((FXuint)(i&HALFSGN)<<16)|0x7f800000u)) bad("infinity to float",i,0,fbits(f));
        if(back!=i) bad("infinity round trip",i,i,back);
        }
      else{                                             // NaN
        if(!(f!=f)) bad("NaN to float",i,0,fbits(f));
        if(((back&HALFEXP)!=HALFEXP) || (back&HALFMNT)==0) bad("NaN round trip",i,i,back);
        if((back&HALFSGN)!=(i&HALFSGN)) bad("NaN sign round trip",i,i,back);
        }
      }
    else{
      if((FXdouble)f!=reference((FXushort)i)) bad("half to float",i,fbits((FXfloat)reference((FXushort)i)),fbits(f));
      if(back!=i) bad("round trip",i,i,back);
      }
    }
  }

// Values with known conversions
static void testknown(){
  struct { FXfloat f; FXushort h; } cases[]={
    {0.0f,0x0000},
    {-0.0f,0x8000},
    {1.0f,0x3c00},
    {-1.0f,0xbc00},
    {2.0f,0x4000},
    {0.5f,0x3800},
    {-2.0f,0xc000},
    {1.5f,0x3e00},
    {65504.0f,0x7bff},
    {-65504.0f,0xfbff},
    {6.103515625e-05f,0x0400},          // Smallest normal 2^-14
    {5.9604644775390625e-08f,0x0001},   // Smallest denormal 2^-24
    {3.0f,0x4200},
    {0.099975586f,0x2e66},
    {HUGE_VALF,0x7c00},
    {-HUGE_VALF,0xfc00},
    {1.0e10f,0x7c00},
    {-1.0e10f,0xfc00},
    {3.0e38f,0x7c00},
    {1.0e-10f,0x0000},
    {-1.0e-10f,0x8000},
    {1.0e-45f,0x0000}
    };
  FXuint i;
  for(i=0; i<sizeof(cases)/sizeof(cases[0]); ++i){
    FXushort r=bits(FXhalf(cases[i].f));
    if(r!=cases[i].h) bad("known float to half",fbits(cases[i].f),cases[i].h,r);
    }
  // Constants in header
  if(frombits(0x7bff)!=FXhalf((FXfloat)HALF_MAX)) bad("HALF_MAX",0,0x7bff,bits(FXhalf((FXfloat)HALF_MAX)));
  if(bits(FXhalf((FXfloat)HALF_MIN))!=0x0001) bad("HALF_MIN",0,1,bits(FXhalf((FXfloat)HALF_MIN)));
  // Not-a-number stays so, for all payloads and signs
  for(i=0; i<=0x7fffffu; i+=0x1357){
    FXfloat f=ffrombits(0x7f800000u|i);
    FXushort r;
    if(!(f!=f)) continue;
    r=bits(FXhalf(f));
    if((r&HALFEXP)!=HALFEXP || (r&HALFMNT)==0) bad("NaN float to half",fbits(f),0x7e00,r);
    r=bits(FXhalf(-f));
    if((r&HALFEXP)!=HALFEXP || (r&HALFMNT)==0 || !(r&HALFSGN)) bad("negative NaN float to half",fbits(-f),0xfe00,r);
    }
  }

// Floats in between halves; conversion must be monotonic and land on a neighboring half
static void testrange(){
  FXint i;
  for(i=0; i<HALFEXP; ++i){
    FXfloat lo=frombits((FXushort)i);
    FXfloat hi=frombits((FXushort)(i+1));
    FXfloat mid=(FXfloat)(0.5*((FXdouble)lo+(FXdouble)hi));
    FXuint  mb=fbits(mid);
    FXuint  probe[3]={mb-1,mb,mb+1};
    FXint   k;
    if(i==HALFMAX) continue;                            // Beyond max is overflow
    for(k=0; k<3; ++k){
      FXfloat f=ffrombits(probe[k]);
      FXushort r=bits(FXhalf(f));
      FXushort n=bits(FXhalf(-f));
      if(r!=i && r!=i+1) bad("float between halves",probe[k],i,r);
      if(n!=(r|HALFSGN)) bad("sign of float to half",probe[k],r|HALFSGN,n);
      }
    }
  // Monotonic over a spread of positive floats
  FXushort last=0;
  FXuint   u;
  for(u=0; u<0x7f800000u; u+=0x0001a5b3u){
    FXushort r=bits(FXhalf(ffrombits(u)));
    if(r<last) bad("monotonic",u,last,r);
    last=r;
    }
  }

// Arithmetic operators
static void testoperators(){
  FXhalf a(1.5f);
  FXhalf b(0.25f);
  FXhalf c;
  c=a; c+=b;
  if(c!=FXhalf(1.75f)) bad("+=",0,0x3f00,bits(c));
  c=a; c-=b;
  if(c!=FXhalf(1.25f)) bad("-=",0,0x3d00,bits(c));
  c=a; c*=b;
  if(c!=FXhalf(0.375f)) bad("*=",0,0x3600,bits(c));
  c=a; c/=b;
  if(c!=FXhalf(6.0f)) bad("/=",0,0x4600,bits(c));
  c=a; c+=0.5f;
  if(c!=FXhalf(2.0f)) bad("+= float",0,0x4000,bits(c));
  c=-a;
  if(bits(c)!=0xbe00) bad("negate",0,0xbe00,bits(c));
  if(bits(+c)!=0xbe00) bad("plus",0,0xbe00,bits(+c));
  if(!FXhalf(0.0f)!=true) bad("not zero",0,1,0);
  if(!FXhalf(1.0f)) bad("not one",0,0,1);
  }

// Self-check
static int selftest(){
  testexhaustive();
  testknown();
  testrange();
  testoperators();
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
  FXint i;
  CNV c;

  if(2<=argc && strcmp(argv[1],"--test")==0) return selftest();

  FXUNUSED(argc);
  FXUNUSED(argv);
  // Fill initial half data
  for(i=0; i<65536; ++i){ *((FXushort*)&initial[i])=i; }

  // Convert to float
  for(i=0; i<65536; ++i){ converted[i]=initial[i]; }

  // Convert back to half
  for(i=0; i<65536; ++i){ convertedback[i]=converted[i]; }

  // Now check it
  for(i=0; i<65536; ++i){
    c.f=converted[i];
    printf("i=%-5d [0x%04x] FLOAT=%+22.15lf [0x%08x] %s EXP=%-+4d MAN=0x%08x",i,i,(FXdouble)c.f,c.u,((c.u>>31)&1)?"NEG":"POS",((c.u>>23)&255)-127,c.u&0x007fffff);
    if((i&HALFABS)<HALFMIN){
      printf(" DEN");
      }
    else if((i&HALFABS)>HALFMAX){
      if((i&HALFABS)>HALFINF) printf(" NaN"); else printf(" INF");
      }
    else{
      printf(" NOR");
      }
    if(initial[i]==convertedback[i]){
      printf(" OK");
      }
    printf("\n");
    }
  return 0;
  }

