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

#ifndef _HYPER_EDGE_H_
#define _HYPER_EDGE_H_

#include <cassert>
#include <iostream>
#include <map>
#include <queue>
#include <set>
#include <vector>

using namespace std;

namespace cimdb {
class HyperEdge {
 private:
 public:
  int id, weight;
  set<int> pins;            // a group of pin id.
  map<int, int> pin_count;  // <pin id, the number of pins>

  HyperEdge();
  ~HyperEdge();

  void insert_pin(int pin_id);
  void reset();
};
}  // namespace cimdb
#endif