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
#ifndef __OPERATOR_H__
#define __OPERATOR_H__

#include <cmath>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <set>

#include "query/WorkloadPattern.h"
#include "storage/Matrix.h"

using namespace std;

namespace cimdb {
class Operator {
 private:
 public:
  Operator();
  ~Operator();
  static void gaussian_smoothing(Matrix &mat, int kernel_size, int stride, vector<WorkloadPattern> &workload_patterns, int simplification);
};

}  // namespace cimdb

#endif /* __OPERATOR_H__ */