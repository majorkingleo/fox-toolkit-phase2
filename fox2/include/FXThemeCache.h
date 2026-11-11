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
#ifndef FXTHEMECACHE_H
#define FXTHEMECACHE_H

#ifndef FXDEFS_H
#include "fxdefs.h"
#endif

namespace FX {


class FXString;
class FXTheme;


/**
* A resolved theme imported from an external source is expensive to produce,
* so it is serialized next to the user's other cached data and reused while the
* source is unchanged.  The cache is keyed on the source path, its size and
* modification time, and on the versions of the importer and of the cache
* schema itself.  Anything that fails validation makes the cache disappear
* silently: a theme is a convenience, never a reason not to start.
*/
class FXAPI FXThemeCache {
protected:
  static FXint  importerVersion;  // Version of the importer that produced cached themes
  static FXint  hits;             // Number of cache hits so far
  static FXint  misses;           // Number of cache misses so far
public:

  /// Directory holding the cache files
  static FXString directory();

  /// Cache file belonging to the given source
  static FXString location(const FXString& source);

  /// Read a cached theme; returns false when missing, stale or corrupt
  static FXbool fetch(const FXString& source,FXTheme& theme);

  /// Store a resolved theme for the given source
  static FXbool store(const FXString& source,const FXTheme& theme);

  /// Remove the cache file of the given source
  static void discard(const FXString& source);

  /// Return the version of the importer that produced the cached themes
  static FXint getImporterVersion();

  /// Override the importer version, to force revalidation; 0 restores the default
  static void setImporterVersion(FXint version);

  /// Number of cache hits
  static FXint getHits();

  /// Number of cache misses
  static FXint getMisses();
  };

}

#endif
