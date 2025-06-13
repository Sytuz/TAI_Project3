#!/bin/bash

echo "===== Cleaning and rebuilding TAI Project 3 ====="
echo "Removing build directory..."
rm -rf build

echo "Creating fresh build..."
mkdir -p build
cd build
cmake .. && cmake --build .
BUILD_STATUS=$?
cd ..

if [ $BUILD_STATUS -eq 0 ]; then
    echo "✅ Rebuild completed successfully"
    echo "Run './scripts/run.sh --help' to see available options"
else
    echo "❌ Rebuild failed with status $BUILD_STATUS"
    echo "Check the error messages above"
fi