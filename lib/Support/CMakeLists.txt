add_danbi_library(DANBISupport
  MachineConfig.cpp
  Thread.cpp
#  ContextSwitchIA32.s
  ContextSwitchIA64.s
  ContextSwitch.c
  FIFOSchedulePolicy.cpp
  PerformanceCounter.cpp
  Random.cpp
  NUMAColoredSlabAllocator.cpp
  )
