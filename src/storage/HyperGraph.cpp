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

#include "storage/HyperGraph.h"

using namespace cimdb;

HyperGraph::HyperGraph() {}

HyperGraph::HyperGraph(int num_nets) {
  nets = vector<HyperEdge>(num_nets);
  int id = 0;
  for (auto& net : nets) {
    net.weight = 1;
    net.id = id++;
  }
}

HyperGraph::~HyperGraph() {}

void HyperGraph::merge_nets(HyperEdge &a, HyperEdge &b, int lower, int upper) {
  for (auto it = b.pins.begin(); it != b.pins.end(); it++) {
    if (*it >= lower && *it <= upper) a.insert_pin(*it);
  }
}

void HyperGraph::merge_nets(HyperEdge &a, HyperEdge &b, set<int> &accessed_columns) {
  for (auto it = b.pins.begin(); it != b.pins.end(); it++) {
    if (accessed_columns.find(*it) != accessed_columns.end()) {
      a.insert_pin(*it);
    }
  }
}

void HyperGraph::nets_uncontraction(HyperEdge &a, HyperEdge &b) {
  for (auto it = b.pins.begin(); it != b.pins.end(); it++) {
    if (a.pin_count[*it] == 1) {
      a.pins.erase(*it);
    }
    a.pin_count[*it] -= 1;
  }
}

void HyperGraph::clear() {
  nets.clear();
  return;
}