#include "ChartDemoUtils.h"
#include <algorithm>

namespace FXCHARTDEMO {

using namespace FX;
using namespace FXCHART;


FXString chartHash(const FXString& text){
  FXulong hash=14695981039346656037ULL;
  for(FXint i=0; i<text.length(); i++){
    hash^=(FXuchar)text[i];
    hash*=1099511628211ULL;
    }
  return FXString::value(hash,16);
  }


FXString chartColumnKey(const FXCHART::CSVTable& table,FXint column){
  FXint occurrence=0;
  for(FXint i=0; i<column; i++){
    if(table.columns[i].numeric && table.columns[i].name==table.columns[column].name && table.columns[i].timestamp==table.columns[column].timestamp) occurrence++;
    }
  return table.columns[column].name+"|"+(table.columns[column].timestamp?"T":"N")+"|"+FXString::value(occurrence);
  }


FXString chartSchemaKey(const std::vector<FXString>& keys){
  std::vector<FXString> sorted=keys;
  FXString signature;
  std::sort(sorted.begin(),sorted.end());
  for(FXuval i=0; i<sorted.size(); i++) signature+=FXString::value(sorted[i].length())+":"+sorted[i]+";";
  return "Schema-"+chartHash(signature);
  }


FXFont* makeChartFont(FXApp* app,const FXFontDesc& desc,FXint angle,FXint dpi,FXdouble scale){
  FXFontDesc scaled=desc;
  scaled.size=FXint(FXdouble(desc.size)*(scale/(FXdouble(dpi)/96.0))+0.5);
  FXFont* font=new FXFont(app,scaled);
  if(angle) font->setAngle(angle);
  font->setResolution(dpi);
  font->create();
  return font;
  }


FXString displayFileName(const FXString& filename){
  FXint start=0;
  for(FXint i=0; i<filename.length(); i++){
    if(filename[i]=='/' || filename[i]=='\\') start=i+1;
    }
  return filename.mid(start,filename.length()-start);
  }

}
