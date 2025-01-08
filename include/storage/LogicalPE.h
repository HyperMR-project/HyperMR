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
#ifndef _LOGICAL_PE_H_
#define _LOGICAL_PE_H_

#include <set>

using namespace std;

namespace cimdb {
class LogicalPE {
 private:
 public:
  LogicalPE(){};
  ~LogicalPE(){};
  int id;  // phsical hardware info
  set<int> row_imprint, col_imprint;
  set<int> accessed_rows, accessed_columns;
};
}  // namespace cimdb

#endif