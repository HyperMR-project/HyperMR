# Copyright (c) .
# All rights reserved.
# 
# This file is covered by the LICENSE.txt license file in the root directory.

include_directories("${CMAKE_SOURCE_DIR}/include" "${CMAKE_SOURCE_DIR}/NeuroSim")

add_library(matrix_storage SHARED)

target_sources(matrix_storage
        PRIVATE
        storage/HyperEdge.cpp
        storage/HyperGraph.cpp
        storage/HyperMR.cpp
        storage/Matrix.cpp
        query/WorkloadPattern.cpp
        query/Executor.cpp
        query/Operator.cpp
        simulator/NeuroSimAPI.cpp)