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

#ifndef _ARRAY_DIMENSION_H_
#define _ARRAY_DIMENSION_H_

#include <string>

using namespace std;

namespace cimdb {
typedef int64_t Coordinate;
class Dimension {
 private:
  string name;

 public:
 Dimension(string name, Coordinate lower_bound, Coordinate upper_bound) {
  this->name = name;
  this->lower_bound = lower_bound;
  this->upper_bound = upper_bound;
}
  ~Dimension(){};
  Coordinate lower_bound;
  Coordinate upper_bound;

    /**
   * @brief:
   * if lower_bound is 0, upper_bound is 99, I = 99 - 0 + 1
   * if lower_bound is 9, upper_bound is 99, I = 99 - 9 + 1
   * @return {*}
   */
  Coordinate getElementNumber() {
    return upper_bound - lower_bound + 1;
  }
};
}  // namespace cimdb

#endif /* _ARRAY_DIMENSION_H_ */