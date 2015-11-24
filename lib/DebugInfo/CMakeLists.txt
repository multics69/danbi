add_danbi_library(DANBIDebugInfo
  ProgramVisualizer.cpp
  MemEventLogger.cpp
  )

add_danbi_library_dependencies(DANBIDebugInfo DANBICore DANBISupport)