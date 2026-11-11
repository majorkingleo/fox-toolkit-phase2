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
