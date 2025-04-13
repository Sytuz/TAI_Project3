# Create an executable for your main application
add_executable(MetaClass MetaClass.cpp)
target_link_libraries(MetaClass PRIVATE tai_src)
target_include_directories(MetaClass PRIVATE
    ${CMAKE_SOURCE_DIR}/src/core
    ${CMAKE_SOURCE_DIR}/src/utils
)

# Create an executable for your test runner
add_executable(tests tests.cpp)
target_link_libraries(tests PRIVATE tai_src)
target_include_directories(tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/core
    ${CMAKE_SOURCE_DIR}/src/utils
)

# Install executables (optional)
install(TARGETS MetaClass tests
        RUNTIME DESTINATION bin)