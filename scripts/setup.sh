#!/bin/bash

echo "===== Setting up TAI Project 3 ====="
mkdir -p build
cd build
cmake .. && cmake --build .
BUILD_STATUS=$?
cd ..

if [ $BUILD_STATUS -eq 0 ]; then
    echo "✅ Build completed successfully"
    echo "Run './scripts/run.sh --help' to see available options"
else
    echo "❌ Build failed with status $BUILD_STATUS"
    echo "Check the error messages above"
fi