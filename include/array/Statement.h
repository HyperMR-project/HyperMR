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

#ifndef _STATEMENT_H_
#define _STATEMENT_H_

#include <string>
#include <vector>

#include "Dimension.h"

using namespace std;
using namespace cimdb;

namespace cimdb {
enum COMPRESSION {
  HyperMR = 0,
  GSMR,
  ReSpar,
  kMeans,
  TraNNsformer,
  METIS,
  GMR
};

enum HypergraphModel { ColumnNet = 0, RowNet };
enum AccessPatternType { Range = 0, Smoothing };
typedef vector<Coordinate> Coordinates;
typedef vector<Dimension> BOX;

/* hardware info */
const int num_row_Tile_per_Chip = 16;
const int num_column_Tile_per_Chip = 16;
const int num_row_PE_per_Tile = 4;
const int num_column_PE_per_Tile = 4;
const int PE_unit_height = 128;
const int PE_unit_width = 128;
}  // namespace cimdb

#endif
