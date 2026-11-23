#include "fx.h"
#include "chartdefs.h"
#include "CSVReader.h"
#include "FXDate.h"
#include "FXUTF8Codec.h"
#include "FXUTF16Codec.h"
#include "FXUTF32Codec.h"
#include "FXCP1252Codec.h"
#include <string>
#include <vector>
#include <cstdio>
#include <cstdlib>
#include <cctype>
#include <climits>
#include <limits>

using namespace FX;

namespace FXCHART {

namespace {

template<typename Callback>
static void parseRecords(const std::string& text,FXchar delimiter,Callback callback,FXint limit=-1){
  std::vector<FXString> row;
  std::string field;
  FXbool quoted=false;
  FXbool atstart=true;
  FXint records=0;
  for(std::string::size_type i=0; i<text.size(); i++){
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


static FXbool isMissing(const FXString& value){
  FXString s=trimField(value).lower();
  return s.empty() || s=="null" || s=="na" || s=="n/a" || s=="missing" || s=="-";
  }

}


FXbool readCSV(const FXString& filename,CSVTable& table,FXString& error){
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
  if(file.readBlock(&bytes[0],bytes.size())!=(FXival)bytes.size()){
    error="Could not read the entire CSV file";
    return false;
    }
  std::string::size_type first=0;
  while(first<bytes.size() && bytes[first]=='\0') first++;
  if(first==bytes.size()){
    error="CSV file contains no text";
    return false;
    }

  table=CSVTable();
  FXString text=decodeCSV(bytes,table.encoding);
  std::string utf8(text.text(),text.length());
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
  for(FXint c=0; c<columns; c++){
    FXint tested=0,numericok=0,dateok=0;
    for(FXuval r=firstdata; r<samples.size(); r++){
      if(c>=samples[r].size() || isMissing(samples[r][c])) continue;
      FXdouble value;
      tested++;
      if(parseDateTime(samples[r][c],value)) dateok++;
      if(parseNumber(samples[r][c],value,decimalComma)) numericok++;
      }
    if(tested>=2 && dateok*5>=tested*4) timestamps[c]=true;
    else if(tested>=2 && numericok*5>=tested*4) numeric[c]=true;
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
        if(timestamps[c]){
          if(!parseDateTime(row[c],value)) value=std::numeric_limits<FXdouble>::quiet_NaN();
          }
        else if(!parseNumber(row[c],value,decimalComma)) value=std::numeric_limits<FXdouble>::quiet_NaN();
        }
      else value=std::numeric_limits<FXdouble>::quiet_NaN();
      table.columns[c].values.push_back(value);
      }
    rownumber++;
    });
  table.rows=FXMAX(0,rownumber-(hasheader?1:0));

  if(timestampColumn>=0){
    std::vector<FXdouble>& values=table.columns[timestampColumn].values;
    FXdouble origin=0.0;
    for(FXuval i=0; i<values.size(); i++) if(Math::fpFinite(values[i])){ origin=values[i]; break; }
    for(FXuval i=0; i<values.size(); i++) if(Math::fpFinite(values[i])) values[i]-=origin;
    }
  FXbool hasnumeric=false;
  for(FXint c=0; c<columns; c++) if(table.columns[c].numeric) hasnumeric=true;
  if(!hasnumeric){
    error="No numeric or date/time columns were detected";
    return false;
    }
  return true;
  }

}
