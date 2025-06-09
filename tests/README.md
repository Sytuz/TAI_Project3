# Music Identification Tests

This folder contains test scripts for the project.

## Test Scripts

### 1. simple_test
- **File**: `simple_test.cpp`
- **Purpose**: Quick test with 5 sample songs
- **Features**: Self-similarity testing, cross-song comparison matrix, identification ranking
- **Output**: `simple_test_results.txt`

### 2. complete_test
- **File**: `complete_test.cpp`
- **Purpose**: Comprehensive test with all 34 sample songs
- **Features**: Full pairwise comparison, accuracy analysis, confusion matrix, genre detection
- **Output**: `complete_test_results.txt`

### 3. youtube_test
- **File**: `youtube_test.cpp`
- **Purpose**: Large-scale test with 100 YouTube songs
- **Features**: Subset testing, compressor comparison, genre diversity analysis
- **Output**: `youtube_test_results.txt`

## Usage

```bash
# Compile all tests
cd tests
g++ -std=c++17 -o simple_test simple_test.cpp
g++ -std=c++17 -o complete_test complete_test.cpp
g++ -std=c++17 -o youtube_test youtube_test.cpp

# Run tests
./simple_test      # Quick 5-song test
./complete_test    # All 34 sample songs
./youtube_test     # YouTube songs analysis
```

## Output Files

All tests generate detailed results in text files:
- `simple_test_results.txt`
- `complete_test_results.txt`
- `youtube_test_results.txt`

## Features

### All Tests Include:
- **Dual Output**: Results displayed on console AND saved to file
- **Timing**: Complete execution time tracking
- **Automatic Feature Extraction**: Tests extract features if needed
- **Error Handling**: Graceful handling of missing files
- **Progress Tracking**: Real-time status updates

### Advanced Analysis:
- **NCD Calculation**: Using LZMA compression for similarity measurement
- **Accuracy Metrics**: Identification success rates
- **Confusion Analysis**: Most problematic song pairs
- **Genre Classification**: Automatic genre detection and diversity testing
- **Compressor Comparison**: Performance with different compression algorithms
