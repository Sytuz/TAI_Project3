# T.A.I. - Project 3
## Music Identification using Normalized Compression Distance

## Table of Contents
- [Introduction](#introduction)
- [Project Organization](#the-project-is-organized-as-follows)
- [Overview](#overview)
- [Getting Started](#getting-started)
  - [Prerequisites](#prerequisites)
  - [Automation Scripts](#automation-scripts)
  - [Manual Usage](#manual-usage)
- [Examples](#examples)
  - [Basic Music Identification](#example-1-basic-music-identification)
  - [Testing Robustness with Noise](#example-2-testing-robustness-with-noise)
  - [Comparing Different Compressors](#example-3-comparing-different-compressors)
- [License](#license)

### Introduction:
This project implements a music identification system based on the Normalized Compression Distance (NCD). 
It requires no domain-specific features - it measures similarity purely via compression.
*Cilibrasi et al.* demonstrated a "fully automatic" music classification method that uses only compression.

In our system, the audio files are converted to feature strings (e.g., frequency sequences), and then compared using NCD:

```
NCD(x, y) = (C(xy) - min(C(x), C(y))) / (max(C(x), C(y)))
```

where C(.) is the compressed length (using standard compressors like gzip).
NCD is universal in the sense that it captures all the effective notions of similarity at once.

### The project is organized as follows:
- [apps/](apps/): Executables: music_id, extract_features, compute_ncd, build_tree
- [include/core/](include/core/): Core classes: WAVReader, FFTExtractor, MaxFreqExtractor, NCD
- [include/utils/](include/utils/): Utility classes: Segmenter, NoiseInjector, CompressorWrapper
- [src/core/](src/core/): Implementations of core modules.
- [src/utils/](src/utils/): Implementations of utilities.
- [data/samples/](data/samples/): Input WAV files.
- [data/generated/](data/generated/): Generated feature files per method.
- [results/](results/): Output similarity matrices, Newick trees and images.
- [scripts/](scripts/): Shell scripts and external calls.
- [visualization/](visualization/): Python script to plot trees and heatmaps.
- [CMakeLists.txt](CMakeLists.txt): CMake configuration.

### Overview

- **Input:** WAV files (mono or stereo) in `data/samples/`.
- **Feature Extraction:** Audio is segmented into overlapping frames. Two methods are available:
  - **MaxFreq (provided)**: find dominant frequency per frame.
  - **FFT-based (custom)**: compute FFT magnitudes and select top frequencies.
- **Noise (optional):** Add Gaussian noise at a specified SNR via CLI.
- **Compression:** Use a chosen compressor (`gzip`, `bzip2`, `lzma`, or `zstd`) to compress feature strings.
- **NCD Matrix:** Compute pairwise NCD = `(C(xy) - min(C(x),C(y))) / max(C(x),C(y))` for all features.
- **Tree Building:** Use a quartet-based method to build a tree. Output Newick in `results/` and images.
- **Visualization:** Python scripts (`visualization/`) plot the tree (PNG) or heatmap from the NCD matrix.

## Getting Started

### Prerequisites
- CMake (3.10+)
- C++17 compatible compiler
- Python 3.6+ (for visualization)
- Standard compressors:
  - gzip
  - bzip2
  - lzma
  - zstd

### Automation Scripts

The project includes several automation scripts to simplify common operations:

#### 1. Making Scripts Executable

First, make the scripts executable:

```bash
chmod +x scripts/*.sh
```

#### 2. Building the Project

To build the project:

```bash
./scripts/setup.sh
```

This will create a build directory, run CMake, and build the project.

#### 3. Running Applications

The project includes a launcher script that can run any of the available applications:

```bash
./scripts/run.sh [APP] [APP_OPTIONS]
```

Available applications:
- **music_id** - Full pipeline: extract features, compute NCD, build tree
- **extract_features** - Extract frequency features from WAV files
- **compute_ncd** - Compute NCD matrix between feature files
- **build_tree** - Build a similarity tree from NCD matrix

Examples:

```bash
# Get help for the music_id application
./scripts/run.sh music_id --help

# Run full pipeline with default settings
./scripts/run.sh music_id data/samples results/run1

# Run with custom parameters
./scripts/run.sh music_id --method maxfreq --compressor bzip2 --add-noise 30 data/samples results/custom_run

# Extract features only
./scripts/run.sh extract_features --method fft data/samples data/generated/features
```

#### 4. Cleaning and Rebuilding

If you need to clean and rebuild the project:

```bash
./scripts/rebuild.sh
```

This will remove the build directory and rebuild the project from scratch.

### Manual Usage

If you prefer to run commands manually:

1. Build the project:
```bash
mkdir -p build
cd build
cmake ..
cmake --build .
cd ..
```

2. Run applications directly:
```bash
./apps/music_id --help
./apps/extract_features --help
./apps/compute_ncd --help
./apps/build_tree --help
```

## Examples

### Example 1: Basic Music Identification

```bash
./scripts/run.sh music_id data/samples results/basic_run
```

This will:
1. Extract features from all WAV files in data/samples using FFT method
2. Compute NCD matrix using gzip compression
3. Generate a similarity tree and visualization

### Example 2: Testing Robustness with Noise

```bash
./scripts/run.sh music_id --method fft --add-noise 20 data/samples results/noisy_run
```

This adds noise with SNR = 20dB to test identification robustness.

### Example 3: Comparing Different Compressors

```bash
# Run with different compressors to compare results
./scripts/run.sh music_id --compressor gzip data/samples results/gzip_run
./scripts/run.sh music_id --compressor bzip2 data/samples results/bzip2_run
./scripts/run.sh music_id --compressor lzma data/samples results/lzma_run
```

## License

This project is licensed under the MIT License. See the [LICENSE](LICENSE) file for more details.
