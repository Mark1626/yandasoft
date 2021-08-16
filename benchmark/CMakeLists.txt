add_executable(bgridding bgridding.cc)
include_directories(${CMAKE_CURRENT_LIST_DIR})
target_link_libraries(bgridding
  PUBLIC askap::yandasoft
  PUBLIC benchmark::benchmark -pthread
)
