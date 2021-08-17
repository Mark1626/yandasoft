add_executable(bgridding bgridding.cc)
include_directories(${CMAKE_CURRENT_LIST_DIR})
target_link_libraries(bgridding
  PUBLIC askap::yandasoft
  PUBLIC benchmark::benchmark -pthread
)

add_executable(raw-bench raw-bench.cc)
include_directories(${CMAKE_CURRENT_LIST_DIR})
target_link_libraries(raw-bench
  PUBLIC benchmark::benchmark -pthread
)
