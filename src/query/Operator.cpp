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

#include <query/Operator.h>

using namespace cimdb;

Operator::Operator() {}

Operator::~Operator() {}

void Operator::gaussian_smoothing(Matrix &mat, int kernel_size, int stride, vector<WorkloadPattern> &workload_patterns, int simplification) {
  int num_vectors = kernel_size;
  
  if (simplification) num_vectors = 1;

  long long total_row_size = 0;

  if (stride == 1) { // access pattern type == Range
    for (int i = 0; i < num_vectors; i++) { // Split 2D convolution kernel into $kernel_size$ vectors
      for (int j = 0; j <= mat.rows - kernel_size; j++) {
        WorkloadPattern query;
        int l1 = j, u1 = j + kernel_size - 1; // rows
        int l2 = i, u2 = mat.columns - kernel_size + i; // columns
        query.range.push_back(Dimension("d1", l1, u1));
        query.range.push_back(Dimension("d2", l2, u2));
        query.weight = 1;
        workload_patterns.push_back(query);
        total_row_size += (u1 - l1 + 1);
      }
    }
  } else { // access pattern type == Smoothing
    for (int i = 0; i < num_vectors; i++) {
      for (int j = 0; j <= mat.rows - kernel_size; j+= stride) {
        WorkloadPattern query;
        query.type = Smoothing;
        query.weight = 1;
        int c = i;
        while (c < mat.columns) {
          query.columns.insert(c);
          c += stride;
        }
        for (int r = j; r < j + kernel_size; r++) query.rows.insert(r);
        workload_patterns.push_back(query);
      }
    }
  }

  cout << "total_row_size: "<< total_row_size << endl;
  return;
}