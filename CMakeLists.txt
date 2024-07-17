cmake_minimum_required(VERSION 3.5)
project(lane_detection)

# Set C++ standard to C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# OpenCV
find_package(OpenCV REQUIRED)

# Source files
file(GLOB CPP_SOURCES *.cpp)

# Add executables
add_executable(main main.cpp ${CPP_SOURCES})

# Specify include dir
#include_directories(src/include)

# Link libraries
target_link_libraries(main ${OpenCV_LIBS})
