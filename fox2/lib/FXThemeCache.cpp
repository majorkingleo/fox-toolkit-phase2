/********************************************************************************
*                                                                              *
*                   T h e m e   C a c h e   (serialized)                       *
*                                                                              *
*********************************************************************************
* Copyright (C) 1997,2026 by Jeroen van der Zijp.   All Rights Reserved.        *
*********************************************************************************
* This library is free software; you can redistribute it and/or modify          *
* it under the terms of the GNU Lesser General Public License as published by   *
* the Free Software Foundation; either version 3 of the License, or             *
* (at your option) any later version.                                           *
*                                                                              *
* This library is distributed in the hope that it will be useful,               *
* but WITHOUT ANY WARRANTY; without even the implied warranty of                *
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the                 *
* GNU Lesser General Public License for more details.                           *
*                                                                              *
* You should have received a copy of the GNU Lesser General Public License      *
* along with this program.  If not, see <http://www.gnu.org/licenses/>          *
********************************************************************************/
#include "xincs.h"
#include "fxver.h"
#include "fxdefs.h"
#include "fxmath.h"
#include "fxascii.h"
#include "FXElement.h"
#include "FXArray.h"
#include "FXMetaClass.h"
#include "FXHash.h"
#include "FXStream.h"
#include "FXString.h"
#include "FXIO.h"
#include "FXFile.h"
#include "FXStat.h"
#include "FXDir.h"
#include "FXSystem.h"
#include "FXMemoryStream.h"
#include "FXTheme.h"
#include "FXThemeCache.h"


/*
  Notes:
  - The cache is an optimization only: every failure path deletes the file and
    lets the caller import the source again.
  - The cache is untrusted input, so the file size is bounded before reading
    and every string inside the stream is length-checked.
  - The key covers the source path, its size and modification time, the
    importer version and the schema version of the cache itself.
*/


using namespace FX;

/*******************************************************************************/

namespace FX {

// Magic number at the start of a cache file, and the layout version
#define FXTCMAGIC   0x46585443U
#define FXTCSCHEMA  1

// Largest cache file we are willing to read
#define FXTCSIZE    (64*1024)

// Version of the importer; bump this whenever imported values change
static const FXint fxtcdefaultversion=1;

// Cache counters
FXint FXThemeCache::importerVersion=fxtcdefaultversion;
FXint FXThemeCache::hits=0;
FXint FXThemeCache::misses=0;


// Directory holding the cache files
FXString FXThemeCache::directory(){
  FXString base=FXSystem::getEnvironment("XDG_CACHE_HOME");
  if(base.empty()) base=FXSystem::getHomeDirectory()+"/.cache";
  return base+"/fox/themes";
  }


// Cache file belonging to the given source
FXString FXThemeCache::location(const FXString& source){
  FXString name=source.rafter('/');
  FXString cleaned;
  FXint i;
  for(i=0; i<name.length(); i++){
    FXchar ch=name[i];
    cleaned.append(isalnum((FXuchar)ch)?ch:'_');
    }
  return directory()+"/"+cleaned+"-"+FXString::value(source.hash(),16)+".fxtc";
  }


// Test if a file can be opened for reading
static FXbool readable(const FXString& path){
  FXFile file(path,FXIO::Reading);
  return file.isOpen();
  }


// Resolve a theme directory to the style sheet that will be read from it
static FXString resolve(const FXString& source){
  if(readable(source+"/gtk-3.0/gtk.css")) return source+"/gtk-3.0/gtk.css";
  if(readable(source+"/gtk-4.0/gtk.css")) return source+"/gtk-4.0/gtk.css";
  if(readable(source+"/gtk-2.0/gtkrc"))  return source+"/gtk-2.0/gtkrc";
  return source;
  }


// Source size and modification time
static FXbool sourceStamp(const FXString& source,FXlong& size,FXTime& modified){
  FXStat info;
  if(!FXStat::statFile(resolve(source),info)) return false;
  if(!info.isFile()) return false;
  size=info.size();
  modified=info.modified();
  return true;
  }


// Remove the cache file of the given source
void FXThemeCache::discard(const FXString& source){
  FXFile::remove(location(source));
  }


// Read a cached theme
FXbool FXThemeCache::fetch(const FXString& source,FXTheme& theme){
  FXString path=location(source);
  FXlong size=0;
  FXTime modified=0;
  FXFile file;
  FXuchar *buffer=nullptr;
  FXival got;
  FXMemoryStream stream;
  FXuint magic=0;
  FXint schema=0;
  FXint importer=0;
  FXint length=0;
  FXlong cachedsize=0;
  FXlong cachedtime=0;
  FXString cachedsource;

  // Validate the source stamp first
  if(!sourceStamp(source,size,modified)){ misses++; return false; }

  // Read the whole cache, bounded
  file.open(path,FXIO::Reading);
  if(!file.isOpen()){ misses++; return false; }
  if(FXTCSIZE<file.size()){ file.close(); discard(source); misses++; return false; }
  if(!FXMALLOC(&buffer,FXuchar,FXTCSIZE)){ file.close(); misses++; return false; }
  got=file.readBlock(buffer,FXTCSIZE);
  file.close();
  if(got<=0){ FXFREE(&buffer); discard(source); misses++; return false; }
  stream.open(FXStreamLoad,buffer,(FXuval)got,false);

  // Validate the header
  stream >> magic;
  stream >> schema;
  stream >> importer;
  stream >> length;
  if(stream.status()!=FXStreamOK || magic!=FXTCMAGIC || schema!=FXTCSCHEMA || importer!=importerVersion || length<0 || 1024<length){
    FXFREE(&buffer);
    discard(source);
    misses++;
    return false;
    }
  cachedsource.length(length);
  stream.load(cachedsource.text(),length);
  stream >> cachedsize;
  stream >> cachedtime;
  if(stream.status()!=FXStreamOK || cachedsource!=source || cachedsize!=size || cachedtime!=modified){
    FXFREE(&buffer);
    discard(source);
    misses++;
    return false;
    }

  // Deserialize the theme and make sure it read completely
  theme.load(stream);
  if(stream.status()!=FXStreamOK){
    FXFREE(&buffer);
    theme.setClassic();
    discard(source);
    misses++;
    return false;
    }
  FXFREE(&buffer);
  hits++;
  return true;
  }


// Store a resolved theme
FXbool FXThemeCache::store(const FXString& source,const FXTheme& theme){
  FXString path=location(source);
  FXString dir=directory();
  FXlong size=0;
  FXTime modified=0;
  FXFile file;
  FXMemoryStream stream;
  FXuchar *buffer=nullptr;
  FXuval length=0;
  FXint namelen=0;
  if(!sourceStamp(source,size,modified)) return false;
  if(!FXDir::createDirectories(dir)) return false;
  if(!FXMALLOC(&buffer,FXuchar,FXTCSIZE)) return false;
  stream.open(FXStreamSave,buffer,FXTCSIZE,false);
  stream << (FXuint)FXTCMAGIC;
  stream << (FXint)FXTCSCHEMA;
  stream << importerVersion;
  namelen=source.length();
  stream << namelen;
  stream.save(source.text(),namelen);
  stream << size;
  stream << modified;
  theme.save(stream);
  if(stream.status()!=FXStreamOK){
    FXFREE(&buffer);
    return false;
    }
  length=(FXuval)stream.position();
  file.open(path,FXIO::Writing);
  if(!file.isOpen()){
    FXFREE(&buffer);
    return false;
    }
  file.writeBlock(buffer,length);
  file.close();
  FXFREE(&buffer);
  return true;
  }


// Return the version of the importer
FXint FXThemeCache::getImporterVersion(){
  return importerVersion;
  }


// Override the importer version
void FXThemeCache::setImporterVersion(FXint version){
  importerVersion=(0<version)?version:fxtcdefaultversion;
  }


// Number of cache hits
FXint FXThemeCache::getHits(){
  return hits;
  }


// Number of cache misses
FXint FXThemeCache::getMisses(){
  return misses;
  }

}
