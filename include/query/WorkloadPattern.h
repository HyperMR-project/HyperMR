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

#ifndef __WORKLOAD_PATTERN_H__
#define __WORKLOAD_PATTERN_H__

#include <cmath>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <set>

#include "array/Statement.h"

using namespace std;

namespace cimdb {
class WorkloadPattern {
 private:
 public:
  WorkloadPattern();
  WorkloadPattern(BOX &a, BOX &qr, double p);
  ~WorkloadPattern();
  int type, weight;
  BOX range;
  set<int> rows, columns; // accessed rows and columns;
};

}  // namespace cimdb

#endif /* __WORKLOAD_PATTERN_H__ */
