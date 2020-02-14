cmake_minimum_required(VERSION 3.5)
project(tdoku VERSION 1.0)
set(CMAKE_CXX_STANDARD 14)

set(ARGS "" CACHE STRING "Additional compiler args")
# e.g., cmake . -DOPT=O2
set(OPT "O3" CACHE STRING "Optimization level")
# e.g., cmake . -DARCH=sandybridge
set(ARCH "native" CACHE STRING "Target architecture")
# these options specify maximum SSE level and override ARCH
option(SSE2        "Compile with SSE2 support"         OFF)
option(SSSE3       "Compile with SSSE3 support"        OFF)
option(SSE4_1      "Compile with SSE4.1 support"       OFF)
option(SSE4_2      "Compile with SSE4.2 support"       OFF)
option(AVX         "Compile with AVX support"          OFF)
option(AVX2        "Compile with AVX2 support"         OFF)
option(AVX512      "Compile with AVX512BITALG support" OFF)

option(BB_SUDOKU     "Include BB_SUDOKU"     OFF)
option(FAST_SOLV_9R2 "Include FAST_SOLV_9R2" OFF)
option(FSSS2         "Include fsss2"         OFF)
option(JCZSOLVE      "Include JCZSolve"      OFF)
option(JSOLVE        "Include JSolve"        OFF)
option(KUDOKU        "Include KUDOKU"        OFF)
option(GSS           "Include GSS"           OFF)
option(GUROBI        "Include GUROBI"        OFF)
option(MINISAT       "Include MiniSat"       OFF)
option(NORVIG        "Include norvig"        OFF)
option(RUST_SUDOKU   "Include rust_sudoku"   OFF)
option(SK_BFORCE2    "Include SK_BFORCE2"    OFF)

set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS}   -${OPT} ${ARGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -${OPT} ${ARGS}")

if(AVX512)
    set(ArchFlags "-mavx512vl -mavx512bitalg")
elseif(AVX2)
    set(ArchFlags "-mavx2")
elseif(AVX)
    set(ArchFlags "-mavx")
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
add_library(tdoku STATIC src/solver_dpll_triad_simd.cc src/util.cc)
target_compile_options(tdoku PUBLIC -fno-exceptions -fno-rtti -fpic)

add_library(rust_sudoku SHARED IMPORTED)
set_target_properties(rust_sudoku PROPERTIES IMPORTED_LOCATION
        ${CMAKE_SOURCE_DIR}/other/rust_sudoku/libsudoku.so)

set(GUROBI_DIR "gurobi900/linux64" CACHE STRING "Gurobi installation directory")

add_library(gurobi_c++ STATIC IMPORTED)
set_target_properties(gurobi_c++ PROPERTIES
        IMPORTED_LOCATION ${GUROBI_DIR}/src/build/libgurobi_c++.a
        INTERFACE_INCLUDE_DIRECTORIES ${GUROBI_DIR}/include)

add_library(gurobi90 SHARED IMPORTED)
set_target_properties(gurobi90 PROPERTIES
        IMPORTED_LOCATION ${GUROBI_DIR}/lib/libgurobi90.so
        INTERFACE_INCLUDE_DIRECTORIES ${GUROBI_DIR}/include)

set(BENCHMARK_SOLVER_SOURCES
        src/solver_basic.cc
        src/solver_dpll_triad_scc.cc
        src/solver_dpll_triad_simd.cc
        other/other_solvers.cc)

set(GENERATE_SOLVER_SOURCES
        src/solver_dpll_triad_simd.cc)

if (BB_SUDOKU)
    add_definitions(-DBB_SUDOKU)
    set(BENCHMARK_SOLVER_SOURCES ${BENCHMARK_SOLVER_SOURCES}
            other/bb_sudoku/bb_sudoku_solver.cpp)
endif()

if (FAST_SOLV_9R2)
    add_definitions(-DFAST_SOLV_9R2)
    set(BENCHMARK_SOLVER_SOURCES ${BENCHMARK_SOLVER_SOURCES}
            other/fast_solv_9r2/fast_solv_9r2.c)
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

if (GSS)
    add_definitions(-DGSS)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DM32bit -DGCC_POPCNT32")
    set(BENCHMARK_SOLVER_SOURCES ${BENCHMARK_SOLVER_SOURCES}
            other/gss/sudoku.c
            other/gss/sudoku_parser.c
            other/gss/gss.c)
endif()

if (GUROBI)
    add_definitions(-DGUROBI)
    set(BENCHMARK_SOLVER_SOURCES ${BENCHMARK_SOLVER_SOURCES}
            other/other_gurobi.cc)
    set(GENERATE_SOLVER_SOURCES ${GENERATE_SOLVER_SOURCES}
            other/other_gurobi.cc)
endif()

if (MINISAT)
    add_definitions(-DMINISAT)
    set(BENCHMARK_SOLVER_SOURCES ${BENCHMARK_SOLVER_SOURCES}
            other/other_minisat.cc)
    set(GENERATE_SOLVER_SOURCES ${GENERATE_SOLVER_SOURCES}
            other/other_minisat.cc)
endif()

if (NORVIG)
    add_definitions(-DNORVIG)
    set(BENCHMARK_SOLVER_SOURCES ${BENCHMARK_SOLVER_SOURCES}
            other/norvig/sudoku.en.cc)
endif()

if (RUST_SUDOKU)
    add_definitions(-DRUST_SUDOKU)
endif()

if (SK_BFORCE2)
  add_definitions(-DSK_BFORCE2)
  set(BENCHMARK_SOLVER_SOURCES ${BENCHMARK_SOLVER_SOURCES}
          other/SK_BFORCE2/sk_bitfields.cpp
          other/SK_BFORCE2/sk_t.cpp)
endif()

add_executable(run_benchmark src/run_benchmark.cc src/util.cc ${BENCHMARK_SOLVER_SOURCES})
add_executable(run_tests test/run_tests.cc src/util.cc ${BENCHMARK_SOLVER_SOURCES})
add_executable(generate src/generate.cc src/util.cc ${GENERATE_SOLVER_SOURCES})

if (GUROBI)
    target_link_libraries(run_benchmark gurobi_c++)
    target_link_libraries(run_benchmark gurobi90)
    target_link_libraries(run_tests gurobi_c++)
    target_link_libraries(run_tests gurobi90)
    target_link_libraries(generate gurobi_c++)
    target_link_libraries(generate gurobi90)
endif()

if (MINISAT)
    target_link_libraries(run_benchmark minisat)
    target_link_libraries(run_tests minisat)
    target_link_libraries(generate minisat)
endif()

if (RUST_SUDOKU)
    target_link_libraries(run_benchmark rust_sudoku)
    target_link_libraries(run_tests rust_sudoku)
endif()
