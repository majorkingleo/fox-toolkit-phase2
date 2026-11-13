/********************************************************************************
*                                                                               *
*                  P a t h   N a m e   M a n i p u l a t i o n                  *
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


/*
  Notes:

  - Windows 95 and NT:
      -  1 to 255 character name.
      -  Complete path for a file or project name cannot exceed 259
         characters, including the separators.
      -  May not begin or end with a space.
      -  May not begin with a $
      -  May contain 1 or more file extensions (eg. MyFile.Ext1.Ext2.Ext3.Txt).
      -  Legal characters in the range of 32 - 255 but not ?"/\<>*|:
      -  Filenames may be mixed case.
      -  Filename comparisons are case insensitive (eg. ThIs.TXT = this.txt).

  - MS-DOS and Windows 3.1:
      -  1 to 11 characters in the 8.3 naming convention.
      -  Legal characters are A-Z, 0-9, Double Byte Character Set (DBCS)
         characters (128 - 255), and _^$~!#%&-{}@'()
      -  May not contain spaces, 0 - 31, and "/\[]:;|=,
      -  Must not begin with $
      -  Uppercase only filename.

  - Some examples of Windows paths:

    "C:\temp\file.txt"                   File "file.txt" in folder "temp" on drive "C:".
    "\\Server\Share\Test\Foo.txt"        File "Foo.txt" in "Test" on "Share" of "Server".
    "\\Server\C$\"                       Root directory "\" on drive "C:" on "Server".
    "\\127.0.0.1\C$\temp\file.txt"       You can see there may be dots in server name!!
    "\\LOCALHOST\C$\temp\file.txt"       Special "\\LOCALHOST\C$" maps to "C:".
    "\\.\UNC\LOCALHOST\C$\temp\file.txt" This is just nuts.
    "\\.\C:\temp\file.txt"               DOS device path.
    "\\?\C:\temp\file.txt"               DOS device path.
    "\\.\CON"                            Devices like LPT1, COM1, etc.

  - Paths "\\?\" are supposedly long pathnames on Windows.

  - On Windows environment variables:

      - Variables have a percent sign on both sides: %ThisIsAVariable%.

      - Name can include spaces, punctuation and mixed case: %_Another Ex.ample%.

      - A variable name may include any of the following characters:

          A-Z, a-z, 0-9, # $ ' ( ) * + , - . ? @ [ ] _ ` { } ~

      - The first character of the name must not be numeric.
*/

using namespace FX;

/*******************************************************************************/

namespace FX {

// Perform tilde and environment variable expansion.
// A prefix of the form "~" or "~user" is expanded to the (current) user's
// home directory.
// Environment variables of the form $VAR or ${VAR} (or %VAR% on Windows)
// are expanded by substituting the value of the variable, recusively up to
// given maximum level.
FXString FXPath::expand(const FXString& file,FXint level){
  if(!file.empty()){
    FXString result;
    if(file[0]=='~'){
      if(file[1] && file[1]!=PATHSEP){
        FXint e=1;
        do{
          if(file[e]=='\\' && file[e+1]) ++e;
          ++e;
          }
        while(file[e] && file[e]!=PATHSEP);
        result=FXSystem::getUserDirectory(file.mid(1,e-1));
        if(!result.empty()){
          if(e<file.length()){
            result.append(expandEnvironmentVariables(file.mid(e,file.length()-e),level));
            }
          return result;
          }
        }
      else{
        result=FXSystem::getHomeDirectory();
        if(!result.empty()){
          if(1<file.length()){
            result.append(expandEnvironmentVariables(file.mid(1,file.length()-1),level));
            }
          return result;
          }
        }
      }
    result=expandEnvironmentVariables(file,level);
    return result;
    }
  return FXString::null;
  }

/*******************************************************************************/

// Convert a UNIX path to Windows conventions:
//
//  - Replace "/" with "\" for path separator.
//  - Replace "${ENVVAR}" or "$ENVVAR" with "%ENVVAR%".
//
// Also, if path starting with a single "/", prefix it by:
//
//  - Drive letter "C:" from the current working directory.
//  - Share name "\\Server\Share" from the current working directory.
//
FXString FXPath::convertToWindows(const FXString& path){
  FXString result;
  if(!path.empty()){
    FXint p=0,q=0,s=0;
    if(path[0]=='/' && path[1]!='/'){                           // Not fully qualified absolute
      result=FXSystem::getCurrentDirectory();
      if(Ascii::isLetter(result[0]) && result[1]==':'){
        p=2;                                                    // Prefix "C:"
        }
      else if(result[0]=='\\' && result[1]=='\\' && result[2]!='\\' && result[2]){
        s=3;
        while(result[s] && result[s]!='\\') s++;
        if(result[s]=='\\'){
          s++;
          if(result[s] && result[s]!='\\'){
            s++;
            while(result[s] && result[s]!='\\') s++;
            p=s;                                                // Prefix "\\Server\Share"
            }
          }
        }
      }
    if(result.length(p+path.length()+path.contains('$'))){      // Size estimate
      while(path[q]){
        if(path[q]=='/'){                                       // Path separator
          result[p++]='\\';
          q++;
          continue;
          }
        if(path[q]=='$'){                                       // Possible variable
          s=q+1;
          if(path[s]=='{'){
            s++;
            while(Ascii::isWord(path[s])) s++;
            if(path[s]=='}' && q+2<s){                          // Non-empty ${VARIABLE}
              result[p++]='%';
              q+=2;
              do{
                result[p++]=path[q++];
                }
              while(q<s);
              result[p++]='%';
              q++;
              continue;
              }
            }
          else{
            while(Ascii::isWord(path[s])) s++;
            if(q+1<s){                                          // Non-empty $VARIABLE
              result[p++]='%';
              q+=1;
              do{
                result[p++]=path[q++];
                }
              while(q<s);
              result[p++]='%';
              continue;
              }
            }
          }
        result[p++]=path[q++];                                  // Normal character
        }
      FXASSERT(p<=result.length());
      result.trunc(p);
      }
    }
  return result;
  }


// Convert Windows path to UNIX conventions:
//
//  - Replace "\" with "/" for path separator.
//  - Replace "%ENVVAR%" with "${ENVVAR}".
//
// If prefixed by drive letter "C:" or share name "\\Server\Share":
//
//  - Drop drive letter "C:" from the front.
//  - Drop share name "\\Server\Share" from the front.
//
FXString FXPath::convertFromWindows(const FXString& path){
  FXString result;
  if(!path.empty()){
    if(result.length(path.length()+path.contains('%')/2)){      // Size estimate
      FXint p=0,q=0,s;
      if(Ascii::isLetter(path[0]) && path[1]==':'){
        q=2;                                                    // Strip "C:"
        }
      else if(path[0]=='\\' && path[1]=='\\' && path[2]!='\\' && path[2]){
        s=3;
        while(path[s] && path[s]!='\\') s++;
        if(path[s]=='\\'){
          s++;
          if(path[s] && path[s]!='\\'){
            s++;
            while(path[s] && path[s]!='\\') s++;
            q=s;                                                // Strip "\\Server\Share"
            }
          }
        }
      while(path[q]){
        if(path[q]=='\\'){                                      // Path separator
          result[p++]='/';
          q++;
          continue;
          }
        if(path[q]=='%'){                                       // Environment variable
          s=q+1;
          while(Ascii::isWord(path[s])) s++;
          if(path[s]=='%' && q+1<s){                            // Non-empty %VARIABLE%
            result[p++]='$';
            result[p++]='{';
            q+=1;
            do{
              result[p++]=path[q++];
              }
            while(q<s);
            result[p++]='}';
            q++;
            continue;
            }
          }
        result[p++]=path[q++];                                  // Normal character
        }
      FXASSERT(p<=result.length());
      result.trunc(p);
      }
    }
  return result;
  }

/*******************************************************************************/

// Return true if file name is relative
// Which means "." or "./blabla" or ".." or "../blabla".
// But ".blabla" is NOT relative.
FXbool FXPath::isRelative(const FXString& file){
  return file[0]=='.' && ((file[1]=='\0' || file[1]==PATHSEP) || (file[1]=='.' && (file[2]=='\0' || file[2]==PATHSEP)));
  }


// Return relative path of file to the current directory
FXString FXPath::relative(const FXString& file){
  return FXPath::relative(FXSystem::getCurrentDirectory(),file);
  }

/*******************************************************************************/

// Return path to directory above input directory name
// Only append a PATHSEP if there isn't one already;
// necessary because don't want to create a UNC from
// a regular path.
FXString FXPath::upLevel(const FXString& file){
  if(!file.empty()){
    if(file.tail()==PATHSEP){
      return FXPath::simplify(file+"..");
      }
    return FXPath::simplify(file+PATHSEPSTRING "..");
    }
  return ".";
  }

/*******************************************************************************/

// Return valid part of absolute path
FXString FXPath::validPath(const FXString& file){
  if(FXPath::isAbsolute(file)){
    FXString result(file);
    while(!FXPath::isTopDirectory(result) && !FXStat::exists(result)){
      result=FXPath::upLevel(result);
      }
    return result;
    }
  return FXString::null;
  }

/*******************************************************************************/

// Return true if absolute path is valid
FXbool FXPath::isValidPath(const FXString& file){
  if(FXPath::isAbsolute(file)){
    return FXStat::exists(file);
    }
  return false;
  }

/*******************************************************************************/

// Parse command to argc and argv
FXint FXPath::parseArgs(FXchar**& argv,const FXString& command){
  return parseArgs(argv,command.text());
  }

/*******************************************************************************/

// Skip over part of pattern
//
// This is a bit complex: we need to count matching parentheses, while
// also keeping track of whether we're inside a charset, as parentheses
// in a charset are not grouping parentheses.
// Charsets are delimited by brackets, but a closing bracket appearing
// as the first character in the set does not close the charset, but is
// taken as just a character.  Note, the first actual character in a
// charset may actually follow a set-negation character, '!' or '^'.
// If alt=true, we don't scan all the way to the end of the group, but
// just skip to the start of the next alternative, which immediately
// follows a '|' or ',' character.
// A last and final concern is the escape sequence (if enabled), which
// prevents interpretation of the escaped character as a special directive.
static const FXchar* skip(const FXchar* p,FXuint flags,FXbool alt){
  FXint set=0;          // Characters in set [...]
  FXint brk=0;          // Bracket set
  FXint par=0;          // Group level
  FXint neg=0;          // Negated
n:switch(*p){
    case '\0':
      goto x;
    case '[':           // Enter character set [...]
      p++;
      if(!brk){ brk=1; set=0; neg=0; goto n; }
      set++;
      goto n;
    case ']':           // Leave character set [...] (if not first character in set []...] or [!]...] or [^]...])
      p++;
      if(brk && set){ brk=0; goto n; }
      set++;
      goto n;
    case '!':
    case '^':           // Invert character set [^...] or [!...] (if in set [...])
      p++;
      if(brk && !set && !neg){ neg=1; goto n; }
      set++;
      goto n;
    case '(':           // Enter sub group (...) (if not in character set [...])
      p++;
      if(!brk){ ++par; }
      set++;
      goto n;
    case ')':           // Leave sub group (...) (if not in character set [...])
      if(!brk){ if(--par<0) goto x; }
      p++;
      set++;
      goto n;
    case ',':           // End of alternatives
    case '|':
      if(!brk && !par && alt){ goto x; }
      p++;
      set++;
      goto n;
    case '\\':          // Escape code
      if(!(flags&FXPath::NoEscape)){ if(*++p=='\0') goto x; }
      /*FALL*/
    default:            // Regular character
      p=wcinc(p);       // Next unicode character
      set++;
      goto n;
    }
x:return p;
  }


// Perform match
//
// Special pattern characters are interpreted as follows:
//
//   ?   Normally matches a single unicode character. However, when the DotFile flag is passed,
//       the ? does NOT match a '.' if it appears at the start of the string, or if it follows a
//       '/' and the PathName flag is also passed.
//
//   *   Normally matches zero or more unicode characters. However, when the DotFile flag is passed,
//       the * does NOT match a '.' if the '.' appears at the start of the string, or if it follows a
//       '/' and the PathName flag is also passed.
//
//  [ ]  Character set matches a single unicode character in the set.  However, then the DotFile flag
//       is passed, the [] does NOT match a '.' if it appears at the start of the string, or if it follows
//       a '/' and the PathName flag is also passed.
//       A set is negated (matches when the corresponding character is NOT in the set), by following the
//       opening '[' with '^' or '!'.
//
//   \   Escaped characters match the unicode character.  If \ is NOT followed by a character, the match
//       will fail.
//
// ( | ) Will start, separate, and end a group of alternatives, each of which may contain pattern
//       characters as well as sub-groups thereof; The ',' may also be used.
//
static FXbool domatch(const FXchar* string,const FXchar* s,const FXchar* p,FXuint flags){
  const FXchar* q;
  FXwchar pp,qq,ss;
  FXbool neg,ok;
  while(*p!='\0'){
    switch(*p){
      case '(':         // Start subpattern
        p++;
        q=skip(p,flags,false);                                  // End of sub group
        if(*q!=')') return false;                               // Missing ')'
        while(p<q){
          if(domatch(string,s,p,flags)) return true;
          p=skip(p,flags,true);                                 // End of alternative
          if(*p!=',' && *p!='|') continue;
          p++;
          }
        return false;
      case ')':         // End subpattern
        p++;
        break;
      case '|':         // Alternatives
      case ',':
        p=skip(p+1,flags,false);                                // Continue to match the rest
        break;
      case '?':         // Single character wildcard
        p++;                                                    // Eat '?'
        if(*s=='\0') return false;
        if((*s==PATHSEP) && (flags&FXPath::PathName)) return false;
        if((*s=='.') && (flags&FXPath::DotFile) && ((s==string) || ((*(s-1)==PATHSEP) && (flags&FXPath::PathName)))) return false;
        s=wcinc(s);
        break;
      case '*':         // Multiple character wildcard
        while(*p=='*') p++;                                     // Eat '*'
        while(*s!='\0'){
          if(domatch(string,s,p,flags)) return true;
          if((*s=='.') && (flags&FXPath::DotFile) && ((s==string) || ((*(s-1)==PATHSEP) && (flags&FXPath::PathName)))) return false;
          if((*s==PATHSEP) && (flags&FXPath::PathName)) return false;
          s=wcinc(s);
          }
        break;
      case '[':         // Single character against character-set
        p++;                                                    // Eat '['
        if(*s=='\0') return false;
        if((*s==PATHSEP) && (flags&FXPath::PathName)) return false;
        if((*s=='.') && ((s==string) || (*(s-1)==PATHSEP)) && (flags&FXPath::DotFile)) return false;
        neg=(*p=='!' || *p=='^');
        if(neg) p++;
        ss=wc(s);
        if(flags&FXPath::CaseFold){ ss=Unicode::toLower(ss); }
        s=wcinc(s);
        ok=false;
        do{
          if(*p=='\\' && !(flags&FXPath::NoEscape)) p++;
          if(*p=='\0') return false;
          pp=wc(p);
          if(flags&FXPath::CaseFold){ pp=Unicode::toLower(pp); }
          p=wcinc(p);
          if(*p=='-' && *(p+1)!=']'){                           // Range match
            p++;
            if(*p=='\\' && !(flags&FXPath::NoEscape)) p++;
            if(*p=='\0') return false;
            qq=wc(p);
            if(flags&FXPath::CaseFold){ qq=Unicode::toLower(qq); }
            p=wcinc(p);
            if(pp<=ss && ss<=qq) ok=true;
            }
          else{                                                 // Single match
            if(pp==ss) ok=true;
            }
          }
        while(*p!=']');
        p++;
        if(ok==neg) return false;
        break;
      case '\\':        // Escaped character follows
        if(!(flags&FXPath::NoEscape)){ if(*++p=='\0') return false; }
        /*FALL*/
      default:          // Match characters against pattern
        pp=wc(p);
        ss=wc(s);
        if(flags&FXPath::CaseFold){
          pp=Unicode::toLower(pp);
          ss=Unicode::toLower(ss);
          }
        if(pp!=ss) return false;
        s=wcinc(s);
        p=wcinc(p);
        break;
      }
    }
  return (*s=='\0') || ((*s==PATHSEP) && (flags&FXPath::LeadDir));
  }


// Match string against a pattern, subject to flags
FXbool FXPath::match(const FXchar* string,const FXchar* pattern,FXuint flags){
  const FXchar* end=skip(pattern,flags,false);  // Check pattern syntax
  if(*end=='\0'){
    while(*pattern){
      if(domatch(string,string,pattern,flags)) return true;
      pattern=skip(pattern,flags,true);
      if(*pattern!=',' && *pattern!='|') break;
      pattern++;
      }
    }
  return false;
  }


// Match string against pattern (like *, ?, [^a-z], and so on)
FXbool FXPath::match(const FXString& string,const FXchar* pattern,FXuint flags){
  return FXPath::match(string.text(),pattern,flags);
  }


// Match string against pattern (like *, ?, [^a-z], and so on)
FXbool FXPath::match(const FXchar* string,const FXString& pattern,FXuint flags){
  return FXPath::match(string,pattern.text(),flags);
  }


// Match string against pattern (like *, ?, [^a-z], and so on)
FXbool FXPath::match(const FXString& string,const FXString& pattern,FXuint flags){
  return FXPath::match(string.text(),pattern.text(),flags);
  }

/*******************************************************************************/

// Generate unique filename of the form pathnameXXX.ext, where pathname.ext is the
// original input file, and XXX is a number, possibly empty, that makes the file unique.
FXString FXPath::unique(const FXString& file){
  if(!FXStat::exists(file)) return file;
  FXString ext=FXPath::extension(file);
  FXString path=FXPath::stripExtension(file);   // Use the new API (Jeroen)
  FXString filename;
  FXint count=0;
  if(!ext.empty()) ext.prepend('.');            // Only add period when non-empty extension
  while(count<1000){
    filename.format("%s%i%s",path.text(),count,ext.text());
    if(!FXStat::exists(filename)) return filename;      // Return result here (Jeroen)
    count++;
    }
  return FXString::null;
  }

/*******************************************************************************/

// Search pathlist for file.
// If the file is absolute, return it.
// If the file is relative, make it absolute w.r.t. current directory, and return it.
// Otherwise, check each component of expanded path list and return the absolute path
// under the component of path list where the file is found.
// If file was empty string, isn't found, or doesn't exist, return the empty string.
FXString FXPath::search(const FXString& list,const FXString& file){
  if(!file.empty()){
    if(FXPath::isAbsolute(file)){
      if(FXStat::exists(file)) return file;
      return FXString::null;
      }
    if(FXPath::isRelative(file)){
      FXString path=FXPath::absolute(file);
      if(FXStat::exists(path)) return path;
      return FXString::null;
      }
    if(!list.empty()){
      FXString paths(FXPath::expand(list));
      FXString path;
      FXint beg=0;
      FXint end=0;
      while(end<paths.length()){
        while(end<paths.length() && paths[end]!=PATHLISTSEP) ++end;
        if(beg<end){
          path=FXPath::absolute(paths.mid(beg,end-beg),file);
          if(FXStat::exists(path)) return path;
          }
        beg=++end;
        }
      }
    }
  return FXString::null;
  }

/*******************************************************************************/

// Relativize file to path list.
// If file is not absolute, its already relative, so just return file.
// Otherwise, check if file is inside one of the folders of path list.
// If not located under any component of pathlist, then just keep the
// absolute path; it can not be relativized to the given list.
FXString FXPath::relativize(const FXString& list,const FXString& file){
  FXString result(file);
  if(FXPath::isAbsolute(result)){
    FXString paths(FXPath::expand(list));
    FXString base;
    FXint beg=0;
    FXint end=0;
    while(end<paths.length()){
      while(end<paths.length() && paths[end]!=PATHLISTSEP) ++end;
      if(beg<end){
        base=paths.mid(beg,end-beg);
        if(FXPath::isInside(base,file)){
          result=FXPath::relative(base,file);
          break;
          }
        }
      beg=++end;
      }
    }
  return result;
  }

}
