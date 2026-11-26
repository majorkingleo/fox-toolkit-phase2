/********************************************************************************
*                                                                               *
*                         T e r m i n a l   P r o f i l e s                     *
*                                                                               *
*********************************************************************************
* Copyright (C) 2026 by Jeroen van der Zijp.   All Rights Reserved.             *
*********************************************************************************
* This program is free software: you can redistribute it and/or modify          *
* it under the terms of the GNU General Public License as published by          *
* the Free Software Foundation, either version 3 of the License, or             *
* (at your option) any later version.                                           *
*                                                                               *
* This program is distributed in the hope that it will be useful,               *
* but WITHOUT ANY WARRANTY; without even the implied warranty of                *
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the                 *
* GNU General Public License for more details.                                  *
*                                                                               *
* You should have received a copy of the GNU General Public License             *
* along with this program.  If not, see <http://www.gnu.org/licenses/>.         *
********************************************************************************/
#ifndef PROFILES_H
#define PROFILES_H

#include "fx.h"
#include "FXTerminal.h"
#include <vector>


/// One terminal launch profile
struct TerminalProfile {
  FXString id;
  FXString name;
  FXString shell;
  FXString arguments;
  FXString directory;
  FXString term;
  FXint    kind;
  FXint    scrollback;
  FXint    cursor;
  FXbool   confirmClose;
  FXbool   closeOnExit;
  };


/**
* Profiles loaded from the application registry. The built-in default and vt100
* profiles are always present; a missing or corrupt entry falls back to them.
*/
class Profiles {
protected:
  std::vector<TerminalProfile> entries;
  FXString active;
public:
  Profiles();

  /// Load profiles and the active id from the registry
  void load(FXRegistry& reg);

  /// Save profiles and the active id to the registry
  void save(FXRegistry& reg) const;

  /// Return number of profiles
  FXint count() const { return (FXint)entries.size(); }

  /// Return one profile
  TerminalProfile& at(FXint index){ return entries[index]; }
  const TerminalProfile& at(FXint index) const { return entries[index]; }

  /// Find a profile by id; returns -1 when absent
  FXint find(const FXString& id) const;

  /// Return the active profile id
  const FXString& getActive() const { return active; }

  /// Set the active profile id
  void setActive(const FXString& id);

  /// Build a launch description from one profile
  FXTerminalLaunch launch(const TerminalProfile& profile) const;

  /// Return the platform default shell
  static FXString defaultShell();
  };

#endif
