cmake_minimum_required(VERSION 3.10)
project(TAI_Project3 VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add compiler flags similar to the original GetMaxFreqs
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -W")
set(CMAKE_C_FLAGS_RELEASE "-O3")
set(CMAKE_C_FLAGS_DEBUG "-g3")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -W -Wall -std=c++11")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")
set(CMAKE_CXX_FLAGS_DEBUG "-g3")

# Keep library output at root
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR})

# Include directories
include_directories(include)

# Core library
add_library(core
    src/core/WAVReader.cpp
    src/core/SpectralExtractor.cpp
    src/core/MaxFreqExtractor.cpp
    src/core/MaxFreqImprovedExtractor.cpp
    src/core/ProfMaxFreqExtractor.cpp
    src/core/NCD.cpp
    src/core/FeatureExtractor.cpp
    src/core/GetMaxFreqs.cpp
    src/core/Resampler.cpp
)

# Utils library - depends on core
add_library(utils
    src/utils/CompressorWrapper.cpp
)
# Add dependency - utils needs core
target_link_libraries(utils core)
# Link all required libraries for GetMaxFreqs (audio formats + FFT + math)
target_link_libraries(core sndfile FLAC vorbisfile vorbis ogg vorbisenc fftw3 m)

# Executables with output placed in apps/ folder
add_executable(extract_features apps/extract_features.cpp)
target_link_libraries(extract_features core utils)  # Link to both libraries explicitly
set_target_properties(extract_features PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/apps)

add_executable(music_id apps/music_id.cpp)
target_link_libraries(music_id core utils)  # Link to both libraries explicitly
set_target_properties(music_id PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/apps)