# Create a library from your core source files
add_library(tai_core
    core/FCMModel.cpp
    utils/utils.cpp
)

# Set include directories for this library
target_include_directories(tai_core PUBLIC 
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/core
    ${CMAKE_SOURCE_DIR}/src/utils
)