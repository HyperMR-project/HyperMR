/*
 * Copyright (c) .
 * All rights reserved.
 *
 * This file is covered by the LICENSE.txt license file in the root directory.
 *
 * @Description:
 *
 * 
 */

#ifndef _STORAGE_H_
#define _STORAGE_H_

#include <algorithm>
#include <cassert>
#include <climits>
#include <cmath>
#include <iostream>
#include <map>
#include <memory>
#include <queue>
#include <set>
#include <vector>
#include <random>

#include "HyperGraph.h"
#include "array/Dimension.h"
#include "query/WorkloadPattern.h"
#include "storage/Matrix.h"
#include "query/Executor.h"
#include "query/Operator.h"

using namespace std;

namespace cimdb {
class Storage {
 private:
 public:
  Storage();
  ~Storage();

  /* Hypergraph Partitioning Based */
  static void HyperMR(map<string, int> &keyWords, 
                          Matrix &matrix, 
                          int storage_unit_rows,
                          int storage_unit_columns,
                          vector<WorkloadPattern> &patterns); // hypergraph partitioning based Storage shceme
};

}  // namespace cimdb

#endif /* _STORAGE_H_ */