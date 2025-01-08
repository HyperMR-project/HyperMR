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
#include "query/Executor.h"

extern Param *param;

void Executor::physical_plan(Matrix &mat, vector<WorkloadPattern> &workload_patterns, bool cost_model) {
  int num_invalid_pattern = 0;
  num_accessed_PEs = 0;
  num_accessed_tiles = 0;
  total_communication_data_size = 0;
  int n = 0;

  /* range queries */
  for (auto& pattern : workload_patterns) {
    int rows, columns;
    vector<int> accessed_PEs;
    set<int> accessed_tiles;
    map<int, pair<int, int>> PE_query_size;
    map<int, LogicalTile> tile_query_size; // <tile_id, tile>
    input_vector_size.push_back(0);
    output_vector_size.push_back(0);

    if (pattern.type == Range) {
      auto it = pattern.range.begin();
      auto d1 = *it, d2 = *(++it);
      rows = d1.upper_bound - d1.lower_bound + 1;
      columns = d2.upper_bound - d2.lower_bound + 1;
      if (rows == 0 || columns == 0) continue;
      if (cost_model) {
        // ----------- cost model -----------
        // exclude the all-zero columns
        for (size_t i = d2.lower_bound; i <= d2.upper_bound; i++) {
          bool all_zero = true;
          for (int j = 0; j < mat.valid_map.size(); j++) {  
            if (mat.valid_map[j][i]) {
              all_zero = false;
              break;
            }
          }
          if (all_zero) {
            -- columns;
          }
        }
        // exclude the all-zero rows
        for (int i = 0; i < mat.valid_map.size(); i++) {
          bool all_zero = true;
          for (auto item : mat.valid_map[i]) {
            if (item) {
              all_zero = false;
              break;
            }
          }
          if (all_zero) {
            -- rows;
          }
        }
        output_vector_size[n] += columns;
        total_communication_data_size += columns;
        // ----------- cost model -----------
      }
      for (auto &PE : mat.storage_units) {
        bool accessed = false;
        PE.accessed_rows.clear();
        PE.accessed_columns.clear();
        auto &location = mat.PE_to_location[PE.id];
        int tile_id = location[0] * mat.TileColumns + location[1];
        for (auto rid : PE.row_imprint) {
          if (rid <= d1.upper_bound && rid >= d1.lower_bound) {  // rid should be activted
            for (auto cid : PE.col_imprint) {
              if (cid <= d2.upper_bound && cid >= d2.lower_bound) {  // cid should be activted
                if (mat.valid_map[rid][cid]) {
                  accessed = true;
                  PE.accessed_rows.insert(rid);
                  PE.accessed_columns.insert(cid);
                  tile_query_size[tile_id].accessed_rows.insert(rid);
                  tile_query_size[tile_id].accessed_columns.insert(cid);
                  accessed_tiles.insert(tile_id);
                }
              }
            }
          }
        }
        if (accessed) {
          tile_query_size[tile_id].input_vector_size += PE.accessed_rows.size();
          accessed_PEs.push_back(PE.id);
          PE_query_size[PE.id] = pair<int, int> (PE.accessed_rows.size(), PE.accessed_columns.size());
        }
      }
    } else if (pattern.type == Smoothing) {
      rows = pattern.rows.size();
      columns = pattern.columns.size();
      if (rows == 0 || columns == 0) continue;
      if (cost_model) {
        // ----------- cost model -----------
        // exclude the all-zero columns
        for (auto c : pattern.columns) {
          bool all_zero = true;
          for (auto r : pattern.rows) {  
            if (mat.valid_map[r][c]) {
              all_zero = false;
              break;
            }
          }
          if (all_zero) {
            -- columns;
          }
        }
        // exclude the all-zero rows
        for (auto r : pattern.rows) {
          bool all_zero = true;
          for (auto c : pattern.columns) {
            if (mat.valid_map[r][c]) {
              all_zero = false;
              break;
            }
          }
          if (all_zero) {
            -- rows;
          }
        }
        output_vector_size[n] += columns;
        total_communication_data_size += columns;
        // ----------- cost model -----------
      }
      for (auto &PE : mat.storage_units) {
        bool accessed = false;
        PE.accessed_rows.clear();
        PE.accessed_columns.clear();
        auto &location = mat.PE_to_location[PE.id];
        int tile_id = location[0] * mat.TileColumns + location[1];
        for (auto rid : PE.row_imprint) {
          if (pattern.rows.find(rid) != pattern.rows.end()) {  // rid should be activted
            for (auto cid : PE.col_imprint) {
              if (pattern.columns.find(cid) != pattern.columns.end()) {  // cid should be activted
                if (mat.valid_map[rid][cid]) {
                  accessed = true;
                  PE.accessed_rows.insert(rid);
                  PE.accessed_columns.insert(cid);
                  tile_query_size[tile_id].accessed_rows.insert(rid);
                  tile_query_size[tile_id].accessed_columns.insert(cid);
                  accessed_tiles.insert(tile_id);
                }
              }
            }
          }
        }
        if (accessed) {
          tile_query_size[tile_id].input_vector_size += PE.accessed_rows.size();
          accessed_PEs.push_back(PE.id);
          PE_query_size[PE.id] = pair<int, int> (PE.accessed_rows.size(), PE.accessed_columns.size());
        }
      }
    }
    // statistical info which is used for simulation
    range_query_size.push_back(pair<int, int>(rows, columns));
    if (accessed_PEs.size() == 0) { num_invalid_pattern ++; }
    else {
      accessed_PEs_list.push_back(accessed_PEs);
      accessed_PE_size.push_back(PE_query_size);
      query_tile_size_per_pattern.push_back(tile_query_size);
      num_accessed_PEs += (accessed_PEs.size() * pattern.weight);
      num_accessed_tiles += accessed_tiles.size();
      for (auto item : tile_query_size) {
        input_vector_size[n] += tile_query_size[item.first].accessed_rows.size();
        total_communication_data_size += tile_query_size[item.first].accessed_rows.size();
      }
    }
    ++n;
    // each pattern
  }
  return;
}

inline double tile_buffer_model(double numBufferRead, double numBufferWrite) {
  double avgBitReadLatency = 6.85988e-10, avgBitWriteLatency = 6.85988e-10;
  double readLatency = avgBitReadLatency * numBufferRead;
	double writeLatency = avgBitWriteLatency * numBufferWrite;
  return readLatency + writeLatency;
}

inline double global_buffer_model(double numBufferRead, double numBufferWrite) {
  double avgBitReadLatency = 1.04589e-09, avgBitWriteLatency = 1.04589e-09;
  double readLatency = avgBitReadLatency * numBufferRead;
	double writeLatency = avgBitWriteLatency * numBufferWrite;
  return readLatency + writeLatency;
}

inline double global_communication_model(double numHTreeRead) {
  double global_hTree_k = 2.48503e-08;
  double latency = global_hTree_k * numHTreeRead;
  return latency;
}

inline double tile_communication_model(double numHTreeRead) {
  double tile_hTree_k = 3.48622e-10;
  double latency = tile_hTree_k * numHTreeRead;
  return latency;
}

inline double global_accumulation_model(double numRead, int numUnitAdd) {
  double latency = 0;
  double readLatencyIntermediate = 6.39847e-11;
  int i = ceil(log2(numUnitAdd));
  double base_adder_readLatency = 1.00676e-09;
  int numBitEachStage = 0;

  while (i != 0) {   // calculate the total # of full adder in each Adder Tree
    latency += (base_adder_readLatency + numBitEachStage * readLatencyIntermediate);
    numBitEachStage += 1;
    i -= 1;
  }

  latency *= numRead;
  return latency;
}

inline double tile_accumulation_model(double numRead, int numUnitAdd) {
  double latency = 0;
  double readLatencyIntermediate = 6.39847e-11;
  int i = ceil(log2(numUnitAdd));
  int j = numUnitAdd;
  double base_adder_readLatency = 1.90255e-09;
  int numBitEachStage = 0;

  while (i != 0) {   // calculate the total # of full adder in each Adder Tree
    latency += (base_adder_readLatency + numBitEachStage * readLatencyIntermediate);
    numBitEachStage += 1;
    i -= 1;
  }

  latency *= numRead;
  return latency;
}

int Executor::cost_model(Matrix &mat, ofstream &costModel, int cost_model_test) {
  int matrixCol, numUnitAdd;
  int numBitInput = 16, numTileAdderBit = 30, numInVector = 1, numColPerSynapse = 8, numAdderTree = 64, numColMuxed = 8;
  int chip_communication_bandwidth = 576, tile_communication_bandwidth = 512;
  int global_buffer_interface_width = 128, tile_buffer_interface_width = 32;
  int TileColumns = mat.TileColumns;
  auto &PE_to_location = mat.PE_to_location;
  double global_buffer_latency = 0, global_communication_latency = 0, tile_latency = 0, global_accumulation_latency = 0;
  double max_PE_latency = 2.95975e-07, numRead;

  size_t qid = 0;
  for (auto &accessed_PEs : accessed_PEs_list) {
    // initialize
    matrixCol = range_query_size[qid].second;
    vector<vector<int>> PEs_list;
    for (auto PE_id : accessed_PEs) {
      PEs_list.push_back(PE_to_location[PE_id]);
    }

    auto &tile_query_size = query_tile_size_per_pattern[qid];
    map<int, int> query_tile_accessed_columns;
    for (auto item : tile_query_size) {
      query_tile_accessed_columns[item.first] = item.second.accessed_columns.size();
    }
    
    map<int, vector<vector<int>>> PE_map;
    map<int, set<int>> TileColSize;
    for (auto accessed_PEs : PEs_list) {
      vector<int> PEs;
      int tile_id = accessed_PEs[0] * TileColumns + accessed_PEs[1];
      PEs.push_back(accessed_PEs[2]);
      PEs.push_back(accessed_PEs[3]);
      PEs.push_back(accessed_PEs[4]);
      PE_map[tile_id].push_back(PEs);
      TileColSize[accessed_PEs[1]].insert(accessed_PEs[0]);
    }

    /* global buffer latency */
    double numBitToLoadOut = input_vector_size[qid] * numBitInput * numInVector;
    double numBitToLoadIn = ceil(matrixCol * numColPerSynapse / (double)numColPerSynapse) * numBitInput * numInVector;
    double numBufferRead, numBufferWrite;
    numBufferRead = numBitToLoadOut / global_buffer_interface_width;
    numBufferWrite = numBitToLoadIn / global_buffer_interface_width;
    global_buffer_latency += global_buffer_model(numBufferRead, numBufferWrite);
    
    /* global communication latency */
    double numHTreeRead = ceil((numBitToLoadOut + numBitToLoadIn) / chip_communication_bandwidth);
    global_communication_latency += global_communication_model(numHTreeRead);

    /* the most time-consuming tile latency */
    int target_tile_id = 0;
    int max_var = 0;
    for (auto item : tile_query_size) {
      int tile_id = item.first;
      double current_tile_latency = 0;
      // tile-level buffer latency
      numBitToLoadIn = ceil(tile_query_size[tile_id].accessed_columns.size()/numColPerSynapse)*(1+numTileAdderBit)*numInVector;
      numBitToLoadOut = tile_query_size[tile_id].input_vector_size * numInVector * numBitInput;
      numBufferRead = numBitToLoadOut / tile_buffer_interface_width;
      numBufferWrite = numBitToLoadOut / tile_buffer_interface_width;
      double tile_buffer_latency = 0;
      tile_buffer_latency += tile_buffer_model(numBufferRead, numBufferWrite);
      numBufferRead = numBitToLoadIn / tile_buffer_interface_width;
      numBufferWrite = numBitToLoadIn / tile_buffer_interface_width;
      tile_buffer_latency += tile_buffer_model(numBufferRead, numBufferWrite);
      current_tile_latency += tile_buffer_latency;

      // tile-level communication latency
      numHTreeRead = ceil((numBitToLoadOut + numBitToLoadIn) / tile_communication_bandwidth);
      current_tile_latency += tile_communication_model(numHTreeRead);

      // computing in PE units
      current_tile_latency += max_PE_latency;

      // tile-level accumulation latency
      set<int> PE_rows;
      for (auto xbar : PE_map[tile_id]) PE_rows.insert(xbar[0]);
      numRead = ((double)numInVector)*ceil((double)numColMuxed/(double)numColPerSynapse);
      numUnitAdd = PE_rows.size();
      current_tile_latency += tile_accumulation_model(numRead, numUnitAdd);

      // Find the tile with the longest communication time among all tiles.
      if (current_tile_latency > tile_latency) {
        tile_latency = current_tile_latency;
        target_tile_id = tile_id;
      }
    }

    /* global accumulation latency */
    map<int, int> numColSize;
    for (auto item : TileColSize) {
      int col = item.first;
      for (auto row : TileColSize[col]) {
        int tile_id = row * TileColumns + col;
        int numColMatrix = query_tile_accessed_columns[tile_id];
        if (numColSize[col] < numColMatrix) numColSize[col] = numColMatrix;
      }
      if (TileColSize[col].size() > 1) {
        numRead = ceil(numColSize[col]*numColPerSynapse*(numInVector/(double)numAdderTree));
        numUnitAdd = TileColSize[col].size();
        global_accumulation_latency += global_accumulation_model(numRead, numUnitAdd);
      }
    }
    ++qid;
  }

  global_buffer_latency *= 1e9;
  global_communication_latency *= 1e9;
  tile_latency *= 1e9;
  global_accumulation_latency *= 1e9;

  cost = global_buffer_latency + global_communication_latency + tile_latency + global_accumulation_latency;
  cout << endl;
  cout << "predicted cost: " << cost << " ns" << endl;
  cout << "predicted global_buffer_latency: " << (int) global_buffer_latency << " ns" << endl;
  cout << "predicted global_communication_latency: " << (int) global_communication_latency << " ns" << endl;
  cout << "predicted tile_latency: " << (int) tile_latency << " ns" << endl;
  cout << "predicted global_accumulation_latency: " << (int) global_accumulation_latency << " ns" << endl;
  if (cost_model_test) {
    costModel << cost << " " << (int) global_buffer_latency 
              << " " << (int) global_communication_latency 
              << " " << (int) tile_latency 
              << " " << (int) global_accumulation_latency << " ";
  }
  return cost;
}

void Executor::simulation(Matrix &mat, ofstream &costModel, int cost_model_test) {
  auto start = chrono::high_resolution_clock::now();
  param = new Param();
  initParam();

  /* Random number generator engine */
  InputParameter inputParameter;
  Technology tech;
  MemCell cell;
  string newWeightFile, oldWeightFile, inputFile;
  vector<vector<double>> netStructure;
  vector<double> matrix;
  matrix.push_back(1);
  matrix.push_back(1);
  matrix.push_back(mat.rows);  // rows
  matrix.push_back(1);
  matrix.push_back(1);
  matrix.push_back(mat.columns);  // columns
  matrix.push_back(0);
  matrix.push_back(1);
  netStructure.push_back(matrix);

  double maxPESizeNM = 0, maxTileSizeCM = 0, numPENM = 0;
  vector<int> markNM, pipelineSpeedUp;
  markNM.push_back(0);

  double desiredNumTileNM, desiredPESizeNM, desiredNumTileCM, desiredTileSizeCM, desiredPESizeCM;
  int numTileRow, numTileCol;

  vector<vector<double>> numTileEachLayer(2, vector<double>(2, 0)), utilizationEachLayer, speedUpEachLayer(2, vector<double>(2, 1)), tileLocaEachLayer(2, vector<double>(2, 0));

  // We can ignore the chip floor plan
  /* Customized Architecture Parameters */
  desiredPESizeCM = PE_unit_width;
  desiredTileSizeCM = desiredPESizeCM * num_row_PE_per_Tile;  // 16 PE per Tile
  numTileRow = num_row_Tile_per_Chip * param->numRowPerSynapse;
  numTileCol = num_column_Tile_per_Chip * param->numColPerSynapse;
  param->totalNumTile = numTileRow * numTileCol;
  desiredNumTileCM = param->totalNumTile;
  numTileEachLayer[0][0] = numTileRow;
  numTileEachLayer[1][0] = numTileCol;

  numTileRow = ceil(sqrt((double)(param->totalNumTile)));
  numTileCol = ceil(param->totalNumTile/(double)(numTileRow));

  ChipDesignInitializeV1(inputParameter, tech, cell, false, netStructure, &maxPESizeNM, &maxTileSizeCM, &numPENM);
  
  ChipInitializeV1(inputParameter, tech, cell, netStructure, markNM, numTileEachLayer, numPENM, desiredNumTileNM, desiredPESizeNM,
                 desiredNumTileCM, desiredTileSizeCM, desiredPESizeCM, numTileRow, numTileCol);

  double chipHeight, chipWidth, chipArea, chipAreaIC, chipAreaADC, chipAreaAccum, chipAreaOther, chipAreaArray;
  double CMTileheight = 0, CMTilewidth = 0, NMTileheight = 0, NMTilewidth = 0;

  ChipCalculateAreaV1(inputParameter, tech, cell, desiredNumTileNM, numPENM, desiredPESizeNM, desiredNumTileCM, desiredTileSizeCM,
                    desiredPESizeCM, numTileRow, &chipHeight, &chipWidth, &CMTileheight, &CMTilewidth, &NMTileheight, &NMTilewidth);

  double clkPeriod = 0;
  double layerclkPeriod = 0;

  double chipReadLatency = 0;
  double chipReadDynamicEnergy = 0;
  double chipLeakageEnergy = 0;
  double chipLeakage = 0;
  double chipbufferLatency = 0;
  double chipbufferReadDynamicEnergy = 0;
  double chipicLatency = 0;
  double chipicReadDynamicEnergy = 0;

  double chipLatencyADC = 0;
  double chipLatencyAccum = 0;
  double chipLatencyOther = 0;
  double chipEnergyADC = 0;
  double chipEnergyAccum = 0;
  double chipEnergyOther = 0;

  double layerReadLatency = 0;
  double layerReadDynamicEnergy = 0;
  double tileLeakage = 0;
  double layerbufferLatency = 0;
  double layerbufferDynamicEnergy = 0;
  double layericLatency = 0;
  double layericDynamicEnergy = 0;
  double layerMaxReadLatencyTile = 0;

  double coreLatencyADC = 0;
  double coreLatencyAccum = 0;
  double coreLatencyOther = 0;
  double coreEnergyADC = 0;
  double coreEnergyAccum = 0;
  double coreEnergyOther = 0;

  // param->synchronous = false;
  auto &PE_to_location = mat.PE_to_location;
  numTileEachLayer[0][0] = 0;
  numTileEachLayer[0][1] = 0;
  param->numUsedTiles = 0;
  /* activating Tiles and PEs */
  size_t qid = 0;
  total_latency = 0;
  int totalLatencyBuffer = 0, totalLatencyAccum = 0, totalLatencyIC = 0, totalMaxReadLatencyTile = 0;
  int totalLatencyADC = 0, totalLatencyIO = 0;
  
  // with storage_layout --> physical storage
  for (auto &accessed_PEs : accessed_PEs_list) {
    netStructure[0][2] = range_query_size[qid].first;
    netStructure[0][5] = range_query_size[qid].second;
    vector<vector<int>> PEs_list;
    for (auto PE_id : accessed_PEs)
      PEs_list.push_back(PE_to_location[PE_id]);

    auto &PE_query_size = accessed_PE_size[qid]; // int --> PE_id
    auto &tile_query_size = query_tile_size_per_pattern[qid];
    map<int, int> query_tile_accessed_rows, query_tile_accessed_columns;
    for (auto item : tile_query_size) {
      query_tile_accessed_rows[item.first] = item.second.accessed_rows.size();
      query_tile_accessed_columns[item.first] = item.second.accessed_columns.size();
    }
    ChipCalculatePerformanceV1(
      inputParameter, tech, cell, 0, newWeightFile, oldWeightFile, inputFile,
      0, netStructure, markNM, numTileEachLayer, utilizationEachLayer,
      speedUpEachLayer, tileLocaEachLayer, numPENM, desiredPESizeNM,
      desiredTileSizeCM, desiredPESizeCM, CMTileheight, CMTilewidth,
      NMTileheight, NMTilewidth, &layerReadLatency, &layerReadDynamicEnergy,
      &tileLeakage, &layerbufferLatency, &layerbufferDynamicEnergy,
      &layericLatency, &layericDynamicEnergy, &coreLatencyADC,
      &coreLatencyAccum, &coreLatencyOther, &coreEnergyADC, &coreEnergyAccum,
      &coreEnergyOther, false, &layerclkPeriod, 
      PEs_list, PE_query_size, mat.TileColumns, query_tile_accessed_rows, query_tile_accessed_columns, &layerMaxReadLatencyTile);

    if (param->synchronous) {
      layerReadLatency *= clkPeriod;
      layerbufferLatency *= clkPeriod;
      layericLatency *= clkPeriod;
      coreLatencyADC *= clkPeriod;
      coreLatencyAccum *= clkPeriod;
      coreLatencyOther *= clkPeriod;
    }
    total_latency += layerReadLatency * 1e9;
    totalLatencyBuffer += layerbufferLatency * 1e9;
    totalLatencyAccum += coreLatencyAccum * 1e9;
    totalLatencyIO += coreLatencyOther * 1e9;
    totalLatencyIC += layericLatency * 1e9;
    totalMaxReadLatencyTile += layerMaxReadLatencyTile * 1e9;
    totalLatencyADC += coreLatencyADC * 1e9;
    ++ qid;
  }
  
  auto stop = chrono::high_resolution_clock::now();
  auto duration = chrono::duration_cast<chrono::seconds>(stop - start);
  std::cout << endl;
  std::cout << "The amount of queries is : " << accessed_PEs_list.size() << endl;
  std::cout << "Total latency is : " << total_latency << " ns" << endl;
  std::cout << "Total global buffer I/O latency is : " << totalLatencyBuffer << " ns"  << endl;
  std::cout << "Total communication latency (Chip-level) is : " << totalLatencyIC << " ns"  << endl;
  std::cout << "The max latency (Tile-level) is : " << totalMaxReadLatencyTile << " ns"  << endl;
  std::cout << "Total accumulation latency (Chip-level) is : " << totalLatencyAccum << " ns" << endl;
  std::cout << "Total I/O latency is : " << totalLatencyIO << " ns"  << endl;
  std::cout << "Total run-time of NeuroSim: " << duration.count() << " seconds" << endl;
  std::cout << "Total number of used logical tiles: " << param->numUsedTiles / param->numColPerSynapse << endl;
  if (cost_model_test) {
    costModel << total_latency << " " << totalLatencyBuffer 
              << " " << totalLatencyIC 
              << " " << totalMaxReadLatencyTile 
              << " " << totalLatencyAccum << " ";
  }
  freeGlobalVar();
}