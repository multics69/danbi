set( CMAKE_C_FLAGS "-ftree-vectorize -ftree-vectorizer-verbose=0 -msse3 -march=native -mtune=native -finline-functions ${DANBI_CONF}" )
set( CMAKE_CXX_FLAGS "--std=c++0x -ftree-vectorize -ftree-vectorizer-verbose=0 -msse3 -march=native -mtune=native -finline-functions -fno-exceptions -fno-rtti -Wno-invalid-offsetof ${DANBI_CONF}" )
set( CMAKE_C_FLAGS_RELEASE "-O3 -fselective-scheduling -fselective-scheduling2 -ftree-vectorize -ftree-vectorizer-verbose=0 -msse3 -march=native -mtune=native -finline-functions -DNDEBUG" )
set( CMAKE_CXX_FLAGS_RELEASE "-O3 -fselective-scheduling -fselective-scheduling2 -ftree-vectorize -ftree-vectorizer-verbose=0 -msse3 -march=native -mtune=native -finline-functions -DNDEBUG" )

set( DANBI_SYSTEM_LIBS m pthread dl numa papi )

macro(add_danbi_library name)
  add_library( ${name} ${ARGN} )
  set_property( GLOBAL APPEND PROPERTY DANBI_LIBS ${name} )
endmacro(add_danbi_library name)

macro(add_danbi_library_dependencies name)
  set_property(GLOBAL PROPERTY DANBI_LIB_DEPS_${name} ${ARGN})
  target_link_libraries(${name} ${ARGN})
  target_link_libraries( ${name} ${DANBI_SYSTEM_LIBS} )
endmacro(add_danbi_library_dependencies name)

macro(add_danbi_executable name)
  get_property(danbi_libs GLOBAL PROPERTY DANBI_LIBS)
  add_executable(${name} ${ARGN})
  target_link_libraries( ${name} ${danbi_libs} )
endmacro(add_danbi_executable name)
