/*
 * Copyright (c) .
 * All rights reserved.
 *
 * This file is covered by the LICENSE.txt license file in the root directory.
 *
 * @Description: Hypergraph Partitioning
 *
 * 
 */
#include <libkahypar.h>

#include "storage/Storage.h"

using namespace cimdb;
using namespace std;

int balanced_k_way_hypergraph_partitioning(HyperGraph &HG, int num_partitions, int k, vector<vector<int>> &partitions) {
  kahypar_context_t *context = kahypar_context_new();
  string config_path = "../test/config/HyperMR.ini";
  kahypar_configure_context_from_file(context, config_path.c_str());
  const double imbalance = 0.00;
  kahypar_hyperedge_weight_t objective = 0;
  const kahypar_hyperedge_id_t num_hyperedges = HG.nets.size();
  std::vector<kahypar_partition_id_t> partition(num_partitions, -1);
  std::unique_ptr<kahypar_hyperedge_weight_t[]> hyperedge_weights = std::make_unique<kahypar_hyperedge_weight_t[]>(num_hyperedges);
  for (int i = 0; i < num_hyperedges; i++) {
    hyperedge_weights[i] = HG.nets[i].weight;
  }
  std::unique_ptr<size_t[]> hyperedge_indices = std::make_unique<size_t[]>(num_hyperedges + 1);
  int num_pins = 0;
  for (int i = 0; i < num_hyperedges; i++) {
    hyperedge_indices[i] = num_pins;
    num_pins += HG.nets[i].pins.size();
  }

  hyperedge_indices[num_hyperedges] = num_pins;

  std::unique_ptr<kahypar_hyperedge_id_t[]> hyperedges = std::make_unique<kahypar_hyperedge_id_t[]>(num_pins);
  int idx = 0;
  for (int i = 0; i < num_hyperedges; i++) {
    for (auto item : HG.nets[i].pins) {
      hyperedges[idx] = item;
      ++idx;
    }
  }
  /* Balanced k-way Hypergraph Partitioning */
  kahypar_partition(num_partitions, num_hyperedges, imbalance, k,
                    /* vertex_weights */ nullptr, hyperedge_weights.get(),
                    hyperedge_indices.get(), hyperedges.get(), &objective,
                    context, partition.data());
  // result
  vector<vector<int>> temporal_partition(k);
  for (int i = 0; i < num_partitions; ++i) {
    temporal_partition[partition[i]].push_back(i);
  }
  partitions = temporal_partition;
  kahypar_context_free(context);
  return objective;
}

void hybrid_partitioning(HyperGraph &QueryRowNetHG,
                         HyperGraph &MergedRowHG,
                         vector<vector<int>> &column_partitions, int factor) {
  HyperGraph HybridRowNetHG;
  int column_k = column_partitions.size();
  int columns = column_k * column_partitions[0].size();
  if (factor != 0) {  // net.weight can not be 0
    for (auto net : MergedRowHG.nets) {
      net.weight = factor;
      HybridRowNetHG.nets.push_back(net);
    }
  }
  for (auto net : QueryRowNetHG.nets) {
    HybridRowNetHG.nets.push_back(net);
  }
  balanced_k_way_hypergraph_partitioning(HybridRowNetHG, columns, column_k, column_partitions);
}

// two-phase hypergraph partitioning (TPHP)
void communication_optimized_TPHP (int factor, HyperGraph &RowHG, HyperGraph &ColumnHG,
                                   vector<vector<int>> &row_partitions,
                                   vector<vector<int>> &column_partitions,
                                   vector<WorkloadPattern> &patterns) {
  int row_k = row_partitions.size(), column_k = column_partitions.size();
  int rows = row_k * row_partitions[0].size(), columns = column_k * column_partitions[0].size();
  int matrix_row = RowHG.nets.size(), matrix_column = ColumnHG.nets.size();
  int l0, u0, l1, u1;
  int obj, obj1, obj2, obj3, obj4, obj5, obj6;

  /* Query-aware Hypergraph Generation */
  // find a good merged-row-net hypergraph
  HyperGraph MergedColumnHG;

  obj1 = balanced_k_way_hypergraph_partitioning(RowHG, columns, column_k, column_partitions);
  
  // second phase: hypergraph partitioning (Merged Column-Net)
  // improve the compression rate

  l0 = 0, u0 = matrix_row - 1;  // row
  for (auto partition : column_partitions) {
    HyperEdge net;
    for (auto item : partition) {
      if (item >= matrix_column) continue;
      MergedColumnHG.merge_nets(net, ColumnHG.nets[item], l0, u0);
    }
    net.weight = 1;
    net.id = MergedColumnHG.nets.size();
    MergedColumnHG.nets.push_back(net);
  }

  obj3 = balanced_k_way_hypergraph_partitioning(MergedColumnHG, rows, row_k, row_partitions);
  return;                       
}

// two-phase hypergraph partitioning (TPHP)
void storage_optimized_TPHP (HyperGraph &RowHG, HyperGraph &ColumnHG,
                                       vector<vector<int>> &row_partitions,
                                       vector<vector<int>> &column_partitions,
                                       vector<WorkloadPattern> &patterns) {
  int row_k = row_partitions.size(), column_k = column_partitions.size();
  int rows = row_k * row_partitions[0].size(), columns = column_k * column_partitions[0].size();
  int matrix_row = RowHG.nets.size(), matrix_column = ColumnHG.nets.size();
  int l0, u0, l1, u1;
  int obj, obj1, obj2, obj3, obj4, obj5, obj6;

  // first phase: hypergraph partitioning (Merged Column-Net)
  // improve the compression rate

  // find a good merged-row-net hypergraph
  HyperGraph MergedColumnHG;
  l0 = 0, u0 = matrix_row - 1;  // row
  for (auto partition : column_partitions) {
    HyperEdge net;
    for (auto item : partition) {
      if (item >= matrix_column) continue;
      MergedColumnHG.merge_nets(net, ColumnHG.nets[item], l0, u0);
    }
    net.weight = 1;
    net.id = MergedColumnHG.nets.size();
    MergedColumnHG.nets.push_back(net);
  }

  obj1 = balanced_k_way_hypergraph_partitioning(MergedColumnHG, rows, row_k, row_partitions);
  return;                       
}

void query_aware_hypergraph_generation(HyperGraph &QueryRowNetHG, HyperGraph &RowHG, vector<WorkloadPattern> &patterns) {
  int l0, u0, l1, u1;
  int matrix_row = RowHG.nets.size();

  for (auto pattern : patterns) {
    if (pattern.type == Range) {
      l0 = pattern.range[0].lower_bound,
      u0 = pattern.range[0].upper_bound;  // row
      l1 = pattern.range[1].lower_bound,
      u1 = pattern.range[1].upper_bound;  // column
      for (int r = 0; r < matrix_row; r++) {
        if (r >= l0 && r <= u0) {
          HyperEdge net;
          QueryRowNetHG.merge_nets(net, RowHG.nets[r], l1, u1);
          net.weight = pattern.weight;
          net.id = QueryRowNetHG.nets.size();
          QueryRowNetHG.nets.push_back(net);
        }
      }
    } else if (pattern.type == Smoothing) {
      for (auto r : pattern.rows) {
        HyperEdge net;
        QueryRowNetHG.merge_nets(net, RowHG.nets[r], pattern.columns);
        net.weight = pattern.weight;
        net.id = QueryRowNetHG.nets.size();
        QueryRowNetHG.nets.push_back(net);
      }
    }
  }

  set<int> accessed_columns;
  for (auto net : QueryRowNetHG.nets) {
    for (auto pin : net.pins) {
      accessed_columns.insert(pin);
    }
  }

  // unaccessed columns
  for (int r = 0; r < matrix_row; r++) {
    HyperEdge net;
    for (auto pin : RowHG.nets[r].pins) {
      if (accessed_columns.find(pin) == accessed_columns.end()) {
        net.insert_pin(pin);
      }
    }
    if (net.pins.size() == 0) continue;
    net.weight = 1;
    net.id = QueryRowNetHG.nets.size();
    QueryRowNetHG.nets.push_back(net);
  }

  return;
}

void two_phase_hypergraph_partitioning(int factor, HyperGraph &QueryRowNetHG, HyperGraph &RowHG, 
                                       HyperGraph &ColumnHG, vector<vector<int>> &row_partitions,
                                       vector<vector<int>> &column_partitions) {
  int row_k = row_partitions.size(), column_k = column_partitions.size();
  int rows = row_k * row_partitions[0].size(), columns = column_k * column_partitions[0].size();
  int matrix_row = RowHG.nets.size(), matrix_column = ColumnHG.nets.size();
  int l0, u0, l1, u1;
  int obj, obj1, obj2, obj3, obj4, obj5, obj6;

  /* Query-aware Hypergraph Generation */
  // find a good merged-row-net hypergraph
  HyperGraph MergedColumnHG;

  obj1 = balanced_k_way_hypergraph_partitioning(QueryRowNetHG, columns, column_k, column_partitions);
  
  // second phase: hypergraph partitioning (Merged Column-Net)
  // improve the compression rate

  l0 = 0, u0 = matrix_row - 1;  // row
  for (auto partition : column_partitions) {
    HyperEdge net;
    for (auto item : partition) {
      if (item >= matrix_column) continue;
      MergedColumnHG.merge_nets(net, ColumnHG.nets[item], l0, u0);
    }
    net.weight = 1;
    net.id = MergedColumnHG.nets.size();
    MergedColumnHG.nets.push_back(net);
  }

  obj3 = balanced_k_way_hypergraph_partitioning(MergedColumnHG, rows, row_k, row_partitions);
  return;                             
}

void Storage::HyperMR(map<string, int> &keyWords, 
                          Matrix &matrix, 
                          int storage_unit_rows,
                          int storage_unit_columns,
                          vector<WorkloadPattern> &patterns) {
  auto &mat = matrix.valid_map;
  int rows = matrix.rows, columns = matrix.columns;

  bool query_aware = keyWords["query-aware-optimization"];
  bool storage_optimized = keyWords["storage-optimized"];
  int factor = keyWords["hybrid-partitioning-factor"];

  // expand rows & columns
  rows = rows % storage_unit_rows == 0 ? rows : rows + (storage_unit_rows - rows % storage_unit_rows);
  columns = columns % storage_unit_columns == 0 ? columns : columns + (storage_unit_columns - columns % storage_unit_columns);

  // Two Hypergraph Models: Column Net and Row Net
  // We assume that there are no empty rows or columns in the sparse matrix.
  HyperGraph RowHG(matrix.rows), ColumnHG(matrix.columns);

  for (int r = 0; r < matrix.rows; r++) {
    for (int c = 0; c < matrix.columns; c++) {
      if (mat[r][c]) {
        RowHG.nets[r].insert_pin(c);
        ColumnHG.nets[c].insert_pin(r);
      }
    }
  }

  vector<vector<int>> row_partitions(rows / storage_unit_rows), column_partitions(columns / storage_unit_columns);  // contraction schemes
  // merge row vertices to row nets
  for (int rid = 0, r = 0; rid < rows; rid++) {
    if (rid != 0 && rid % storage_unit_rows == 0) r++;
    row_partitions[r].push_back(rid);
  }
  // merge column vertices to column nets
  for (int cid = 0, c = 0; cid < columns; cid++) {
    if (cid != 0 && cid % storage_unit_columns == 0) c++;
    column_partitions[c].push_back(cid);
  }
  /* Iterative Hypergraph Partitioning */
  // This flag specifies which vertex set needs to be contracted, rc == true
  // specifies the column vertex set.

  if (query_aware) {
    HyperGraph QueryRowNetHG;
    query_aware_hypergraph_generation(QueryRowNetHG, RowHG, patterns);
    two_phase_hypergraph_partitioning(factor, QueryRowNetHG, RowHG, ColumnHG, row_partitions, column_partitions);
  } else if (storage_optimized) {
    storage_optimized_TPHP(RowHG, ColumnHG, row_partitions, column_partitions, patterns);
  } else {
    communication_optimized_TPHP(factor, RowHG, ColumnHG, row_partitions, column_partitions, patterns);
  }

  // result
  vector<int> row_order, column_order;

  for (auto partition : row_partitions) {
    for (auto item : partition) {
      row_order.push_back(item);
    }
  }

  for (auto partition : column_partitions) {
    for (auto item : partition) {
      column_order.push_back(item);
    }
  }
  matrix.row_imprint = row_order;
  matrix.col_imprint = column_order;
}