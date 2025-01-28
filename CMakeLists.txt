cmake_minimum_required(VERSION 3.19)

project(STICSIM)

set (CMAKE_CXX_STANDARD 20)

# path cmake modules
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/cmake)

find_package(Seccomp REQUIRED)

# library
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/src/lib)

# main executable
add_executable(sandbox_exe ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp)
target_include_directories(sandbox_exe PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src/lib)
target_link_libraries(sandbox_exe PRIVATE libsandbox)
set_property(TARGET sandbox_exe PROPERTY PROPERTY OUTPUT_NAME sandbox)

# tests
add_executable(tests ${CMAKE_CURRENT_SOURCE_DIR}/tests/tests.cpp)
target_include_directories(tests PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src/lib)
target_link_libraries(tests PRIVATE libsandbox)
 set(SAMPLES_DIR "${CMAKE_BINARY_DIR}/samples")

# let tests know where to find the samples (see next)
target_compile_definitions(tests PUBLIC
  -D SAMPLES_DIR="${SAMPLES_DIR}"
)

# some tests run samples - simple external programs
# that could be sandboxed
add_subdirectory(samples)
