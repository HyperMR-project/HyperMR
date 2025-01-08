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

#ifndef _HYPER_GRAPH_H_
#define _HYPER_GRAPH_H_

#include <cassert>
#include <iostream>
#include <map>
#include <queue>
#include <set>
#include <vector>

#include "HyperEdge.h"

using namespace std;

namespace cimdb {
class HyperGraph {
 private:
 public:
  vector<HyperEdge> nets;

  HyperGraph();
  HyperGraph(int num_nets);
  ~HyperGraph();
  void merge_nets(HyperEdge &a, HyperEdge &b, int lower, int upper);
  void merge_nets(HyperEdge &a, HyperEdge &b, set<int> &accessed_columns);
  void nets_uncontraction(HyperEdge &a, HyperEdge &b);
  void clear();
};
}  // namespace cimdb
#endif