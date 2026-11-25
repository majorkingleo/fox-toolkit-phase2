#ifndef CSVREADER_H
#define CSVREADER_H

#include "fx.h"
#include <vector>

namespace FXCHART {

struct CSVColumn {
  FXString name;
  FXbool numeric;
  FXbool timestamp;
  FXdouble timeOrigin;
  std::vector<FXdouble> values;
  };

struct CSVTable {
  std::vector<CSVColumn> columns;
  FXint rows;
  FXString encoding;
  FXString delimiter;
  };

typedef void (*CSVProgressCallback)(void*,FXuint);

/// Read a delimited text file, an OpenDocument spreadsheet or an Excel workbook
FXbool readCSV(const FXString& filename,CSVTable& table,FXString& error,CSVProgressCallback progress=nullptr,void* context=nullptr);

}

#endif
