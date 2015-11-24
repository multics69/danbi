project(DANBI)
cmake_minimum_required(VERSION 2.8)

# Load custom modules
set(CMAKE_MODULE_PATH
  ${CMAKE_MODULE_PATH}
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake"
  )
include(DANBI)

# Package information
set(PACKAGE_VERSION "0.1")
set(PACKAGE_NAME danbi)
set(PACKAGE_STRING "${PACKAGE_NAME} ${PACKAGE_VERSION}")
set(PACKAGE_BUGREPORT "multics69@gmail.com")

# Enable Assembly
enable_language(ASM-ATT)

# Configuration for OpenCL
set(DANBI_OPENCL_INCLUDE_DIR $ENV{CUDA_HOME}/include)

# Define directories
set(DANBI_MAIN_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(DANBI_MAIN_INCLUDE_DIR 
  ${DANBI_MAIN_SRC_DIR}/include 
  ${DANBI_OPENCL_INCLUDE_DIR})
set(DANBI_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR})

# Add #include directories
include_directories(${DANBI_MAIN_INCLUDE_DIR})

# Add directories
add_subdirectory(utils)
add_subdirectory(lib)
add_subdirectory(test)
add_subdirectory(benchmark)

# CSCOPE target
add_custom_target(cscope
  COMMAND find ${DANBI_MAIN_SRC_DIR} -name "*.[chsCHS]" > ${DANBI_MAIN_SRC_DIR}/cscope.files 
          && cscope -bi ${DANBI_MAIN_SRC_DIR}/cscope.files)