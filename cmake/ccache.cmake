option(USE_CCACHE "Use ccache" OFF)

if(NOT DEFINED USE_CCACHE AND DEFINED ENV{USE_CCACHE})
    set(USE_CCACHE $ENV{USE_CCACHE})
endif()

message(STATUS "Use ccache:\t ${USE_CCACHE}")

if(${USE_CCACHE})
    find_program(CCACHE_PROGRAM ccache)
    if(CCACHE_PROGRAM)
        set(CMAKE_C_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
        set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
    endif()
endif()
