/********************************************************************************
*                                                                               *
*               P a t h   N a m e   F u n c t i o n s   ( X 1 1 )               *
*                                                                               *
*********************************************************************************
* Copyright (C) 2000,2026 by Jeroen van der Zijp.   All Rights Reserved.        *
*********************************************************************************
* This library is free software; you can redistribute it and/or modify          *
* it under the terms of the GNU Lesser General Public License as published by   *
* the Free Software Foundation; either version 3 of the License, or             *
* (at your option) any later version.                                           *
*                                                                               *
* This library is distributed in the hope that it will be useful,               *
* but WITHOUT ANY WARRANTY; without even the implied warranty of                *
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the                 *
* GNU Lesser General Public License for more details.                           *
*                                                                               *
* You should have received a copy of the GNU Lesser General Public License      *
* along with this program.  If not, see <http://www.gnu.org/licenses/>          *
********************************************************************************/
#include "xincs.h"
#include "fxver.h"
#include "fxdefs.h"
#include "fxchar.h"
#include "fxascii.h"
#include "fxunicode.h"
#include "FXElement.h"
#include "FXArray.h"
#include "FXMetaClass.h"
#include "FXHash.h"
#include "FXStream.h"
#include "FXString.h"
#include "FXPath.h"
#include "FXSystem.h"
#include "FXIO.h"
#include "FXStat.h"
#include "FXFile.h"
#include "FXDir.h"
#include "fxpathpriv.h"

#if !defined(WIN32)

/*
  Notes:

  - Platform implementation of the FXPath members which parse, convert, quote and test path names according to the conventions of the platform; the portable path functions stay in FXPath.cpp.
*/

using namespace FX;

/*******************************************************************************/

namespace FX {

// Return root of absolute path; on Unix, this is just "/".
// The empty string is returned if the path is not absolute.
FXString FXPath::root(const FXString& file){
  if(file[0]==PATHSEP){
    return PATHSEPSTRING;
    }
  return FXString::null;
  }

// Return share name part of filename.
// Always empty string on Linux.
FXString FXPath::share(const FXString&){
  return FXString::null;
  }

// Return server name part filename.
// Always empty string on Linux.
FXString FXPath::server(const FXString&){
  return FXString::null;
  }

// Return drive letter prefix "C:".
// Always empty string on Linux.
FXString FXPath::drive(const FXString&){
  return FXString::null;
  }

// Check if file represents a file share
// Always false on Linux.
FXbool FXPath::isShare(const FXString&){
  return false;
  }

// Return true if input path is a file server
// Always false on Linux.
FXbool FXPath::isServer(const FXString&){
  return false;
  }


// Return directory part of pathname, assuming full pathname.
// Note that directory("/bla/bla/") is "/bla/bla" and NOT "/bla".
// However, directory("/bla/bla") is "/bla" as we expect!
// The "\\Server\Share\", "C:\" or "\" ("/" for Linux) are the
// top of that path's file system, and will not be dropped.
FXString FXPath::directory(const FXString& file){
  if(!file.empty()){
    FXint p=0,s;
    if(file[p]==PATHSEP) p++;
    s=p;
    while(file[p]){
      if(file[p]==PATHSEP){ s=p++; continue; }
      p++;
      }
    return FXString(&file[0],s);
    }
  return FXString::null;
  }


// Return name and extension part of pathname.
// Note that name("/bla/bla/") is "" and NOT "bla".
// However, name("/bla/bla") is "bla" as we expect!
FXString FXPath::name(const FXString& file){
  if(!file.empty()){
    FXint p=0,s;
    s=p;
    while(file[s]){
      if(file[s]==PATHSEP){ p=++s; continue; }
      ++s;
      }
    return FXString(&file[p],s-p);
    }
  return FXString::null;
  }


// Return file stem, i.e. document name only:
//
//  /path/aa        -> aa
//  /path/aa.bb     -> aa
//  /path/aa.bb.cc  -> aa.bb
//  /path/.aa       -> .aa
FXString FXPath::stem(const FXString& file){
  if(!file.empty()){
    FXint p=0,s,b,e;
    s=p;
    while(file[s]){
      if(file[s]==PATHSEP){ p=++s; continue; }
      ++s;
      }
    b=p;
    e=s;
    if(file[p]=='.') p++;     // Stuff following leading '.' is NOT an extension
    while(p<s){
      if(file[--s]=='.'){ e=s; break; }
      }
    return FXString(&file[b],e-b);
    }
  return FXString::null;
  }


// Return extension, if there is one:
//
//  /path/aa        -> ""
//  /path/aa.bb     -> bb
//  /path/aa.bb.cc  -> cc
//  /path/.aa       -> ""
FXString FXPath::extension(const FXString& file){
  if(!file.empty()){
    FXint p=0,s,b,e;
    s=p;
    while(file[s]){
      if(file[s]==PATHSEP){ p=++s; continue; }
      ++s;
      }
    b=e=s;
    if(file[p]=='.') p++;     // Stuff following leading '.' is NOT an extension
    while(p<s){
      if(file[--s]=='.'){ b=s+1; break; }
      }
    return FXString(&file[b],e-b);
    }
  return FXString::null;
  }


// Return file name less the extension
//
//  /path/aa        -> /path/aa
//  /path/aa.bb     -> /path/aa
//  /path/aa.bb.cc  -> /path/aa.bb
//  /path/.aa       -> /path/.aa
FXString FXPath::stripExtension(const FXString& file){
  if(!file.empty()){
    FXint p=0,s,e;
    s=p;
    while(file[s]){
      if(file[s]==PATHSEP){ p=++s; continue; }
      ++s;
      }
    e=s;
    if(file[p]=='.') p++;     // Stuff following leading '.' is NOT an extension
    while(p<s){
      if(file[--s]=='.'){ e=s; break; }
      }
    return FXString(&file[0],e);
    }
  return FXString::null;
  }


// Expand environment variables recursively, stopping when recursion goes too deep
FXString expandEnvironmentVariables(const FXString& file,FXint level){
  FXint s=0,r=0,b=0,e=0;
  FXString result;
  while(e<file.length()){
    r=e;
    if(file[e++]=='$'){
      b=e;
      if(file[e]=='{'){       // ${VAR}...
        b=++e;
        while(Ascii::isWord(file[e])) e++;
        if(file[e]=='}' && b<e && 0<level){
          result.append(&file[s],r-s);
          result.append(expandEnvironmentVariables(FXSystem::getEnvironment(file.mid(b,e-b)),level-1));
          s=++e;
          }
        }
      else{                   // $VAR...
        while(Ascii::isWord(file[e])) e++;
        if(b<e && 0<level){
          result.append(&file[s],r-s);
          result.append(expandEnvironmentVariables(FXSystem::getEnvironment(file.mid(b,e-b)),level-1));
          s=e;
          }
        }
      }
    }
  result.append(&file[s],e-s);
  return result;
  }


// Convert a path to local conventions
FXString FXPath::convert(const FXString& path){
  return FXPath::convertFromWindows(path);
  }


// Convert a path-list to local conventions
FXString FXPath::convertPathList(const FXString& path){
  FXString result;
  if(!path.empty()){
    FXint b=0,e=0;
    while(e<path.length()){
      while(e<path.length() && path[e]!=';') e++;
      result+=FXPath::convertFromWindows(path.mid(b,e-b));
      if(path[e]==';'){ result+=':'; e++; }
      b=e;
      }
    }
  return result;
  }


// Contract path based on an environment variable.
//
// For example, on UNIX:
//
//   "/home/jeroen/junk"                -> "~/junk"
//   "/home/someoneelse/junk"           -> "~someoneelse/junk"
//   "/usr/local/ACE_wrappers/TAO"      -> "$ACE_ROOT/TAO"
//
// On Windows:
//
//   "C:\usr\local\ACE_wrappers\TAO"    -> "%ACE_ROOT%\TAO"
//
FXString FXPath::contract(const FXString& file,const FXString& user,const FXString& var){
  const FXchar legalcharacters[]="0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz_";
  if(!file.empty()){
    FXString result(file);
    if(FXPath::isAbsolute(result)){
      FXString dir=user.empty()?FXSystem::getHomeDirectory():FXSystem::getUserDirectory(user);
      if(!dir.empty()){
        if(FXString::compare(result,dir,dir.length())==0 && (dir.length()==result.length() || result[dir.length()]==PATHSEP)){
          result.replace(0,dir.length(),"~"+user);
          }
        }
      }
    if(!var.empty() && var.find_first_not_of(legalcharacters)<0){
      FXString val=FXSystem::getEnvironment(var);
      if(!val.empty()){
        FXint pos=result.find(val);
        if(0<=pos){
          if((pos==0 || result[pos-1]==PATHSEP) && (pos+val.length()==result.length() || result[pos+val.length()]==PATHSEP)){
            result.replace(pos,val.length(),"$"+var);
            }
          }
        }
      }
    return result;
    }
  return FXString::null;
  }


// Simplify a file path; the path will remain relative if it was relative,
// or absolute if it was absolute.  Also, a trailing "/" will be preserved
// as this is important in other functions.  Finally, returned path should
// be non-empty unless the input path was empty, and pathological paths
// will be fixed.
// The result is a shorter, canonical form of the original path.
//
// Examples:
//
//    /aa/bb/../cc      -> /aa/cc
//    /aa/bb/../cc/     -> /aa/cc/
//    /aa/bb/../..      -> /
//    /..               -> /
//    ./aa/bb/../../    -> ./
//    ./aa/bb/../..     -> .
//    ./aa/bb/../../../ -> ../
//    ./aa/bb/../../..  -> ..
//    /aa/bb/../../../  -> /
//    /aa/bb/../../..   -> /
//    a/..              -> .
//    a/../..           -> ..
//    a/../             -> ./
//    /aa/ccc/../../bb  -> /bb
//    ./a               -> a
//    /////./././       -> /
//    /.                -> /
//    ./                -> ./
//    /a/b/./           -> /a/b/
//    /a/b/.            -> /a/b
//    /a/./b/.          -> /a/b
//    /a/./b/./         -> /a/b/
//    ./..              -> ..
//    /aa/bb/..         -> /aa
//    ../..             -> ../..
//    ../a/../b         -> ../b
//    a/../b            -> b
//    a/b/../../../     -> ../
//    /a/b/../../../    -> /
//    ../a/./b          -> ../a/b
//    /a/.              -> /a
//    /a../c/.          -> /a../c
//    /..a/c/.          -> /..a/c
//    ../../c/.         -> ../../c
//
FXString FXPath::simplify(const FXString& file){
  if(!file.empty()){
    FXString result(file);
    FXint p=0,q=0,s=0,r=0;
    if(result[q]==PATHSEP){
      while(result[q]==PATHSEP) q++;
      result[p++]=PATHSEP;
      }
    s=r=p;
    while(result[q]){
      if(result[q]=='.'){
        q++;
        if(!result[q]){                 // '.'
          if(s<p && result[p-1]==PATHSEP) p--;
          if(!p){
            result[p++]='.';
            }
          continue;
          }
        if(result[q]==PATHSEP){        // './'
          while(result[q]==PATHSEP) q++;
          if(!result[q] && !p){
            result[p++]='.';
            result[p++]=PATHSEP;
            }
          continue;
          }
        if(result[q]=='.'){
          q++;
          if(!result[q]){               // '..'
            if(s<p && result[p-1]==PATHSEP){
              p--;
              while(s<p && result[p-1]!=PATHSEP) p--;
              if(s<p && result[p-1]==PATHSEP) p--;
              if(!p){
                result[p++]='.';
                }
              continue;
              }
            if(!r){                     // Can't go up from root
              result[p++]='.';
              result[p++]='.';
              s=p;
              }
            continue;
            }
          if(result[q]==PATHSEP){       // '../'
            while(result[q]==PATHSEP) q++;
            if(s<p && result[p-1]==PATHSEP){
              p--;
              while(s<p && result[p-1]!=PATHSEP) p--;
              if(!result[q] && !p){
                result[p++]='.';
                result[p++]=PATHSEP;
                }
              continue;
              }
            if(!r){                     // Can't go up from root
              result[p++]='.';
              result[p++]='.';
              result[p++]=PATHSEP;
              s=p;
              }
            continue;
            }
          q--;
          }
        q--;
        }
      while(result[q] && result[q]!=PATHSEP){
        result[p++]=result[q++];
        }
      if(result[q]==PATHSEP){
        while(result[q]==PATHSEP) q++;
        result[p++]=PATHSEP;
        }
      }
    return result.trunc(p);
    }
  return FXString::null;
  }


// Check if file represents absolute pathname
//
// On Linux, isAbsolute() returns true for paths of the form:
//
//   "/"
//
FXbool FXPath::isAbsolute(const FXString& file){
  return file[0]==PATHSEP;
  }

// Return absolute path name
// If file is not absolute, place the current directory in
// front, and simplify the result to obtain canonical path.
FXString FXPath::absolute(const FXString& file){
  if(file[0]!=PATHSEP){
    FXString result(FXSystem::getCurrentDirectory());
    if(!file.empty()){
      if(result.tail()!=PATHSEP) result.append(PATHSEP);
      result.append(file);
      }
    return FXPath::simplify(result);
    }
  return FXPath::simplify(file);
  }

// Return absolute path from base directory and file name
// If file is not absolute, place the base-directory in
// front, and simplify the result to obtain canonical path.
FXString FXPath::absolute(const FXString& base,const FXString& file){
  if(file[0]!=PATHSEP){
    FXString result(FXPath::absolute(base));
    if(!file.empty()){
      if(result.tail()!=PATHSEP) result.append(PATHSEP);
      result.append(file);
      }
    return FXPath::simplify(result);
    }
  return FXPath::simplify(file);
  }


// Return relative path of file to given absolute base directory
//
// Examples:
//
//  Base       File         Result      Comment
//  a          /b           /b          Base is relative but file is not
//  /a         b            b           Base is absolute but file is not
//
//  /a/b/c     /a/b/c/d     d           Branch point is /a/b/c
//  /a/b/c/    /a/b/c/d     d           Branch point is /a/b/c
//
//  a          b            ../b        Branch point is assumed ..
//  ./a        ./b          ../b        Branch point is assumed ..
//
//  /a/b/c     /a/b/c       ../c        Branch point is /a/b
//  /a/b/c/    /a/b/c       ../c        Branch point is /a/b
//  /a/b/c/    /a/b/c/      .           Branch point is /a/b/c
//  /a/b/c     /a/b/c/      .           Branch point is /a/b/c
//
//  ../a/b/c   ../a/b/c/d   d           Branch point is ../a/b/c
//
//  /a/b/c/d   /a/b/c       ../         Branch point is /a/b/c
//
//  /a/b/c/d   /a/b/q       ../../q     Branch point is /a/b
//
//  /          /a           a           Branch point is /
//  /a         /b           ../b        Branch point is /
//  /a/b       /c           ../../c     Branch point is /
//  /          /a/b         a/b         Branch point is /
//  /p/q       /a/b         ../../a/b   Branch point is /
//
FXString FXPath::relative(const FXString& base,const FXString& file){
  if(!base.empty() && !file.empty()){
    FXint p=0,q=0,bp=0,bq=0;

    // Find branch point
    while(base[p] && file[q]){
      if(base[p]==PATHSEP && file[q]==PATHSEP){
        bp=p; while(base[p]==PATHSEP) p++;  // Eat multiple slashes
        bq=q; while(file[q]==PATHSEP) q++;
        continue;
        }
      if(base[p]==file[q]){
        p++;
        q++;
        continue;
        }
      break;
      }

   // Base and file should be either both absolute or both relative.
   // The branch point MUST be at a proper head, i.e. at least a
   // "C:\" or "\\Server\Share\" on Windows ["/" on Linux] if it is
   // an absolute path.
   if((base[bq]==PATHSEP) == (file[bq]==PATHSEP)){

      // Check if common prefix extends to the end of the base-path.
      // If the file-path ends in "/", the final component is a directory,
      // in which case it is OK to return ".".  Otherwise we prefer
      // to prefix the file-part with "../".
      if((base[p]=='\0' || base[p]==PATHSEP) && file[q]==PATHSEP){
        bp=p;
        bq=q;
        }

      // Strip leading path character off, if any
      while(file[bq]==PATHSEP) bq++;

      // Non trivial
      if(file[bq]){
        FXString result;

        // Up to branch point
        while(base[bp]){
          while(base[bp]==PATHSEP) bp++;
          if(base[bp]){
            while(base[bp] && base[bp]!=PATHSEP) bp++;
            result.append(".." PATHSEPSTRING);
            }
          }

        // Append tail end
        result.append(&file[bq]);
        return result;
        }
      return ".";
      }
    }
  return file;
  }


// Return true if file is inside base directory
//
// Examples:
//
//  Base       File         Result      Comment
//  /a/b/c     /a/b/c/d      yes        /a/b/c/d is under directory /a/b/c
//  /a/b/c     /a/b          no         /a/b is NOT under directory /a/b/c
//  ./a/b      ./a/b/c       yes        ./a/b/c is under directory ./a/b
//  a          b             no         ./b is NOT under ./a
//  a/b        a/c           no         ./a/c is NOT under ./a/b
//  /a/b/c     c             no         ./c is NOT (necessarily) under /a/b/c
//  a          /a/b          no         /a/b is NOT under ./a
//  .          b             yes        ./b is under .
//  ..         b             yes        ./b is under ./..
//  ../a       b             no         ./b is NOT under ../a
//  ./a/b      a/b           yes        ./a/b is under ./a/b
//  a/b        ./a/b/c       yes        ./a/b/c is under ./a/b
//  ./a/b      a/b           yes        ./a/b is under ./a/b
//  .          .             yes        . is under .
//  ..         .             yes        . is under ./..
//  ..         ..            yes        .. is under ..
//  .          ..            no         ./.. is NOT under .
//  ../a/b     ../a/b/c      yes        ../a/b/c is under ../a/b
//  ../a/b     ../d          no         ../d is NOT under ../a/b
//  (empty)    (something)   no         ./something is NOT under empty
FXbool FXPath::isInside(const FXString& base,const FXString& file){
  if(!base.empty() && !file.empty()){
    FXint p=0,q=0,v=0;
    // Process potentially non-canonical paths
    while(base[p]){
      if(base[p]==PATHSEP){
        if(file[q]!=PATHSEP) return false;
        while(base[p]==PATHSEP) p++;                            // Eat '/'
        while(file[q]==PATHSEP) q++;                            // Eat '/'
        }
a:    if(base[p]=='.'){
        if(base[p+1]=='\0'){ p+=1; goto a; }                    // Eat '.'
        if(base[p+1]==PATHSEP){ p+=2; goto a; }                 // Eat './'
        if(base[p+1]=='.'){
          if(base[p+2]=='\0'){ p+=2; v++; goto a; }             // Eat '..'
          if(base[p+2]==PATHSEP){ p+=3; v++; goto a; }          // Eat '../'
          }
        }
b:    if(file[q]=='.'){
        if(file[q+1]=='\0'){ q+=1; goto b; }                    // Eat '.'
        if(file[q+1]==PATHSEP){ q+=2; goto b; }                 // Eat './'
        if(file[q+1]=='.'){
          if(file[q+2]=='\0'){ q+=2; v--; goto b; }             // Eat '..'
          if(file[q+2]==PATHSEP){ q+=3; v--; goto b; }          // Eat '../'
          }
        }
      if(v<0) return false;
      while(base[p] && base[p]!=PATHSEP){
        if(base[p]!=file[q]) return false;
        p++;
        q++;
        }
      }
    return true;
    }
  return false;
  }
 


// Does file represent topmost directory
//
// On Windows, isTopDirectory() returns true for paths of the form:
//
//   "\"
//   "C:\"
//   "\\Server\Share\"
//
// On Linux, isTopDirectory() returns true for paths of the form:
//
//   "/"
//
FXbool FXPath::isTopDirectory(const FXString& file){
  if(!file.empty()){
    return file[0]==PATHSEP && file[1]=='\0';
    }
  return false;
  }


// Return true if input path is a hidden file or directory
FXbool FXPath::isHidden(const FXString& file){
  if(!file.empty()){
    FXint i=file.length();
    while(0<i && file[i-1]!=PATHSEP){ --i; }
    return file[i]=='.';
    }
  return false;
  }


// Enquote filename to make safe for shell
//
// Enclose with single quotes when:
//
//   - If force is true.
//   - If filename is empty.
//   - If white space before, in, or after letters.
//   - If special shell characters encountered.
//
// Escaping is done when:
//
//   - Quote character (') are encountered
//
FXString FXPath::enquote(const FXString& file,FXbool force){
  if(!file.empty()){
    FXint p,q,e,c;
    p=q=e=0;
    while((c=file[q++])!='\0'){
      switch(c){
      case '\'':                // Single quote
        p+=2;                   // Escaped as xxx\'xxx if outside quotes
        e+=2;                   // Escaped as 'xxx'\''xxx' if inside quotes
        continue;
      case '`':                 // Command substitution
      case '"':                 // Double quote
      case '\\':                // Back slash
      case '!':                 // History expansion
      case '$':                 // Variable substitution
      case '(':
      case ')':
      case '&':
      case ';':
      case '<':                 // Redirections, pipe
      case '>':
      case '|':
      case '*':                 // Wildcard characters
      case '^':
      case '+':
      case '?':
      case '[':
      case ']':
      case '{':                 // Brace expansion
      case '}':
      case '=':                 // Equals
      case '%':                 // Job control
      case '#':                 // Comments
      case '\r':
      case '\n':
      case ' ':                 // White space
      case '\t':
        force=true;             // Force quotes
        p++;
        continue;
      case '~':                 // Username substitution
        if(p==1) force=true;    // Force quotes if at beginning
        /*FALL*/
      default:                  // Normal character
        p++;
        continue;
        }
      }

    // Extra space when quoted
    if(force) p+=e+2;

    // File must be escaped or quoted
    if(file.length()<p){
      FXString result('\0',p);
      p=q=0;
      if(force) result[p++]='\'';       // Opening quote
      while((c=file[q++])!='\0'){
        if(c=='\''){                    // Quote needs to be escaped
          if(force) result[p++]='\'';   // Closing quote
          result[p++]='\\';
          result[p++]='\'';
          if(force) result[p++]='\'';   // Opening quote
          continue;
          }
        result[p++]=c;                  // Normal characters
        }
      if(force) result[p++]='\'';       // Closing quote
      FXASSERT(p<=result.length());
      return result;
      }

    // Unchanged filename
    return file;
    }
  return "''";
  }

// Dequote filename that was enquoted for the command line.
//
//  Input       Result          Comment
//
//  ""          (empty)         To pass empty string.
//
//  "a b c"     a b c           Double quotes for embedded spaces.
//  'a b c'     a b c           Single quotes for embedded spaces.
//  a\ b\ c     a b c           Escapes for embedded spaces.
//
//  \<          <               Outside of quotes, '\' escapes characters.
//  \\          \               Embed escape character itself.
//
//  '\$ENV'     \$ENV           Inside single quotes, '\' has no special meaning.
//  "\$ENV"     $ENV            Inside double quotes, '\' only escapes '$', '\', '`', and '"'.
//  "\ENV"      \ENV            No special meaning for '\', unless followed by '$', '\', '`', or '"'.
//
//  'a'\''b'    a'b             Cannot escape '\'' inside single quotes.
//
//  a b         a               Only first parameter will be returned.
//
//  '"'         "               Enquote '"' inside '..' is fine.
//  "'"         '               As is '\'' inside "...".
//
//  \<NL>       (nothing)       Line continuation for '\' followed by newline.
//
FXString FXPath::dequote(const FXString& file){
  if(!file.empty()){
    FXString result(file);
    FXint quote=0,param=0,p=0,q=0;
    FXchar c;
    while((c=file[q++])!='\0'){
      switch(c){
      case '\'':                        // Single quoted, no escape codes in here
        if(quote==2) goto nrm;          // "...'..."
        quote^=1;
        param=1;
        break;
      case '"':                         // Double quoted, escape codes only for $\`"
        if(quote==1) goto nrm;          // '..."...'
        quote^=2;
        param=1;
        break;
      case '\r':
      case '\n':
      case ' ':                         // Whitespace
      case '\t':
        if(quote) goto nrm;             // Embedded whitespace
        if(param) goto fin;             // Finished parameter
        break;
      case '\\':
        if(quote==1) goto nrm;
        if(file[q]){
          c=file[q++];                  // Escaped character
          if(c=='\n') break;            // Line continuation
          if(quote==2 && c!='$' && c!='\\' && c!='`' && c!='"'){
            result[p++]='\\';           // Backslash remains inside "..."
            }
          }
        /*FALL*/
      default:                          // Normal characters
nrm:    result[p++]=c;                  // Append it
        param=1;
        break;
        }
      }
    FXASSERT(p<=result.length());
fin:result.trunc(p);
    return result;
    }
  return FXString::null;
  }


// Parse command to argc and argv, according to os-native rules
//
// \<nl>        : Line continuation NOT counted as a space
// \X           : Escaped character (when outside quotes)
// "......"     : Only $, \, and ' need escaping inside double-quoted text
// '......'     : No characters can be escaped inside single-quoted text
//
FXint FXPath::parseArgs(FXchar**& argv,const FXchar* command){
  argv=nullptr;
  if(command){
    const FXchar *p;
    FXint token=0;
    FXint quote=0;
    FXint count=0;
    FXint space=0;
    FXchar* buffer;
    FXchar** ptr;
    FXchar* arg;
    FXchar* a;
    FXchar c;

    // First pass: measure output
    p=command;
    while((c=*p++)!='\0'){
      switch(c){
      case '\\':                        // Escape
        if(quote!=1) c=*p++;            // No escape inside single quote
        if(c=='\n') break;              // Line continuation
        if(quote==2 && c!='$' && c!='\\' && c!='`' && c!='"'){
          space++;                      // Backslash remains inside "..."
          }
        space++;
        token=1;
        break;
      case '\'':                        // Single quoted, no escape codes in here
        if(quote==2) goto nrm1;         // "...'..."
        quote^=1;
        token=1;
        break;
      case '"':                         // Double quoted, escape codes only for $\`"
        if(quote==1) goto nrm1;         // '..."...'
        quote^=2;
        token=1;
        break;
      case ' ':                         // White space
      case '\t':
      case '\r':
      case '\n':
        if(quote) goto nrm1;            // White space in parameter
        if(!token) break;               // No preceeding token
        token=0;
        count++;                        // Another argument
        space++;
        break;
      default:                          // Normal characters
nrm1:   space++;
        token=1;
        break;
        }
      }

    // Final closeout
    count+=token;
    space+=token;

    // We got at least one argument
    if(count){

      // Allocate one buffer for the whole thing
      if(allocElms(buffer,(count+1)*sizeof(FXchar*)+space)){

        // First part of buffer contains pointers
        argv=ptr=(FXchar**)buffer;

        // Point to where the characters start
        arg=a=(FXchar*)&argv[count+1];

        quote=token=0;

        // Second pass: generate output
        p=command;
        while((c=*p++)!='\0'){
          switch(c){
          case '\\':                    // Escape
            if(quote!=1) c=*p++;        // No escape inside single quote
            if(c=='\n') break;          // Line continuation
            if(quote==2 && c!='$' && c!='\\' && c!='`' && c!='"'){
              *a++='\\';                // Backslash remains inside "..."
              }
            *a++=c;
            token=1;
            break;
          case '\'':                    // Single quoted, no escape codes in here
            if(quote==2) goto nrm2;     // "...'..."
            quote^=1;
            token=1;                    // Allow for empty argument ''
            break;
          case '"':                     // Double quoted, escape codes only for $\`"
            if(quote==1) goto nrm2;     // '..."...'
            quote^=2;
            token=1;                    // Allow for empty argument ""
            break;
          case ' ':                     // White space
          case '\t':
          case '\r':
          case '\n':
            if(quote) goto nrm2;        // White space in parameter
            if(!token) break;           // No preceeding token
            token=0;
            *ptr++=arg;                 // Another argument
            *a++='\0';
            arg=a;
            break;
          default:                      // Normal characters
nrm2:       *a++=c;
            token=1;
            break;
            }
          }

        // Final token closeout
        if(token){
          *ptr++=arg;
          *a++='\0';
          }

        // Argv closeout
        *ptr=nullptr;

        // Make sure it all fitted
        FXASSERT(a<=buffer+(count+1)*sizeof(FXchar*)+space);
        return count;
        }
      }
    }
  return 0;
  }


// Check if file has executable extension
FXbool FXPath::hasExecExtension(const FXString&){
  return false;
  }

// Check if given name is controversial
FXbool FXPath::isReservedName(const FXString&){
  return false;
  }

}

#endif
