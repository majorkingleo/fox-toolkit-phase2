#ifndef CHARTDEMOUTILS_H
#define CHARTDEMOUTILS_H

#include "fx.h"
#include "CSVReader.h"
#include <vector>

namespace FXCHARTDEMO {

using namespace FX;


FXString chartHash(const FXString& text);
FXString chartColumnKey(const FXCHART::CSVTable& table,FXint column);
FXString chartSchemaKey(const std::vector<FXString>& keys);
FXulong chartDataBufferBytes(const FXCHART::CSVTable& table);
FXString chartMemorySize(FXulong bytes);
FXFont* makeChartFont(FXApp* app,const FXFontDesc& desc,FXint angle,FXint dpi,FXdouble scale);
FXString displayFileName(const FXString& filename);

}

#endif
