#!/bin/bash

cd ..
cmake -DDANBI_CONF="" -DCMAKE_BUILD_TYPE=release . 
make clean
make -j40 
cd scripts
./mRCQ r910-opt yes
./mRCQ r910-opt no

cd ..
cmake -DDANBI_CONF="-DDANBI_DISABLE_PER_CPU_ORDERING_TICKET_SERVER -DDANBI_DISABLE_QUEUE_BASED_COMMIT_ORDERING" -DCMAKE_BUILD_TYPE=release . 
make clean
make -j40 
cd scripts
./mRCQ r910-baseline yes
./mRCQ r910-baseline no


