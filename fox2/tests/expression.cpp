/********************************************************************************
*                                                                               *
*                           Expression Evaluator Test                           *
*                                                                               *
*********************************************************************************
* Copyright (C) 2006,2026 by Jeroen van der Zijp.   All Rights Reserved.        *
********************************************************************************/
#include"fx.h"
#include "testargs.h"

/*
  Notes:

  - Call as follows:

      expression <expression> [ <variables> [ <values> ]]

    where:

       <expression>     math expression to be evaluated
       <variables>      comma-separated list of variable names
       <values>         numbers, one for each variable

  - Example:

      expression a+b   a,b   5 6

    Will print out:

      parse("a+b","a,b") : 11

  - With --test, a built-in table of expressions and their expected values is
    evaluated, as well as a table of expressions with errors; the exit code is 0
    only if all of them hold.  Without it, a syntax error gives exit code 1.

*/


/*******************************************************************************/

// Expression, variable names, values of the variables, expected result
struct ExprCase {
  const FXchar* expr;
  const FXchar* vars;
  FXdouble      v[3];
  FXdouble      expected;
  };


// Expression with error
struct ExprError {
  const FXchar*        expr;
  FXExpression::Error  error;
  };


static const ExprCase cases[]={
  {"1+2*3","",{0,0,0},7.0},
  {"(1+2)*3","",{0,0,0},9.0},
  {"2*3+4*5","",{0,0,0},26.0},
  {"10/4","",{0,0,0},2.5},
  {"10%3","",{0,0,0},1.0},
  {"7-10","",{0,0,0},-3.0},
  {"0.5+0.25","",{0,0,0},0.75},
  {"1e3","",{0,0,0},1000.0},
  {"2**10","",{0,0,0},1024.0},
  {"2**3**2","",{0,0,0},512.0},
  {"-2**2","",{0,0,0},-4.0},
  {"-(3)","",{0,0,0},-3.0},
  {"1<2","",{0,0,0},1.0},
  {"2<=1","",{0,0,0},0.0},
  {"2>1","",{0,0,0},1.0},
  {"2>=3","",{0,0,0},0.0},
  {"3==3","",{0,0,0},1.0},
  {"3!=3","",{0,0,0},0.0},
  {"1+2<4","",{0,0,0},1.0},
  {"1&&0","",{0,0,0},0.0},
  {"1||0","",{0,0,0},1.0},
  {"1<<4","",{0,0,0},16.0},
  {"256>>4","",{0,0,0},16.0},
  {"6&3","",{0,0,0},2.0},
  {"6|3","",{0,0,0},7.0},
  {"6^3","",{0,0,0},5.0},
  {"~0","",{0,0,0},-1.0},
  {"!0","",{0,0,0},1.0},
  {"!5","",{0,0,0},0.0},
  {"x?10:20","x",{1,0,0},10.0},
  {"x?10:20","x",{0,0,0},20.0},
  {"a+b","a,b",{5,6,0},11.0},
  {"b**2-4*a*c","a,b,c",{1,4,2},8.0},
  {"abs(-3)","",{0,0,0},3.0},
  {"sqrt(16)","",{0,0,0},4.0},
  {"sqr(5)","",{0,0,0},25.0},
  {"cub(3)","",{0,0,0},27.0},
  {"cbrt(27)","",{0,0,0},3.0},
  {"max(3,7)","",{0,0,0},7.0},
  {"min(3,7)","",{0,0,0},3.0},
  {"pow(2,8)","",{0,0,0},256.0},
  {"floor(2.7)","",{0,0,0},2.0},
  {"ceil(2.2)","",{0,0,0},3.0},
  {"trunc(-2.7)","",{0,0,0},-2.0},
  {"round(2.4)","",{0,0,0},2.0},
  {"exp(0)","",{0,0,0},1.0},
  {"log(E)","",{0,0,0},1.0},
  {"log2(8)","",{0,0,0},3.0},
  {"log10(1000)","",{0,0,0},3.0},
  {"sin(0)","",{0,0,0},0.0},
  {"cos(0)","",{0,0,0},1.0},
  {"atan2(1,1)*4","",{0,0,0},3.14159265358979323846},
  {"PI","",{0,0,0},3.14159265358979323846},
  {"E","",{0,0,0},2.71828182845904523536},
  {"DTOR*180","",{0,0,0},3.14159265358979323846},
  {"RTOD*PI","",{0,0,0},180.0},
  {"isnan(0)","",{0,0,0},0.0},
  {"isfin(1)","",{0,0,0},1.0},
  {nullptr,nullptr,{0,0,0},0.0}
  };


static const ExprError errcases[]={
  {"",FXExpression::ErrEmpty},
  {"(1+2",FXExpression::ErrParent},
  {"foo+1",FXExpression::ErrIdent},
  {"1 2",FXExpression::ErrMore},
  {"max(1 2)",FXExpression::ErrComma},
  {"1?2",FXExpression::ErrColon},
  {"1+2",FXExpression::ErrOK},
  {nullptr,FXExpression::ErrOK}
  };


// Run the tables, return the number of failed checks
static FXint testexpression(){
  FXint failures=0;
  FXExpression::Error err;
  FXExpression expression;
  FXdouble result;
  FXint i;

  // Values
  for(i=0; cases[i].expr; i++){
    const ExprCase& c=cases[i];
    err=expression.parse(c.expr,c.vars);
    if(err!=FXExpression::ErrOK){
      printf("BAD: \"%s\" does not parse: %s\n",c.expr,FXExpression::getError(err));
      failures++;
      continue;
      }
    result=expression.evaluate(c.v);
    if(!(Math::fabs(result-c.expected)<=1E-12*(1.0+Math::fabs(c.expected)))){
      printf("BAD: \"%s\" gives %.17g, expected %.17g\n",c.expr,result,c.expected);
      failures++;
      }
    }

  // Errors
  for(i=0; errcases[i].expr; i++){
    err=expression.parse(errcases[i].expr,"");
    if(err!=errcases[i].error){
      printf("BAD: \"%s\" gives error \"%s\", expected \"%s\"\n",errcases[i].expr,FXExpression::getError(err),FXExpression::getError(errcases[i].error));
      failures++;
      }
    }

  // Compiled once, evaluated for several values
  err=expression.parse("a*a+b","a,b");
  for(i=0; i<5; i++){
    FXdouble v[2]={(FXdouble)i,1.0};
    result=expression.evaluate(v);
    if(result!=(FXdouble)(i*i+1)){
      printf("BAD: a*a+b for a=%d gives %.17g\n",i,result);
      failures++;
      }
    }
  return failures;
  }

// Print command line help
static void printusage(){
  fxmessage("Usage: rex [options] pat [string]\n");
  fxmessage("  options:\n");
  fxmessage("  -?, -h, --help                      Print help.\n");
  fxmessage("  -tracetopics <topics>               Enable trace topics.\n");
  }



/* Evaluate expression with arguments */
int main(int argc,char **argv){
  fxtestargs(argc,argv);
  const FXchar *expr="";
  const FXchar *vars="";
  FXdouble parameter[256];
  FXExpression::Error err;
  FXExpression expression;
  FXdouble result;
  FXint arg=1;
  FXint i=0;

  // Zero parameters
  clearElms(parameter,256);

  // Self-check with the built-in tables
  if(1<argc && FXString::compare(argv[1],"--test")==0){
    FXint failures=testexpression();
    if(failures){
      printf("FAILED: %d check(s)\n",failures);
      return 1;
      }
    printf("all checks passed\n");
    return 0;
    }

  // Parse options
  while(arg<argc){
    if(FXString::compare(argv[arg],"-?")==0 || FXString::compare(argv[arg],"-h")==0 || FXString::compare(argv[arg],"--help")==0){
      printusage();
      return 0;
      }
    if(FXString::compare(argv[arg],"-tracetopics")==0){
      if(++arg>=argc){ fxwarning("%s: missing argument for -tracetopics.\n",argv[0]); return 0; }
      setTraceTopics(argv[arg++]);
      continue;
      }
    break;
    }

  // Expression
  if(arg<argc){
    expr=argv[arg++];
    if(arg<argc){
      vars=argv[arg++];
      }
    fxmessage("parse(\"%s\",\"%s\") ",expr,vars);
    err=expression.parse(expr,vars);
    if(err!=FXExpression::ErrOK){
      fxwarning("Error: %s\n",FXExpression::getError(err));
      return 1;
      }
    while(arg<argc){
      parameter[i++]=strtod(argv[arg++],nullptr);
      }
    result=expression.evaluate(parameter);
    fxmessage(" : %.17lg\n",result);
    }
  return 0;
  }
