#ifndef CSVREADER_H
#define CSVREADER_H

#include "fx.h"
#include <vector>

namespace FXCHART {

struct CSVColumn {
  FXString name;
  FXbool numeric;
  FXbool timestamp;
  std::vector<FXdouble> values;
  };

struct CSVTable {
  std::vector<CSVColumn> columns;
  FXint rows;
  FXString encoding;
  FXString delimiter;
  };

typedef void (*CSVProgressCallback)(void*,FXuint);

/// Read a delimited text file or an OpenDocument spreadsheet
FXbool readCSV(const FXString& filename,CSVTable& table,FXString& error,CSVProgressCallback progress=nullptr,void* context=nullptr);

}

#endif
