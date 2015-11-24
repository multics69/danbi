add_danbi_library(DANBICore
  QueueFactory.cpp
  Program.cpp
  Kernel.cpp
  CPUCode.cpp
  QueueInfo.cpp
  AbstractPartitioner.cpp
  AllCPUPartitioner.cpp
  DistributedDynamicScheduler.cpp
  Runtime.cpp
  IntrinsicsCPU.cpp
  CPUUberKernel.cpp
  UberKernelFactory.cpp
  CPUSchedulePolicyAdapter.cpp
  CPUComputeDevice.cpp
  ComputeDeviceFactory.cpp
  SystemContext.cpp
  ProgramFactory.cpp
  RuntimeFactory.cpp
  )

add_danbi_library_dependencies(DANBICore DANBISupport)