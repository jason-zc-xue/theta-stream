cmake_minimum_required(VERSION 3.10)

# Set the project name and version
project(ThetaStream VERSION 1.0)

# Specify the C++ standard
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Find the required packages
find_package(PkgConfig REQUIRED)
pkg_search_module(gstreamer-app REQUIRED IMPORTED_TARGET gstreamer-app-1.0>=1.4)
pkg_search_module(libuvc REQUIRED IMPORTED_TARGET libuvc)

# Define source files
set(SOURCES
    gst_viewer.cpp
    thetauvc.c
)

# Create an executable for gst_viewer
add_executable(gst_viewer ${SOURCES})

# Link the gst_viewer executable with the required libraries
target_link_libraries(gst_viewer
    PkgConfig::gstreamer-app PkgConfig::libuvc pthread
)
