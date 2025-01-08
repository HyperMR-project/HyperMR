# Copyright (c) .
# All rights reserved.
# 
# This file is covered by the LICENSE.txt license file in the root directory.

cmake_minimum_required(VERSION 3.10)

set(CMAKE_CXX_STANDARD 17)

SET(CMAKE_BUILD_TYPE "Debug")
SET(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O0 -Wall -g2 -ggdb -fopenmp")
SET(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -O3 -Wall -fopenmp")

project(CIM-DB)

add_subdirectory(src)
add_subdirectory(NeuroSim)

include_directories("${CMAKE_SOURCE_DIR}/include" "${CMAKE_SOURCE_DIR}/NeuroSim")

find_package(Boost REQUIRED)
find_package(OpenMP REQUIRED)

add_executable(HyperMR test/test.cpp)

target_link_libraries(HyperMR
        PRIVATE
        matrix_storage
        neurosim
        -lkahypar
        -lboost_system
        -lboost_filesystem
        -lmetis
        OpenMP::OpenMP_CXX)