cmake_minimum_required (VERSION 2.8.3)


## Metadata

project (medida)
set (medida_VERSION_MAJOR 0)
set (medida_VERSION_MINOR 1)
set (medida_VERSION_PATCH 0)
set (medida_VERSION_TAG "")
set (medida_VERSION "${medida_VERSION_MAJOR}.${medida_VERSION_MINOR}.${medida_VERSION_PATCH}${medida_VERSION_TAG}")


## Configuration

if(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release")
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -fno-diagnostics-show-option -std=c++0x -Wall -Wextra -pedantic -Wno-long-long -Wno-deprecated -fno-strict-aliasing")

if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
  include(FindPkgConfig)
endif()


## Source

set(medida_SOURCES
  src/medida/stats/snapshot.cc
  src/medida/stats/ewma.cc
  src/medida/stats/uniform_sample.cc
  src/medida/stats/exp_decay_sample.cc
  src/medida/stats/sliding_window_sample.cc
  src/medida/buckets.cc
  src/medida/counter.cc
  src/medida/meter.cc
  src/medida/metric_name.cc
  src/medida/metric_processor.cc
  src/medida/metrics_registry.cc
  src/medida/timer.cc
  src/medida/timer_context.cc
  src/medida/reporting/abstract_polling_reporter.cc
  src/medida/reporting/collectd_reporter.cc
  src/medida/reporting/console_reporter.cc
  src/medida/reporting/json_reporter.cc
  src/medida/reporting/util.cc
  src/medida/histogram.cc
)

set(medida_HEADERS
  src/medida/buckets.h
  src/medida/medida.h
  src/medida/counter.h
  src/medida/histogram.h
  src/medida/meter.h
  src/medida/metered_interface.h
  src/medida/metric_interface.h
  src/medida/metric_name.h
  src/medida/metric_processor.h
  src/medida/metrics_registry.h
  src/medida/sampling_interface.h
  src/medida/summarizable_interface.h
  src/medida/timer.h
  src/medida/timer_context.h
  src/medida/reporting/abstract_polling_reporter.h
  src/medida/reporting/collectd_reporter.h
  src/medida/reporting/console_reporter.h
  src/medida/reporting/json_reporter.h
  src/medida/reporting/util.h
  src/medida/stats/ewma.h
  src/medida/stats/exp_decay_sample.h
  src/medida/stats/sliding_window_sample.h
  src/medida/stats/sample.h
  src/medida/stats/snapshot.h
  src/medida/stats/uniform_sample.h
)

## Dependencies

find_package(Threads REQUIRED)

include_directories(
  ${medida_SOURCE_DIR}/src
  ${medida_BINARY_DIR}
)

add_library(medida SHARED
  ${medida_SOURCES}
  ${medida_PROTO_SRCS}
)

target_link_libraries(medida
  ${CMAKE_THREAD_LIBS_INIT}
)

set_target_properties(medida PROPERTIES
  OUTPUT_NAME medida
  VERSION ${medida_VERSION}
  SOVERSION ${medida_VERSION_MAJOR}
)

install(TARGETS medida
  LIBRARY DESTINATION lib
)

install(FILES
  src/medida/medida.h
  src/medida/counter.h
  src/medida/histogram.h
  src/medida/meter.h
  src/medida/metered_interface.h
  src/medida/metric_interface.h
  src/medida/metric_name.h
  src/medida/metric_processor.h
  src/medida/metrics_registry.h
  src/medida/sampling_interface.h
  src/medida/summarizable_interface.h
  src/medida/timer.h
  src/medida/timer_context.h
  src/medida/types.h
  DESTINATION include/medida/
)

install(FILES
  src/medida/reporting/abstract_polling_reporter.h
  src/medida/reporting/collectd_reporter.h
  src/medida/reporting/console_reporter.h
  src/medida/reporting/json_reporter.h
  src/medida/reporting/util.h
  DESTINATION include/medida/reporting/
)

install(FILES
  src/medida/stats/ewma.h
  src/medida/stats/exp_decay_sample.h
  src/medida/stats/sliding_window_sample.h
  src/medida/stats/sample.h
  src/medida/stats/snapshot.h
  src/medida/stats/uniform_sample.h
  DESTINATION include/medida/stats/
)


## pkgconfig

set(PKGCONFIG_FILE ${medida_BINARY_DIR}/libmedida.pc)
configure_file(libmedida.pc.in ${PKGCONFIG_FILE} @ONLY)
install(FILES ${PKGCONFIG_FILE}
  DESTINATION lib/pkgconfig
)


## Testing

enable_testing()
add_subdirectory(test)


## Documentation

option(BUILD_DOCS "Build HTML docs with Doxygen" OFF)
if(BUILD_DOCS)
  find_package(Doxygen)
  if (NOT DOXYGEN_FOUND)
    message(FATAL_ERROR "Doxygen not installed.")
  endif()
  file(COPY doc DESTINATION ${PROJECT_BINARY_DIR})
  configure_file(doc/doxygen.cfg.in
    ${PROJECT_BINARY_DIR}/doxygen.cfg @ONLY IMMEDIATE
  )

  add_custom_target (docs ALL
    COMMAND ${DOXYGEN_EXECUTABLE} ${PROJECT_BINARY_DIR}/doxygen.cfg
    SOURCES ${PROJECT_BINARY_DIR}/doxygen.cfg
  )
endif()

install(DIRECTORY ${PROJECT_BINARY_DIR}/doc/
  DESTINATION share/doc/libmedida-dev/html/
)


## Examples

add_subdirectory(examples)

install(FILES
  examples/medida_example1.cc
  DESTINATION share/doc/libmedida-dev/examples/
)

