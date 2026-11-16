/********************************************************************************
*                                                                               *
*                            I c o n   S o u r c e                              *
*                                                                               *
*********************************************************************************
* Copyright (C) 2005,2026 by Jeroen van der Zijp.   All Rights Reserved.        *
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
#include "fxmath.h"
#include "FXElement.h"
#include "FXMetaClass.h"
#include "FXHash.h"
#include "FXStream.h"
#include "FXFile.h"
#include "FXFileStream.h"
#include "FXMemoryStream.h"
#include "FXString.h"
#include "FXPath.h"
#include "FXIcon.h"
#include "FXImage.h"
#include "FXIconSource.h"

// Built-in image formats
#include "FXBMPImage.h"
#include "FXGIFImage.h"

// Built-in icon formats
#include "FXBMPIcon.h"
#include "FXGIFIcon.h"


/*
  Notes:
  - Either load an icon from a file, or load from already open stream.
  - Recognition of some image/icon types based on contents may be less
    certain due to poorly defined signature information in the file.
*/

#define TOPIC_DEBUG     1002

using namespace FX;

/*******************************************************************************/

namespace FX {


// Default icon source used when none provided
FXIconSource FXIconSource::defaultIconSource;


// Object implementation
FXIMPLEMENT(FXIconSource,FXObject,nullptr,0)


// Scale image or icon to size
FXImage* FXIconSource::scaleToSize(FXImage *image,FXint size,FXint qual) const {
  if(image){
    if((image->getWidth()>size) || (image->getHeight()>size)){
      if(image->getWidth()>image->getHeight()){
        image->scale(size,(size*image->getHeight())/image->getWidth(),qual);
        }
      else{
        image->scale((size*image->getWidth())/image->getHeight(),size,qual);
        }
      }
    }
  return image;
  }


// Create icon from file type
FXIcon *FXIconSource::iconFromType(FXApp* app,const FXString& type) const {
  if(FXString::comparecase(FXBMPIcon::fileExt,type)==0){
    return new FXBMPIcon(app,nullptr,0,IMAGE_ALPHAGUESS);
    }
  if(FXString::comparecase(FXGIFIcon::fileExt,type)==0){
    return new FXGIFIcon(app);
    }
  return nullptr;
  }


// Create image from file type
FXImage *FXIconSource::imageFromType(FXApp* app,const FXString& type) const {
  if(FXString::comparecase(FXBMPImage::fileExt,type)==0){
    return new FXBMPImage(app);
    }
  if(FXString::comparecase(FXGIFImage::fileExt,type)==0){
    return new FXGIFImage(app);
    }
  return nullptr;
  }


// Determine icon type from first header bytes in stream
FXIcon *FXIconSource::iconFromStream(FXApp* app,FXStream& store) const {
  if(fxcheckBMP(store)){
    return new FXBMPIcon(app,nullptr,0,IMAGE_ALPHAGUESS);
    }
  if(fxcheckGIF(store)){
    return new FXGIFIcon(app);
    }
  return nullptr;
  }


// Determine image type from first header bytes in stream
FXImage *FXIconSource::imageFromStream(FXApp* app,FXStream& store) const {
  if(fxcheckBMP(store)){
    return new FXBMPImage(app);
    }
  if(fxcheckGIF(store)){
    return new FXGIFImage(app);
    }
  return nullptr;
  }


// Load from file
FXIcon *FXIconSource::loadIconFile(FXApp* app,const FXString& filename,const FXString& type) const {
  FXIcon *icon=nullptr;
  FXTRACE(TOPIC_DEBUG,"FXIconSource loadIcon(%s)\n",filename.text());
  if(!filename.empty()){
    FXFileStream store;
    if(store.open(filename,FXStreamLoad,65536)){
      if(type.empty()){
        icon=loadIconStream(app,store,FXPath::extension(filename));
        }
      else{
        icon=loadIconStream(app,store,type);
        }
      store.close();
      }
    }
  return icon;
  }


// Load from data array
FXIcon *FXIconSource::loadIconData(FXApp* app,const FXuchar *pixels,const FXString& type) const {
  FXIcon *icon=nullptr;
  if(pixels){
    FXMemoryStream store;
    store.open(FXStreamLoad,const_cast<FXuchar*>(pixels));
    icon=loadIconStream(app,store,type);
    store.close();
    }
  return icon;
  }


// Load from already open stream
FXIcon *FXIconSource::loadIconStream(FXApp* app,FXStream& store,const FXString& type) const {
  FXIcon *icon=nullptr;
  if(!type.empty()){
    icon=iconFromType(app,type);
    }
  if(!icon){
    icon=iconFromStream(app,store);
    }
  if(icon){
    if(icon->loadPixels(store)) return icon;
    delete icon;
    }
  return nullptr;
  }


// Load from file
FXImage *FXIconSource::loadImageFile(FXApp* app,const FXString& filename,const FXString& type) const {
  FXImage *image=nullptr;
  FXTRACE(TOPIC_DEBUG,"FXIconSource loadImage(%s)\n",filename.text());
  if(!filename.empty()){
    FXFileStream store;
    if(store.open(filename,FXStreamLoad,65536)){
      if(type.empty()){
        image=loadImageStream(app,store,FXPath::extension(filename));
        }
      else{
        image=loadImageStream(app,store,type);
        }
      store.close();
      }
    }
  return image;
  }


// Load from data array
FXImage *FXIconSource::loadImageData(FXApp* app,const FXuchar *pixels,const FXString& type) const {
  FXImage *image=nullptr;
  if(pixels){
    FXMemoryStream store;
    store.open(FXStreamLoad,const_cast<FXuchar*>(pixels));
    image=loadImageStream(app,store,type);
    store.close();
    }
  return image;
  }


// Load from already open stream
FXImage *FXIconSource::loadImageStream(FXApp* app,FXStream& store,const FXString& type) const {
  FXImage *image=nullptr;
  if(!type.empty()){
    image=imageFromType(app,type);
    }
  if(!image){
    image=imageFromStream(app,store);
    }
  if(image){
    if(image->loadPixels(store)) return image;
    delete image;
    }
  return nullptr;
  }


// Load icon and scale it such that its dimensions does not exceed given size
FXIcon *FXIconSource::loadScaledIconFile(FXApp* app,const FXString& filename,FXint size,FXint qual,const FXString& type) const {
  return (FXIcon*)scaleToSize(loadIconFile(app,filename,type),size,qual);
  }


// Load from data array
FXIcon *FXIconSource::loadScaledIconData(FXApp* app,const FXuchar *pixels,FXint size,FXint qual,const FXString& type) const {
  return (FXIcon*)scaleToSize(loadIconData(app,pixels,type),size,qual);
  }


// Load icon and scale it such that its dimensions does not exceed given size
FXIcon *FXIconSource::loadScaledIconStream(FXApp* app,FXStream& store,FXint size,FXint qual,const FXString& type) const {
  return (FXIcon*)scaleToSize(loadIconStream(app,store,type),size,qual);
  }


// Load image and scale it such that its dimensions does not exceed given size
FXImage *FXIconSource::loadScaledImageFile(FXApp* app,const FXString& filename,FXint size,FXint qual,const FXString& type) const {
  return scaleToSize(loadImageFile(app,filename,type),size,qual);
  }


// Load from data array
FXImage *FXIconSource::loadScaledImageData(FXApp* app,const FXuchar *pixels,FXint size,FXint qual,const FXString& type) const {
  return (FXImage*)scaleToSize(loadImageData(app,pixels,type),size,qual);
  }


// Load image and scale it such that its dimensions does not exceed given size
FXImage *FXIconSource::loadScaledImageStream(FXApp* app,FXStream& store,FXint size,FXint qual,const FXString& type) const {
  return scaleToSize(loadImageStream(app,store,type),size,qual);
  }

}
