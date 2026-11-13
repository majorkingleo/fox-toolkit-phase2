/********************************************************************************
*                                                                               *
*                         P r o c e s s   S u p p o r t                         *
*                                                                               *
*********************************************************************************
* Copyright (C) 1997,2026 by Jeroen van der Zijp.   All Rights Reserved.        *
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
#include "FXElement.h"
#include "FXArray.h"
#include "FXMetaClass.h"
#include "FXString.h"
#include "FXIO.h"
#include "FXIODevice.h"
#include "FXProcess.h"

/*
  Notes:
  - On Windows, if custom environment is passed, we must merge some stock environment
    variables in with the custom ones.

    A tentative list (please let me know if this is a complete list):

        Variable:               Typical value:
        =========               =============================================
        PATH                    C:\Windows\System32\;C:\Windows\;C:\Windows\System32
        PATHEXT                 .COM; .EXE; .BAT; .CMD; .VBS; .VBE; .JS ; .WSF; .WSH
        COMSPEC                 C:\Windows\System32\cmd.exe
        SYSTEMDRIVE             C:
        SYSTEMROOT              C:\Windows
        OS                      Windows_NT
        TMP                     C:\DOCUME~1\{username}\LOCALS~1\TEMP
        TEMP                    C:\DOCUME~1\{username}\LOCALS~1\TEMP
        HOMEDRIVE               C:
        HOMEPATH                \Documents and Settings\{username}
        PROGRAMFILES            C:\Program Files
        COMPUTERNAME            {computername}
        USERNAME                {username}
        USERDOMAIN              {servername}
        USERPROFILE             C:\Documents and Settings\{username}
        ALLUSERSPROFILE         C:\Documents and Settings\All Users
        COMMONPROGRAMFILES      C:\Program Files\Common Files
        NUMBER_OF_PROCESSORS    1
        PROCESSOR_ARCHITECTURE  x86
        PROCESSOR_IDENTIFIER    {name}
        PROCESSOR_LEVEL         {stepping}
        PROCESSOR_REVISION      {flags}
        WINDIR                  C:\Windows
        LOGONSERVER             \\{servername}

    These should be copied from the parent process environment settings.

  - On Windows, no file descriptors are inherited from the parent process.  We do pass
    stdin, stdout, stderr.

  - The following reference was used to achieve proper enquoting of commandline parameters
    to pass along to CreateProcess():

        http://www.daviddeley.com/autohotkey/parameters/parameters.htm

  - On *NIX, we close all file descriptors except the stdin, stdout, stderr.

  - The Microsoft C/C++ Parameter Parsing Rules Rephrased:

     o Parameters are always separated by a space or tab (multiple spaces/tabs OK).

     o If the parameter does not contain any spaces, tabs, or double quotes, then all the characters
       in the parameter are accepted as is (there is no need to enclose the parameter in double quotes).

     o Enclose spaces and tabs in a double quoted part.

     o A double quoted part can be anywhere within a parameter.

     o 2n backslashes followed by a " produce n backslashes + start/end double quoted part.

     o 2n+1 backslashes followed by a " produce n backslashes + a literal quotation mark.

     o n backslashes not followed by a quotation mark produce n backslashes.

     o undocumented rules regarding double quotes:

       Prior to 2008:
         A " outside a double quoted block starts a double quoted block.
         A " inside a double quoted block ends the double quoted block.
         If a closing " is followed immediately by another ", the 2nd " is accepted literally and
         added to the parameter.

       Post 2008:
         Outside a double quoted block a " starts a double quoted block.
         Inside a double quoted block a " followed by a different character (not another ") ends the
         double quoted block.
         Inside a double quoted block a " followed immediately by another " (i.e. "") causes a
         single " to be added to the output, and the double quoted block continues.

     o Thus:

        Use "    to start/end a double quoted part
        Use \"   to insert a literal "
        Use \\"  to insert a \ then start or end a double quoted part
        Use \\\" to insert a literal \"
        Use \    to insert a literal \

     o Examples given:

       Command-line input      argv[1]         argv[2]         argv[3]
       ----------------------------------------------------------------
       "a b c" d e             a b c           d               e
       "ab\"c" "\\" d          ab"c            \               d
       a\\\b d"e f"g h         a\\\b           de fg           h
       a\\\"b c d              a\"b            c               d
       a\\\\"b c" d e          a\\b c          d               e
       a"b"" c d               ab"             c               d

  - On Linux, consider using posix_spawn() instead of the old fork()/exec() pair.
    This means setting up some posix_spawn_file_actions and posix_spawnattr to
    deal with file descriptors, signal masks, process groups, etc.
    Need POSIX >= 2008 for this.

  - Maybe pidfd_open() is useful to obtain waitable handle to process.
*/

#define TOPIC_CONSTRUCT 1000

using namespace FX;

/*******************************************************************************/

namespace FX {

// Initialize process
FXProcess::FXProcess():pid(0),input(nullptr),output(nullptr),errors(nullptr){
  FXTRACE(TOPIC_CONSTRUCT,"FXProcess::FXProcess\n");
  }


// Return handle of this process object
FXProcessID FXProcess::id() const {
  return pid;
  }

}
