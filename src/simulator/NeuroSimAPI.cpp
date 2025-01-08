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
#include "simulator/NeuroSimAPI.h"

using namespace std;

// important ptr
Param *param;
double globalBusWidth = 0;
int numBufferCore = 0;

/*** Chip-level Modules ***/
Buffer *globalBuffer;
HTree *GhTree;
AdderTree *Gaccumulation;
Sigmoid *Gsigmoid;
BitShifter *GreLu;
MaxPooling *maxPool;

/* Tile-level Modules*/
int numInBufferCore = 0;
int numOutBufferCore = 0;										 

SubArray *subArrayInPE;
Buffer *inputBufferCM;
Buffer *outputBufferCM;
HTree *hTreeCM;
AdderTree *accumulationCM;
Sigmoid *sigmoidCM;
BitShifter *reLuCM;
Buffer *inputBufferNM;
Buffer *outputBufferNM;
HTree *hTreeNM;
AdderTree *accumulationNM;
Sigmoid *sigmoidNM;
BitShifter *reLuNM;

/* PE-level Modules*/
AdderTree *adderTreeNM;
Bus *busInputNM;
Bus *busOutputNM;
DFF *bufferInputNM;
DFF *bufferOutputNM;

AdderTree *adderTreeCM;
Bus *busInputCM;
Bus *busOutputCM;
DFF *bufferInputCM;
DFF *bufferOutputCM;

double ChipCalculatePerformanceV1(
    InputParameter &inputParameter, Technology &tech, MemCell &cell,
    int layerNumber, const string &newweightfile, const string &oldweightfile,
    const string &inputfile, bool followedByMaxPool,
    const vector<vector<double> > &netStructure, const vector<int> &markNM,
    const vector<vector<double> > &numTileEachLayer,
    const vector<vector<double> > &utilizationEachLayer,
    const vector<vector<double> > &speedUpEachLayer,
    const vector<vector<double> > &tileLocaEachLayer, double numPENM,
    double desiredPESizeNM, double desiredTileSizeCM, double desiredPESizeCM,
    double CMTileheight, double CMTilewidth, double NMTileheight,
    double NMTilewidth, double *readLatency, double *readDynamicEnergy,
    double *leakage, double *bufferLatency, double *bufferDynamicEnergy,
    double *icLatency, double *icDynamicEnergy, double *coreLatencyADC,
    double *coreLatencyAccum, double *coreLatencyOther, double *coreEnergyADC,
    double *coreEnergyAccum, double *coreEnergyOther, bool CalculateclkFreq, double *clkPeriod,
    vector<vector<int>> &PEs_list, map<int, pair<int, int>> &PE_query_size, int TileColumns
    , map<int, int> &query_tile_accessed_rows, map<int, int> &query_tile_accessed_columns, double *maxReadLatencyTile) {
  int numRowPerSynapse, numColPerSynapse;
  numRowPerSynapse = param->numRowPerSynapse; // default = 1
  numColPerSynapse = param->numColPerSynapse;

  // only get performance of single layer
  int l = layerNumber;
  // get weight matrix file Size
  int weightMatrixRow = netStructure[l][2] * netStructure[l][3] * netStructure[l][4] * numRowPerSynapse;
  int weightMatrixCol = netStructure[l][5] * numColPerSynapse;

  *readLatency = 0;
  *bufferLatency = 0;
  *icLatency = 0;
  *coreLatencyADC = 0;
  *coreLatencyAccum = 0;
  *coreLatencyOther = 0;
  *maxReadLatencyTile = 0;
  
  *readDynamicEnergy = 0;
  *leakage = 0;
  *bufferDynamicEnergy = 0;
  *icDynamicEnergy = 0;
  *coreEnergyADC = 0;
  *coreEnergyAccum = 0;
  *coreEnergyOther = 0;

  double tileLeakage = 0;

  int numInVector =
      (netStructure[l][0] - netStructure[l][3] + 1) / netStructure[l][7] *
      (netStructure[l][1] - netStructure[l][4] + 1) / netStructure[l][7];
  
  // load in whole file
  vector<vector<double> > inputVector, newMemory;

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

  int inputVectorSize = 0;
  int used_tiles = 0;
  map<int, int> numColSize;
  for (auto item : TileColSize) {
    int col = item.first;
    used_tiles += TileColSize[col].size() * numColPerSynapse;
    for (auto row : TileColSize[col]) {
      double tileReadLatency = 0;
      double tileReadDynamicEnergy = 0;
      double tilebufferLatency = 0;
      double tilebufferDynamicEnergy = 0;
      double tileicLatency = 0;
      double tileicDynamicEnergy = 0;
      double tileLatencyADC = 0;
      double tileLatencyAccum = 0;
      double tileLatencyOther = 0;
      double tileEnergyADC = 0;
      double tileEnergyAccum = 0;
      double tileEnergyOther = 0;

      int tile_id = row * TileColumns + col;
      int numRowMatrix = query_tile_accessed_rows[tile_id];
      int numColMatrix = query_tile_accessed_columns[tile_id];
      inputVectorSize += numRowMatrix;
      if (numColSize[col] < numColMatrix) numColSize[col] = numColMatrix;

      // assign weight and input to specific tile
      vector<vector<double> > tileMemory, tileInput;
      tileMemory = CopyArrayV1(newMemory, 0, 0, numRowMatrix, numColMatrix);
      tileInput = CopyInputV1(inputVector, 0, numInVector*param->numBitInput, numColMatrix);

      TileCalculatePerformanceV1(
          tileMemory, tileMemory, tileInput, markNM[l],
          ceil((double)desiredTileSizeCM / (double)desiredPESizeCM),
          desiredPESizeCM, speedUpEachLayer[0][l], speedUpEachLayer[1][l],
          numRowMatrix, numColMatrix, numInVector * param->numBitInput, cell,
          &tileReadLatency, &tileReadDynamicEnergy, &tileLeakage,
          &tilebufferLatency, &tilebufferDynamicEnergy, &tileicLatency,
          &tileicDynamicEnergy, &tileLatencyADC, &tileLatencyAccum,
          &tileLatencyOther, &tileEnergyADC, &tileEnergyAccum,
          &tileEnergyOther, CalculateclkFreq, clkPeriod, PE_map[tile_id], PE_query_size);

      *readLatency = MAX(tileReadLatency, (*readLatency));
      // *bufferLatency = MAX(tilebufferLatency, (*bufferLatency));
      // *icLatency = MAX(tileicLatency, (*icLatency));

      *coreLatencyADC = MAX(tileLatencyADC, (*coreLatencyADC));
      // *coreLatencyAccum = MAX(tileLatencyAccum, (*coreLatencyAccum));
      *coreLatencyOther = MAX(tileLatencyOther, (*coreLatencyOther));
    }
  }

  *maxReadLatencyTile = *readLatency;
  param->numUsedTiles += used_tiles;

  if (!CalculateclkFreq) {
    for (auto item : TileColSize) {
      int col = item.first;
      if (TileColSize[col].size() > 1) {
        Gaccumulation->CalculateLatency(ceil(numColSize[col]*numColPerSynapse*(numInVector/(double)Gaccumulation->numAdderTree)), TileColSize[col].size(), 0);
        *readLatency += Gaccumulation->readLatency;
        *coreLatencyAccum += Gaccumulation->readLatency;
      }
    }
    double numBitToLoadOut = inputVectorSize * param->numBitInput * numInVector;
    double numBitToLoadIn = ceil(weightMatrixCol / (double)param->numColPerSynapse) * param->numBitInput * numInVector / (netStructure[l][6] ? 4 : 1);
    GhTree->CalculateLatency(0, 0, tileLocaEachLayer[0][l], tileLocaEachLayer[1][l], CMTileheight, CMTilewidth , ceil((numBitToLoadOut + numBitToLoadIn) / GhTree->busWidth));
    
    globalBuffer->CalculateLatency(globalBuffer->interface_width, numBitToLoadOut / globalBuffer->interface_width , globalBuffer->interface_width, numBitToLoadIn / globalBuffer->interface_width);

    // since multi-core buffer has improve the parallelism
    // default: the num of Buffer core is 1
    globalBuffer->readLatency /= MIN(numBufferCore, ceil(globalBusWidth/globalBuffer->interface_width));
    globalBuffer->writeLatency /= MIN(numBufferCore, ceil(globalBusWidth/globalBuffer->interface_width));
    
    *bufferLatency += globalBuffer->readLatency + globalBuffer->writeLatency;
    *icLatency += GhTree->readLatency;

    *readLatency += globalBuffer->readLatency + globalBuffer->writeLatency + GhTree->readLatency;
    *coreLatencyOther += globalBuffer->readLatency + globalBuffer->writeLatency + GhTree->readLatency;
  }
  return 0;
}

vector<int> ChipDesignInitializeV1(InputParameter &inputParameter, Technology &tech, MemCell &cell, bool pip, 
                                 const vector<vector<double> > &netStructure, double *maxPESizeNM, 
                                 double *maxTileSizeCM, double *numPENM) {
  // Chip-level Modules                             
  globalBuffer = new Buffer(inputParameter, tech, cell);
  GhTree = new HTree(inputParameter, tech, cell);
  Gaccumulation = new AdderTree(inputParameter, tech, cell);

  int numRowPerSynapse, numColPerSynapse;
  numRowPerSynapse = param->numRowPerSynapse;
  numColPerSynapse = param->numColPerSynapse;

  double numLayer, minCube;

  // get information of network structure
  numLayer = netStructure.size();
  *maxPESizeNM = 0;
  *maxTileSizeCM = 0;
  *numPENM = 0;

  vector<int> markNM;
  // all layers use conventional mapping
  for (int i = 0; i < numLayer; i++) {
    markNM.push_back(0);
    minCube = pow(2, ceil((double) log2((double) netStructure[i][5]*(double) numColPerSynapse)));
    *maxTileSizeCM = max(minCube, (*maxTileSizeCM));
  }
  return markNM;
}

void ChipInitializeV1(InputParameter &inputParameter, Technology &tech,
                    MemCell &cell, const vector<vector<double> > &netStructure,
                    const vector<int> &markNM,
                    const vector<vector<double> > &numTileEachLayer,
                    double numPENM, double desiredNumTileNM,
                    double desiredPESizeNM, double desiredNumTileCM,
                    double desiredTileSizeCM, double desiredPESizeCM,
                    int numTileRow, int numTileCol) {
  /*** Initialize Tile ***/
  TileInitialize(inputParameter, tech, cell, numPENM, desiredPESizeNM, ceil((double)(desiredTileSizeCM) / (double)(desiredPESizeCM)), desiredPESizeCM);

  // find max layer and define the global buffer: enough to hold the max layer inputs
  double maxLayerInput = num_row_Tile_per_Chip * desiredTileSizeCM; // fixed
  globalBusWidth = (desiredTileSizeCM) + (desiredTileSizeCM) / param->numColMuxed;
  // find max # tiles needed to be added at the same time
  double maxTileAdded = 0;
  if (numTileEachLayer[0][0] > maxTileAdded) {
    maxTileAdded = numTileEachLayer[0][0];
  }
  // have to limit the global bus width --> cannot grow dramatically with num of tile
  while (globalBusWidth > param->maxGlobalBusWidth) {
    globalBusWidth /= 2;
  }
  // define bufferSize for inference operation
  int bufferSize = param->numBitInput * maxLayerInput;

  // globalBuffer->Initialize(param->numBitInput*maxLayerInput, globalBusWidth, 1, param->unitLengthWireResistance, param->clkFreq, param->globalBufferType);
  numBufferCore = ceil((double)bufferSize / ((double)param->globalBufferCoreSizeRow * (double)param->globalBufferCoreSizeCol));
  numBufferCore = 1; // default: 1
  // numBufferCore = ceil(1.5*numBufferCore);
  globalBuffer->Initialize((param->globalBufferCoreSizeRow * param->globalBufferCoreSizeCol), param->globalBufferCoreSizeCol, 
      1, param->unitLengthWireResistance, param->clkFreq, param->globalBufferType);

  // maxPool->Initialize(param->numBitInput, 2*2, (desiredTileSizeCM), param->clkFreq);
  GhTree->Initialize((numTileRow), (numTileCol), param->globalBusDelayTolerance, globalBusWidth, param->clkFreq);

  if (param->parallelRead) {
    Gaccumulation->Initialize((int)maxTileAdded, param->numBitInput, ceil((double)(desiredTileSizeCM) / (double)param->numColMuxed), param->clkFreq);
  } else {
    Gaccumulation->Initialize((int)maxTileAdded, param->numBitInput, ceil((double)(desiredTileSizeCM) / (double)param->numColMuxed), param->clkFreq);
  }
}

vector<double> ChipCalculateAreaV1(
    InputParameter &inputParameter, Technology &tech, MemCell &cell,
    double desiredNumTileNM, double numPENM, double desiredPESizeNM,
    double desiredNumTileCM, double desiredTileSizeCM, double desiredPESizeCM,
    int numTileRow, double *height, double *width, double *CMTileheight,
    double *CMTilewidth, double *NMTileheight, double *NMTilewidth) {
  vector<double> areaResults;

  double area = 0;
  double areaIC = 0;
  double areaADC = 0;
  double areaAccum = 0;
  double areaOther = 0;
  double areaArray = 0;

  double NMheight = 0;
  double NMwidth = 0;
  double CMheight = 0;
  double CMwidth = 0;

  *NMTileheight = 0;
  *NMTilewidth = 0;
  *CMTileheight = 0;
  *CMTilewidth = 0;
  *height = 0;
  *width = 0;

  vector<double> areaCMTile;
  vector<double> areaNMTile;

  if (param->novelMapping) {
    areaNMTile = TileCalculateArea(numPENM, desiredPESizeNM, true, &NMheight, &NMwidth);
    double NMTileArea = areaNMTile[0];
    double NMTileAreaIC = areaNMTile[1];
    double NMTileAreaADC = areaNMTile[2];
    double NMTileAreaAccum = areaNMTile[3];
    double NMTileAreaOther = areaNMTile[4];
    double NMTileAreaArray = areaNMTile[5];
    area += NMTileArea * desiredNumTileNM;
    areaIC += NMTileAreaIC * desiredNumTileNM;
    areaADC += NMTileAreaADC * desiredNumTileNM;
    areaAccum += NMTileAreaAccum * desiredNumTileNM;
    areaOther += NMTileAreaOther * desiredNumTileNM;
    areaArray += NMTileAreaArray * desiredNumTileNM;
    *NMTileheight = NMheight;
    *NMTilewidth = NMwidth;
  }
  areaCMTile = TileCalculateArea(pow(ceil((double)desiredTileSizeCM / (double)desiredPESizeCM), 2), desiredPESizeCM, false, &CMheight, &CMwidth);

  *CMTileheight = CMheight;
  *CMTilewidth = CMwidth;

  // global buffer is made up by multiple cores
  globalBuffer->CalculateArea(numTileRow * max(NMheight, CMheight), NULL, NONE);
  double globalBufferArea = globalBuffer->area * numBufferCore;
  double globalBufferHeight = numTileRow * max(NMheight, CMheight);
  double globalBufferWidth = globalBufferArea / globalBufferHeight;
  GhTree->CalculateArea(max(NMheight, CMheight), max(NMwidth, CMwidth), param->treeFoldedRatio);
  // maxPool->CalculateUnitArea(NONE);
  // maxPool->CalculateArea(globalBufferWidth);
  Gaccumulation->CalculateArea(NULL, globalBufferHeight / 3, NONE);

  double areaGreLu = 0;
  double areaGsigmoid = 0;

  area += globalBufferArea + GhTree->area + Gaccumulation->area;
  areaIC += GhTree->area;

  *height = sqrt(area);
  *width = area/(*height);

  return areaResults;
}

void TileCalculatePerformanceV1(const vector<vector<double> > &newMemory, const vector<vector<double> > &oldMemory
  , const vector<vector<double> > &inputVector, int novelMap, double numPE, double peSize, int speedUpRow
  , int speedUpCol, int weightMatrixRow, int weightMatrixCol, int numInVector, MemCell& cell, double *readLatency
  , double *readDynamicEnergy, double *leakage, double *bufferLatency, double *bufferDynamicEnergy, double *icLatency
  , double *icDynamicEnergy, double *coreLatencyADC, double *coreLatencyAccum, double *coreLatencyOther
  , double *coreEnergyADC, double *coreEnergyAccum, double *coreEnergyOther, bool CalculateclkFreq, double*clkPeriod
  , vector<vector<int>> &PE_map, map<int, pair<int, int>> &PE_query_size) {
  
	/*** sweep PE ***/
	int numRowPerSynapse, numColPerSynapse;
	numRowPerSynapse = param->numRowPerSynapse;
	numColPerSynapse = param->numColPerSynapse;
	double PEreadLatency, PEreadDynamicEnergy, PEleakage, PEbufferLatency, PEbufferDynamicEnergy, PEicLatency, PEicDynamicEnergy;
	double peLatencyADC, peLatencyAccum, peLatencyOther, peEnergyADC, peEnergyAccum, peEnergyOther;
	int numSubArrayRow = ceil((double)peSize/(double)param->numRowSubArray);
	int numSubArrayCol = ceil((double)peSize/(double)param->numColSubArray);
	
	*readLatency = 0;
	*bufferLatency = 0;
	*icLatency = 0;
	*coreLatencyADC = 0;
	*coreLatencyAccum = 0;
	*coreLatencyOther = 0;

	*readDynamicEnergy = 0;
	*leakage = 0;
	*bufferDynamicEnergy = 0;
	*icDynamicEnergy = 0;
	*coreEnergyADC = 0;
	*coreEnergyAccum = 0;
	*coreEnergyOther = 0;

  set<int> PE_rows;
  int inputVectorSize = 0;
  for (auto xbar : PE_map) {
    PE_rows.insert(xbar[0]);
    // assign weight and input to specific tile
    int numRowMatrix = PE_query_size[xbar[2]].first; // xbar[2] is the id of xbar
    int numColMatrix = PE_query_size[xbar[2]].second;
    inputVectorSize += numRowMatrix;
    vector<vector<double>> pEMemory, pEInput;
    pEMemory = CopyPEArrayV1(newMemory, 0, 0, numRowMatrix, numColMatrix);
    pEInput = CopyPEInputV1(inputVector, 0, numInVector, numRowMatrix);
    
    ProcessingUnitCalculatePerformanceV1(subArrayInPE, pEMemory, pEMemory, pEInput, 1, 1, numSubArrayRow, numSubArrayCol, numRowMatrix,
                numColMatrix, numInVector, cell, false, &PEreadLatency, &PEreadDynamicEnergy, &PEleakage,
                &PEbufferLatency, &PEbufferDynamicEnergy, &PEicLatency, &PEicDynamicEnergy,
                &peLatencyADC, &peLatencyAccum, &peLatencyOther, &peEnergyADC, &peEnergyAccum, &peEnergyOther, CalculateclkFreq, clkPeriod);
    
    *readLatency = max(PEreadLatency, (*readLatency));
    *bufferLatency = max(PEbufferLatency, (*bufferLatency));
    *icLatency = max(PEicLatency,(*icLatency));
    
    *coreLatencyADC = MAX(peLatencyADC, (*coreLatencyADC));
    *coreLatencyAccum = MAX(peLatencyAccum, (*coreLatencyAccum));
    *coreLatencyOther = MAX(peLatencyOther, (*coreLatencyOther));
  }

  accumulationCM->CalculateLatency(((double)numInVector/param->numBitInput)*ceil((double)param->numColMuxed/(double)param->numColPerSynapse), PE_rows.size(), 0);
  *readLatency += accumulationCM->readLatency;
  *coreLatencyAccum += accumulationCM->readLatency;

  if(!CalculateclkFreq) {
    double numBitToLoadOut, numBitToLoadIn;
    numBitToLoadIn = MAX(ceil(weightMatrixCol/param->numColPerSynapse)*(1+accumulationCM->numAdderBit)*numInVector/param->numBitInput, 0);
    numBitToLoadOut = MAX(inputVectorSize*numInVector, 0);
    outputBufferCM->CalculateLatency(outputBufferCM->interface_width, numBitToLoadIn/outputBufferCM->interface_width, outputBufferCM->interface_width, numBitToLoadIn/outputBufferCM->interface_width);
    // considering buffer activation: no matter speedup or not, the total number of data transferred is fixed
    inputBufferCM->CalculateLatency(inputBufferCM->interface_width, numBitToLoadOut/inputBufferCM->interface_width, inputBufferCM->interface_width, numBitToLoadOut/inputBufferCM->interface_width);
    // since multi-core buffer has improve the parallelism
    numInBufferCore = numOutBufferCore = 1; // set to 1
    inputBufferCM->readLatency /= MIN(numInBufferCore, ceil(hTreeCM->busWidth/inputBufferCM->interface_width));
    inputBufferCM->writeLatency /= MIN(numInBufferCore, ceil(hTreeCM->busWidth/inputBufferCM->interface_width));
    outputBufferCM->readLatency /= MIN(numOutBufferCore, ceil(hTreeCM->busWidth/outputBufferCM->interface_width));
    outputBufferCM->writeLatency /= MIN(numOutBufferCore, ceil(hTreeCM->busWidth/outputBufferCM->interface_width));																							   
    
    *readLatency += (inputBufferCM->readLatency + inputBufferCM->writeLatency);
    *readLatency += (outputBufferCM->readLatency + outputBufferCM->writeLatency);
    
    // used to define travel distance
    double PEheight, PEwidth, PEbufferArea;
    int numSubArray = ceil((double) peSize/(double) param->numRowSubArray)*ceil((double) peSize/(double) param->numColSubArray);
    vector<double> PEarea;
    PEarea = ProcessingUnitCalculateArea(subArrayInPE, ceil((double)sqrt((double)numSubArray)), ceil((double)sqrt((double)numSubArray)), false, &PEheight, &PEwidth, &PEbufferArea);
    hTreeCM->CalculateLatency(NULL, NULL, NULL, NULL, PEheight, PEwidth, ceil((numBitToLoadOut+numBitToLoadIn)/hTreeCM->busWidth));

    *readLatency += hTreeCM->readLatency;
    *bufferLatency += (inputBufferCM->readLatency + outputBufferCM->readLatency + inputBufferCM->writeLatency + outputBufferCM->writeLatency);
    *icLatency += hTreeCM->readLatency;
    *coreLatencyOther += (inputBufferCM->readLatency + inputBufferCM->writeLatency + outputBufferCM->readLatency + outputBufferCM->writeLatency + hTreeCM->readLatency);
  }
}

vector<vector<double> > CopyPEArrayV1(const vector<vector<double> > &orginal, int positionRow, int positionCol, int numRow, int numCol) {
	vector<vector<double> > copy(numRow, vector<double>(numCol, 1));
	return copy;
} 

vector<vector<double> > CopyPEInputV1(const vector<vector<double> > &orginal, int positionRow, int numInputVector, int numRow) {
  vector<vector<double> > copy(numRow, vector<double>(numInputVector, 1));
	return copy;
}


vector<vector<double> > CopyArrayV1(const vector<vector<double> > &orginal, int positionRow, int positionCol, int numRow, int numCol) {
  vector<vector<double> > copy(numRow, vector<double>(numCol, 1));
  return copy;
}

vector<vector<double> > CopyInputV1(const vector<vector<double> > &orginal, int positionRow, int numInputVector, int numRow) {
	vector<vector<double> > copy(numRow, vector<double>(numInputVector, 1));
  return copy;
}

vector<vector<double> > CopySubArrayV1(const vector<vector<double> > &orginal, int positionRow, int positionCol, int numRow, int numCol) {
	vector<vector<double> > copy(numRow, vector<double>(numCol, 1));
	return copy;
} 

vector<vector<double> > CopySubInputV1(const vector<vector<double> > &orginal, int positionRow, int numInputVector, int numRow) {
	vector<vector<double> > copy(numRow, vector<double>(numInputVector, 1));
	return copy;
}

double ProcessingUnitCalculatePerformanceV1(SubArray *subArray, const vector<vector<double> > &newMemory, const vector<vector<double> > &oldMemory, 
											const vector<vector<double> > &inputVector,
											int arrayDupRow, int arrayDupCol, int numSubArrayRow, int numSubArrayCol, int weightMatrixRow,
											int weightMatrixCol, int numInVector, MemCell& cell, bool NMpe, double *readLatency, double *readDynamicEnergy, double *leakage, 
											double *bufferLatency, double *bufferDynamicEnergy, double *icLatency, double *icDynamicEnergy,
											double *coreLatencyADC, double *coreLatencyAccum, double *coreLatencyOther, double *coreEnergyADC, 
											double *coreEnergyAccum, double *coreEnergyOther, bool CalculateclkFreq, double *clkPeriod) {
	
	/*** define how many subArray are used to map the whole layer ***/
	*readLatency = 0;
	*bufferLatency = 0;
	*icLatency = 0;
	*coreLatencyADC = 0;
	*coreLatencyAccum = 0;
	*coreLatencyOther = 0;

	*readDynamicEnergy = 0;
	*leakage = 0;
	*bufferDynamicEnergy = 0;
	*icDynamicEnergy = 0;
	*coreEnergyADC = 0;
	*coreEnergyAccum = 0;
	*coreEnergyOther = 0;
	double maxSubarrayWriteLatency = 0;
  double maxSubarrayReadLatency = 0;
	double subArrayReadLatency, subArrayReadDynamicEnergy, subArrayLeakage, subArrayLatencyADC, subArrayLatencyAccum, subArrayLatencyOther;
  // weight matrix is further partitioned inside PE (among subArray) --> no duplicated
  for (int i = 0; i < numSubArrayRow/*ceil((double) weightMatrixRow/(double) param->numRowSubArray)*/; i++) {
    for (int j = 0; j < numSubArrayCol/*ceil((double) weightMatrixCol/(double) param->numColSubArray)*/; j++) {
      if ((i*param->numRowSubArray < weightMatrixRow) && (j*param->numColSubArray < weightMatrixCol) && (i*param->numRowSubArray < weightMatrixRow)) {
        int numRowMatrix = min(param->numRowSubArray, weightMatrixRow-i*param->numRowSubArray);
        int numColMatrix = min(param->numColSubArray, weightMatrixCol-j*param->numColSubArray);
        // assign weight and input to specific subArray
        vector<vector<double> > subArrayMemory, subArrayInput;
        subArrayMemory = CopySubArrayV1(newMemory, i*param->numRowSubArray, j*param->numColSubArray, numRowMatrix, numColMatrix);
        subArrayInput = CopySubInputV1(inputVector, i*param->numRowSubArray, numInVector, numRowMatrix);

        subArrayReadLatency = 0;
        subArrayLatencyADC = 0;
        subArrayLatencyAccum = 0;
        subArrayLatencyOther = 0;
        
        for (int k = 0; k < numInVector; k++) {                 // calculate single subArray through the total input vectors
          double activityRowRead = 0;
          vector<double> input;
          input = GetInputVector(subArrayInput, k, &activityRowRead);
          subArray->activityRowRead = activityRowRead;
          int cellRange = pow(2, param->cellBit);

          if (param->parallelRead) {
            subArray->levelOutput = param->levelOutput;               // # of levels of the multilevelSenseAmp output
          } else {
            subArray->levelOutput = cellRange;
          }
          
          vector<double> columnResistance;
          columnResistance = GetColumnResistance(input, subArrayMemory, cell, param->parallelRead, subArray->resCellAccess);
          subArray->CalculateLatency(1e20, columnResistance, CalculateclkFreq);
          // if (subArray->writeLatency > maxSubarrayWriteLatency) maxSubarrayWriteLatency = subArray->writeLatency;
          // if (subArray->readLatency > maxSubarrayReadLatency) maxSubarrayReadLatency = subArray->readLatency;

          if(CalculateclkFreq && (*clkPeriod < subArray->readLatency)) {
            *clkPeriod = subArray->readLatency;					//clk freq is decided by the longest sensing latency
          }
          
          if(!CalculateclkFreq) {
            subArrayLatencyADC += subArray->readLatencyADC;			//sensing cycle
            subArrayLatencyAccum += subArray->readLatencyAccum;		//#cycles
            subArrayReadLatency += subArray->readLatency;		//#cycles + sensing cycle
            subArrayLatencyOther += subArray->readLatencyOther;
          }
        }

        *readLatency = MAX(subArrayReadLatency, (*readLatency));
        *coreLatencyADC = MAX(subArrayLatencyADC, (*coreLatencyADC));
        *coreLatencyAccum = MAX(subArrayLatencyAccum, (*coreLatencyAccum));
        *coreLatencyOther = MAX(subArrayLatencyOther, (*coreLatencyOther));
      }
    }
  }
  if (NMpe) { // default: false
    adderTreeNM->CalculateLatency((int)(numInVector/param->numBitInput)*ceil(param->numColMuxed/param->numColPerSynapse), ceil((double) weightMatrixRow/(double) param->numRowSubArray), 0);
    *readLatency += adderTreeNM->readLatency;
    *coreLatencyAccum += adderTreeNM->readLatency;
  } else {
    adderTreeCM->CalculateLatency((int)(numInVector/param->numBitInput)*ceil(param->numColMuxed/param->numColPerSynapse), ceil((double) weightMatrixRow/(double) param->numRowSubArray), 0);
    *readLatency += adderTreeCM->readLatency;
    *coreLatencyAccum += adderTreeCM->readLatency;
  }
		
	if(!CalculateclkFreq) {
		//considering buffer activation: no matter speedup or not, the total number of data transferred is fixed
		// input buffer: total num of data loaded in = weightMatrixRow*numInVector
		// output buffer: total num of data transferred = weightMatrixRow*numInVector/param->numBitInput (total num of IFM in the PE) *adderTree->numAdderTree*adderTree->numAdderBit (bit precision of OFMs) 
		if (NMpe) {
			bufferInputNM->CalculateLatency(0, weightMatrixRow/param->numRowPerSynapse*numInVector/(bufferInputNM->numDff));
			bufferOutputNM->CalculateLatency(0, weightMatrixCol/param->numColPerSynapse*adderTreeNM->numAdderBit*numInVector/param->numBitInput/(bufferOutputNM->numDff));
			busInputNM->CalculateLatency(weightMatrixRow/param->numRowPerSynapse*numInVector/(busInputNM->busWidth)); 
			if (param->parallelRead) {
				busOutputNM->CalculateLatency((weightMatrixCol/param->numColPerSynapse*log2((double)param->levelOutput)*numInVector/param->numBitInput)/(busOutputNM->numRow*busOutputNM->busWidth));
			} else {
				busOutputNM->CalculateLatency((weightMatrixCol/param->numColPerSynapse*(log2((double)param->numRowSubArray)+param->cellBit-1)*numInVector/param->numBitInput)/(busOutputNM->numRow*busOutputNM->busWidth));
			}
			*bufferLatency = bufferInputNM->readLatency + bufferOutputNM->readLatency;	//considered in ic
			if (!param->synchronous) {
				*icLatency = busInputNM->readLatency + busOutputNM->readLatency;	
			}				
		} else {
			bufferInputCM->CalculateLatency(0, weightMatrixRow/param->numRowPerSynapse*numInVector/(bufferInputCM->numDff));
			bufferOutputCM->CalculateLatency(0, weightMatrixCol/param->numColPerSynapse*adderTreeCM->numAdderBit*numInVector/param->numBitInput/(bufferOutputCM->numDff));
			busInputCM->CalculateLatency(weightMatrixRow/param->numRowPerSynapse*numInVector/(busInputCM->busWidth)); 
			if (param->parallelRead) {
				busOutputCM->CalculateLatency((weightMatrixCol/param->numColPerSynapse*log2((double)param->levelOutput)*numInVector/param->numBitInput)/(busOutputCM->numRow*busOutputCM->busWidth));
			} else {
				busOutputCM->CalculateLatency((weightMatrixCol/param->numColPerSynapse*(log2((double)param->numRowSubArray)+param->cellBit-1)*numInVector/param->numBitInput)/(busOutputCM->numRow*busOutputCM->busWidth));
			}
			*bufferLatency = bufferInputCM->readLatency + bufferOutputCM->readLatency;	//considered in ic
			if (!param->synchronous) {
				*icLatency = busInputCM->readLatency + busOutputCM->readLatency;	
			}
		}
		*readLatency += (*bufferLatency) + (*icLatency);
		*coreLatencyOther += (*bufferLatency) + (*icLatency);	
	}

  // cout << "maxSubarrayWriteLatency: " << maxSubarrayWriteLatency << endl;
  // cout << "maxSubarrayReadLatency: " << maxSubarrayReadLatency << endl;
	return 0;
}

void initParam() {
  // define weight/input/memory precision from wrapper
  param->synapseBit = 16;   // precision of synapse weight
  param->numBitInput = 16;  // precision of input neural activation
  if (param->cellBit > param->synapseBit) {
    cout << "ERROR!: Memory precision is even higher than synapse precision, please modify 'cellBit' in Param.cpp!" << endl;
    param->cellBit = param->synapseBit;
  }

  /*** initialize operationMode as default ***/
  param->conventionalParallel = 0;
  param->conventionalSequential = 0;
  param->BNNparallelMode = 0;     // parallel BNN
  param->BNNsequentialMode = 0;   // sequential BNN
  param->XNORsequentialMode = 0;  // Use several multi-bit RRAM as one synapse
  param->XNORparallelMode = 0;    // Use several multi-bit RRAM as one synapse
  switch (param->operationmode) {
    case 6:
      param->XNORparallelMode = 1;
      break;
    case 5:
      param->XNORsequentialMode = 1;
      break;
    case 4:
      param->BNNparallelMode = 1;
      break;
    case 3:
      param->BNNsequentialMode = 1;
      break;
    case 2:
      param->conventionalParallel = 1;
      break;
    case 1:
      param->conventionalSequential = 1;
      break;
    case -1:
      break;
    default:
      exit(-1);
  }

  if (param->XNORparallelMode || param->XNORsequentialMode) {
    param->numRowPerSynapse = 2;
  } else {
    param->numRowPerSynapse = 1;
  }
  if (param->BNNparallelMode) {
    param->numColPerSynapse = 2;
  } else if (param->XNORparallelMode || param->XNORsequentialMode || param->BNNsequentialMode) {
    param->numColPerSynapse = 1;
  } else {
    param->numColPerSynapse = ceil((double)param->synapseBit / (double)param->cellBit);
  }
}

double matrix_writing(int non_zero, int numTiles) {
  // Since the global buffer can put down one tile of matrix data, we calculate the matrix writing time by writing one tile at a time.
  // Since the global buffer can store the matrix data of all subarrays in a tile, NeuroSim can write to all subarrays in parallel.
  // the size of a value is 4 bytes, and then CIM quantize values to 2 bytes
  double main_memory_to_cim_bandwith = 32 * 1024 * 1024 * 1024; // 32 GB/sec
  double global_buffer_to_tiles_bandwith = 2.27 * 1024 * 1024 * 1024; // 2.27 GB/sec
  double tile_to_PEs_bandwith = 5.26 * 1024 * 1024 * 1024; // 2.27 GB/sec
  double subarray_write_latency = 93.09; // ns, refer to NeuroSim V2.1
  double latency = numTiles * subarray_write_latency / 1e9 + (non_zero * 4.0 / main_memory_to_cim_bandwith) + (non_zero * 2.0 / global_buffer_to_tiles_bandwith) + (non_zero * 2.0 / tile_to_PEs_bandwith);
}

void freeGlobalVar() {
// important ptr
delete param;
globalBusWidth = 0;
numBufferCore = 0;

/*** Chip-level Modules ***/
delete globalBuffer;
delete GhTree;
delete Gaccumulation;
// delete Gsigmoid;
// delete GreLu;
// delete maxPool;

/* Tile-level Modules*/
numInBufferCore = 0;
numOutBufferCore = 0;										 

delete subArrayInPE;
delete inputBufferCM;
delete outputBufferCM;
delete hTreeCM;
delete accumulationCM;
// delete sigmoidCM;
// delete reLuCM;
delete inputBufferNM;
delete outputBufferNM;
delete hTreeNM;
delete accumulationNM;
// delete sigmoidNM;
// delete reLuNM;

/* PE-level Modules*/
delete adderTreeNM;
delete busInputNM;
delete busOutputNM;
delete bufferInputNM;
delete bufferOutputNM;

delete adderTreeCM;
delete busInputCM;
delete busOutputCM;
delete bufferInputCM;
delete bufferOutputCM;
}