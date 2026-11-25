#include "fx.h"
#include "FXChartDefs.h"
#include "CSVReader.h"
#include "FXDate.h"
#include "FXUTF8Codec.h"
#include "FXUTF16Codec.h"
#include "FXUTF32Codec.h"
#include "FXCP1252Codec.h"
#include "FXXMLString.h"
#include <string>
#include <vector>
#include <cstdio>
#include <cstdlib>
#include <cctype>
#include <climits>
#include <limits>
#if defined(HAVE_ZLIB_H)
#include <zlib.h>
#endif

using namespace FX;

namespace FXCHART {

namespace {

template<typename Callback>
static void parseRecords(const std::string& text,FXchar delimiter,Callback callback,FXint limit=-1,CSVProgressCallback progress=nullptr,void* context=nullptr,FXuint progressStart=0,FXuint progressRange=0){
  std::vector<FXString> row;
  std::string field;
  FXbool quoted=false;
  FXbool atstart=true;
  FXint records=0;
  for(std::string::size_type i=0; i<text.size(); i++){
    if(progress && (i&0xffff)==0 && !text.empty()) progress(context,progressStart+(FXuint)((FXulong)i*progressRange/text.size()));
    FXchar c=text[i];
    if(quoted){
      if(c=='"'){
        if(i+1<text.size() && text[i+1]=='"'){
          field.push_back('"');
          i++;
          }
        else quoted=false;
        }
      else field.push_back(c);
      }
    else if(c=='"' && atstart){
      quoted=true;
      atstart=false;
      }
    else if(c==delimiter){
      row.push_back(FXString(field.c_str(),field.size()));
      field.clear();
      atstart=true;
      }
    else if(c=='\r' || c=='\n'){
      if(c=='\r' && i+1<text.size() && text[i+1]=='\n') i++;
      row.push_back(FXString(field.c_str(),field.size()));
      field.clear();
      atstart=true;
      if(!(row.size()==1 && row[0].empty())){
        callback(row);
        records++;
        }
      row.clear();
      if(limit>=0 && records>=limit) return;
      }
    else{
      field.push_back(c);
      atstart=false;
      }
    }
  if(!field.empty() || !row.empty()){
    row.push_back(FXString(field.c_str(),field.size()));
    callback(row);
    }
  if(progress) progress(context,progressStart+progressRange);
  }


static FXchar detectDelimiter(const std::string& text){
  const FXchar candidates[]={',',';','\t','|'};
  FXchar selected=',';
  FXint best=-1;
  for(FXint c=0; c<4; c++){
    std::vector<FXint> counts(4096,0);
    FXint rows=0;
    parseRecords(text,candidates[c],[&](const std::vector<FXString>& row){
      if(row.size()>1 && row.size()<counts.size()){ counts[row.size()]++; rows++; }
      },40);
    FXint score=0;
    for(FXuval n=2; n<counts.size(); n++) score=FXMAX(score,counts[n]);
    if(score>best || (score==best && rows>0 && candidates[c]==';')){
      best=score;
      selected=candidates[c];
      }
    }
  return selected;
  }


static FXbool validUTF8(const std::string& text){
  FXUTF8Codec codec;
  FXint pos=0;
  while(pos<(FXint)text.size()){
    FXwchar wc;
    FXint n=codec.mb2wc(wc,text.data()+pos,(FXint)text.size()-pos);
    if(n<=0) return false;
    pos+=n;
    }
  return true;
  }


static FXString decodeCSV(const std::string& bytes,FXString& encoding){
  if(bytes.size()>=4 && (FXuchar)bytes[0]==0x00 && (FXuchar)bytes[1]==0x00 && (FXuchar)bytes[2]==0xFE && (FXuchar)bytes[3]==0xFF){
    FXUTF32BECodec codec;
    encoding="UTF-32BE BOM";
    return codec.mb2utf(bytes.data()+4,(FXint)bytes.size()-4);
    }
  if(bytes.size()>=4 && (FXuchar)bytes[0]==0xFF && (FXuchar)bytes[1]==0xFE && (FXuchar)bytes[2]==0x00 && (FXuchar)bytes[3]==0x00){
    FXUTF32LECodec codec;
    encoding="UTF-32LE BOM";
    return codec.mb2utf(bytes.data()+4,(FXint)bytes.size()-4);
    }
  if(bytes.size()>=3 && (FXuchar)bytes[0]==0xEF && (FXuchar)bytes[1]==0xBB && (FXuchar)bytes[2]==0xBF){
    encoding="UTF-8 BOM";
    return FXString(bytes.data()+3,(FXint)bytes.size()-3);
    }
  if(bytes.size()>=2 && (FXuchar)bytes[0]==0xFE && (FXuchar)bytes[1]==0xFF){
    FXUTF16BECodec codec;
    encoding="UTF-16BE BOM";
    return codec.mb2utf(bytes.data()+2,(FXint)bytes.size()-2);
    }
  if(bytes.size()>=2 && (FXuchar)bytes[0]==0xFF && (FXuchar)bytes[1]==0xFE){
    FXUTF16LECodec codec;
    encoding="UTF-16LE BOM";
    return codec.mb2utf(bytes.data()+2,(FXint)bytes.size()-2);
    }
  if(bytes.size()>=32){
    FXint zero[4]={0,0,0,0};
    FXint total[4]={0,0,0,0};
    for(FXuval i=0; i<FXMIN((FXuval)4096,bytes.size()); i++){
      FXint lane=i%4;
      total[lane]++;
      if(bytes[i]==0) zero[lane]++;
      }
    if(zero[1]>total[1]*3/4 && zero[2]>total[2]*3/4 && zero[3]>total[3]*3/4 && zero[0]<total[0]/8){
      FXUTF32LECodec codec;
      encoding="UTF-32LE (detected)";
      return codec.mb2utf(bytes.data(),(FXint)bytes.size());
      }
    if(zero[0]>total[0]*3/4 && zero[1]>total[1]*3/4 && zero[2]>total[2]*3/4 && zero[3]<total[3]/8){
      FXUTF32BECodec codec;
      encoding="UTF-32BE (detected)";
      return codec.mb2utf(bytes.data(),(FXint)bytes.size());
      }
    if(zero[1]>total[1]/2 && zero[0]<total[0]/8){
      FXUTF16LECodec codec;
      encoding="UTF-16LE (detected)";
      return codec.mb2utf(bytes.data(),(FXint)bytes.size());
      }
    if(zero[0]>total[0]/2 && zero[1]<total[1]/8){
      FXUTF16BECodec codec;
      encoding="UTF-16BE (detected)";
      return codec.mb2utf(bytes.data(),(FXint)bytes.size());
      }
    }
  if(validUTF8(bytes)){
    encoding="UTF-8 (validated)";
    return FXString(bytes.data(),bytes.size());
    }
  FXCP1252Codec codec;
  encoding="Windows-1252 fallback";
  return codec.mb2utf(bytes.data(),(FXint)bytes.size());
  }


static FXString trimField(const FXString& input){
  FXint first=0,last=input.length();
  while(first<last && (input[first]==' ' || input[first]=='\t')) first++;
  while(last>first && (input[last-1]==' ' || input[last-1]=='\t')) last--;
  return input.mid(first,last-first);
  }


static FXbool parseNumber(const FXString& input,FXdouble& value,FXbool decimalComma){
  FXString s=trimField(input);
  if(s.empty()) return false;
  FXString lower=s.lower();
  if(lower=="nan" || lower=="+nan" || lower=="-nan"){
    value=std::numeric_limits<FXdouble>::quiet_NaN();
    return true;
    }
  if(lower=="null" || lower=="na" || lower=="n/a" || lower=="missing" || lower=="-") return false;
  if(decimalComma){ s.substitute(".",""); s.substitute(',', '.'); }
  else s.substitute(",","");
  FXbool ok=false;
  value=s.toDouble(&ok);
  return ok;
  }


static FXbool parseDateTime(const FXString& input,FXdouble& value){
  FXString trimmed=trimField(input);
  if(trimmed.length()<8) return false;
  std::string s=trimmed.text();
  FXint first=0,second=0,third=0,pos=0;
  FXchar separator=0;
  FXint n=std::sscanf(s.c_str(),"%d-%d-%d%n",&first,&second,&third,&pos);
  if(n!=3){
    pos=0;
    n=std::sscanf(s.c_str(),"%d/%d/%d%n",&first,&second,&third,&pos);
    if(n!=3){
      pos=0;
      n=std::sscanf(s.c_str(),"%d.%d.%d%n",&first,&second,&third,&pos);
      separator='.';
      }
    else separator='/';
    }
  else separator='-';
  if(n!=3) return false;

  FXint year,month,day;
  if(first>=1000){ year=first; month=second; day=third; }
  else if(third>=1000){
    year=third;
    if(separator=='.' || first>12){ day=first; month=second; }
    else{ month=first; day=second; }
    }
  else return false;

  FXint hour=0,minute=0,secondOfMinute=0;
  FXdouble fraction=0.0;
  FXint end=pos;
  while(s[end]==' ' || s[end]=='T' || s[end]=='t') end++;
  if(s[end]){
    FXint timeend=end;
    if(std::sscanf(s.c_str()+end,"%d:%d:%d%n",&hour,&minute,&secondOfMinute,&timeend)!=3) return false;
    end+=timeend;
    if(s[end]=='.' || s[end]==','){
      FXdouble place=0.1;
      end++;
      while(std::isdigit((FXuchar)s[end])){
        fraction+=(s[end]-'0')*place;
        place*=0.1;
        end++;
        }
      }
    }
  if(year<1 || year>9999 || month<1 || month>12 || day<1 || day>31 || hour>23 || minute>59 || secondOfMinute>60) return false;
  FXDate date(year,month,day);
  if(date.year()!=year || date.month()!=month || date.day()!=day) return false;
  FXdouble timezone=0.0;
  if(s[end]=='Z' || s[end]=='z') end++;
  else if(s[end]=='+' || s[end]=='-'){
    FXint sign=s[end]=='+'?1:-1;
    FXint th=0,tm=0;
    if(std::sscanf(s.c_str()+end+1,"%2d:%2d",&th,&tm)>=1) timezone=sign*(th*3600+tm*60);
    }
  value=(FXdouble)date.getTime()/1.0e9+hour*3600.0+minute*60.0+secondOfMinute+fraction-timezone;
  return true;
  }


static FXbool parseDateOnly(const FXString& input,FXdouble& value){
  FXString trimmed=trimField(input);
  if(trimmed.find(':')>=0) return false;
  return parseDateTime(trimmed,value);
  }


static FXbool parseTimeOnly(const FXString& input,FXdouble& value){
  FXString trimmed=trimField(input);
  if(trimmed.length()<4) return false;
  std::string s=trimmed.text();
  FXint hour=0,minute=0,secondOfMinute=0,pos=0;
  if(std::sscanf(s.c_str(),"%d:%d:%d%n",&hour,&minute,&secondOfMinute,&pos)!=3){
    pos=0;
    if(std::sscanf(s.c_str(),"%d:%d%n",&hour,&minute,&pos)!=2) return false;
    secondOfMinute=0;
    }
  FXdouble fraction=0.0;
  if(s[pos]=='.' || s[pos]==','){
    FXdouble place=0.1;
    pos++;
    while(std::isdigit((FXuchar)s[pos])){
      fraction+=(s[pos]-'0')*place;
      place*=0.1;
      pos++;
      }
    }
  while(s[pos]==' ' || s[pos]=='\t') pos++;
  if(s[pos]=='Z' || s[pos]=='z') pos++;
  while(s[pos]==' ' || s[pos]=='\t') pos++;
  if(s[pos]) return false;
  if(hour>23 || minute>59 || secondOfMinute>60) return false;
  value=hour*3600.0+minute*60.0+secondOfMinute+fraction;
  return true;
  }


static FXbool isMissing(const FXString& value){
  FXString s=trimField(value).lower();
  return s.empty() || s=="null" || s=="na" || s=="n/a" || s=="missing" || s=="-";
  }


// The local part of an XML name, without its namespace prefix
static FXString xmlLocalName(const FXString& name){
  FXint colon=name.rfind(':');
  return colon>=0?name.mid(colon+1,name.length()-colon-1):name;
  }


static FXString xmlAttribute(const FXStringDictionary& atts,const FXchar* key){
  return atts.has(key)?atts[key]:FXString();
  }


static FXint xmlRepeat(const FXStringDictionary& atts,const FXchar* key){
  if(!atts.has(key)) return 1;
  std::string value=atts[key].text();
  long count=std::strtol(value.c_str(),nullptr,10);
  if(count<1) return 1;
  if(count>INT_MAX) return INT_MAX;
  return (FXint)count;
  }


// ODF stores a time cell as an ISO duration such as PT16H36M22S
static FXString odsTimeToText(const FXString& value){
  FXString upper=value;
  upper.upper();
  if(upper.length()<3 || upper[0]!='P') return value;
  FXint start=upper.find('T');
  if(start<0) return value;
  std::string text=upper.text();
  FXint hour=0,minute=0;
  FXdouble second=0.0;
  FXint pos=start+1;
  while(pos<(FXint)text.size()){
    char* end=nullptr;
    FXdouble number=std::strtod(text.c_str()+pos,&end);
    if(end==text.c_str()+pos) break;
    pos=(FXint)(end-text.c_str());
    if(pos>=(FXint)text.size()) break;
    FXchar unit=text[pos++];
    if(unit=='H') hour=(FXint)number;
    else if(unit=='M') minute=(FXint)number;
    else if(unit=='S') second=number;
    }
  if(hour>23 || minute>59 || second>=61.0) return value;
  return FXString::value("%d:%02d:%06.3f",hour,minute,second);
  }


static FXuint zip16(const std::string& bytes,std::string::size_type pos){
  return (FXuint)(FXuchar)bytes[pos] | ((FXuint)(FXuchar)bytes[pos+1]<<8);
  }


static FXuint zip32(const std::string& bytes,std::string::size_type pos){
  return (FXuint)(FXuchar)bytes[pos]
       | ((FXuint)(FXuchar)bytes[pos+1]<<8)
       | ((FXuint)(FXuchar)bytes[pos+2]<<16)
       | ((FXuint)(FXuchar)bytes[pos+3]<<24);
  }


// Pull one entry out of a ZIP archive; OpenDocument is a ZIP container
static FXbool extractZipEntry(const std::string& bytes,const FXString& wanted,std::string& result,FXString& error){
  const FXuval maxsize=(FXuval)256*1024*1024;
  std::string::size_type eocd=std::string::npos;
  std::string::size_type scan=bytes.size()>65557?bytes.size()-65557:0;
  if(bytes.size()<22){ error="OpenDocument file is too short"; return false; }
  std::string::size_type last=bytes.size()-22;
  for(std::string::size_type pos=last; ; pos--){
    if(zip32(bytes,pos)==0x06054b50){ eocd=pos; break; }
    if(pos==scan) break;
    }
  if(eocd==std::string::npos){ error="OpenDocument file has no ZIP directory"; return false; }
  FXuint entries=zip16(bytes,eocd+10);
  FXuint dirsize=zip32(bytes,eocd+12);
  FXuint diroffset=zip32(bytes,eocd+16);
  if(entries==0 || dirsize==0 || diroffset==0xffffffffU || dirsize==0xffffffffU){ error="OpenDocument ZIP directory is unsupported or empty"; return false; }
  if((FXuval)diroffset+(FXuval)dirsize>bytes.size()){ error="OpenDocument ZIP directory is outside the file"; return false; }
  std::string::size_type pos=diroffset;
  std::string::size_type end=diroffset+dirsize;
  for(FXuint entry=0; entry<entries && pos+46<=end; entry++){
    if(zip32(bytes,pos)!=0x02014b50){ error="OpenDocument ZIP directory is corrupt"; return false; }
    FXuint flags=zip16(bytes,pos+8);
    FXuint method=zip16(bytes,pos+10);
    FXuint compressed=zip32(bytes,pos+20);
    FXuint uncompressed=zip32(bytes,pos+24);
    FXuint namelen=zip16(bytes,pos+28);
    FXuint extralen=zip16(bytes,pos+30);
    FXuint commentlen=zip16(bytes,pos+32);
    FXuint localoffset=zip32(bytes,pos+42);
    if(pos+46+namelen+extralen+commentlen>end){ error="OpenDocument ZIP directory is truncated"; return false; }
    FXString name(bytes.data()+pos+46,namelen);
    if(name==wanted){
      if(flags&1){ error="OpenDocument content is encrypted"; return false; }
      if(compressed==0xffffffffU || uncompressed==0xffffffffU || localoffset==0xffffffffU){ error="ZIP64 OpenDocument files are not supported"; return false; }
      if(uncompressed>maxsize){ error="OpenDocument content is too large"; return false; }
      if((FXuval)localoffset+30>bytes.size() || zip32(bytes,localoffset)!=0x04034b50){ error="OpenDocument ZIP entry is corrupt"; return false; }
      FXuint localname=zip16(bytes,localoffset+26);
      FXuint localextra=zip16(bytes,localoffset+28);
      std::string::size_type dataoffset=(std::string::size_type)localoffset+30+localname+localextra;
      if(dataoffset>bytes.size() || (FXuval)compressed>bytes.size()-dataoffset){ error="OpenDocument ZIP entry is truncated"; return false; }
      if(method==0){
        result.assign(bytes.data()+dataoffset,compressed);
        return true;
        }
      if(method==8){
#if defined(HAVE_ZLIB_H)
        result.resize(uncompressed);
        z_stream stream={};
        stream.next_in=(Bytef*)(bytes.data()+dataoffset);
        stream.avail_in=compressed;
        stream.next_out=(Bytef*)(uncompressed?&result[0]:nullptr);
        stream.avail_out=uncompressed;
        if(inflateInit2(&stream,-MAX_WBITS)!=Z_OK){ error="Could not start decompression of OpenDocument content"; return false; }
        FXint status=inflate(&stream,Z_FINISH);
        FXuval produced=stream.total_out;
        inflateEnd(&stream);
        if(status!=Z_STREAM_END || produced!=uncompressed){ error="Could not decompress OpenDocument content"; return false; }
        return true;
#else
        error="OpenDocument import needs zlib support in this build";
        return false;
#endif
        }
      error="OpenDocument content uses unsupported compression";
      return false;
      }
    pos+=46+namelen+extralen+commentlen;
    }
  error="OpenDocument file has no content.xml";
  return false;
  }


// Read the first non-empty sheet of an OpenDocument spreadsheet
class ODSContentParser {
public:
  std::vector<std::vector<FXString> > rows;
private:
  std::vector<FXString> row;
  FXString cellText;
  FXString cellType;
  FXString cellValue;
  FXString cellDate;
  FXString cellTime;
  FXint cellRepeat;
  FXint rowRepeat;
  FXint paragraphDepth;
  FXbool inTable;
  FXbool inRow;
  FXbool inCell;
  FXbool done;
private:
  void finishCell(){
    FXString value=cellText;
    FXString type=cellType.lower();
    if((type=="float" || type=="percentage" || type=="currency") && !cellValue.empty()) value=cellValue;
    else if(type=="date" && !cellDate.empty()) value=cellDate;
    else if(type=="time" && !cellTime.empty()) value=odsTimeToText(cellTime);
    FXint copies=cellRepeat;
    while(copies-->0 && row.size()<(FXuval)(1<<12)) row.push_back(value);
    cellText.clear();
    cellType.clear();
    cellValue.clear();
    cellDate.clear();
    cellTime.clear();
    paragraphDepth=0;
    inCell=false;
    }
  void finishRow(){
    while(!row.empty() && trimField(row.back()).empty()) row.pop_back();
    if(!row.empty()){
      FXint copies=rowRepeat;
      while(copies-->0 && rows.size()<(FXuval)1000000) rows.push_back(row);
      }
    row.clear();
    inRow=false;
    inCell=false;
    }
public:
  ODSContentParser():cellRepeat(1),rowRepeat(1),paragraphDepth(0),inTable(false),inRow(false),inCell(false),done(false){ }

  FXXML::Error startElement(const FXString& tag,const FXStringDictionary& atts){
    FXString name=xmlLocalName(tag);
    if(name=="table"){
      if(!done){ inTable=true; row.clear(); inRow=false; inCell=false; }
      return FXXML::ErrOK;
      }
    if(!inTable || done) return FXXML::ErrOK;
    if(name=="table-row"){
      if(inRow) finishRow();
      row.clear();
      rowRepeat=xmlRepeat(atts,"table:number-rows-repeated");
      inRow=true;
      inCell=false;
      return FXXML::ErrOK;
      }
    if(inRow && (name=="table-cell" || name=="covered-table-cell")){
      if(inCell) finishCell();
      cellText.clear();
      cellType=xmlAttribute(atts,"office:value-type");
      if(cellType.empty()) cellType=xmlAttribute(atts,"calcext:value-type");
      cellValue=xmlAttribute(atts,"office:value");
      cellDate=xmlAttribute(atts,"office:date-value");
      cellTime=xmlAttribute(atts,"office:time-value");
      cellRepeat=xmlRepeat(atts,"table:number-columns-repeated");
      paragraphDepth=0;
      inCell=true;
      return FXXML::ErrOK;
      }
    if(inCell){
      if(name=="p"){
        if(!cellText.empty()) cellText+='\n';
        paragraphDepth++;
        }
      else if(name=="s"){
        FXint count=xmlRepeat(atts,"text:c");
        while(count-->0) cellText+=' ';
        }
      else if(name=="tab") cellText+='\t';
      else if(name=="line-break") cellText+='\n';
      }
    return FXXML::ErrOK;
    }

  FXXML::Error characters(const FXString& text){
    if(inTable && inCell && !done && paragraphDepth>0) cellText+=text;
    return FXXML::ErrOK;
    }

  FXXML::Error endElement(const FXString& tag){
    FXString name=xmlLocalName(tag);
    if(!inTable || done) return FXXML::ErrOK;
    if(inCell && (name=="table-cell" || name=="covered-table-cell")){
      finishCell();
      return FXXML::ErrOK;
      }
    if(inRow && name=="table-row"){
      finishRow();
      return FXXML::ErrOK;
      }
    if(name=="table"){
      if(inRow) finishRow();
      inTable=false;
      if(!rows.empty()) done=true;
      }
    return FXXML::ErrOK;
    }
  };


static FXbool parseODSContent(const FXString& xml,std::vector<std::vector<FXString> >& rows,FXString& error){
  ODSContentParser parser;
  FXXMLString reader(xml,FXXML::Load);
  reader.startElementCB.connect<ODSContentParser,&ODSContentParser::startElement>(&parser);
  reader.charactersCB.connect<ODSContentParser,&ODSContentParser::characters>(&parser);
  reader.endElementCB.connect<ODSContentParser,&ODSContentParser::endElement>(&parser);
  FXXML::Error result=reader.parse();
  if(result!=FXXML::ErrOK){
    error=FXString("Could not parse OpenDocument content: ")+FXXML::getError(result);
    return false;
    }
  if(parser.rows.empty()){
    error="OpenDocument spreadsheet has no data";
    return false;
    }
  rows.swap(parser.rows);
  return true;
  }


static std::string odsRowsToCSV(const std::vector<std::vector<FXString> >& rows){
  FXuval columns=0;
  std::string out;
  for(FXuval r=0; r<rows.size(); r++) if(columns<rows[r].size()) columns=rows[r].size();
  for(FXuval r=0; r<rows.size(); r++){
    for(FXuval c=0; c<columns; c++){
      if(c) out+=',';
      std::string field=(c<rows[r].size())?std::string(rows[r][c].text()):std::string();
      if(field.find_first_of(",\"\r\n")!=std::string::npos){
        out+='"';
        for(FXuval i=0; i<field.size(); i++){
          if(field[i]=='"') out+='"';
          out+=field[i];
          }
        out+='"';
        }
      else out+=field;
      }
    out+='\n';
    }
  return out;
  }


static FXbool readODS(const std::string& bytes,std::string& csv,FXString& encoding,FXString& error){
  std::string content;
  if(!extractZipEntry(bytes,"content.xml",content,error)) return false;
  FXString xml(content.data(),(FXint)content.size());
  std::vector<std::vector<FXString> > rows;
  if(!parseODSContent(xml,rows,error)) return false;
  csv=odsRowsToCSV(rows);
  if(csv.empty()){ error="OpenDocument spreadsheet has no rows"; return false; }
  encoding="ODS (content.xml)";
  return true;
  }

}


FXbool readCSV(const FXString& filename,CSVTable& table,FXString& error,CSVProgressCallback progress,void* context){
  FXFile file(filename,FXIO::Reading);
  if(!file.isOpen()){
    error=FXString("Cannot open file: ")+filename;
    return false;
    }
  FXlong length=file.size();
  if(length<=0 || length>INT_MAX){
    error="CSV file is empty or too large to import";
    return false;
    }
  std::string bytes((FXuval)length,'\0');
    FXuval offset=0;
    while(offset<bytes.size()){
      FXuval amount=FXMIN(bytes.size()-offset,(FXuval)(1<<20));
      if(file.readBlock(&bytes[offset],amount)!=(FXival)amount){
        error="Could not read the entire CSV file";
        return false;
        }
      offset+=amount;
      if(progress) progress(context,(FXuint)(20*offset/bytes.size()));
    }
  std::string::size_type first=0;
  while(first<bytes.size() && bytes[first]=='\0') first++;
  if(first==bytes.size()){
    error="CSV file contains no text";
    return false;
    }

  table=CSVTable();
  std::string utf8;
  if(bytes.size()>=4 && (FXuchar)bytes[0]=='P' && (FXuchar)bytes[1]=='K' && (FXuchar)bytes[2]==3 && (FXuchar)bytes[3]==4){
    if(!readODS(bytes,utf8,table.encoding,error)) return false;
    }
  else{
    FXString text=decodeCSV(bytes,table.encoding);
    utf8.assign(text.text(),text.length());
    }
  if(progress) progress(context,25);
  FXchar separator=detectDelimiter(utf8);
  table.delimiter=(separator=='\t')?"tab":FXString(&separator,1);

  std::vector<std::vector<FXString> > samples;
  parseRecords(utf8,separator,[&](const std::vector<FXString>& row){
    if(samples.size()<256) samples.push_back(row);
    },256);
  if(samples.empty() || samples[0].empty()){
    error="No CSV records found";
    return false;
    }

  FXint columns=samples[0].size();
  while(columns>0 && samples[0][columns-1].empty()) columns--;
  if(columns<=0){ error="CSV header is empty"; return false; }
  FXint commaDecimals=0,commaGroups=0;
  for(FXuval r=0; r<samples.size(); r++){
    for(FXuval c=0; c<samples[r].size(); c++){
      FXString value=trimField(samples[r][c]);
      FXint comma=value.rfind(',');
      if(comma<0 || value.find('.')>=0) continue;
      FXint digits=value.length()-comma-1;
      if(digits>0 && digits!=3) commaDecimals++;
      else if(digits==3) commaGroups++;
      }
    }
  FXbool decimalComma=separator!=',' && commaDecimals>0 && commaDecimals>commaGroups;
  FXbool hasheader=false;
  for(FXint c=0; c<columns; c++){
    if(isMissing(samples[0][c])) continue;
    FXdouble number;
    if(!parseNumber(samples[0][c],number,decimalComma)){
      FXdouble stamp;
      if(!parseDateTime(samples[0][c],stamp)) hasheader=true;
      }
    }
  FXint firstdata=hasheader?1:0;
  std::vector<FXbool> numeric(columns,false),timestamps(columns,false);
  std::vector<FXbool> dates(columns,false),times(columns,false);
  for(FXint c=0; c<columns; c++){
    FXint tested=0,numericok=0,dateok=0,dateonlyok=0,timeonlyok=0;
    for(FXuval r=firstdata; r<samples.size(); r++){
      if(c>=samples[r].size() || isMissing(samples[r][c])) continue;
      FXdouble value;
      tested++;
      if(parseDateTime(samples[r][c],value)) dateok++;
      if(parseDateOnly(samples[r][c],value)) dateonlyok++;
      if(parseTimeOnly(samples[r][c],value)) timeonlyok++;
      if(parseNumber(samples[r][c],value,decimalComma)) numericok++;
      }
    if(tested>=2 && dateonlyok*5>=tested*4) dates[c]=true;
    else if(tested>=2 && timeonlyok*5>=tested*4) times[c]=true;
    if(tested>=2 && dateok*5>=tested*4) timestamps[c]=true;
    else if(tested>=2 && !dates[c] && !times[c] && numericok*5>=tested*4) numeric[c]=true;
    }

  // A date in one column and a time in the next are one timestamp; combine
  // them so a CSV exported as "date;time;value" gets a usable x-axis
  FXint dateColumn=-1,timeColumn=-1;
  for(FXint c=0; c+1<columns; c++){
    if(dates[c] && times[c+1]){ dateColumn=c; timeColumn=c+1; break; }
    if(times[c] && dates[c+1]){ dateColumn=c+1; timeColumn=c; break; }
    }

  FXint timestampColumn=-1;
  for(FXint c=0; c<columns; c++) if(timestamps[c]){ timestampColumn=c; break; }
  table.columns.resize(columns);
  for(FXint c=0; c<columns; c++){
    CSVColumn& column=table.columns[c];
    column.name=hasheader?samples[0][c]:FXString("Column ")+FXString::value(c+1);
    column.numeric=numeric[c] || timestamps[c];
    column.timestamp=timestamps[c];
    }

  FXint rownumber=0;
  parseRecords(utf8,separator,[&](const std::vector<FXString>& row){
    if(hasheader && rownumber==0){ rownumber++; return; }
    for(FXint c=0; c<columns; c++){
      if(!table.columns[c].numeric) continue;
      FXdouble value=0.0;
      if(c<row.size()){
        if(c==dateColumn && timeColumn>=0){
          FXdouble datevalue,timevalue;
          if(timeColumn<row.size() && parseDateOnly(row[c],datevalue) && parseTimeOnly(row[timeColumn],timevalue)) value=datevalue+timevalue;
          else value=std::numeric_limits<FXdouble>::quiet_NaN();
          }
        else if(timestamps[c]){
          if(!parseDateTime(row[c],value)) value=std::numeric_limits<FXdouble>::quiet_NaN();
          }
        else if(!parseNumber(row[c],value,decimalComma)) value=std::numeric_limits<FXdouble>::quiet_NaN();
        }
      else value=std::numeric_limits<FXdouble>::quiet_NaN();
      table.columns[c].values.push_back(value);
      }
    rownumber++;
    },-1,progress,context,30,65);
  table.rows=FXMAX(0,rownumber-(hasheader?1:0));

  if(timestampColumn>=0){
    std::vector<FXdouble>& values=table.columns[timestampColumn].values;
    FXdouble origin=0.0;
    for(FXuval i=0; i<values.size(); i++) if(Math::fpFinite(values[i])){ origin=values[i]; break; }
    for(FXuval i=0; i<values.size(); i++) if(Math::fpFinite(values[i])) values[i]-=origin;
    }
  if(progress) progress(context,98);
  FXbool hasnumeric=false;
  for(FXint c=0; c<columns; c++) if(table.columns[c].numeric) hasnumeric=true;
  if(!hasnumeric){
    error="No numeric or date/time columns were detected";
    return false;
    }
  if(progress) progress(context,100);
  return true;
  }

}
