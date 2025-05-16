cmake_minimum_required(VERSION 3.10)
project(TAI_Project3 VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Keep library output at root
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR})

# Include directories
include_directories(include)

# Core and utils libraries
add_library(core
    src/core/WAVReader.cpp
    src/core/FFTExtractor.cpp
    src/core/MaxFreqExtractor.cpp
    src/core/NCD.cpp
    src/core/TreeBuilder.cpp
)

add_library(utils
    src/utils/Segmenter.cpp
    src/utils/NoiseInjector.cpp
    src/utils/CompressorWrapper.cpp
)

# Executables with output placed in apps/ folder
add_executable(extract_features apps/extract_features.cpp)
target_link_libraries(extract_features core utils)
set_target_properties(extract_features PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/apps)

add_executable(compute_ncd apps/compute_ncd.cpp)
target_link_libraries(compute_ncd core utils)
set_target_properties(compute_ncd PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/apps)

add_executable(build_tree apps/build_tree.cpp)
target_link_libraries(build_tree core utils)
set_target_properties(build_tree PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/apps)

add_executable(music_id apps/music_id.cpp)
target_link_libraries(music_id core utils)
set_target_properties(music_id PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/apps)
