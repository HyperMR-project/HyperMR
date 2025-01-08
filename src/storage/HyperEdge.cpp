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

#include "storage/HyperEdge.h"

using namespace cimdb;

HyperEdge::HyperEdge() {
  weight = 0;
  pins.clear();
  pin_count.clear();
}

HyperEdge::~HyperEdge() {}

void HyperEdge::insert_pin(int pin_id) {
  pins.insert(pin_id);
  pin_count[pin_id] += 1;
}

void HyperEdge::reset() {
  pins.clear();
  pin_count.clear();
}