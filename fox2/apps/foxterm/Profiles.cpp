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
#include "fx.h"
#include "Profiles.h"

using namespace FX;

/*******************************************************************************/

Profiles::Profiles():active("default"){
  TerminalProfile def;
  TerminalProfile vt;
  def.id="default";
  def.name="Default";
  def.shell=defaultShell();
  def.term="xterm-256color";
  def.encoding="UTF-8";
  def.kind=TERMINAL_XTERM;
  def.scrollback=10000;
  def.cursor=0;
  def.confirmClose=true;
  def.closeOnExit=true;
  vt.id="vt100";
  vt.name="VT100";
  vt.shell=def.shell;
  vt.term="vt100";
  vt.encoding="UTF-8";
  vt.kind=TERMINAL_VT100;
  vt.scrollback=10000;
  vt.cursor=0;
  vt.confirmClose=true;
  vt.closeOnExit=true;
  entries.push_back(def);
  entries.push_back(vt);
  }


// Return platform default shell
FXString Profiles::defaultShell(){
  FXString comspec=FXSystem::getEnvironment("COMSPEC");
  FXString shell=FXSystem::getEnvironment("SHELL");
  if(!comspec.empty()) return comspec;
  if(!shell.empty()) return shell;
  return FXString("/bin/sh");
  }


// Load known profile fields
void Profiles::load(FXRegistry& reg){
  FXString id;
  FXString key;
  for(FXint i=0; i<(FXint)entries.size(); i++){
    id=entries[i].id;
    key="PROFILE."+id;
    entries[i].name=reg.readStringEntry(key,"name",entries[i].name.text());
    entries[i].shell=reg.readStringEntry(key,"shell",entries[i].shell.text());
    entries[i].arguments=reg.readStringEntry(key,"args",entries[i].arguments.text());
    entries[i].directory=reg.readStringEntry(key,"dir",entries[i].directory.text());
    entries[i].term=reg.readStringEntry(key,"term",entries[i].term.text());
    entries[i].encoding=reg.readStringEntry(key,"encoding",entries[i].encoding.text());
    if(FXTerminal::findEncoding(entries[i].encoding)<0) entries[i].encoding="UTF-8";
    entries[i].kind=reg.readIntEntry(key,"kind",entries[i].kind)==TERMINAL_VT100 ? TERMINAL_VT100 : TERMINAL_XTERM;
    if(entries[i].kind==TERMINAL_VT100 && entries[i].term.find("vt100")<0) entries[i].term="vt100";
    if(entries[i].kind==TERMINAL_XTERM && entries[i].term.empty()) entries[i].term="xterm-256color";
    entries[i].scrollback=FXCLAMP(0,reg.readIntEntry(key,"scrollback",entries[i].scrollback),100000);
    entries[i].cursor=FXCLAMP(0,reg.readIntEntry(key,"cursor",entries[i].cursor),6);
    entries[i].confirmClose=reg.readBoolEntry(key,"confirm",entries[i].confirmClose);
    entries[i].closeOnExit=reg.readBoolEntry(key,"closeonexit",entries[i].closeOnExit);
    }
  active=reg.readStringEntry("SETTINGS","activeProfile","default");
  if(find(active)<0) active="default";
  }


// Save all profile fields
void Profiles::save(FXRegistry& reg) const {
  FXString key;
  for(FXint i=0; i<(FXint)entries.size(); i++){
    key="PROFILE."+entries[i].id;
    reg.writeStringEntry(key,"name",entries[i].name.text());
    reg.writeStringEntry(key,"shell",entries[i].shell.text());
    reg.writeStringEntry(key,"args",entries[i].arguments.text());
    reg.writeStringEntry(key,"dir",entries[i].directory.text());
    reg.writeStringEntry(key,"term",entries[i].term.text());
    reg.writeStringEntry(key,"encoding",entries[i].encoding.text());
    reg.writeIntEntry(key,"kind",entries[i].kind);
    reg.writeIntEntry(key,"scrollback",entries[i].scrollback);
    reg.writeIntEntry(key,"cursor",entries[i].cursor);
    reg.writeBoolEntry(key,"confirm",entries[i].confirmClose);
    reg.writeBoolEntry(key,"closeonexit",entries[i].closeOnExit);
    }
  reg.writeStringEntry("SETTINGS","activeProfile",active.text());
  }


// Find a profile
FXint Profiles::find(const FXString& id) const {
  for(FXint i=0; i<(FXint)entries.size(); i++) if(entries[i].id==id) return i;
  return -1;
  }


// Set active profile
void Profiles::setActive(const FXString& id){
  if(find(id)>=0) active=id;
  }


// Build launch
FXTerminalLaunch Profiles::launch(const TerminalProfile& profile) const {
  FXTerminalLaunch launch;
  launch.shell=profile.shell;
  launch.arguments=profile.arguments;
  launch.directory=profile.directory;
  if(launch.directory.empty()) launch.directory=FXSystem::getHomeDirectory();
  launch.term=profile.term;
  launch.kind=profile.kind;
  launch.columns=80;
  launch.rows=24;
  return launch;
  }

/*******************************************************************************/
