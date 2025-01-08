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

#include <boost/filesystem.hpp>
#include <chrono>
#include <ctime>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include "storage/Storage.h"
// #include "query/Executor.h"
// #include "query/Operator.h"

using namespace std;
using namespace cimdb;
using namespace chrono;

void setup_key_words(map<string, int> &keyWords) {
  keyWords["storage-unit-height"] = 512;
  keyWords["storage-unit-width"] = 512;
  keyWords["check-result"] = 0;
  keyWords["datasets"] = 0;
  keyWords["runtime"] = 0;
  keyWords["compression-ratio"] = 0;
  keyWords["range-queries"] = 0;
  keyWords["query-workloads"] = 0;
  keyWords["query-aware-optimization"] = 0;
  keyWords["enable-simulator"] = 0;
  keyWords["only-show-compression"] = 0;
  keyWords["hybrid-partitioning-factor"] = 1;
  keyWords["kernel-size"] = 0;
  keyWords["stride"] = 1;
  keyWords["accessed-rows-percentage"] = 0;
  keyWords["accessed-columns-percentage"] = 0;
  keyWords["smoothing-simplification"] = 0;
  keyWords["load-compressed-mat"] = 0;
  keyWords["time-limit"] = 0;
  keyWords["output-file"] = 0;
  keyWords["cost-model"] = 0;
  keyWords["adaptive-storage-test"] = 0;
  keyWords["gaussian-smoothing-test"] = 0;
  keyWords["cost-model-test"] = 0;
  keyWords["cost-analysis-test"] = 0;
  keyWords["storage-optimized"] = 0;
  keyWords["l1"] = 0;
  keyWords["u1"] = 0;
  keyWords["l2"] = 0;
  keyWords["u2"] = 0;
  // Datasets
  keyWords["symmetric-matrices"] = 0;
  keyWords["small-matrices"] = 0;
  keyWords["large-matrices"] = 0;
  keyWords["sdss"] = 0;
  // Storage Schemes
  keyWords["HyperMR"] = 0;
  keyWords["GSMR"] = 0;
  keyWords["ReSpar"] = 0;
  keyWords["kMeans"] = 0;
  keyWords["TraNNsformer"] = 0;
  keyWords["METIS"] = 0;
  keyWords["GMR"] = 0;
}

void check_symmetric(Matrix &matrix, string dataset, vector<string> &symmetric_matrices) {
  for (int i = 0; i < 4; i++) dataset.pop_back(); // delete ".mtx"
  for (auto item : symmetric_matrices) {
    if (item == dataset) {
      matrix.symmetric = true;
      break;
    }
  }
  if (matrix.symmetric) {
    auto &mat = matrix.valid_map;
    int rows = mat.size();
    int columns = mat[0].size();
    // The .mtx file of a symmetric square may show only the upper or lower triangle.
    for (int i = 0; i < rows; ++i) {
      for (int j = 0; j < columns; ++j) {
        if (mat[i][j]) mat[j][i] = true;
      }
    }
  }
  return;
}

void output_compressed_mat(string &absolute_path, string &file_name, ofstream& ofs, 
                           vector<pair<string, int>> &storage_schemes, map<string, int> &keyWords,
                           vector<string> &symmetric_matrices) {
  ifstream ifs;
  ifs.open(absolute_path, ios::in);
  if (!ifs.is_open()) {
    ofs << "read fail." << endl;
    return;
  }
  long long int rows, columns, non_zeros;
  string buf;
  while (getline(ifs, buf)) {
    if (buf[0] == '%')
      continue;
    else {
      stringstream ss(buf);
      ss >> rows >> columns >> non_zeros;
      break;
    }
  }
  Matrix mat(rows, columns);
  int r, c;
  double val;
  bool flag = false;
  int non_zeros_found = 0;
  while (getline(ifs, buf)) {
    stringstream ss(buf);
    ss >> r >> c >> val;
    if (val != 0) {
      mat.valid_map[r - 1][c - 1] = true;
      ++non_zeros_found;
    }
    else flag = true;
  }
  ifs.close();
  if (flag) cout << file_name << " : " << non_zeros_found << endl;

  check_symmetric(mat, file_name, symmetric_matrices);

  ofs << "rows: " << rows << endl;
  ofs << "columns: " << columns << endl;
  // evaluation
  mat.storage_model();
  size_t origin_storage_cost = mat.numTiles;
  ofs << "Allocated PEs (before optimization): " << mat.storage_units.size() << endl;
  ofs << "Allocated Tiles (before optimization): " << mat.numTiles << endl;

  vector<WorkloadPattern> sparse_mvm_query;
  WorkloadPattern query;
  int l1, l2, u1, u2;
  /* Compression Schemes */
  for (auto item : storage_schemes) {
    if (keyWords[item.first] != true) continue;
    auto start = system_clock::now();
    switch (item.second) {
      case HyperMR:
        ofs << endl << "HyperMR: " << endl;
        // defualt: keyWords["query-aware-optimization"] == false;
        l1 = 0;
        u1 = rows - 1;
        l2 = 0;
        u2 = columns - 1;
        query.range.push_back(Dimension("d1", l1, u1));
        query.range.push_back(Dimension("d2", l2, u2));
        query.weight = 1;
        sparse_mvm_query.push_back(query);
        keyWords["query-aware-optimization"] == false;
        Storage::HyperMR(keyWords, mat, keyWords["storage-unit-height"], keyWords["storage-unit-width"], sparse_mvm_query);
        break;
      case GSMR:
        break;
      case ReSpar:
        
        break;
      case kMeans:
        
        break;
      case GMR:
        
        break;
      case METIS:
        
        break;
      default:
        break;
    }
    auto end = system_clock::now();
    if (keyWords["runtime"]) {
      auto duration = duration_cast<microseconds>(end - start);
      ofs << "running time: " << double(duration.count()) * microseconds::period::num / microseconds::period::den << endl;  // seconds
    }
    if (keyWords["check-result"]) {
      bool flagCheck = true;
      long long int res = 0;
      for (size_t r = 0; r < mat.row_imprint.size(); r++) {
        if (mat.row_imprint[r] >= mat.rows) continue;
        res += mat.row_imprint[r];
      }
      // check sum (row)
      if (res != (rows - 1) * rows / 2) flagCheck = false;
      res = 0;
      for (size_t c = 0; c < mat.col_imprint.size(); c++) {
        if (mat.col_imprint[c] >= mat.columns) continue;
        res += mat.col_imprint[c];
      }
      // check sum (column)
      if (res != (columns - 1) * columns / 2) flagCheck = false;
      if (flagCheck)
        ofs << "check result: true" << endl;
      else
        ofs << "check result: false" << endl;
      ofs << "row imprint size: " << mat.row_imprint.size() << endl;
      ofs << "column imprint size: " << mat.col_imprint.size() << endl;
    }
    mat.storage_model();
    int compressed_storage_cost = mat.numTiles;
    // compression ratio = uncompressed size / compressed size
    mat.compression_rate = (double)origin_storage_cost / (double)compressed_storage_cost;
    ofs << "Allocated PEs (after optimization): " << mat.storage_units.size() << endl;
    ofs << "Allocated Tiles (after optimization): " << mat.numTiles << endl;
    ofs << "compression rate: " << mat.compression_rate << endl;

    string dir_path = "../test/output/" + item.first;
    if (keyWords["time-limit"] && item.second == HyperMR) {
      dir_path = dir_path + "-v1";
    }
    if (!boost::filesystem::is_directory(dir_path)) {
      if (!boost::filesystem::create_directories(dir_path)) {
        cout << "create_directories failed" << endl;
      }
    }

    string output_path = dir_path + "/" + file_name;
    ofstream outfs;
    outfs.open(output_path, ios::out);
    outfs << rows << " " << columns << " " << non_zeros << " " << mat.numTiles << " " << mat.compression_rate << endl;
    // output : row_imprints and col_imprints
    for (auto r : mat.row_imprint) outfs << r << " ";
    outfs << endl;
    for (auto c : mat.col_imprint) outfs << c << " ";
    outfs.close();
    mat.reset_imprints();
    ofs << endl;
  }
}

void load_mat(Matrix &mat, string baseline, string mat_file) {
  string dir_path = "../test/output/";
  string absolute_path = dir_path + baseline + "/" + mat_file;
  ifstream ifs;
  ifs.open(absolute_path, ios::in);
  string buf;
  int rows, columns, non_zeros, numTiles;
  double compression_rate;
  getline(ifs, buf);
  stringstream line1(buf);
  line1 >> rows >> columns >> non_zeros >> numTiles >> compression_rate;
  
  getline(ifs, buf);
  stringstream line2(buf);
  int x;
  mat.row_imprint.clear();
  mat.col_imprint.clear();
  while (line2 >> x) mat.row_imprint.push_back(x);
  getline(ifs, buf);
  stringstream line3(buf);
  while (line3 >> x) mat.col_imprint.push_back(x);
  ifs.close();
  return;
}

void init_configuration(int expID, vector<string> &exp, string &absolute_path, map<string, int> &keyWords, 
                        string &query_workload, string &hypergraph_config, vector<string> &config_buf, vector<string> &datasets , 
                        vector<string> &small_matrices, vector<string> &large_matrices, vector<string> &symmetric_matrices, vector<string> &sdss_images) {
  exp.push_back("exp1_MVM.txt");
  exp.push_back("exp2_synthetic_workloads.txt");
  exp.push_back("exp3_gaussian_smoothing.txt");
  exp.push_back("exp4_baselines.txt");
  switch (expID) {
    case 1:
      absolute_path = absolute_path + exp[0];
      break;
    case 2:
      absolute_path = absolute_path + exp[1];
      break;
    case 3:
      absolute_path = absolute_path + exp[2];
      break;
    case 4: 
      absolute_path = absolute_path + exp[3];
      break;
    default:
      break;
  }

  ifstream ifs;
  ifs.open(absolute_path, ios::in);
  if (!ifs.is_open()) {
    cout << "read fail." << endl;
    return;
  }

  setup_key_words(keyWords);
  string buf, time_limit;
  while (getline(ifs, buf)) {
    config_buf.push_back(buf);
    if (buf[0] == '%' || buf[0] == ' ' || buf[0] == '\n') continue;
    stringstream ss(buf);
    string kw;
    ss >> kw;
    int len = kw.size();
    if (len == 0) continue;
    if (kw.at(len - 1) != ':') {
      cout << "wrong parameter!" << endl;
      return;
    }
    kw.erase(kw.end() - 1);
    string tmp;
    if (kw == "datasets") {
      while (ss >> tmp) {
        if (tmp != "NULL" and tmp != "null") datasets.push_back(tmp);
      }
      keyWords[kw] = datasets.size();
    } else if (kw == "query-workloads") {
      ss >> query_workload;
      keyWords[kw] = 1;
    } else if (kw == "hypergraph-partitioning-config") {
      ss >> hypergraph_config;
      hypergraph_config = hypergraph_config + ".ini";
      keyWords[kw] = 1;
    } else if (kw == "time-limit"){
      ss >> time_limit;
      if (stoi(time_limit) == -1) keyWords[kw] = 0;
      else keyWords[kw] = 1;
    } else {
      ss >> tmp;
      if (tmp == "Yes")
        keyWords[kw] = 1;
      else if (tmp == "No")
        keyWords[kw] = 0;
      else {
        keyWords[kw] = stoi(tmp);
      }
    }
  }
  cout << " OK!" << endl;
  ifs.close();

  ifs.open("../test/info/small_matrices.txt", ios::in);
  while (getline(ifs, buf)) small_matrices.push_back(buf);
  ifs.close();

  ifs.open("../test/info/large_matrices.txt", ios::in);
  while (getline(ifs, buf)) large_matrices.push_back(buf);
  ifs.close();

  ifs.open("../test/info/symmetric_matrices.txt", ios::in);
  while (getline(ifs, buf)) symmetric_matrices.push_back(buf);
  ifs.close();

  ifs.open("../test/info/sdss.txt", ios::in);
  while (getline(ifs, buf)) sdss_images.push_back(buf);
  ifs.close();

  string config_path = "../test/config/" + hypergraph_config;
  ifs.open(config_path, ios::in);
  vector<string> ini;
  while (getline(ifs, buf)) {
    if(buf.size() > 11) { // time-limit=, 11 chars
      if (buf.substr(0, 11) == "time-limit=") {
        buf = "time-limit=" + time_limit;
      }
    }
    ini.push_back(buf);
  }
  ifs.close();
  ofstream ofs("../test/config/HyperMR.ini", ios::out);
  for (auto line : ini) ofs << line << endl;
  ofs.close();

  return;
}

void execution_test(int expID, map<string, int> &keyWords, vector<string> &datasets, ofstream &ofs, 
                    vector<string> &symmetric_matrices, vector<pair<string, int>> &storage_schemes) {
  string dir_path, file_name, absolute_path, buf;
  ofstream costModel, costAnalysis;
  if (keyWords["cost-model-test"]) {
    dir_path = "../test/output/Uncompression";
    if (!boost::filesystem::is_directory(dir_path)) {
      if (!boost::filesystem::create_directories(dir_path)) {
        cout << "create_directories failed" << endl;
      }
    }
    file_name = "cost_model.txt";
    string output_path = dir_path + "/" + file_name;
    costModel.open(output_path, ios::app);
    costModel << datasets[0] << " " << keyWords["l1"] << " " << keyWords["u1"] << " " << keyWords["l2"] << " " << keyWords["u2"] << " ";
  }

  for (auto it = datasets.begin(); it != datasets.end(); it++) {
    if (keyWords["cost-model-test"]) costModel << *it << " ";
    ifstream ifs;
    if (expID == 1) { // MVM
      // read sparse matrices (.mtx)
      dir_path = "../test/datasets/sparse_matrix/" + *it;  // exectue in build dir
      file_name = *it + ".mtx";
      absolute_path = dir_path + "/" + file_name;
      ofs << "dataset: " << *it << endl;
    } else {
      // read SDSS images (.mtx)
      dir_path = "../test/datasets/SDSS/mtx";  // exectue in build dir
      file_name = *it + ".mtx";
      absolute_path = dir_path + "/" + file_name;
      ofs << "dataset: " << *it << endl;
    }
    // read the mat file of 
    ifs.open(absolute_path, ios::in);
    if (!ifs.is_open()) {
      ofs << "read fail." << endl;
      return;
    }
    long long int rows, columns, non_zeros;
    while (getline(ifs, buf)) {
      if (buf[0] == '%')
        continue;
      else {
        stringstream ss(buf);
        ss >> rows >> columns >> non_zeros;
        break;
      }
    }
    Matrix mat(rows, columns);
    int r, c;
    double val;
    while (getline(ifs, buf)) {
      stringstream ss(buf);
      ss >> r >> c >> val;
      if (val != 0) mat.valid_map[r - 1][c - 1] = true;
    }
    ifs.close();
    
    ofs << "rows: " << rows << endl;
    ofs << "columns: " << columns << endl;

    check_symmetric(mat, file_name, symmetric_matrices);
    
    // evaluation
    mat.storage_model();
    size_t origin_storage_cost = mat.numTiles;
    ofs << "Allocated PEs (before optimization): " << mat.storage_units.size() << endl;
    ofs << "Allocated Tiles (before optimization): " << mat.numTiles << endl;

    vector<WorkloadPattern> workload_patterns, sparse_mvm_query;  // used for query execution and building hypergraph
    Executor executor1;
    int l1, l2, u1, u2;

    /* Query Workloads */
    if (expID == 1) { // MVM
      WorkloadPattern query;
      l1 = 0;
      u1 = rows - 1;
      l2 = 0;
      u2 = columns - 1;
      if (keyWords["cost-model-test"]) {
        query.range.push_back(Dimension("d1", keyWords["l1"], keyWords["u1"]));
        query.range.push_back(Dimension("d2", keyWords["l2"], keyWords["u2"]));
      } else {
        query.range.push_back(Dimension("d1", l1, u1));
        query.range.push_back(Dimension("d2", l2, u2));
      }
      query.weight = 1;
      sparse_mvm_query.push_back(query);
      executor1.physical_plan(mat, sparse_mvm_query, keyWords["cost-model"]);
      if (keyWords["cost-model"]) executor1.cost_model(mat, costModel, keyWords["cost-model-test"]);
    } else if (expID == 2) { // Synthetic Workloads
      int num_workload_patterns = 1;
      workload_patterns = vector<WorkloadPattern>(num_workload_patterns);
      l1 = l2 = 0;
      u1 = ceil(rows * keyWords["accessed-rows-percentage"] / 100.0);
      u2 = ceil(columns * keyWords["accessed-columns-percentage"] / 100.0);
      workload_patterns[0].range.push_back(Dimension("d1", l1, u1));
      workload_patterns[0].range.push_back(Dimension("d2", l2, u2));
      workload_patterns[0].weight = 1;
      executor1.physical_plan(mat, workload_patterns, keyWords["cost-model"]);
    } else if (expID == 3) { // Gaussian Smoothing
      Operator::gaussian_smoothing(mat, keyWords["kernel-size"], keyWords["stride"], workload_patterns, keyWords["smoothing-simplification"]);
      if(!keyWords["only-show-compression"]) executor1.physical_plan(mat, workload_patterns, keyWords["cost-model"]);
    }

    if (!keyWords["only-show-compression"]) {
      ofs << "The total number of accessed PEs (before optimization): " << executor1.num_accessed_PEs << endl;
      ofs << "The total number of accessed Tiles (before optimization): " << executor1.num_accessed_tiles << endl;
      ofs << "The total communication data size (before optimization): " << executor1.total_communication_data_size << endl;
    }

    if (keyWords["enable-simulator"] && !keyWords["only-show-compression"]) {
      executor1.simulation(mat, costModel, keyWords["cost-model-test"]);
      ofs << "The total latency (before optimization): " << executor1.total_latency<< endl;
    }
    int flag = 0;

    if (keyWords["enable-simulator"] && keyWords["cost-analysis-test"]) {
      dir_path = "../test/output/Uncompression";
      if (!boost::filesystem::is_directory(dir_path)) {
        if (!boost::filesystem::create_directories(dir_path)) {
          cout << "create_directories failed" << endl;
        }
      }
      string file_name = "cost_analysis.txt";
      string output_path = dir_path + "/" + file_name;
      costAnalysis.open(output_path, ios::app);
      if (keyWords["cost-analysis-test"]) costAnalysis << *it << " ";
      executor1.simulation(mat, costAnalysis, keyWords["cost-analysis-test"]);
      costAnalysis << endl;
      costAnalysis.close();
    }

    /* Storage Schemes */
    for (auto item : storage_schemes) {
      if (keyWords[item.first] != true) continue;
      ++ flag;
      auto start = system_clock::now();
      switch (item.second) {
        case HyperMR:
          ofs << endl << "HyperMR: " << endl;
          if (keyWords["query-aware-optimization"]) {
            Storage::HyperMR(keyWords, mat, keyWords["storage-unit-height"], keyWords["storage-unit-width"], workload_patterns);
          } else {
            if (keyWords["load-compressed-mat"]) {load_mat(mat, item.first, file_name);} // find the mat file at output dir
            else Storage::HyperMR(keyWords, mat, keyWords["storage-unit-height"], keyWords["storage-unit-width"], sparse_mvm_query);
          }
          break;
        case GSMR:

          break;
        case ReSpar:

          break;
        case kMeans:

          break;
        case GMR:

          break;
        case METIS:
          
         break;
        case TraNNsformer:
          
          break;
        default:
          break;
      }
      auto end = system_clock::now();
      if (keyWords["runtime"]) {
        auto duration = duration_cast<microseconds>(end - start);
        ofs << "running time: " << double(duration.count()) * microseconds::period::num / microseconds::period::den << endl;  // seconds
      }
      if (keyWords["check-result"]) {
        bool flagCheck = true;
        long long int res = 0;
        for (size_t r = 0; r < mat.row_imprint.size(); r++) {
          if (mat.row_imprint[r] >= mat.rows) continue;
          res += mat.row_imprint[r];
        }
        // check sum (row)
        if (res != (rows - 1) * rows / 2) flagCheck = false;
        res = 0;
        for (size_t c = 0; c < mat.col_imprint.size(); c++) {
          if (mat.col_imprint[c] >= mat.columns) continue;
          res += mat.col_imprint[c];
        }
        // check sum (column)
        if (res != (columns - 1) * columns / 2) flagCheck = false;
        if (flagCheck)
          ofs << "check result: true" << endl;
        else
          ofs << "check result: false" << endl;
        ofs << "row imprint size: " << mat.row_imprint.size() << endl;
        ofs << "column imprint size: " << mat.col_imprint.size() << endl;
      }
      if (keyWords["compression-ratio"]) {
        mat.storage_model();
        int compressed_storage_cost = mat.numTiles;
        // compression ratio = uncompressed size / compressed size
        mat.compression_rate = (double)origin_storage_cost / (double)compressed_storage_cost;
        ofs << "Allocated PEs (after optimization): " << mat.storage_units.size() << endl;
        ofs << "Allocated Tiles (after optimization): " << mat.numTiles << endl;
        ofs << "compression rate: " << mat.compression_rate << endl;
      }
      Executor executor2;
      if (expID == 1) { // MVM
        executor2.physical_plan(mat, sparse_mvm_query, keyWords["cost-model"]);
        if (keyWords["cost-model"]) executor2.cost_model(mat, costModel, keyWords["cost-model-test"]);
      } else {
        executor2.physical_plan(mat, workload_patterns, keyWords["cost-model"]);
      }
      ofs << "The total number of accessed PEs (after optimization): " << executor2.num_accessed_PEs << endl;
      ofs << "The total number of accessed Tiles (after optimization): " << executor2.num_accessed_tiles << endl;
      ofs << "The total communication data size (after optimization): " << executor2.total_communication_data_size << endl;

      if (keyWords["cost-analysis-test"]) {
          dir_path = "../test/output/" + item.first;
          if (!boost::filesystem::is_directory(dir_path)) {
            if (!boost::filesystem::create_directories(dir_path)) {
              cout << "create_directories failed" << endl;
            }
          }
          string file_name = "cost_analysis.txt";
          string output_path = dir_path + "/" + file_name;
          costAnalysis.open(output_path, ios::app);
          if (keyWords["cost-analysis-test"]) costAnalysis << *it << " ";
          executor2.simulation(mat, costAnalysis, keyWords["cost-analysis-test"]);
          costAnalysis << endl;
          costAnalysis.close();
          ofs << "The total latency (after optimization): " << executor2.total_latency<< endl;
      } else if (keyWords["enable-simulator"]) {
        executor2.simulation(mat, costModel, keyWords["cost-model-test"]);
        ofs << "The total latency (after optimization): " << executor2.total_latency<< endl;
      }

      ofs << endl;
      // output file
      if (keyWords["output-file"] && !keyWords["cost-analysis-test"] && expID == 1) {
        dir_path = "../test/output/" + item.first;
        if (keyWords["time-limit"] && item.second == HyperMR) dir_path = dir_path + "-v1";
        if (!boost::filesystem::is_directory(dir_path)) {
          if (!boost::filesystem::create_directories(dir_path)) {
            cout << "create_directories failed" << endl;
          }
        }
        file_name = "MVM.txt";
        string output_path = dir_path + "/" + file_name;
        ofstream outfs;
        outfs.open(output_path, ios::app);
        // dataset, total latency, predicted cost
        outfs << *it << " " << executor2.total_latency << " " << executor2.cost << endl;

        /* test */
        // outfs << rows << " " << columns << " " << non_zeros << " " << mat.numTiles << " " << mat.compression_rate << endl;
        // if (mat.compression_rate >= 1) {
        //   // output : row_imprints and col_imprints
        //   for (auto r : mat.row_imprint) outfs << r << " ";
        //   outfs << endl;
        //   for (auto c : mat.col_imprint) outfs << c << " ";
        //   outfs << endl;
        // }
        /* test */

        // outfs << executor1.total_latency << " " << executor2.total_latency << " " << executor2.cost << endl;
        outfs.close();
      } else if (keyWords["output-file"] && expID == 2) {
        dir_path = "../test/output/" + item.first;
        if (keyWords["time-limit"] && item.second == HyperMR) dir_path = dir_path + "-v1";
        if (!boost::filesystem::is_directory(dir_path)) {
          if (!boost::filesystem::create_directories(dir_path)) {
            cout << "create_directories failed" << endl;
          }
        }
        file_name = "adaptive_storage.txt";
        string output_path = dir_path + "/" + file_name;
        ofstream outfs;
        outfs.open(output_path, ios::app);
        // image, accessed-rows-percentage, accessed-columns-percentage, performance
        outfs << *it << " " << keyWords["accessed-rows-percentage"] << " " << keyWords["accessed-columns-percentage"] 
              << " " << executor2.total_latency;
        if (item.second == HyperMR) outfs << " " << mat.numTiles;
        outfs << endl;
        outfs.close();
      } else if (keyWords["output-file"] && expID == 3) {
        dir_path = "../test/output/" + item.first;
        if (keyWords["time-limit"] && item.second == HyperMR) dir_path = dir_path + "-v1";
        if (!boost::filesystem::is_directory(dir_path)) {
          if (!boost::filesystem::create_directories(dir_path)) {
            cout << "create_directories failed" << endl;
          }
        }
        file_name = "gaussian_smoothing.txt";
        string output_path = dir_path + "/" + file_name;
        ofstream outfs;
        outfs.open(output_path, ios::app);
        // image, accessed-rows-percentage, accessed-columns-percentage, performance
        outfs << *it << " " << keyWords["kernel-size"] << " " << keyWords["stride"] << " " << executor2.total_latency;
        if (item.second == HyperMR) outfs << " " << mat.numTiles;
        outfs << endl;
        outfs.close();
      }
      mat.reset_imprints();
    } // storage schemes

    // if flag == 0, then the output is the uncompression one
    if (keyWords["cost-model-test"] && expID == 1) {
      costModel.close();
    } else if (keyWords["output-file"] && flag == 0 && !keyWords["only-show-compression"] && expID == 1) {
      dir_path = "../test/output/Uncompression";
      if (!boost::filesystem::is_directory(dir_path)) {
        if (!boost::filesystem::create_directories(dir_path)) {
          cout << "create_directories failed" << endl;
        }
      }
      file_name = "MVM.txt";
      string output_path = dir_path + "/" + file_name;
      ofstream outfs;
      outfs.open(output_path, ios::app);
      // dataset, total latency
      outfs << *it << " " << executor1.total_latency << " " << executor1.cost << endl;
      outfs.close();
    } else if (keyWords["output-file"] && flag == 0 && !keyWords["only-show-compression"] && expID == 2) {
      dir_path = "../test/output/Uncompression";
      if (!boost::filesystem::is_directory(dir_path)) {
        if (!boost::filesystem::create_directories(dir_path)) {
          cout << "create_directories failed" << endl;
        }
      }
      file_name = "adaptive_storage.txt";
      string output_path = dir_path + "/" + file_name;
      ofstream outfs;
      outfs.open(output_path, ios::app);
      // image, accessed-rows-percentage, accessed-columns-percentage, performance
      outfs << *it << " " << keyWords["accessed-rows-percentage"] << " " 
            << keyWords["accessed-columns-percentage"] << " " << executor1.total_latency << endl;
      outfs.close();
    } else if (keyWords["output-file"] && flag == 0 && !keyWords["only-show-compression"] && expID == 3) {
      dir_path = "../test/output/Uncompression";
      if (!boost::filesystem::is_directory(dir_path)) {
        if (!boost::filesystem::create_directories(dir_path)) {
          cout << "create_directories failed" << endl;
        }
      }
      file_name = "gaussian_smoothing.txt";
      string output_path = dir_path + "/" + file_name;
      ofstream outfs;
      outfs.open(output_path, ios::app);
      // image, accessed-rows-percentage, accessed-columns-percentage, performance
      outfs << *it << " " << keyWords["kernel-size"] << " " << keyWords["stride"] << " " << executor1.total_latency << endl;
      outfs.close();
    }
    ofs << endl;
  }
}

int main() {
  int expID;
  cout << "HyperMR:" << endl;
  cout << "Experiment 1: MVM." << endl;
  cout << "Experiment 2: Synthetic Workloads." << endl;
  cout << "Experiment 3: Gaussian Smoothing." << endl;
  cout << "Experiment 4: Baselines." << endl;
  cout << "Please enter the experiment ID: ";
  cin >> expID;
  
  cout << "The experiment configuration file is being read. ";
  
  vector<string> exp, datasets, config_buf;
  string absolute_path = "../test/config/";
  map<string, int> keyWords;
  string query_workload, hypergraph_config;
  vector<string> small_matrices, large_matrices, symmetric_matrices, sdss_images;
  init_configuration(expID, exp, absolute_path, keyWords, query_workload, hypergraph_config, config_buf,
                     datasets, small_matrices, large_matrices, symmetric_matrices, sdss_images);

  absolute_path = "../test/log/";
  time_t now = time(0);  // UTC Time
  tm *ltm = localtime(&now);
  string date = to_string(1900 + ltm->tm_year) + "-" + to_string(1 + ltm->tm_mon) + "-" + to_string(ltm->tm_mday);
  string day_time = to_string(ltm->tm_hour) + ":" + to_string(ltm->tm_min) + ":" + to_string(ltm->tm_sec);
  string log_dir = absolute_path + date;
  if (!boost::filesystem::is_directory(log_dir)) {
    if (!boost::filesystem::create_directories(log_dir)) {
      cout << "create_directories failed" << endl;
    }
  }
  string logFile = date + "-" + day_time;
  switch (expID)
  {
    case 1:
      logFile = logFile + "-MVM";
      break;
    case 2:
      logFile = logFile + "-synthetic-workloads";
      break;
    case 3:
      logFile = logFile + "-gaussian-smoothing";
      break;
    case 4:
      logFile = logFile + "-baselines";
      break;
    break;
    default:
    break;
  }
  absolute_path = log_dir + "/" + logFile + ".log";
  cout << "logFile = " << logFile << endl;
  // create log file
  ofstream ofs;
  ofs.open(absolute_path, ios::out);

  for (auto line : config_buf) ofs << line << endl;
  ofs << "-------------------------------- Configuration --------------------------------" << endl;
  ofs << endl;

  vector<pair<string, int>> storage_schemes;
  storage_schemes.push_back(pair<string, int>("HyperMR", HyperMR));
  storage_schemes.push_back(pair<string, int>("GSMR", GSMR));
  storage_schemes.push_back(pair<string, int>("ReSpar", ReSpar));
  storage_schemes.push_back(pair<string, int>("kMeans", kMeans)); 
  storage_schemes.push_back(pair<string, int>("GMR", GMR)); 
  storage_schemes.push_back(pair<string, int>("METIS", METIS));
  storage_schemes.push_back(pair<string, int>("TraNNsformer", TraNNsformer));

  if (expID == 4) {
    if(keyWords["small-matrices"]) {
      for (auto mat : small_matrices) datasets.push_back(mat);
    } 
    if(keyWords["large-matrices"]) {
      for (auto mat : large_matrices) datasets.push_back(mat);
    }
    if (keyWords["symmetric-matrices"]) {
      for (auto mat : symmetric_matrices) datasets.push_back(mat);
    }
    if (keyWords["sdss"]) {
      for (auto mat : sdss_images) datasets.push_back(mat);
    }

    for(auto it = datasets.begin(); it != datasets.end(); it++) {
      ofs << "dataset: " << *it << endl;
      string dir_path, file_name;
      if ((*it).length() == 19) {
        dir_path = "../test/datasets/SDSS/mtx";
        file_name = *it + ".mtx";
      } else {
        dir_path = "../test/datasets/sparse_matrix/" + *it;
        file_name = *it + ".mtx";
      }
      absolute_path = dir_path + "/" + file_name;
      output_compressed_mat(absolute_path, file_name, ofs, storage_schemes, keyWords, symmetric_matrices);
    }
    ofs.close();
    return 0;
  }

  if(keyWords["small-matrices"]) {
    for (auto mat : small_matrices) datasets.push_back(mat);
  } 
  if(keyWords["large-matrices"]) {
    for (auto mat : large_matrices) datasets.push_back(mat);
  }
  if (keyWords["symmetric-matrices"]) {
    for (auto mat : symmetric_matrices) datasets.push_back(mat);
  }
  if (keyWords["sdss"]) {
    for (auto mat : sdss_images) datasets.push_back(mat);
  }

  if (keyWords["cost-model-test"] && expID == 1) {
    string dataset = "crystm03";
    datasets.clear();
    datasets.push_back(dataset);
    for (auto item : storage_schemes) {
      keyWords[item.first] = false;
    }
    keyWords["only-show-compression"] = false;
    keyWords["cost-model"] = true;
    int accessed_rows[] = {2500, 5000, 7500, 10000, 12500, 15000};
    int accessed_columns[] = {2500, 5000, 7500, 10000, 12500, 15000};
    for (auto r : accessed_rows) {
      keyWords["l1"] = 0;
      keyWords["u1"] = r - 1;
      keyWords["l2"] = 0;
      keyWords["u2"] = 20000 - 1;
      execution_test(expID, keyWords, datasets, ofs, symmetric_matrices, storage_schemes);
    }
    for (auto c : accessed_columns) {
      keyWords["l1"] = 0;
      keyWords["u1"] = 20000 - 1;
      keyWords["l2"] = 0;
      keyWords["u2"] = c - 1;
      execution_test(expID, keyWords, datasets, ofs, symmetric_matrices, storage_schemes);
    }
  } else if (keyWords["cost-analysis-test"] && expID == 1) {
    keyWords["cost-model"] = false;
    keyWords["cost-model-test"] = false;
    keyWords["only-show-compression"] = true;
    keyWords["enable-simulator"] = true;
    keyWords["query-aware-optimization"] = false;
    execution_test(expID, keyWords, datasets, ofs, symmetric_matrices, storage_schemes);
  } else if (keyWords["adaptive-storage-test"] && expID == 2) {
    for (int accessed_rows = 20; accessed_rows <= 100; accessed_rows += 20) {
      for (int accessed_columns = 20; accessed_columns <= 100; accessed_columns += 20) {
        keyWords["accessed-rows-percentage"] = accessed_rows;
        keyWords["accessed-columns-percentage"] = accessed_columns;
        execution_test(expID, keyWords, datasets, ofs, symmetric_matrices, storage_schemes);
      }
    }
  } else if (keyWords["gaussian-smoothing-test"] && expID == 3) {
    vector<int> kernel_size;
    int ks[] = {3, 5, 7, 9, 25, 49, 27, 125, 243, 343, 81, 625, 2401, 2187};
    int ks_size = sizeof(ks)/sizeof(ks[0]);
    kernel_size.insert(kernel_size.end(), ks, ks + ks_size);
    for (auto k : kernel_size) {
      keyWords["kernel-size"] = k;
      execution_test(expID, keyWords, datasets, ofs, symmetric_matrices, storage_schemes);
    } 
  } else {
    execution_test(expID, keyWords, datasets, ofs, symmetric_matrices, storage_schemes);
  }

  ofs.close();
  return 0;
}