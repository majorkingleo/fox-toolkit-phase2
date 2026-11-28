/********************************************************************************
*                                                                               *
*                    T e r m i n a l   C h a r s e t                            *
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
#include "xincs.h"
#include "fxver.h"
#include "fxdefs.h"
#include "FXString.h"
#include "FXTextCodec.h"
#include "FXUTF8Codec.h"
#include "FX88591Codec.h"
#include "FX88592Codec.h"
#include "FXCP1252Codec.h"
#include "FXKOI8RCodec.h"
#include "fxterminalcharset.h"

/*
  Notes:
  - The high half of a single byte encoding is a table of 128 codepoints for
    the bytes 0x80..0xFF; 0xFFFF means the byte has no character there.
  - The tables agree with the FXTextCodec classes the library already has, so
    those encodings delegate to the codec instead of carrying a copy.
  - Bytes below 0x80 are the same in every supported encoding; the parser
    handles them as ASCII and controls before it asks the charset.
*/

using namespace FX;

/*******************************************************************************/

namespace FX {

static const FXushort iso885915_high[128]={
  0x0080, 0x0081, 0x0082, 0x0083, 0x0084, 0x0085, 0x0086, 0x0087,
  0x0088, 0x0089, 0x008A, 0x008B, 0x008C, 0x008D, 0x008E, 0x008F,
  0x0090, 0x0091, 0x0092, 0x0093, 0x0094, 0x0095, 0x0096, 0x0097,
  0x0098, 0x0099, 0x009A, 0x009B, 0x009C, 0x009D, 0x009E, 0x009F,
  0x00A0, 0x00A1, 0x00A2, 0x00A3, 0x20AC, 0x00A5, 0x0160, 0x00A7,
  0x0161, 0x00A9, 0x00AA, 0x00AB, 0x00AC, 0x00AD, 0x00AE, 0x00AF,
  0x00B0, 0x00B1, 0x00B2, 0x00B3, 0x017D, 0x00B5, 0x00B6, 0x00B7,
  0x017E, 0x00B9, 0x00BA, 0x00BB, 0x0152, 0x0153, 0x0178, 0x00BF,
  0x00C0, 0x00C1, 0x00C2, 0x00C3, 0x00C4, 0x00C5, 0x00C6, 0x00C7,
  0x00C8, 0x00C9, 0x00CA, 0x00CB, 0x00CC, 0x00CD, 0x00CE, 0x00CF,
  0x00D0, 0x00D1, 0x00D2, 0x00D3, 0x00D4, 0x00D5, 0x00D6, 0x00D7,
  0x00D8, 0x00D9, 0x00DA, 0x00DB, 0x00DC, 0x00DD, 0x00DE, 0x00DF,
  0x00E0, 0x00E1, 0x00E2, 0x00E3, 0x00E4, 0x00E5, 0x00E6, 0x00E7,
  0x00E8, 0x00E9, 0x00EA, 0x00EB, 0x00EC, 0x00ED, 0x00EE, 0x00EF,
  0x00F0, 0x00F1, 0x00F2, 0x00F3, 0x00F4, 0x00F5, 0x00F6, 0x00F7,
  0x00F8, 0x00F9, 0x00FA, 0x00FB, 0x00FC, 0x00FD, 0x00FE, 0x00FF
  };

static const FXushort windows1250_high[128]={
  0x20AC, 0xFFFF, 0x201A, 0xFFFF, 0x201E, 0x2026, 0x2020, 0x2021,
  0xFFFF, 0x2030, 0x0160, 0x2039, 0x015A, 0x0164, 0x017D, 0x0179,
  0xFFFF, 0x2018, 0x2019, 0x201C, 0x201D, 0x2022, 0x2013, 0x2014,
  0xFFFF, 0x2122, 0x0161, 0x203A, 0x015B, 0x0165, 0x017E, 0x017A,
  0x00A0, 0x02C7, 0x02D8, 0x0141, 0x00A4, 0x0104, 0x00A6, 0x00A7,
  0x00A8, 0x00A9, 0x015E, 0x00AB, 0x00AC, 0x00AD, 0x00AE, 0x017B,
  0x00B0, 0x00B1, 0x02DB, 0x0142, 0x00B4, 0x00B5, 0x00B6, 0x00B7,
  0x00B8, 0x0105, 0x015F, 0x00BB, 0x013D, 0x02DD, 0x013E, 0x017C,
  0x0154, 0x00C1, 0x00C2, 0x0102, 0x00C4, 0x0139, 0x0106, 0x00C7,
  0x010C, 0x00C9, 0x0118, 0x00CB, 0x011A, 0x00CD, 0x00CE, 0x010E,
  0x0110, 0x0143, 0x0147, 0x00D3, 0x00D4, 0x0150, 0x00D6, 0x00D7,
  0x0158, 0x016E, 0x00DA, 0x0170, 0x00DC, 0x00DD, 0x0162, 0x00DF,
  0x0155, 0x00E1, 0x00E2, 0x0103, 0x00E4, 0x013A, 0x0107, 0x00E7,
  0x010D, 0x00E9, 0x0119, 0x00EB, 0x011B, 0x00ED, 0x00EE, 0x010F,
  0x0111, 0x0144, 0x0148, 0x00F3, 0x00F4, 0x0151, 0x00F6, 0x00F7,
  0x0159, 0x016F, 0x00FA, 0x0171, 0x00FC, 0x00FD, 0x0163, 0x02D9
  };

static const FXushort windows1251_high[128]={
  0x0402, 0x0403, 0x201A, 0x0453, 0x201E, 0x2026, 0x2020, 0x2021,
  0x20AC, 0x2030, 0x0409, 0x2039, 0x040A, 0x040C, 0x040B, 0x040F,
  0x0452, 0x2018, 0x2019, 0x201C, 0x201D, 0x2022, 0x2013, 0x2014,
  0xFFFF, 0x2122, 0x0459, 0x203A, 0x045A, 0x045C, 0x045B, 0x045F,
  0x00A0, 0x040E, 0x045E, 0x0408, 0x00A4, 0x0490, 0x00A6, 0x00A7,
  0x0401, 0x00A9, 0x0404, 0x00AB, 0x00AC, 0x00AD, 0x00AE, 0x0407,
  0x00B0, 0x00B1, 0x0406, 0x0456, 0x0491, 0x00B5, 0x00B6, 0x00B7,
  0x0451, 0x2116, 0x0454, 0x00BB, 0x0458, 0x0405, 0x0455, 0x0457,
  0x0410, 0x0411, 0x0412, 0x0413, 0x0414, 0x0415, 0x0416, 0x0417,
  0x0418, 0x0419, 0x041A, 0x041B, 0x041C, 0x041D, 0x041E, 0x041F,
  0x0420, 0x0421, 0x0422, 0x0423, 0x0424, 0x0425, 0x0426, 0x0427,
  0x0428, 0x0429, 0x042A, 0x042B, 0x042C, 0x042D, 0x042E, 0x042F,
  0x0430, 0x0431, 0x0432, 0x0433, 0x0434, 0x0435, 0x0436, 0x0437,
  0x0438, 0x0439, 0x043A, 0x043B, 0x043C, 0x043D, 0x043E, 0x043F,
  0x0440, 0x0441, 0x0442, 0x0443, 0x0444, 0x0445, 0x0446, 0x0447,
  0x0448, 0x0449, 0x044A, 0x044B, 0x044C, 0x044D, 0x044E, 0x044F
  };

static const FXushort cp437_high[128]={
  0x00C7, 0x00FC, 0x00E9, 0x00E2, 0x00E4, 0x00E0, 0x00E5, 0x00E7,
  0x00EA, 0x00EB, 0x00E8, 0x00EF, 0x00EE, 0x00EC, 0x00C4, 0x00C5,
  0x00C9, 0x00E6, 0x00C6, 0x00F4, 0x00F6, 0x00F2, 0x00FB, 0x00F9,
  0x00FF, 0x00D6, 0x00DC, 0x00A2, 0x00A3, 0x00A5, 0x20A7, 0x0192,
  0x00E1, 0x00ED, 0x00F3, 0x00FA, 0x00F1, 0x00D1, 0x00AA, 0x00BA,
  0x00BF, 0x2310, 0x00AC, 0x00BD, 0x00BC, 0x00A1, 0x00AB, 0x00BB,
  0x2591, 0x2592, 0x2593, 0x2502, 0x2524, 0x2561, 0x2562, 0x2556,
  0x2555, 0x2563, 0x2551, 0x2557, 0x255D, 0x255C, 0x255B, 0x2510,
  0x2514, 0x2534, 0x252C, 0x251C, 0x2500, 0x253C, 0x255E, 0x255F,
  0x255A, 0x2554, 0x2569, 0x2566, 0x2560, 0x2550, 0x256C, 0x2567,
  0x2568, 0x2564, 0x2565, 0x2559, 0x2558, 0x2552, 0x2553, 0x256B,
  0x256A, 0x2518, 0x250C, 0x2588, 0x2584, 0x258C, 0x2590, 0x2580,
  0x03B1, 0x00DF, 0x0393, 0x03C0, 0x03A3, 0x03C3, 0x00B5, 0x03C4,
  0x03A6, 0x0398, 0x03A9, 0x03B4, 0x221E, 0x03C6, 0x03B5, 0x2229,
  0x2261, 0x00B1, 0x2265, 0x2264, 0x2320, 0x2321, 0x00F7, 0x2248,
  0x00B0, 0x2219, 0x00B7, 0x221A, 0x207F, 0x00B2, 0x25A0, 0x00A0
  };

static const FXushort cp850_high[128]={
  0x00C7, 0x00FC, 0x00E9, 0x00E2, 0x00E4, 0x00E0, 0x00E5, 0x00E7,
  0x00EA, 0x00EB, 0x00E8, 0x00EF, 0x00EE, 0x00EC, 0x00C4, 0x00C5,
  0x00C9, 0x00E6, 0x00C6, 0x00F4, 0x00F6, 0x00F2, 0x00FB, 0x00F9,
  0x00FF, 0x00D6, 0x00DC, 0x00F8, 0x00A3, 0x00D8, 0x00D7, 0x0192,
  0x00E1, 0x00ED, 0x00F3, 0x00FA, 0x00F1, 0x00D1, 0x00AA, 0x00BA,
  0x00BF, 0x00AE, 0x00AC, 0x00BD, 0x00BC, 0x00A1, 0x00AB, 0x00BB,
  0x2591, 0x2592, 0x2593, 0x2502, 0x2524, 0x00C1, 0x00C2, 0x00C0,
  0x00A9, 0x2563, 0x2551, 0x2557, 0x255D, 0x00A2, 0x00A5, 0x2510,
  0x2514, 0x2534, 0x252C, 0x251C, 0x2500, 0x253C, 0x00E3, 0x00C3,
  0x255A, 0x2554, 0x2569, 0x2566, 0x2560, 0x2550, 0x256C, 0x00A4,
  0x00F0, 0x00D0, 0x00CA, 0x00CB, 0x00C8, 0x0131, 0x00CD, 0x00CE,
  0x00CF, 0x2518, 0x250C, 0x2588, 0x2584, 0x00A6, 0x00CC, 0x2580,
  0x00D3, 0x00DF, 0x00D4, 0x00D2, 0x00F5, 0x00D5, 0x00B5, 0x00FE,
  0x00DE, 0x00DA, 0x00DB, 0x00D9, 0x00FD, 0x00DD, 0x00AF, 0x00B4,
  0x00AD, 0x00B1, 0x2017, 0x00BE, 0x00B6, 0x00A7, 0x00F7, 0x00B8,
  0x00B0, 0x00A8, 0x00B7, 0x00B9, 0x00B3, 0x00B2, 0x25A0, 0x00A0
  };


// One supported encoding
struct FXTerminalEncoding {
  const FXchar    *name;        // Canonical name
  const FXchar    *aliases;     // Normalized aliases, separated by '|'
  FXTextCodec   *(*codec)();    // Codec factory, or NULL for a table
  const FXushort  *table;       // High half table, or NULL
  };


// Codec instances; the charsets are stateless, so one of each is enough
static FXTextCodec* latin1Codec(){ static FX88591Codec instance; return &instance; }
static FXTextCodec* latin2Codec(){ static FX88592Codec instance; return &instance; }
static FXTextCodec* cp1252Codec(){ static FXCP1252Codec instance; return &instance; }
static FXTextCodec* koi8rCodec(){ static FXKOI8RCodec instance; return &instance; }
static FXTextCodec* utf8Codec(){ static FXUTF8Codec instance; return &instance; }


// Supported encodings in menu order; UTF-8 first
static const FXTerminalEncoding encodings[]={
  { "UTF-8",        "utf8|65001",                        nullptr,     nullptr          },
  { "ISO-8859-1",   "latin1|iso88591|88591|cp819|28591", latin1Codec, nullptr          },
  { "ISO-8859-2",   "latin2|iso88592|88592|28592",       latin2Codec, nullptr          },
  { "ISO-8859-15",  "latin9|iso885915|885915|28605",     nullptr,     iso885915_high   },
  { "Windows-1250", "cp1250|1250",                       nullptr,     windows1250_high },
  { "Windows-1251", "cp1251|1251",                       nullptr,     windows1251_high },
  { "Windows-1252", "cp1252|1252",                       cp1252Codec, nullptr          },
  { "KOI8-R",       "koi8r|20866",                       koi8rCodec,  nullptr          },
  { "CP437",        "ibm437|437|dos437",                 nullptr,     cp437_high       },
  { "CP850",        "ibm850|850|dos850",                 nullptr,     cp850_high       }
  };

static const FXint total=ARRAYNUMBER(encodings);


// Normalize a name: letters lowercased, separators and spaces dropped, so
// "ISO-8859-15", "iso8859_15" and "Latin-9" all compare equal to their alias
static FXString normalizeName(const FXString& name){
  FXString result;
  FXchar ch;
  for(FXint i=0; i<name.length(); i++){
    ch=name[i];
    if('A'<=ch && ch<='Z') ch=ch-'A'+'a';
    if(('a'<=ch && ch<='z') || ('0'<=ch && ch<='9')) result+=ch;
    }
  return result;
  }


// Constructor
FXTerminalCharset::FXTerminalCharset():current(0){
  }


// Number of supported encodings
FXint FXTerminalCharset::count(){
  return total;
  }


// Canonical name of a supported encoding
const FXchar* FXTerminalCharset::name(FXint index){
  return (0<=index && index<total) ? encodings[index].name : encodings[0].name;
  }


// Find a name or alias
FXint FXTerminalCharset::find(const FXString& name){
  FXString wanted=normalizeName(name);
  const FXchar *alias;
  FXint length;
  if(wanted.empty()) return -1;
  for(FXint i=0; i<total; i++){
    if(wanted==normalizeName(encodings[i].name)) return i;
    alias=encodings[i].aliases;
    while(alias && *alias){
      length=0;
      while(alias[length] && alias[length]!='|') length++;
      if(wanted.length()==length && strncmp(wanted.text(),alias,length)==0) return i;
      alias+=length;
      if(*alias=='|') alias++;
      }
    }
  return -1;
  }


// Select an encoding by name or alias
FXString FXTerminalCharset::setName(const FXString& name){
  FXint index=find(name);
  current=(index<0) ? 0 : index;
  return getName();
  }


// Name of the active encoding
const FXchar* FXTerminalCharset::getName() const {
  return encodings[current].name;
  }


// Index of the active encoding
FXint FXTerminalCharset::getIndex() const {
  return current;
  }


// True when the active encoding is UTF-8
FXbool FXTerminalCharset::isUTF8() const {
  return current==0;
  }


// Codec of an encoding, or NULL
FXTextCodec* FXTerminalCharset::codec(FXint index){
  return (0<=index && index<total && encodings[index].codec) ? encodings[index].codec() : nullptr;
  }


// High half table of an encoding, or NULL
const FXushort* FXTerminalCharset::table(FXint index){
  return (0<=index && index<total) ? encodings[index].table : nullptr;
  }


// Decode one high byte of a single byte encoding
FXwchar FXTerminalCharset::decode(FXuchar byte) const {
  FXTextCodec *cdc=codec(current);
  const FXushort *tbl=table(current);
  FXchar ch=(FXchar)byte;
  FXwchar wc=0;
  if(cdc){
    if(cdc->mb2wc(wc,&ch,1)!=1) return 0xFFFD;
    return wc;
    }
  if(tbl){
    wc=tbl[byte&0x7F];
    return (wc==0xFFFF) ? (FXwchar)0xFFFD : wc;
    }
  return byte;
  }


// Encode one codepoint
FXbool FXTerminalCharset::encode(FXwchar ch,FXchar& out) const {
  FXTextCodec *cdc=codec(current);
  const FXushort *tbl=table(current);
  FXchar byte;
  FXwchar back;
  if(ch<0x80){
    out=(FXchar)ch;
    return true;
    }
  if(cdc){
    if(cdc->wc2mb(&byte,1,ch)!=1) return false;
    if(cdc->mb2wc(back,&byte,1)!=1 || back!=ch) return false;
    out=byte;
    return true;
    }
  if(tbl){
    for(FXint i=0; i<128; i++){
      if(tbl[i]==ch){
        out=(FXchar)(0x80+i);
        return true;
        }
      }
    }
  return false;
  }


// Encode UTF-8 text; characters the encoding lacks become '?'
FXString FXTerminalCharset::encode(const FXString& utf8,FXbool& lossy) const {
  FXString result;
  FXTextCodec *utf8decoder=utf8Codec();
  const FXchar *src;
  FXint length;
  FXint pos=0;
  FXint count;
  FXwchar wc;
  FXchar byte;
  lossy=false;
  if(isUTF8()) return utf8;
  src=utf8.text();
  length=utf8.length();
  while(pos<length){
    count=utf8decoder->mb2wc(wc,src+pos,length-pos);
    if(count<=0){
      result+='?';
      lossy=true;
      pos++;
      continue;
      }
    if(encode(wc,byte)) result+=byte;
    else{
      result+='?';
      lossy=true;
      }
    pos+=count;
    }
  return result;
  }

/*******************************************************************************/

}
