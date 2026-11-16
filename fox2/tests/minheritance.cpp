/********************************************************************************
*                                                                               *
*                           Multiple Inheritance Test                           *
*                                                                               *
*********************************************************************************
* Copyright (C) 1998,2026 by Jeroen van der Zijp.   All Rights Reserved.        *
********************************************************************************/
#include "fx.h"
#include "testargs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*

  Test for proper code generation for the case of multiple inheritance.

  We have tabulated the following cases:

    | Message map where handler is found | Class which message handler is member of
  --+------------------------------------+------------------------------------------
  1 |     derived class                  |        derived class
  2 |     one of the base classes        |        one of the base classes
  3 |     derived class                  |        one of the base classes
  4 |     one of the base classes        |        derived class


  Ad 1. The most common scenario, and typically presents no problem, as the
        "this" pointer and pointer-to-member match up.

  Ad 2. The "this" pointer needs to be adjusted properly so as to resolve to
        the correct struct offset in the compound object where one of the bases
        is located.

  Ad 3. A handler which is defined in the base class is entered into the
        message map of the derived class.  This is sometimes done to give an
        existing member function an additional binding to a different message-id.

        According to the C++ rules (contravariance rule, pp. 420 3rd ed. C++ book),
        one can assign a pointer to member of base class to a pointer to
        member of derived class, and thats what we do.

  Ad 4. This case is, of course, impossible!! I list it here for completeness
        sake, and to see if you're paying attention...


  To test properly, we try several orderings of the two base classes.
  We print out the "this" address in the ctor and the handler so as to see
  if the correct offset is being added when the handler is actually being
  called.

  Note we're currently assuming only one of the base classes is a FOX class;
  this limitation could be removed by making more macros:

    FXIMPLEMENT2
    FXIMPLEMENT3
        ...
    ad nauseam

  Nausea starts at N=2 for me...

*/
/*******************************************************************************/

// Non-FOX object
class Base1 {
protected:
  int a;
public:
  Base1();
  virtual ~Base1();
  };


// FOX object
class Base2 : public FXObject {
  FXDECLARE(Base2)
protected:
  int b;
public:
  enum {
    ID_BASE2=1,
    ID_LAST
    };
public:
  long onCmdBase2(FXObject*,FXSelector,void*);
public:
  Base2();
  virtual ~Base2();
  };



// Non-FOX object
class Base3 {
protected:
  int c;
public:
public:
  Base3();
  virtual ~Base3();
  };


// FOX object with mix-in
class TwoBaseOne : public Base1, public Base2 {
  FXDECLARE(TwoBaseOne)
protected:
  int d;
public:
  enum {
    ID_TWOBASEONE=Base2::ID_LAST,
    ID_LAST
    };
public:
  long onCmdTwoBaseOne(FXObject*,FXSelector,void*);
public:
  TwoBaseOne();
  virtual ~TwoBaseOne();
  };



// FOX object with mix-in
class TwoBaseTwo : public Base2, public Base1 {
  FXDECLARE(TwoBaseTwo)
protected:
  int e;
public:
  enum {
    ID_TWOBASETWO=Base2::ID_LAST,
    ID_LAST
    };
public:
  long onCmdTwoBaseTwo(FXObject*,FXSelector,void*);
public:
  TwoBaseTwo();
  virtual ~TwoBaseTwo();
  };


// FOX object with mix-in
class ThreeBase : public Base3, public TwoBaseOne {
  FXDECLARE(ThreeBase)
protected:
  int f;
public:
  enum {
    ID_THREEBASE=TwoBaseOne::ID_LAST,
    ID_TWOBASEONE,
    ID_BASE2,
    ID_LAST
    };
public:
  long onCmdThreeBase(FXObject*,FXSelector,void*);
public:
  ThreeBase();
  virtual ~ThreeBase();
  };


/*******************************************************************************/

// Which object a handler ran on, and the number of failed checks
static const void* seen=nullptr;
static FXint failures=0;


Base1::Base1(){
  FXTRACE(100,"Base1::Base1 at %p\n",this);
  a=1;
  }


Base1::~Base1(){
  FXTRACE(100,"Base1::~Base1\n");
  }

/*******************************************************************************/


FXDEFMAP(Base2) Base2Map[]={
  FXMAPFUNC(SEL_COMMAND,Base2::ID_BASE2,Base2::onCmdBase2),
  };

FXIMPLEMENT(Base2,FXObject,Base2Map,ARRAYNUMBER(Base2Map))

Base2::Base2(){
  FXTRACE(100,"Base2::Base2 at %p\n",this);
  b=2;
  }

long Base2::onCmdBase2(FXObject*,FXSelector,void*){
  seen=this;
  FXTRACE(100,"Base2::onCmdBase2 at %p b=%d\n",this,b);
  return 1;
  }

Base2::~Base2(){
  FXTRACE(100,"Base2::~Base2\n");
  }


/*******************************************************************************/


Base3::Base3(){
  FXTRACE(100,"Base3::Base3 at %p\n",this);
  c=3;
  }


Base3::~Base3(){
  FXTRACE(100,"Base3::~Base3\n");
  }



/*******************************************************************************/


FXDEFMAP(TwoBaseOne) TwoBaseOneMap[]={
  FXMAPFUNC(SEL_COMMAND,TwoBaseOne::ID_TWOBASEONE,TwoBaseOne::onCmdTwoBaseOne),
  };

FXIMPLEMENT(TwoBaseOne,Base2,TwoBaseOneMap,ARRAYNUMBER(TwoBaseOneMap))

TwoBaseOne::TwoBaseOne(){
  FXTRACE(100,"TwoBaseOne::TwoBaseOne at %p\n",this);
  d=4;
  }

long TwoBaseOne::onCmdTwoBaseOne(FXObject*,FXSelector,void*){
  seen=this;
  FXTRACE(100,"TwoBaseOne::onCmdTwoBaseOne at %p d=%d\n",this,d);
  return 1;
  }

TwoBaseOne::~TwoBaseOne(){
  FXTRACE(100,"TwoBaseOne::~TwoBaseOne\n");
  }

/*******************************************************************************/


FXDEFMAP(TwoBaseTwo) TwoBaseTwoMap[]={
  FXMAPFUNC(SEL_COMMAND,TwoBaseTwo::ID_TWOBASETWO,TwoBaseTwo::onCmdTwoBaseTwo),
  };

FXIMPLEMENT(TwoBaseTwo,Base2,TwoBaseTwoMap,ARRAYNUMBER(TwoBaseTwoMap))

TwoBaseTwo::TwoBaseTwo(){
  FXTRACE(100,"TwoBaseTwo::TwoBaseTwo at %p\n",this);
  e=4;
  }

long TwoBaseTwo::onCmdTwoBaseTwo(FXObject*,FXSelector,void*){
  seen=this;
  FXTRACE(100,"TwoBaseTwo::onCmdTwoBaseTwo at %p e=%d\n",this,e);
  return 1;
  }

TwoBaseTwo::~TwoBaseTwo(){
  FXTRACE(100,"TwoBaseTwo::~TwoBaseTwo\n");
  }


/*******************************************************************************/


FXDEFMAP(ThreeBase) ThreeBaseMap[]={
  FXMAPFUNC(SEL_COMMAND,ThreeBase::ID_THREEBASE,ThreeBase::onCmdThreeBase),
  FXMAPFUNC(SEL_COMMAND,ThreeBase::ID_TWOBASEONE,ThreeBase::onCmdTwoBaseOne),
  FXMAPFUNC(SEL_COMMAND,ThreeBase::ID_BASE2,ThreeBase::onCmdBase2),
  };

FXIMPLEMENT(ThreeBase,TwoBaseOne,ThreeBaseMap,ARRAYNUMBER(ThreeBaseMap))

ThreeBase::ThreeBase(){
  FXTRACE(100,"ThreeBase::ThreeBase at %p\n",this);
  f=5;
  }

long ThreeBase::onCmdThreeBase(FXObject*,FXSelector,void*){
  seen=this;
  FXTRACE(100,"ThreeBase::onCmdThreeBase at %p f=%d\n",this,f);
  return 1;
  }

ThreeBase::~ThreeBase(){
  FXTRACE(100,"ThreeBase::~ThreeBase\n");
  }


/*******************************************************************************/


// Send message to target, handler must run once and see the expected object
static void call(const char* what,FXObject* target,FXSelector sel,const void* self){
  seen=nullptr;
  FXbool ok=(target->handle(nullptr,sel,nullptr)==1) && (seen==self);
  if(!ok) failures++;
  printf("%s %s\n",ok?"ok   ":"BAD  ",what);
  }


// Start the whole thing
int main(int argc,char* argv[]){
  fxtestargs(argc,argv);
  setTraceLevel(100);

  {
  TwoBaseOne twobase1;

  // Found in TwoBaseOne
  FXTRACE(100,"calling TwoBaseOne\n");
  call("TwoBaseOne handler",&twobase1,FXSEL(SEL_COMMAND,TwoBaseOne::ID_TWOBASEONE),static_cast<TwoBaseOne*>(&twobase1));

  // Found in Base2
  FXTRACE(100,"calling Base2\n");
  call("Base2 handler of TwoBaseOne",&twobase1,FXSEL(SEL_COMMAND,Base2::ID_BASE2),static_cast<Base2*>(&twobase1));
  }

  FXTRACE(100,"=============\n");

  {
  TwoBaseTwo twobase2;

  // Found in TwoBaseTwo
  FXTRACE(100,"calling TwoBaseTwo\n");
  call("TwoBaseTwo handler",&twobase2,FXSEL(SEL_COMMAND,TwoBaseTwo::ID_TWOBASETWO),static_cast<TwoBaseTwo*>(&twobase2));

  // Found in Base2
  FXTRACE(100,"calling Base2\n");
  call("Base2 handler of TwoBaseTwo",&twobase2,FXSEL(SEL_COMMAND,Base2::ID_BASE2),static_cast<Base2*>(&twobase2));
  }

  FXTRACE(100,"=============\n");

  {
  ThreeBase threebase;

  // Found in ThreeBase
  FXTRACE(100,"calling ThreeBase\n");
  call("ThreeBase handler",&threebase,FXSEL(SEL_COMMAND,ThreeBase::ID_THREEBASE),static_cast<ThreeBase*>(&threebase));

  // Found in TwoBaseOne
  FXTRACE(100,"calling TwoBaseOne\n");
  call("TwoBaseOne handler of ThreeBase",&threebase,FXSEL(SEL_COMMAND,TwoBaseOne::ID_TWOBASEONE),static_cast<TwoBaseOne*>(&threebase));

  // Found in Base2
  FXTRACE(100,"calling Base2\n");
  call("Base2 handler of ThreeBase",&threebase,FXSEL(SEL_COMMAND,Base2::ID_BASE2),static_cast<Base2*>(&threebase));

  // Found in TwoBaseOne
  FXTRACE(100,"calling TwoBaseOne via ThreeBase\n");
  call("TwoBaseOne handler bound in ThreeBase",&threebase,FXSEL(SEL_COMMAND,ThreeBase::ID_TWOBASEONE),static_cast<TwoBaseOne*>(&threebase));

  // Found in Base2
  FXTRACE(100,"calling Base2 via ThreeBase\n");
  call("Base2 handler bound in ThreeBase",&threebase,FXSEL(SEL_COMMAND,ThreeBase::ID_BASE2),static_cast<Base2*>(&threebase));
  }

  if(failures){
    printf("FAILED: %d call(s)\n",failures);
    return 1;
    }
  printf("all calls passed\n");
  return 0;
  }
