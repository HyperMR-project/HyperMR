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

#ifndef __EXECUTOR_H__
#define __EXECUTOR_H__

#include <cmath>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <set>

#include "array/Statement.h"
#include "query/WorkloadPattern.h"
#include "storage/Matrix.h"
#include "simulator/NeuroSimAPI.h"

using namespace std;

namespace cimdb {
class Executor {
 private:
 public:
  Executor() {
    cost = 0;
    total_communication_data_size = 0;
    total_latency = 0;
    num_accessed_tiles = 0;
    num_accessed_PEs = 0;
  };
  ~Executor() {};
  int num_accessed_PEs, num_accessed_tiles, total_latency, total_communication_data_size, cost;
  vector<int> input_vector_size, output_vector_size;  // used for cost model
  vector<vector<int>> accessed_PEs_list;
  vector<pair<int, int>> range_query_size;
  vector<map<int, pair<int, int>>> accessed_PE_size, accessed_Tile_size;
  vector<map<int, LogicalTile>> query_tile_size_per_pattern;
  
  int cost_model(Matrix &mat, ofstream &costModel, int cost_model_test);
  void simulation(Matrix &mat, ofstream &costModel, int cost_model_test);
  void physical_plan(Matrix &mat, vector<WorkloadPattern> &workload_patterns, bool cost_model);  // return the number of accessed PEs
};

}  // namespace cimdb

#endif /* __EXECUTOR_H__ */