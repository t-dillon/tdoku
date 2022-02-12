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

option(ALL           "Include all solvers"             OFF)

option(GSS           "Include GSS"                     OFF)
option(Z3            "Include Z3"                      OFF)
option(GUROBI        "Include GUROBI"                  OFF)
option(MINISAT       "Include MiniSat"                 OFF)
option(TDEV          "Include development solvers"     OFF)
option(LHL           "Include lhl_sudoku"              OFF)
option(ZERODOKU      "Include ZeroDoku"                OFF)
option(FAST_SOLV_9R2 "Include FAST_SOLV_9R2"           OFF)
option(KUDOKU        "Include KUDOKU"                  OFF)
option(NORVIG        "Include norvig"                  OFF)
option(BB_SUDOKU     "Include BB_SUDOKU"               OFF)
option(FSSS          "Include fsss"                    OFF)
option(JSOLVE        "Include JSolve"                  OFF)
option(FSSS2         "Include fsss2"                   OFF)
option(JCZSOLVE      "Include JCZSolve"                OFF)
option(SK_BFORCE2    "Include SK_BFORCE2"              OFF)
option(RUST_SUDOKU   "Include rust_sudoku"             OFF)

if (ALL)
    set(MINISAT ON)
    set(TDEV ON)
    set(LHL ON)
    set(ZERODOKU ON)
    set(FAST_SOLV_9R2 ON)
    set(KUDOKU ON)
    set(NORVIG ON)
    set(BB_SUDOKU ON)
    set(FSSS ON)
    set(JSOLVE ON)
    set(FSSS2 ON)
    set(JCZSOLVE ON)
    set(SK_BFORCE2 ON)
    set(RUST_SUDOKU ON)
endif()

get_filename_component(CCOMPILER "$ENV{CC}" NAME)
if(EXISTS "${CMAKE_SOURCE_DIR}/other/module_rust_sudoku/${CCOMPILER}/libsudoku.so")
    set(HAVE_RUST_SUDOKU_LIB ON)
else()
    set(HAVE_RUST_SUDOKU_LIB OFF)
endif()

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
add_library(tdoku_object OBJECT src/solver_dpll_triad_simd.cc src/util.cc)
target_compile_options(tdoku_object PUBLIC -fno-exceptions -fno-rtti -fpic)

add_library(tdoku_static STATIC $<TARGET_OBJECTS:tdoku_object>)
add_library(tdoku_shared SHARED $<TARGET_OBJECTS:tdoku_object>)

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
        src/solver_dpll_triad_simd.cc
        other/other_solvers.cc)

set(GENERATE_SOLVER_SOURCES
        src/solver_dpll_triad_simd.cc)

if (GSS)
    add_definitions(-DGSS)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DM32bit -DGCC_POPCNT32")
    set(BENCHMARK_SOLVER_SOURCES ${BENCHMARK_SOLVER_SOURCES}
            other/module_gss/sudoku.c
            other/module_gss/sudoku_parser.c
            other/other_gss.c)
endif()

if (Z3)
    add_definitions(-DZ3)
    set(BENCHMARK_SOLVER_SOURCES ${BENCHMARK_SOLVER_SOURCES}
            other/other_z3.cc)
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

if (TDEV)
    add_definitions(-DTDEV)
    set(BENCHMARK_SOLVER_SOURCES ${BENCHMARK_SOLVER_SOURCES}
            src/solver_basic.cc
            src/solver_dpll_triad_scc.cc)
endif()

if (LHL)
    add_definitions(-DLHL)
    set(BENCHMARK_SOLVER_SOURCES ${BENCHMARK_SOLVER_SOURCES}
            other/module_lhl_sudoku/Sudoku2.cpp)
endif()

if (ZERODOKU)
    add_definitions(-DZERODOKU)
    set(BENCHMARK_SOLVER_SOURCES ${BENCHMARK_SOLVER_SOURCES}
            other/zerodoku/zerodoku_v2.c)
endif()

if (FAST_SOLV_9R2)
    add_definitions(-DFAST_SOLV_9R2)
    set(BENCHMARK_SOLVER_SOURCES ${BENCHMARK_SOLVER_SOURCES}
            other/fast_solv_9r2/fast_solv_9r2.c)
endif()

if (KUDOKU)
    add_definitions(-DKUDOKU)
    set(BENCHMARK_SOLVER_SOURCES ${BENCHMARK_SOLVER_SOURCES}
            other/kudoku/kudoku.c)
endif()

if (NORVIG)
    add_definitions(-DNORVIG)
    set(BENCHMARK_SOLVER_SOURCES ${BENCHMARK_SOLVER_SOURCES}
            other/module_norvig_sudoku/sudoku.en.cc)
endif()

if (BB_SUDOKU)
    add_definitions(-DBB_SUDOKU)
    set(BENCHMARK_SOLVER_SOURCES ${BENCHMARK_SOLVER_SOURCES}
            other/bb_sudoku/bb_sudoku_solver.cpp)
endif()

if (FSSS)
    add_definitions(-DFSSS)
    set(BENCHMARK_SOLVER_SOURCES ${BENCHMARK_SOLVER_SOURCES}
            other/fsss/fsss.cc)
endif()

if (JSOLVE)
    add_definitions(-DJSOLVE)
    set(BENCHMARK_SOLVER_SOURCES ${BENCHMARK_SOLVER_SOURCES}
            other/jsolve/JSolve.c)
endif()

if (FSSS2)
    add_definitions(-DFSSS2)
    set(BENCHMARK_SOLVER_SOURCES ${BENCHMARK_SOLVER_SOURCES}
            other/module_fsss2/fsss2.cpp
            other/module_fsss2/t_128.cpp)
    set (CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -Wno-deprecated-register")
endif()

if (JCZSOLVE)
    add_definitions(-DJCZSOLVE)
    set(BENCHMARK_SOLVER_SOURCES ${BENCHMARK_SOLVER_SOURCES}
            other/jczsolve/JCZSolve.c)
endif()

if (SK_BFORCE2)
  add_definitions(-DSK_BFORCE2)
  set(BENCHMARK_SOLVER_SOURCES ${BENCHMARK_SOLVER_SOURCES}
          other/module_sk_bforce2/sk_bitfields.cpp
          other/module_sk_bforce2/sk_t.cpp)
endif()

if (RUST_SUDOKU AND HAVE_RUST_SUDOKU_LIB)
    add_definitions(-DRUST_SUDOKU)
endif()

add_executable(run_benchmark src/run_benchmark.cc src/util.cc ${BENCHMARK_SOLVER_SOURCES})
add_executable(run_tests test/run_tests.cc src/util.cc ${BENCHMARK_SOLVER_SOURCES})
add_executable(generate src/generate.cc src/util.cc ${GENERATE_SOLVER_SOURCES})

add_library(grid_lib STATIC src/grid_lib.cc)
target_compile_options(grid_lib PUBLIC -fno-exceptions -fno-rtti -fpic)
target_include_directories(grid_lib PUBLIC include)
target_link_libraries(grid_lib tdoku_static)

add_executable(grid_tools src/grid_tools.cc)
target_include_directories(grid_tools PUBLIC include)
target_link_libraries(grid_tools grid_lib)
target_link_libraries(grid_tools tdoku_static)

if (Z3)
    target_link_libraries(run_benchmark z3)
    target_link_libraries(run_tests z3)
endif()

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

if (RUST_SUDOKU AND HAVE_RUST_SUDOKU_LIB)
    target_link_libraries(run_benchmark ${CMAKE_SOURCE_DIR}/other/module_rust_sudoku/${CCOMPILER}/libsudoku.so)
    target_link_libraries(run_tests     ${CMAKE_SOURCE_DIR}/other/module_rust_sudoku/${CCOMPILER}/libsudoku.so)
endif()
