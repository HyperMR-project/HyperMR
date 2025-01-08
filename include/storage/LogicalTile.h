/*
 * Copyright (c) .
 * All rights reserved.
 *
 * This file is covered by the LICENSE.txt license file in the root directory.
 *
 * @Description: LogicalPE is the logical process element (PE)
 *
 * 
 */
#ifndef _LOGICAL_TILE_H_
#define _LOGICAL_TILE_H_

#include <vector>
#include <set>

using namespace std;

namespace cimdb {
class LogicalTile {
 private:
 public:
  LogicalTile() {input_vector_size = 0;};
  ~LogicalTile(){};
  int id, input_vector_size;  // phsical hardware info
  set<int> accessed_rows, accessed_columns;
};
}  // namespace cimdb

#endif