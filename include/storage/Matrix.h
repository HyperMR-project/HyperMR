/*
 * Copyright (c) .
 * All rights reserved.
 *
 * This file is covered by the LICENSE.txt license file in the root directory.
 *
 * @Description: 2D Matrix
 *
 * 
 */

#ifndef _MATRIX_H_
#define _MATRIX_H_

#include <stdlib.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <map>
#include <random>
#include <sstream>
#include <string>
#include <vector>

#include "LogicalPE.h"
#include "LogicalTile.h"
#include "array/Statement.h"

using namespace std;

namespace cimdb {
/**
 * @brief: a logical 2D slice, which is mapped to the phsical xbar.
 * default order: row major
 * @return
 */
class Matrix {
 private:
 public:
  Matrix();
  Matrix(int r, int c);
  ~Matrix();
  int rows, columns, TileColumns, numTiles = 0;
  double compression_rate;
  bool symmetric;
  vector<vector<bool>> valid_map;  // used for reordering
  vector<int> row_imprint, col_imprint;
  vector<LogicalPE> storage_units;
  map<int, vector<int>> PE_to_location;

  int get_row_imprint(size_t id) {
    if (id >= row_imprint.size()) {  // for dense array
      return -1;
    } else if (row_imprint[id] >= rows) {  // for reordered array
      return -1;
    } else {
      return row_imprint[id];
    }
  }
  
  int get_col_imprint(size_t id) {
    if (id >= col_imprint.size()) {  // for dense array
      return -1;
    } else if (col_imprint[id] >= columns) {  // for reordered array
      return -1;
    } else {
      return col_imprint[id];
    }
  }

  void reset_imprints();
  void storage_model();
};
}  // namespace cimdb
#endif /* _MATRIX_H_ */