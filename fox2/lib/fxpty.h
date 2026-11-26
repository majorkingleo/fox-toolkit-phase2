/********************************************************************************
*                                                                               *
*                         P s e u d o   T e r m i n a l                         *
*                                                                               *
*********************************************************************************
* Copyright (C) 2026 by Jeroen van der Zijp.   All Rights Reserved.             *
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
#ifndef FXPTY_H
#define FXPTY_H

#ifndef FXDEFS_H
#include "fxdefs.h"
#endif

#ifndef FXSTRING_H
#include "FXString.h"
#endif

#ifndef FXTERMINAL_H
#include "FXTerminal.h"
#endif


namespace FX {


class FXPtyPrivate;


/**
* FXPty starts one shell on a real pseudo terminal and moves bytes between the
* child and the UI thread. The header exposes no platform handles. The UI
* registers wakeHandle() with FXApp::addInput() and drains read() on the event;
* write() queues bytes so the UI never blocks on a full terminal input buffer.
*/
class FXAPI FXPty {
protected:
  FXPtyPrivate *priv;                  // Platform state
private:
  FXPty(const FXPty&);
  FXPty& operator=(const FXPty&);
public:

  /// Construct an idle pseudo terminal
  FXPty();

  /// Start a child process on a new pseudo terminal
  FXbool start(const FXTerminalLaunch& launch,FXString& error);

  /// Return true while the child is running
  FXbool isRunning();

  /// Return the child exit code, or -1 while it has not exited
  FXint getExitCode();

  /// Return the child process id
  FXint getProcessID() const;

  /// Return the UI notification handle; POSIX master fd or Win32 event
  FXInputHandle getWakeHandle() const;

  /// Clear the notification when the output queue is empty
  void clearWake();

  /// Read output without blocking; returns 0 when empty, -1 on EOF/error
  FXival read(FXchar* buffer,FXival size);

  /// Queue input bytes; returns bytes accepted, or -1 on failure
  FXival write(const FXchar* buffer,FXival size);

  /// Try to flush queued input; always true on Win32
  FXbool flushWrites();

  /// Return true if input bytes are queued
  FXbool hasPendingWrites() const;

  /// Resize the terminal size seen by the child
  void resize(FXint columns,FXint rows);

  /// Signal the child; force sends SIGKILL or terminates the Win32 job
  void terminate(FXbool force=false);

  /// Release the PTY and the child process
  void close();

  /// Destructor
  ~FXPty();
  };

}

#endif
