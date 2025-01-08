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
#include "query/WorkloadPattern.h"

using namespace cimdb;

WorkloadPattern::WorkloadPattern() {
  type = Range;
  weight = 0;
}

WorkloadPattern::WorkloadPattern(BOX &a, BOX &qr, double p) {}

WorkloadPattern::~WorkloadPattern() {}
