/********************************************************************************
*                                                                               *
*                           B M P   I c o n   T e s t                          *
*                                                                               *
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
********************************************************************************/
#include "fx.h"
#include "testargs.h"
#include <stdio.h>

/*
  Build a tiny 24 bit BMP in memory and load it through FXBMPImage and
  FXBMPIcon.  The calculator uses FXBMPIcon for its constant memory
  indicator, and this test checks that the loader and both classes decode
  the pixels the same way FOX 1.7.95 does.
*/

namespace {

const FXint W=3;
const FXint H=2;

// Pixel colors; row 0 is the top row of the image
const FXColor top[W]={FXRGB(255,0,0),FXRGB(0,255,0),FXRGB(0,0,255)};
const FXColor bottom[W]={FXRGB(10,20,30),FXRGB(40,50,60),FXRGB(70,80,90)};

// Write a little-endian 16 bit value
void put16(FXuchar* p,FXuint v){
  p[0]=(FXuchar)(v&0xff);
  p[1]=(FXuchar)((v>>8)&0xff);
  }

// Write a little-endian 32 bit value
void put32(FXuchar* p,FXuint v){
  p[0]=(FXuchar)(v&0xff);
  p[1]=(FXuchar)((v>>8)&0xff);
  p[2]=(FXuchar)((v>>16)&0xff);
  p[3]=(FXuchar)((v>>24)&0xff);
  }

// A 24 bit bottom-up BMP with 4 byte aligned rows
FXint makeBMP(FXuchar* bmp){
  const FXint rowbytes=((W*3+3)/4)*4;
  const FXint datalen=rowbytes*H;
  const FXint filelen=14+40+datalen;
  put16(bmp+0,0x4D42);                       // "BM"
  put32(bmp+2,filelen);
  put16(bmp+6,0);
  put16(bmp+8,0);
  put32(bmp+10,14+40);                       // Offset to pixel data
  put32(bmp+14,40);                          // Info header size
  put32(bmp+18,W);
  put32(bmp+22,H);
  put16(bmp+26,1);                           // Planes
  put16(bmp+28,24);                          // Bits per pixel
  put32(bmp+30,0);                           // Compression
  put32(bmp+34,datalen);
  put32(bmp+38,2835);                        // 72 DPI
  put32(bmp+42,2835);
  put32(bmp+46,0);
  put32(bmp+50,0);
  FXuchar* row=bmp+54;
  for(FXint y=H-1; y>=0; y--){
    const FXColor* colors=y? top : bottom;   // BMP stores bottom row first
    for(FXint x=0; x<W; x++){
      row[x*3+0]=(FXuchar)FXBLUEVAL(colors[x]);
      row[x*3+1]=(FXuchar)FXGREENVAL(colors[x]);
      row[x*3+2]=(FXuchar)FXREDVAL(colors[x]);
      }
    for(FXint i=W*3; i<rowbytes; i++) row[i]=0;
    row+=rowbytes;
    }
  return filelen;
  }

// Compare a decoded image against the expected colors
int check(const FXchar* what,const FXColor* data,FXint width,FXint height){
  int errors=0;
  if(width!=W || height!=H){
    fprintf(stderr,"%s: size %dx%d, expected %dx%d\n",what,width,height,W,H);
    return 1;
    }
  for(FXint y=0; y<H; y++){
    for(FXint x=0; x<W; x++){
      FXColor want=(y? top : bottom)[x];
      FXColor got=data[y*width+x];
      if(FXRGB(FXREDVAL(got),FXGREENVAL(got),FXBLUEVAL(got))!=want){
        fprintf(stderr,"%s: pixel %d,%d is %06x, expected %06x\n",what,x,y,FXRGB(FXREDVAL(got),FXGREENVAL(got),FXBLUEVAL(got)),want);
        errors++;
        }
      }
    }
  return errors;
  }

}

int main(int argc,char** argv){
  fxtestargs(argc,argv);

  printf("BMP image and icon test\n");

  FXuchar bmp[14+40+64];
  makeBMP(bmp);

  FXApp application("BMPTest","FoxTest");
  application.init(argc,argv);

  int errors=0;

  // Decode through FXBMPImage
  FXBMPImage image(&application,bmp);
  errors+=check("image",image.getData(),image.getWidth(),image.getHeight());

  // Decode through FXBMPIcon
  FXBMPIcon icon(&application,bmp);
  errors+=check("icon",icon.getData(),icon.getWidth(),icon.getHeight());

  if(errors){
    printf("%d errors\n",errors);
    return 1;
    }
  printf("all checks passed\n");
  return 0;
  }
