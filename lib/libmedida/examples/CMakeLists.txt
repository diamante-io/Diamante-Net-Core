cmake_minimum_required (VERSION 2.8.6)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")

include(FindPkgConfig)

pkg_check_modules(MEDIDA libmedida)

include_directories(${MEDIDA_INCLUDE_DIRS})

link_directories(${MEDIDA_LIBRARY_DIRS})

add_executable(medida-example1 medida_example1.cc)

target_link_libraries(medida-example1 medida)


