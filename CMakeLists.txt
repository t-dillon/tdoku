cmake_minimum_required(VERSION 3.5)
project(tdoku VERSION 1.0)
set(CMAKE_CXX_STANDARD 14)

# e.g., cmake . -DOPT=O2
set(OPT "O3" CACHE STRING "Optimization level")
# e.g., cmake . -DARCH=sandybridge
set(ARCH "native" CACHE STRING "Target architecture")
# these options specify maximum SSE level and override ARCH
option(SSE2       "Compile with SSE2 support"         OFF)
option(SSSE3      "Compile with SSSE3 support"        OFF)
option(SSE4_1     "Compile with SSE4.1 support"       OFF)
option(SSE4_2     "Compile with SSE4.2 support"       OFF)
option(AVX2       "Compile with AVX2 support"         OFF)
option(AVX512     "Compile with AVX512BITALG support" OFF)

option(BB_SUDOKU  "Include BB_SUDOKU"  OFF)
option(JSOLVE     "Include JSolve"     OFF)
option(KUDOKU     "Include KUDOKU"     OFF)
option(FSSS2      "Include fsss2"      OFF)
option(JCZSOLVE   "Include JCZSolve"   OFF)
option(SK_BFORCE2 "Include SK_BFORCE2" OFF)
option(MINISAT    "Include MiniSat"    OFF)

set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS}   -${OPT}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -${OPT}")

if(AVX512)
    set(ArchFlags "-mavx512vl -mavx512bitalg")
elseif(AVX2)
    set(ArchFlags "-mavx2")
elseif(SSE4_2)
    set(ArchFlags "-msse4.2")
elseif(SSE4_1)
    set(ArchFlags "-msse4.1")
elseif(SSSE3)
    set(ArchFlags "-mssse3")
elseif(SSE2)
    set(ArchFlags "-msse2")
else()
    set(ArchFlags "-march=${ARCH}")
endif()

set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS}   ${ArchFlags}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${ArchFlags}")

configure_file (
    "${CMAKE_SOURCE_DIR}/src/build_info.h.in"
    "${CMAKE_SOURCE_DIR}/src/build_info.h")

# a gcc-linkable library with just the fast solver
add_library(tdoku STATIC src/solver_dpll_triad_simd.cc)
target_compile_options(tdoku PUBLIC -fno-exceptions -fno-rtti)

set(BENCHMARK_SOLVER_SOURCES
        src/solver_basic.cc
        src/solver_dpll_triad_scc.cc
        src/solver_dpll_triad_simd.cc
        other/other_solvers.cc)

if (BB_SUDOKU)
    add_definitions(-DBB_SUDOKU)
    set(BENCHMARK_SOLVER_SOURCES ${BENCHMARK_SOLVER_SOURCES}
            other/bb_sudoku/bb_sudoku_solver.cpp)
endif()

if (JSOLVE)
    add_definitions(-DJSOLVE)
    set(BENCHMARK_SOLVER_SOURCES ${BENCHMARK_SOLVER_SOURCES}
            other/JSolve/JSolve.c)
endif()

if (KUDOKU)
    add_definitions(-DKUDOKU)
    set(BENCHMARK_SOLVER_SOURCES ${BENCHMARK_SOLVER_SOURCES}
            other/kudoku/kudoku.c)
endif()

if (FSSS2)
    add_definitions(-DFSSS2)
    set(BENCHMARK_SOLVER_SOURCES ${BENCHMARK_SOLVER_SOURCES}
            other/fsss2/fsss2.cpp
            other/fsss2/t_128.cpp)
    set (CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -Wno-deprecated-register")
endif()

if (JCZSOLVE)
    add_definitions(-DJCZSOLVE)
    set(BENCHMARK_SOLVER_SOURCES ${BENCHMARK_SOLVER_SOURCES}
            other/JCZSolve/JCZSolve.c)
endif()

if (SK_BFORCE2)
  add_definitions(-DSK_BFORCE2)
  set(BENCHMARK_SOLVER_SOURCES ${BENCHMARK_SOLVER_SOURCES}
          other/SK_BFORCE2/sk_bitfields.cpp
          other/SK_BFORCE2/sk_t.cpp)
endif()

if (MINISAT)
    add_definitions(-DMINISAT)
    set(BENCHMARK_SOLVER_SOURCES ${BENCHMARK_SOLVER_SOURCES}
            src/solver_minisat.cc)
endif()

add_executable(run_benchmark src/run_benchmark.cc ${BENCHMARK_SOLVER_SOURCES})
if (MINISAT)
    target_link_libraries(run_benchmark minisat)
endif()

add_executable(run_tests test/run_tests.cc ${BENCHMARK_SOLVER_SOURCES})
if (MINISAT)
    target_link_libraries(run_tests minisat)
endif()
