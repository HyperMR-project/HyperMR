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

#include "storage/Matrix.h"
#include "omp.h"


extern "C" {
#include <cblas.h>
}

using namespace std;
using namespace cimdb;

Matrix::Matrix() {
  rows = 128;
  columns = 128;
  for (int i = 0; i < rows; i++) row_imprint.push_back(i);
  for (int i = 0; i < columns; i++) col_imprint.push_back(i);
  vector<vector<bool>> init(rows, vector<bool>(columns, 0));
  valid_map = init;
  symmetric = false;
  compression_rate = 1.0;
}

Matrix::Matrix(int r, int c) {
  rows = r;
  columns = c;
  for (int i = 0; i < rows; i++) row_imprint.push_back(i);
  for (int i = 0; i < columns; i++) col_imprint.push_back(i);
  vector<vector<bool>> init(rows, vector<bool>(columns, 0));
  valid_map = init;
  symmetric = false;
  compression_rate = 1.0;
}

Matrix::~Matrix() {}

void Matrix::reset_imprints() {
  row_imprint.clear();
  col_imprint.clear();
  for (int r = 0; r < rows; r++) { row_imprint.push_back(r); }
  for (int c = 0; c < columns; c++) { col_imprint.push_back(c);  }
}

void Matrix::storage_model() {
  storage_units.clear();
  PE_to_location.clear();
  double num_column_PE = ceil(col_imprint.size() / (double)PE_unit_width);
  TileColumns = ceil((num_column_PE) / num_column_PE_per_Tile);
  set<int> allocatedTiles;

  for (size_t i = 0; i < row_imprint.size(); i += PE_unit_height) {
    for (size_t j = 0; j < col_imprint.size(); j += PE_unit_width) {
      bool enable = false;  // not full zero
      LogicalPE xbar;
      for (size_t k = i; k < i + PE_unit_height && k < row_imprint.size(); k++) {
        if (row_imprint[k] >= rows) continue;
        for (size_t z = j; z < j + PE_unit_width && z < col_imprint.size(); z++) {
          if (col_imprint[z] >= columns) continue;
          if (valid_map[row_imprint[k]][col_imprint[z]]) {
            enable = true;
            xbar.row_imprint.insert(row_imprint[k]);
            xbar.col_imprint.insert(col_imprint[z]);
          }
        }
      }
      if (enable) {
        xbar.id = storage_units.size();
        storage_units.push_back(xbar);
        int rowIndex = i / PE_unit_height, colIndex = j / PE_unit_width;
        int tile_rid = rowIndex / num_row_PE_per_Tile;
        int tile_cid = colIndex / num_column_PE_per_Tile;
        PE_to_location[xbar.id].push_back(tile_rid);
        PE_to_location[xbar.id].push_back(tile_cid);
        PE_to_location[xbar.id].push_back(rowIndex % num_row_PE_per_Tile);
        PE_to_location[xbar.id].push_back(colIndex % num_column_PE_per_Tile);
        PE_to_location[xbar.id].push_back(xbar.id);
        allocatedTiles.insert(tile_rid * TileColumns + tile_cid);
      }
    }
  }
  numTiles = allocatedTiles.size();
}