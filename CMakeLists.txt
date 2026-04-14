cmake_minimum_required(VERSION 3.16)
project(solver_bench CXX)
set(CMAKE_CXX_STANDARD 17)

option(WITH_MUMPS    "Enable MUMPS solver"      OFF)
option(WITH_SUPERLU  "Enable SuperLU solver"    OFF)
option(WITH_PARDISO  "Enable Intel MKL PARDISO" OFF)

# Fetch Catch2 for tests
include(FetchContent)
FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG        v3.5.2
)
FetchContent_MakeAvailable(Catch2)

# Main library (all non-main sources)
set(BENCH_SOURCES
  src/matrix_io.cpp
  src/rhs_gen.cpp
  src/cond_est.cpp
  src/metrics.cpp
  src/csv_writer.cpp
  src/cli.cpp
  src/worker.cpp
  src/orchestrator.cpp
)

if(WITH_MUMPS)
  list(APPEND BENCH_SOURCES src/mumps_solver.cpp)
  add_compile_definitions(HAVE_MUMPS)
  find_package(MUMPS QUIET)
  if(NOT MUMPS_FOUND)
    find_library(MUMPS_LIB NAMES dmumps dmumps_seq REQUIRED)
    find_library(MUMPS_COMMON NAMES mumps_common mumps_common_seq REQUIRED)
    find_library(MUMPS_PORD NAMES pord QUIET)
    set(MUMPS_LIBRARIES ${MUMPS_LIB} ${MUMPS_COMMON})
    if(MUMPS_PORD)
      list(APPEND MUMPS_LIBRARIES ${MUMPS_PORD})
    endif()
    find_path(MUMPS_INCLUDE_DIR dmumps_c.h REQUIRED)
  endif()
endif()

if(WITH_SUPERLU)
  list(APPEND BENCH_SOURCES src/superlu_solver.cpp)
  add_compile_definitions(HAVE_SUPERLU)
  find_library(SUPERLU_LIB superlu REQUIRED)
  find_path(SUPERLU_INCLUDE_DIR superlu/slu_ddefs.h REQUIRED)
endif()

if(WITH_PARDISO)
  list(APPEND BENCH_SOURCES src/pardiso_solver.cpp)
  add_compile_definitions(HAVE_PARDISO)
  find_package(MKL QUIET)
  if(NOT MKL_FOUND)
    set(MKL_ROOT "/opt/intel/oneapi/mkl/latest" CACHE PATH "MKL root")
    find_library(MKL_CORE_LIB mkl_core HINTS ${MKL_ROOT}/lib/intel64 REQUIRED)
    find_library(MKL_RT_LIB mkl_rt HINTS ${MKL_ROOT}/lib/intel64 REQUIRED)
    set(MKL_LIBRARIES ${MKL_RT_LIB})
    find_path(MKL_INCLUDE_DIR mkl.h HINTS ${MKL_ROOT}/include REQUIRED)
  endif()
endif()

add_library(bench_lib STATIC ${BENCH_SOURCES})
target_include_directories(bench_lib PUBLIC src)

find_package(LAPACK REQUIRED)
target_link_libraries(bench_lib PUBLIC LAPACK::LAPACK)

if(WITH_MUMPS)
  target_include_directories(bench_lib PUBLIC ${MUMPS_INCLUDE_DIR})
  target_link_libraries(bench_lib PUBLIC ${MUMPS_LIBRARIES})
  find_package(MPI REQUIRED)
  target_link_libraries(bench_lib PUBLIC MPI::MPI_CXX)
endif()
if(WITH_SUPERLU)
  target_include_directories(bench_lib PUBLIC ${SUPERLU_INCLUDE_DIR})
  target_link_libraries(bench_lib PUBLIC ${SUPERLU_LIB})
endif()
if(WITH_PARDISO)
  target_include_directories(bench_lib PUBLIC ${MKL_INCLUDE_DIR})
  target_link_libraries(bench_lib PUBLIC ${MKL_LIBRARIES})
endif()

add_executable(solver_bench src/main.cpp)
target_link_libraries(solver_bench PRIVATE bench_lib)

# Tests
enable_testing()
add_executable(tests
  tests/test_stub.cpp
  tests/test_matrix_io.cpp
  tests/test_rhs_gen.cpp
  tests/test_cond_est.cpp
  tests/test_metrics.cpp
  tests/test_csv_writer.cpp
)
target_link_libraries(tests PRIVATE bench_lib Catch2::Catch2WithMain)
include(Catch)
catch_discover_tests(tests WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
