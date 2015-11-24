include_directories(${DANBI_MAIN_SRC_DIR}/utils/unittest/googletest/include)
add_custom_target(UnitTests)

function(add_danbi_unittest test_dirname)
  string(REGEX MATCH "([^/]+)$" test_name ${test_dirname})
  add_danbi_executable(${test_name}Tests ${ARGN})
  add_dependencies(UnitTests ${test_name}Tests)
endfunction()

add_danbi_unittest(Support
  Support/CuBoQueueTest.cpp
  Support/RandomTest.cpp
  Support/VectorTypeTest.cpp
  Support/MachineTest.cpp
  Support/MachineConfigTest.cpp
  Support/PerCPUDataTest.cpp
  Support/ReserveCommitQueueTest.cpp
  Support/ThreadTest.cpp
  Support/ConnectivityMatrixTest.cpp
  Support/ContextSwitchTest.cpp
  Support/FIFOSchedulerTest.cpp
##  Support/SuperScalableQueueTest.cpp
##  Support/ResizableReserveCommitQueueTest.cpp
  )

add_danbi_unittest(Core
  Core/QueueFactoryTest.cpp
  Core/ReadOnlyBufferTest.cpp
  Core/CPUCodeTest.cpp
  Core/ProgramTest.cpp
  Core/SchedulerTest.cpp
  )
