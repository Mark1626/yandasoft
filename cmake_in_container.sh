#!/bin/bash
cmake -DCMAKE_CXX_COMPILER=mpicxx -DCMAKE_CXX_FLAGS="-I/usr/local/include -pthread" -DCMAKE_BUILD_TYPE=Debug  -DIce_HOME=/usr/lib/x86_64-linux-gnu/ -DBUILD_ANALYSIS=ON -DBUILD_PIPELINE=ON -DBUILD_COMPONENTS=ON -DBUILD_ANALYSIS=ON -DBUILD_SERVICES=ON -DCMAKE_CXX_FLAGS="-coverage" -DCMAKE_EXE_LINKER_FLAGS="-coverage" -DCMAKE_INSTALL_PREFIX=/home/yanda-user/all_yandasoft/install ..
