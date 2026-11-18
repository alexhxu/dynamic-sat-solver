cmake_minimum_required(VERSION 3.16)
project(dynamic-sat-solver LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(ZLIB REQUIRED)
find_package(Threads REQUIRED)

# MiniSat 2.2, vendored from https://github.com/niklasso/minisat
# commit 37dc6c67e2af26379d88ce349eb9c4c6160e8543
add_library(minisat STATIC
    third_party/minisat/minisat/utils/Options.cc
    third_party/minisat/minisat/utils/System.cc
    third_party/minisat/minisat/core/Solver.cc
    third_party/minisat/minisat/simp/SimpSolver.cc
)
target_include_directories(minisat SYSTEM PUBLIC third_party/minisat)
target_compile_definitions(minisat PUBLIC __STDC_FORMAT_MACROS __STDC_LIMIT_MACROS)
target_link_libraries(minisat PUBLIC ZLIB::ZLIB)
# MiniSat predates C++11 literal rules ("%"PRIi64). The flag is PUBLIC because
# our translation units include those headers. -w stays private so our code
# is still built with -Wall.
target_compile_options(minisat PRIVATE -w)
target_compile_options(minisat PUBLIC -Wno-reserved-user-defined-literal)

add_executable(dssolver
    src/main.cpp
    src/Cube.cpp
    src/Worker.cpp
    src/Master.cpp
    src/Metrics.cpp
)
target_include_directories(dssolver PRIVATE src)
target_link_libraries(dssolver PRIVATE minisat Threads::Threads)
target_compile_options(dssolver PRIVATE -Wall -Wextra -Wpedantic)
