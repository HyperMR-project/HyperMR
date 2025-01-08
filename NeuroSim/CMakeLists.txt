# Copyright (c) .
# All rights reserved.
# 
# This file is covered by the LICENSE.txt license file in the root directory.

# file(GLOB simulator_src ./NeuroSim/*.cpp)

add_library(neurosim SHARED)

target_sources(neurosim
        PRIVATE
        Adder.cpp
        AdderTree.cpp
        BitShifter.cpp
        Buffer.cpp
        Bus.cpp
        Chip.cpp
        Comparator.cpp
        CurrentSenseAmp.cpp
        DecoderDriver.cpp
        DeMux.cpp
        DFF.cpp
        formula.cpp
        FunctionUnit.cpp
        HTree.cpp
        LevelShifter.cpp
        MaxPooling.cpp
        MultilevelSAEncoder.cpp
        MultilevelSenseAmp.cpp
        Mux.cpp
        NewMux.cpp
        NewSwitchMatrix.cpp
        Param.cpp
        Precharger.cpp
        ProcessingUnit.cpp
        ReadCircuit.cpp
        RowDecoder.cpp
        SarADC.cpp
        SenseAmp.cpp
        ShiftAdd.cpp
        Sigmoid.cpp
        SramNewSA.cpp
        SRAMWriteDriver.cpp
        SubArray.cpp
        SwitchMatrix.cpp
        Technology.cpp
        Tile.cpp
        VoltageSenseAmp.cpp
        WLDecoderOutput.cpp
        WLNewDecoderDriver.cpp
        )