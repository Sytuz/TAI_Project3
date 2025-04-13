# Create a library from your core source files
add_library(tai_src
    core/FCMModel.cpp
    utils/dna_compressor.cpp
    utils/io_utils.cpp
    utils/interface_utils.cpp
)

# Set include directories for this library
target_include_directories(tai_src PUBLIC 
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/core
    ${CMAKE_SOURCE_DIR}/src/utils
)