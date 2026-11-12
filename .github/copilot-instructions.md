* do not modify code in in origin fox-1.7.95 directory,
  except for debugging or testing purposes.

* the target library shall be in fox2 directory, copy everything required from 
  fox-1.7.95 to fox2, and modify the code in fox2 directory.

* don't write fish complex command instead write bash script into tmp dir in
  this workspace.

* follow the FOX formatting rules below in all C++ code (fox2, tests, tools);
  never reformat an existing file to a different style:

  - indent 2 spaces, spaces only, never tabs
  - opening brace on the statement line; closing brace indented to the
    statement, not the block:
        void f(FXuint s){
          if(state!=s){
            state=s;
            }
          }
  - class/enum/message-map closing brace is `  };`, namespace closes with `}`
    in column 0
  - no space between control keyword and parenthesis:
    `if(`, `for(`, `while(`, `switch(`, `else{`, `else if(`, `catch(`
  - no spaces around operators: `state=STATE_UP;`, `if(state!=s)`,
    `if((n-m)>0)`, `if(options&BUTTON_INITIAL)`, `!x`
  - no space after comma in parameter/argument lists; default arguments with
    tight `=`: `FXButton(FXComposite* p,const FXString& text,FXIcon* ic=nullptr)`
  - `for` headers keep a space after each semicolon: `for(i=0; i<=max; i++)`
  - `*` and `&` bind to the type: `FXComposite* p`, `const FXString& text`,
    `void* data`
  - no space just inside parentheses: `f(a,b)`, not `f( a , b )`
  - single-statement bodies may omit braces:
    `if(options&BUTTON_DEFAULT) setDefault(true);`
  - keep long parameter lists on one line; do not re-wrap them for width

# porting guideline

since we may wan't to support more platforms than X11 and WIN32
don't use macros
#ifdef WIN32
..
#else
..
#endif

instead use
#if defined(WIN32)
#endif
#if defined(X11)
#endif

if too much code is in one file more than 100 lines platform specific code, plit it into multiple files,
eg: FXButton_win32.cpp, FXButton_x11.cpp, FXButton.cpp

# screenshots

if you need screenshots for testing store it in an screenshot folder for later reuse, eg screenshots/tests/fox1/x11/button.png, screenshots/tests/fox1/win32/button.png

