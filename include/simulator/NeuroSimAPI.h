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

#ifndef __NEUROSIM_API__
#define __NEUROSIM_API__

#include "array/Statement.h"

/* NeuroSim */
#include "Bus.h"
#include "Chip.h"
#include "Param.h"
#include "ProcessingUnit.h"
#include "SubArray.h"
#include "Tile.h"
#include "constant.h"
#include "formula.h"

using namespace std;

double ChipCalculatePerformanceV1(
    InputParameter &inputParameter, Technology &tech, MemCell &cell,
    int layerNumber, const string &newweightfile, const string &oldweightfile,
    const string &inputfile, bool followedByMaxPool,
    const vector<vector<double>> &netStructure, const vector<int> &markNM,
    const vector<vector<double>> &numTileEachLayer,
    const vector<vector<double>> &utilizationEachLayer,
    const vector<vector<double>> &speedUpEachLayer,
    const vector<vector<double>> &tileLocaEachLayer, double numPENM,
    double desiredPESizeNM, double desiredTileSizeCM, double desiredPESizeCM,
    double CMTileheight, double CMTilewidth, double NMTileheight,
    double NMTilewidth, double *readLatency, double *readDynamicEnergy,
    double *leakage, double *bufferLatency, double *bufferDynamicEnergy,
    double *icLatency, double *icDynamicEnergy, double *coreLatencyADC,
    double *coreLatencyAccum, double *coreLatencyOther, double *coreEnergyADC,
    double *coreEnergyAccum, double *coreEnergyOther, bool CalculateclkFreq,
    double *clkPeriod, vector<vector<int>> &PEs_list,
    map<int, pair<int, int>> &PE_query_size, int TileColumns,
    map<int, int> &query_tile_accessed_rows, map<int, int> &query_tile_accessed_columns, double *maxReadLatencyTile);

vector<int> ChipDesignInitializeV1(InputParameter &inputParameter,
                                   Technology &tech, MemCell &cell, bool pip,
                                   const vector<vector<double>> &netStructure,
                                   double *maxPESizeNM, double *maxTileSizeCM,
                                   double *numPENM);

void ChipInitializeV1(InputParameter &inputParameter, Technology &tech,
                      MemCell &cell, const vector<vector<double>> &netStructure,
                      const vector<int> &markNM,
                      const vector<vector<double>> &numTileEachLayer,
                      double numPENM, double desiredNumTileNM,
                      double desiredPESizeNM, double desiredNumTileCM,
                      double desiredTileSizeCM, double desiredPESizeCM,
                      int numTileRow, int numTileCol);

vector<double> ChipCalculateAreaV1(
    InputParameter &inputParameter, Technology &tech, MemCell &cell,
    double desiredNumTileNM, double numPENM, double desiredPESizeNM,
    double desiredNumTileCM, double desiredTileSizeCM, double desiredPESizeCM,
    int numTileRow, double *height, double *width, double *CMTileheight,
    double *CMTilewidth, double *NMTileheight, double *NMTilewidth);

void TileCalculatePerformanceV1(
    const vector<vector<double>> &newMemory,
    const vector<vector<double>> &oldMemory,
    const vector<vector<double>> &inputVector, int novelMap, double numPE,
    double peSize, int speedUpRow, int speedUpCol, int weightMatrixRow,
    int weightMatrixCol, int numInVector, MemCell &cell, double *readLatency,
    double *readDynamicEnergy, double *leakage, double *bufferLatency,
    double *bufferDynamicEnergy, double *icLatency, double *icDynamicEnergy,
    double *coreLatencyADC, double *coreLatencyAccum, double *coreLatencyOther,
    double *coreEnergyADC, double *coreEnergyAccum, double *coreEnergyOther,
    bool CalculateclkFreq, double *clkPeriod, vector<vector<int>> &PE_map,
    map<int, pair<int, int>> &PE_query_size);

double ProcessingUnitCalculatePerformanceV1(
    SubArray *subArray, const vector<vector<double>> &newMemory,
    const vector<vector<double>> &oldMemory,
    const vector<vector<double>> &inputVector, int arrayDupRow, int arrayDupCol,
    int numSubArrayRow, int numSubArrayCol, int weightMatrixRow,
    int weightMatrixCol, int numInVector, MemCell &cell, bool NMpe,
    double *readLatency, double *readDynamicEnergy, double *leakage,
    double *bufferLatency, double *bufferDynamicEnergy, double *icLatency,
    double *icDynamicEnergy, double *coreLatencyADC, double *coreLatencyAccum,
    double *coreLatencyOther, double *coreEnergyADC, double *coreEnergyAccum,
    double *coreEnergyOther, bool CalculateclkFreq, double *clkPeriod);

vector<vector<double>> CopyArrayV1(const vector<vector<double>> &orginal,
                                   int positionRow, int positionCol, int numRow,
                                   int numCol);
vector<vector<double>> CopyInputV1(const vector<vector<double>> &orginal,
                                   int positionRow, int numInputVector,
                                   int numRow);
vector<vector<double>> CopyPEArrayV1(const vector<vector<double>> &orginal,
                                     int positionRow, int positionCol,
                                     int numRow, int numCol);
vector<vector<double>> CopyPEInputV1(const vector<vector<double>> &orginal,
                                     int positionRow, int numInputVector,
                                     int numRow);
vector<vector<double>> CopySubArrayV1(const vector<vector<double>> &orginal,
                                      int positionRow, int positionCol,
                                      int numRow, int numCol);
vector<vector<double>> CopySubInputV1(const vector<vector<double>> &orginal,
                                      int positionRow, int numInputVector,
                                      int numRow);

void initParam();
void freeGlobalVar();
#endif