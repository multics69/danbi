#!/bin/bash
# ./mRunAll.sh prefix
args=("$@")
prefix=${args[0]}

##########################################################
cd ..
cmake -DCMAKE_BUILD_TYPE=release -DDANBI_CONF="" . 
make clean
make -j40 
cd scripts
./mRunAllBench.sh ${prefix}-all

# wrap up
cd ..
cmake -DCMAKE_BUILD_TYPE=release -DDANBI_CONF="" . 
cd scripts
exit

##########################################################
# default
cd ..
cmake -DCMAKE_BUILD_TYPE=release -DDANBI_CONF="" . 
make clean
make -j40 
cd scripts
./mRunAllBench.sh ${prefix}-all

# scheduling policy
cd ..
cmake -DCMAKE_BUILD_TYPE=release -DDANBI_CONF="-DDANBI_DISABLE_SPECULATIVE_SCHEDULING -DDANBI_DISABLE_RANDOM_SCHEDULING" . 
make clean
make -j40 
cd scripts
./mRunAllBench.sh ${prefix}-no-pss-no-prs

cd ..
cmake -DCMAKE_BUILD_TYPE=release -DDANBI_CONF="-DDANBI_DISABLE_SPECULATIVE_SCHEDULING" . 
make clean
make -j40 
cd scripts
./mRunAllBench.sh ${prefix}-no-pss

cd ..
cmake -DCMAKE_BUILD_TYPE=release -DDANBI_CONF="-DDANBI_DISABLE_RANDOM_SCHEDULING" . 
make clean
make -j40 
cd scripts
./mRunAllBench.sh ${prefix}-no-prs

# ticketing
cd ..
cmake -DCMAKE_BUILD_TYPE=release -DDANBI_CONF="-DDANBI_DO_NOT_TICKET" . 
make clean
make -j40 
cd scripts
./mRunAllBench.sh ${prefix}-no-ticket

# data fifo queue
cd ..
cmake -DCMAKE_BUILD_TYPE=release -DDANBI_CONF="-DDANBI_DISABLE_PER_CPU_ORDERING_TICKET_SERVER" . 
make clean
make -j40 
cd scripts
./mRunAllBench.sh ${prefix}-no-per-cpu-ticket-waiting

cd ..
cmake -DCMAKE_BUILD_TYPE=release -DDANBI_CONF="-DDANBI_DISABLE_QUEUE_BASED_COMMIT_ORDERING" . 
make clean
make -j40 
cd scripts
./mRunAllBench.sh ${prefix}-no-queue-based-commit-ordering

# ready queue 
cd ..
cmake -DCMAKE_BUILD_TYPE=release -DDANBI_CONF="-DDNABI_READY_QUEUE_USE_MSQ" . 
make clean
make -j40 
cd scripts
./mRunAllBench.sh ${prefix}-no-ssq-use-msq

# no numa
cd ..
cmake -DCMAKE_BUILD_TYPE=release -DDANBI_CONF="-DDANBI_DO_NOT_USE_NUMA_INTERLEAVE_MEM_POLICY -DDANBI_DO_NOT_PREVENT_CONCURRENT_PAGE_FAULTS"
make clean
make -j40 
cd scripts
./mRunAllBench.sh ${prefix}-no-numa

cd ..
cmake -DCMAKE_BUILD_TYPE=release -DDANBI_CONF="-DDANBI_DO_NOT_PREVENT_CONCURRENT_PAGE_FAULTS"
make clean
make -j40 
cd scripts
./mRunAllBench.sh ${prefix}-no-conpf

cd ..
cmake -DCMAKE_BUILD_TYPE=release -DDANBI_CONF="-DDANBI_DO_NOT_USE_NUMA_INTERLEAVE_MEM_POLICY"
make clean
make -j40 
cd scripts
./mRunAllBench.sh ${prefix}-no-interleave

# wrap up
cd ..
cmake -DCMAKE_BUILD_TYPE=release -DDANBI_CONF="" . 
cd scripts