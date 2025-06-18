# TAI Project 3: Music Identification System Using Normalized Compression Distance

> **Note:**  
> Due to the large size of the `data/` folder, it is not included in this repository.  
> You can download all necessary data files from OneDrive:
> [OneDrive Data Link](https://uapt33090-my.sharepoint.com/:f:/g/personal/miguel_silva48_ua_pt/EtIouc9ouu9NvwaldgWkJtkB0gpc0nuHcEp-W24OWsZEOA?e=J7YqAl)
>
> The most relevant files for reproducibility are the feature signatures, located in `data/features/`.

## Table of Contents

- [TAI Project 3: Music Identification System Using Normalized Compression Distance](#tai-project-3-music-identification-system-using-normalized-compression-distance)
  - [Table of Contents](#table-of-contents)
  - [Documentation](#documentation)
  - [Introduction](#introduction)
    - [Main Capabilities:](#main-capabilities)
  - [Files Structure:](#files-structure)
    - [Core Applications (`apps/`)](#core-applications-apps)
    - [Core Library (`src/core/`, `include/core/`)](#core-library-srccore-includecore)
    - [Utilities (`src/utils/`, `include/utils/`)](#utilities-srcutils-includeutils)
    - [Scripts (`scripts/`)](#scripts-scripts)
    - [Configuration (`config/`)](#configuration-config)
    - [Data Organization (`data/`)](#data-organization-data)
  - [Setup Instructions](#setup-instructions)
    - [Prerequisites](#prerequisites)
    - [Dependencies Installation](#dependencies-installation)
      - [Ubuntu/Debian:](#ubuntudebian)
      - [Install Python Dependencies:](#install-python-dependencies)
    - [Build Instructions](#build-instructions)
  - [Run Instructions](#run-instructions)
    - [Basic Usage](#basic-usage)
      - [1. Extract Features from Audio Files](#1-extract-features-from-audio-files)
      - [2. Identify Music](#2-identify-music)
    - [Advanced Usage](#advanced-usage)
      - [Automated Testing Pipeline](#automated-testing-pipeline)
      - [Analysis and Visualization](#analysis-and-visualization)
  - [Implementation](#implementation)
    - [Feature Extraction Methods](#feature-extraction-methods)
      - [1. Spectral Method](#1-spectral-method)
      - [2. Maximum Frequency Method](#2-maximum-frequency-method)
      - [3. Professor's Maximum Frequency Method (ProfMaxFreq)](#3-professors-maximum-frequency-method-profmaxfreq)
    - [Normalized Compression Distance (NCD)](#normalized-compression-distance-ncd)
      - [Implementation Features:](#implementation-features)
    - [Audio Processing Pipeline](#audio-processing-pipeline)
    - [Multi-threading Support](#multi-threading-support)
  - [Obtained Results](#obtained-results)
    - [Performance Overview](#performance-overview)
      - [36 Songs Dataset:](#36-songs-dataset)
      - [100 Songs Dataset:](#100-songs-dataset)
    - [Findings](#findings)
      - [1. Genre-Dependent Performance](#1-genre-dependent-performance)
      - [2. Feature Extraction Method Comparison](#2-feature-extraction-method-comparison)
      - [3. Compressor Analysis](#3-compressor-analysis)
      - [4. Noise Robustness](#4-noise-robustness)
    - [Performance Visualizations](#performance-visualizations)
  - [Conclusion](#conclusion)
    - [Achievements](#achievements)
    - [Limitations and Future Work](#limitations-and-future-work)
    - [License](#license)
  - [Changes after the Presentation](#changes-after-the-presentation)
    - [Maximum Frequency Method Comparison: Custom vs. Professor's Implementation](#maximum-frequency-method-comparison-custom-vs-professors-implementation)
      - [Key Algorithmic Differences](#key-algorithmic-differences)
    - [Improved Maximum Frequency Extractor](#improved-maximum-frequency-extractor)
      - [Improvements in MaxFreqImproved](#improvements-in-maxfreqimproved)
      - [Comparison Table](#comparison-table)
    - [Performance Impact](#performance-impact)
      - [36 Songs Dataset](#36-songs-dataset-1)
      - [100 Songs Dataset, reduced to 32 songs for lower computational cost](#100-songs-dataset-reduced-to-32-songs-for-lower-computational-cost)
  - [Authors](#authors)

## Documentation

- [Project Poster (PDF)](doc/poster.pdf)

## Introduction

This project implements a **Music Identification System** that uses **Normalized Compression Distance (NCD)** combined with **audio feature extraction** to identify songs from short audio samples. The system is capable of identifying some music tracks even when they are corrupted with various types of noise (brown, pink, and white) and can work with different audio feature extraction methods.

### Main Capabilities:

- **Audio Feature Extraction:** Extract frequency-domain features from WAV files using two different methods:
    - **Spectral Method:** Binned frequencyy spectrum using FFT (Fast Fourier Transform).
    - **Maximum Frequency Method:** Top dominant frequencies per frame.
- **Music Identification:** Identify songs from short samples (10 seconds) by comparing extracted features usign NCD.
- **Noise Robustness:** Handle audio samples corrupted with different types of noise (brown, pink, white).
- **Mulltiple Compressors:** Support for various compression algorithms (gzip, bzip2, lzma, zstd) for NCD calculation.
- **Batch Processing:** Automated testing and analysis pipelines for large datasets.
- **Performance Analysis**: Accuracy analysis with visualization tools.

The system was tested in two datasets:
- A smaller one (36 songs): With maximum accuracy 26.5%.
- A bigger one (100 songs): With maximum accuracy 23%, depending on the music genre (with Hip-Hop/Rap showing the best performance and Classical/Orchestral being the most challenging).

## Files Structure:

### Core Applications (`apps/`)
- **`extract_features.cpp`**: Main application for extracting frequency features from WAV files
    - Supports both spectral and maxfreq methods
    - Multi-threaded processing
    - Binary and text output formats
- **`music_id.cpp`**: Music identification application that compares query features against a database using NCD
  - Supports multiple compressors
  - Top-K accuracy reporting
  - Direct WAV file processing or pre-extracted features

### Core Library (`src/core/`, `include/core/`)
- **`FeatureExtractor.h/.cpp`**: Feature extraction utilities and file I/O
- **`SpectralExtractor.h/.cpp`**: FFT-based spectral analysis with binned frequency representation
- **`MaxFreqExtractor.h/.cpp`**: Extraction of dominant frequencies per audio frame
- **`WAVReader.h/.cpp`**: WAV file parsing and audio data extraction
- **`NCD.h/.cpp`**: Normalized Compression Distance implementation using external compressors

### Utilities (`src/utils/`, `include/utils/`)
- **`CompressorWrapper.h/.cpp`**: Wrapper for external compression tools (gzip, bzip2, lzma, zstd)
- **`json.hpp`**: JSON parsing library for configuration files

### Scripts (`scripts/`)
- **`batch_identification.sh`**: Process a batch of queries against the database for one compressor (to compute NCD)
- **`compare_compressors.sh`**: Run the `batch_identification.sh` in threads (one thread per processor)
- **`extract_sample.sh`**: Extract audio samples with noise injection
- **`rebuild.sh`**: Rebuild the system setup using CMake
- **`run.sh`**: Unified runner for all applications
- **`setup.sh`**: Build system setup using CMake
- **`tests.sh`**: Perform tests to the datasets
- **`calculate_accuracy.py`**: Accuracy analysis and metrics calculation
- **`generate_plots.py` & `generate_plots_genres.py`**: Visualization and analysis plots generation
- **`download_songs.cpp`**: Realize download of the listed songs directly from Youtube


### Configuration (`config/`)
- **`feature_extraction_spectral_default.json`**: Default spectral method parameters
- **`feature_extraction_maxfreq_default.json`**: Default maxfreq method parameters
- High-resolution variants for improved accuracy

### Data Organization (`data/`)
- **`full_tracks/`**: Complete music tracks for database
- **`samples/`**: Extracted samples with various noise conditions
- **`features/`**: Extracted features in text/binary format
- **`generated/`**: Processed datasets

## Setup Instructions

### Prerequisites

- **C++ Compiler**: GCC 7+ or Clang with C++17 support
- **CMake**: Version 3.10 or higher
- **External Tools**: gzip, bzip2, xz (for lzma), zstd
- **Python 3.10+** (for analysis scripts)
- **FFmpeg** (for audio processing in scripts)

### Dependencies Installation

#### Ubuntu/Debian:
```bash
sudo apt update
sudo apt install build-essential cmake
sudo apt install gzip bzip2 xz-utils zstd   # Usually included in most Linux/Unix installations
sudo apt install python3 python3-pip ffmpeg
```

#### Install Python Dependencies:
```bash
pip install -r requirements.txt
```

### Build Instructions

1. **Clone and Navigate**:
   ```bash
   cd /path/to/TAI_Project3
   ```

2. **Run Setup Script**:
   ```bash
   ./scripts/setup.sh
   ```

   This will:
   - Create build directory
   - Run CMake configuration
   - Compile the project
   - Generate executables in `apps/`

3. **Verify Build**:   ```bash
   ls apps/
   \# Should show: extract_features music_id
   ```

## Run Instructions

### Basic Usage

#### 1. Extract Features from Audio Files
```bash
# Extract spectral features
./scripts/run.sh extract_features --method spectral --bins 64 -i input_folder/ -o output_features/

# Extract maximum frequency features
./scripts/run.sh extract_features --method maxfreq --frequencies 4 -i input_folder/ -o output_features/
```

#### 2. Identify Music
```bash
# Identify using pre-extracted features
./scripts/run.sh music_id query.feat database_folder/ results.json

# Identify directly from WAV file
./scripts/run.sh music_id query.wav database_folder/ results.json --config config/feature_extraction_spectral_default.json
```

### Advanced Usage

#### Automated Testing Pipeline
```bash
# Run test with noise analysis
./scripts/tests.sh  # Change there the dataset to use

# Compare different compressors
./scripts/compare_compressors.sh -q queries/ -d database/ -o results/ -c gzip,bzip2,lzma,zstd
```
#### Analysis and Visualization
```bash
# Calculate accuracy metrics
python3 scripts/calculate_accuracy.py results/

# Generate performance plots
python3 scripts/generate_plots.py
python3 scripts/generate_plots_genres.py
```

## Implementation

### Feature Extraction Methods

#### 1. Spectral Method
- **FFT Implementation**: Cooley-Tukey FFT algorithm
- **Windowing**: Window function for spectral leakage reduction
- **Binning**: Logarithmically scaled frequency bins for perceptual relevance
- **Normalization**: Loggaritmic scaling for numerical stability

#### 2. Maximum Frequency Method
- **Peak Detection**: Identifies dominant frequencies in each frame
- **Frequency Ranking**: Sorts frequencies by magnitude
- **Compact Representation**: Stores only top N frequencies per frame

#### 3. Professor's Maximum Frequency Method (ProfMaxFreq)
- **Reference Implementation**: Based on the original GetMaxFreqs.cpp algorithm provided by professors
- **Optimized Parameters**: Uses empirically validated parameters for music identification
- **Domain-Specific Design**: Tailored specifically for audio fingerprinting applications

### Normalized Compression Distance (NCD)

The core similarity measure is based on Kolmogorov complexity theory:

```
NCD(x,y) = (C(xy) - min(C(x), C(y))) / max(C(x), C(y))
```

Where:
- `C(x)` = compressed size of file x
- `C(xy)` = compressed size of concatenated files x and y
- Result ranges from 0 (identical) to 1 (completely different)

#### Implementation Features:
- **Multiple Compressors**: Support for gzip, bzip2, lzma, zstd
- **Error Handling**: File I/O and compression error management
- **Temporary File Management**: Safe concatenation with cleanup
- **Range Clamping**: Ensures valid NCD values [0,1]

### Audio Processing Pipeline

1. **WAV File Reading**: 8/16/24/32-bit PCM audio parsing with metadata extraction
2. **Frame Segmentation**: Overlapping windows (typically 1024 samples, 512 hop)
3. **Feature Extraction**: Either spectral binning or frequency peak detection
4. **Serialization**: Text or binary feature file output
5. **Database Comparison**: NCD calculation against all database entries
6. **Ranking**: Sort results by similarity score

### Multi-threading Support

- **Parallel Feature Extraction**: Process multiple files simultaneously
- **Thread-safe I/O**: Mutex protection for console output and file operations
- **Load Balancing**: Dynamic work distribution across available cores

## Obtained Results

### Performance Overview

The system was tested on two datasets:

#### 36 Songs Dataset:

**V1**: 10 seconds samples, avoiding the first and last 5 seconds of each song

| Noise | Method | Type | Compressor | Top-1 Accuracy (%) |
|-------|--------|------|------------|----------------|
| Clean | Maxfreq | Text | gzip | 8.8 |
| Clean | Maxfreq | Text | bzip2 | 26.5 |
| Clean | Maxfreq | Text | lzma | 23.5 |
| Clean | Maxfreq | Text | zstd | 17.6 |
| Clean | Maxfreq | Binary | gzip | 5.9 |
| Clean | Maxfreq | Binary | bzip2 | 14.7 |
| Clean | Maxfreq | Binary | lzma | 2.9 |
| Clean | Maxfreq | Binary | zstd | 2.9 |
| Clean | Spectral | Text | gzip | 5.9 |
| Clean | Spectral | Text | bzip2 | 8.8 |
| Clean | Spectral | Text | lzma | 14.7 |
| Clean | Spectral | Text | zstd | 11.8 |
| Clean | Spectral | Binary | gzip | 8.8 |
| Clean | Spectral | Binary | bzip2 | 14.7 |
| Clean | Spectral | Binary | lzma | 2.9 |
| Clean | Spectral | Binary | zstd | 2.9 |
| Brown | Maxfreq | Text | gzip | 5.9 |
| Brown | Maxfreq | Text | bzip2 | 5.9 |
| Brown | Maxfreq | Text | lzma | 5.9 |
| Brown | Maxfreq | Text | zstd | 5.9 |
| Brown | Maxfreq | Binary | gzip | 5.9 |
| Brown | Maxfreq | Binary | bzip2 | 5.9 |
| Brown | Maxfreq | Binary | lzma | 5.9 |
| Brown | Maxfreq | Binary | zstd | 5.9 |
| Brown | Spectral | Text | gzip | 5.9 |
| Brown | Spectral | Text | bzip2 | 8.8 |
| Brown | Spectral | Text | lzma | 8.8 |
| Brown | Spectral | Text | zstd | 2.9 |
| Brown | Spectral | Binary | gzip | 5.9 |
| Brown | Spectral | Binary | bzip2 | 11.8 |
| Brown | Spectral | Binary | lzma | 2.9 |
| Brown | Spectral | Binary | zstd | 2.9 |
| Pink | Maxfreq | Text | gzip | 8.8 |
| Pink | Maxfreq | Text | bzip2 | 17.6 |
| Pink | Maxfreq | Text | lzma | 5.9 |
| Pink | Maxfreq | Text | zstd | 8.8 |
| Pink | Maxfreq | Binary | gzip | 5.9 |
| Pink | Maxfreq | Binary | bzip2 | 8.8 |
| Pink | Maxfreq | Binary | lzma | 5.9 |
| Pink | Maxfreq | Binary | zstd | 2.9 |
| Pink | Spectral | Text | gzip | 2.9 |
| Pink | Spectral | Text | bzip2 | 8.8 |
| Pink | Spectral | Text | lzma | 8.8 |
| Pink | Spectral | Text | zstd | 2.9 |
| Pink | Spectral | Binary | gzip | 5.9 |
| Pink | Spectral | Binary | bzip2 | 8.8 |
| Pink | Spectral | Binary | lzma | 0.0 |
| Pink | Spectral | Binary | zstd | 2.9 |
| White | Maxfreq | Text | gzip | 5.9 |
| White | Maxfreq | Text | bzip2 | 17.6 |
| White | Maxfreq | Text | lzma | 2.9 |
| White | Maxfreq | Text | zstd | 5.9 |
| White | Maxfreq | Binary | gzip | 8.8 |
| White | Maxfreq | Binary | bzip2 | 8.8 |
| White | Maxfreq | Binary | lzma | 5.9 |
| White | Maxfreq | Binary | zstd | 0.0 |
| White | Spectral | Text | gzip | 2.9 |
| White | Spectral | Text | bzip2 | 5.9 |
| White | Spectral | Text | lzma | 5.9 |
| White | Spectral | Text | zstd | 2.9 |
| White | Spectral | Binary | gzip | 8.8 |
| White | Spectral | Binary | bzip2 | 8.8 |
| White | Spectral | Binary | lzma | 2.9 |
| White | Spectral | Binary | zstd | 0.0 |

**V2**: Everything doubled, so 20 seconds samples, avoiding the first and last 10 seconds of each song

| Noise | Method | Type | Compressor | Top-1 Accuracy (%) |
|-------|--------|------|------------|----------------|
| Clean | Maxfreq | Text | gzip | 8.8 |
| Clean | Maxfreq | Text | bzip2 | 32.4 |
| Clean | Maxfreq | Text | lzma | 23.5 |
| Clean | Maxfreq | Text | zstd | 23.5 |
| Clean | Maxfreq | Binary | gzip | 11.8 |
| Clean | Maxfreq | Binary | bzip2 | 29.4 |
| Clean | Maxfreq | Binary | lzma | 14.7 |
| Clean | Maxfreq | Binary | zstd | 8.8 |
| Clean | Spectral | Text | gzip | 2.9 |
| Clean | Spectral | Text | bzip2 | 14.7 |
| Clean | Spectral | Text | lzma | 14.7 |
| Clean | Spectral | Text | zstd | 11.8 |
| Clean | Spectral | Binary | gzip | 2.9 |
| Clean | Spectral | Binary | bzip2 | 8.8 |
| Clean | Spectral | Binary | lzma | 8.8 |
| Clean | Spectral | Binary | zstd | 5.9 |
| Brown | Maxfreq | Text | gzip | 5.9 |
| Brown | Maxfreq | Text | bzip2 | 11.8 |
| Brown | Maxfreq | Text | lzma | 2.9 |
| Brown | Maxfreq | Text | zstd | 5.9 |
| Brown | Maxfreq | Binary | gzip | 8.8 |
| Brown | Maxfreq | Binary | bzip2 | 8.8 |
| Brown | Maxfreq | Binary | lzma | 2.9 |
| Brown | Maxfreq | Binary | zstd | 8.8 |
| Brown | Spectral | Text | gzip | 2.9 |
| Brown | Spectral | Text | bzip2 | 11.8 |
| Brown | Spectral | Text | lzma | 5.9 |
| Brown | Spectral | Text | zstd | 5.9 |
| Brown | Spectral | Binary | gzip | 2.9 |
| Brown | Spectral | Binary | bzip2 | 5.9 |
| Brown | Spectral | Binary | lzma | 0.0 |
| Brown | Spectral | Binary | zstd | 5.9 |
| Pink | Maxfreq | Text | gzip | 5.9 |
| Pink | Maxfreq | Text | bzip2 | 14.7 |
| Pink | Maxfreq | Text | lzma | 2.9 |
| Pink | Maxfreq | Text | zstd | 5.9 |
| Pink | Maxfreq | Binary | gzip | 11.8 |
| Pink | Maxfreq | Binary | bzip2 | 14.7 |
| Pink | Maxfreq | Binary | lzma | 11.8 |
| Pink | Maxfreq | Binary | zstd | 2.9 |
| Pink | Spectral | Text | gzip | 2.9 |
| Pink | Spectral | Text | bzip2 | 14.7 |
| Pink | Spectral | Text | lzma | 5.9 |
| Pink | Spectral | Text | zstd | 2.9 |
| Pink | Spectral | Binary | gzip | 2.9 |
| Pink | Spectral | Binary | bzip2 | 5.9 |
| Pink | Spectral | Binary | lzma | 2.9 |
| Pink | Spectral | Binary | zstd | 5.9 |
| White | Maxfreq | Text | gzip | 5.9 |
| White | Maxfreq | Text | bzip2 | 32.4 |
| White | Maxfreq | Text | lzma | 2.9 |
| White | Maxfreq | Text | zstd | 5.9 |
| White | Maxfreq | Binary | gzip | 11.8 |
| White | Maxfreq | Binary | bzip2 | 32.4 |
| White | Maxfreq | Binary | lzma | 5.9 |
| White | Maxfreq | Binary | zstd | 5.9 |
| White | Spectral | Text | gzip | 2.9 |
| White | Spectral | Text | bzip2 | 5.9 |
| White | Spectral | Text | lzma | 5.9 |
| White | Spectral | Text | zstd | 2.9 |
| White | Spectral | Binary | gzip | 2.9 |
| White | Spectral | Binary | bzip2 | 5.9 |
| White | Spectral | Binary | lzma | 2.9 |
| White | Spectral | Binary | zstd | 2.9 |

#### 100 Songs Dataset:

A diverse music dataset with 100 songs across 10 genres:

| Genre | Top-1 Accuracy | Top-5 Accuracy | Sample Count |
|-------|----------------|----------------|--------------|
| Blues | 10.2% | 17.2% | 640 |
| Hip-Hop / Rap | 10.0% | 16.7% | 640 |
| Rock | 6.4% | 12.2% | 704 |
| Pop | 5.5% | 9.5% | 640 |
| Country / Folk | 4.7% | 12.3% | 576 |
| Reggae | 4.5% | 13.1% | 640 |
| Electronic / Dance | 4.1% | 5.7% | 704 |
| Latin / World | 4.1% | 9.5% | 640 |
| Jazz | 3.1% | 7.2% | 640 |
| Classical / Orchestral | 2.4% | 4.5% | 576 |

| Noise | Method | Type | Compressor | Top-1 Accuracy (%) |
|-------|--------|------|------------|----------------|
| Clean | Maxfreq | Text | gzip | 2.0 |
| Clean | Maxfreq | Text | bzip2 | 11.0 |
| Clean | Maxfreq | Text | lzma | 18.0 |
| Clean | Maxfreq | Text | zstd | 19.0 |
| Clean | Maxfreq | Binary | gzip | 1.0 |
| Clean | Maxfreq | Binary | bzip2 | 17.0 |
| Clean | Maxfreq | Binary | lzma | 16.0 |
| Clean | Maxfreq | Binary | zstd | 11.0 |
| Clean | Spectral | Text | gzip | 1.0 |
| Clean | Spectral | Text | bzip2 | 4.0 |
| Clean | Spectral | Text | lzma | 19.0 |
| Clean | Spectral | Text | zstd | 7.0 |
| Clean | Spectral | Binary | gzip | 1.0 |
| Clean | Spectral | Binary | bzip2 | 6.0 |
| Clean | Spectral | Binary | lzma | 23 |
| Clean | Spectral | Binary | zstd | 23 |
| Brown | Maxfreq | Text | gzip | 1.0 |
| Brown | Maxfreq | Text | bzip2 | 3.0 |
| Brown | Maxfreq | Text | lzma | 1.0 |
| Brown | Maxfreq | Text | zstd | 1.0 |
| Brown | Maxfreq | Binary | gzip | 1.0 |
| Brown | Maxfreq | Binary | bzip2 | 3.0 |
| Brown | Maxfreq | Binary | lzma | 1.0 |
| Brown | Maxfreq | Binary | zstd | 2.0 |
| Brown | Spectral | Text | gzip | 1.0 |
| Brown | Spectral | Text | bzip2 | 0.0 |
| Brown | Spectral | Text | lzma | 0.0 |
| Brown | Spectral | Text | zstd | 2.0 |
| Brown | Spectral | Binary | gzip | 1.0 |
| Brown | Spectral | Binary | bzip2 | 2.0 |
| Brown | Spectral | Binary | lzma | 1.0 |
| Brown | Spectral | Binary | zstd | 1.0 |
| Pink | Maxfreq | Text | gzip | 1.0 |
| Pink | Maxfreq | Text | bzip2 | 6.0 |
| Pink | Maxfreq | Text | lzma | 3.0 |
| Pink | Maxfreq | Text | zstd | 2.0 |
| Pink | Maxfreq | Binary | gzip | 1.0 |
| Pink | Maxfreq | Binary | bzip2 | 3.0 |
| Pink | Maxfreq | Binary | lzma | 4.0 |
| Pink | Maxfreq | Binary | zstd | 5.0 |
| Pink | Spectral | Text | gzip | 1.0 |
| Pink | Spectral | Text | bzip2 | 0.0 |
| Pink | Spectral | Text | lzma | 0.0 |
| Pink | Spectral | Text | zstd | 0.0 |
| Pink | Spectral | Binary | gzip | 1.0 |
| Pink | Spectral | Binary | bzip2 | 2.0 |
| Pink | Spectral | Binary | lzma | 0.0 |
| Pink | Spectral | Binary | zstd | 1.0 |
| White | Maxfreq | Text | gzip | 1.0 |
| White | Maxfreq | Text | bzip2 | 4.0 |
| White | Maxfreq | Text | lzma | 1.0 |
| White | Maxfreq | Text | zstd | 1.0 |
| White | Maxfreq | Binary | gzip | 2.0 |
| White | Maxfreq | Binary | bzip2 | 6.0 |
| White | Maxfreq | Binary | lzma | 1.0 |
| White | Maxfreq | Binary | zstd | 3.0 |
| White | Spectral | Text | gzip | 1.0 |
| White | Spectral | Text | bzip2 | 0.0 |
| White | Spectral | Text | lzma | 0.0 |
| White | Spectral | Text | zstd | 2.0 |
| White | Spectral | Binary | gzip | 1.0 |
| White | Spectral | Binary | bzip2 | 1.0 |
| White | Spectral | Binary | lzma | 0.0 |
| White | Spectral | Binary | zstd | 2.0 |

### Findings

#### 1. Genre-Dependent Performance
- **Best Performance**: Hip-Hop/Rap and Blues show highest accuracy due to distinctive rhythmic and spectral patterns
- **Most Challenging**: Classical music with complex harmonic structures and orchestral arrangements
- **Electronic Music**: Surprisingly low accuracy despite distinctive synthetic sounds

#### 2. Feature Extraction Method Comparison
- **MaxFreq Method**: Better overall performance with richer frequency representation and more compact
- **Binary vs Text**: Minimal accuracy difference, but binary format offers storage efficiency

#### 3. Compressor Analysis
- **Bzip2**: Best overall performance and fast
- **Gzip**: Fastest but with low accuracy
- **LZMA**: Slow with moderate accuracy
- **ZSTD**: Slow with moderate accuracy

#### 4. Noise Robustness
- **Clean Audio**: Baseline performance
- **Brown Noise**: Some degradation
- **Pink Noise**: Lower degradation
- **White Noise**: Most degradation

### Performance Visualizations

The system generates analysis plots, the most useful are:
- **Accuracy Heatmaps**: Method x Compressor x Noise combinations
- **Genre Analysis**: Performance breakdown by music genre
- **Format Comparison**: Binary vs text feature
- **Method Comparison**: Spectral vs Maxfreq
- **Noises Comparison**: Clean vs Brown vs Pink vs White

These can be checked on the results plots folder for the [smaller dataset](/results/plots_small/) and for the [bigger dataset](/results/plots_youtube/).

## Conclusion

This project successfully demonstrates a **complete music identification pipeline** using information theory principles. The combination of **frequency-domain feature extraction** and **Normalized Compression Distance** provides a universal similarity measure that doesn't require machine learning training.

### Achievements

1. **Robust Implementation**: C++ codebase with error handling
2. **Flexible Architecture**: Modular design supporting multiple feature extraction methods and compressors
3. **Comprehensive Analysis**: Testing framework with automated evaluation metrics
4. **Genre Insights**: Identified genre-specific challenges and optimal configurations
5. **Noise Robustness**: Demonstrated resilience to various types of audio corruption

### Limitations and Future Work

1. **Accuracy Constraints**: 6.3% average accuracy
2. **Computational Complexity**: NCD calculation scales quadratically with database size
3. **Feature Engineering**: Opportunities for advanced spectral features (MFCCs, chroma)

**Note:** If we had more time, we would like to:
- Extract more than 1 segment for a song, perform a combined evaluation by making the NCD average (a way to replicate the behavior of an ensemble method).
- Test with more diversified and better chosen songs.
- Test with different sample lengths and compare the results.

## Changes after the Presentation

After our presentation, we learned that our results were far from our colleagues' implementations, and we were asked by the professors to compare our maximum frequency extraction method with theirs. As such, we implemented the professor's version of the maximum frequency extraction method, which is based on the original `GetMaxFreqs.cpp` algorithm provided by the professors, and we also revisited our own implementation, making changes

This section summarizes the key differences between our custom implementation, the professor's version and our improved version, along with the performance impact.

### Maximum Frequency Method Comparison: Custom vs. Professor's Implementation

#### Key Algorithmic Differences

**1. Down-sampling Strategy**
- **Professor's Approach**: Applies 4x down-sampling during preprocessing, reducing computational complexity while focusing on lower frequencies where most musical information resides
- **Custom Approach**: Processes all samples without down-sampling, maintaining full frequency resolution but increasing noise sensitivity

**2. Window Parameters and Overlap**
- **Professor's Approach**: Fixed 1024-sample window with 256-sample shift (75% overlap), providing optimal temporal resolution for music analysis
- **Custom Approach**: User-configurable parameters that may not be optimized for the specific domain

**3. Frequency Range Focus**
- **Professor's Approach**: Limits frequency indices to 255 bins, focusing on the perceptually most relevant range (0-5.5kHz at 44.1kHz sampling)
- **Custom Approach**: Uses full frequency range, potentially including less informative high-frequency content

**4. Normalization Strategy**
- **Professor's Approach**: Preserves raw power spectrum relationships without normalization, maintaining natural energy distribution
- **Custom Approach**: Normalizes FFT magnitudes by window size, potentially losing important relative energy information

**5. Stereo Processing**
- **Professor's Approach**: Direct channel summation during FFT preparation, preserving energy and avoiding precision loss
- **Custom Approach**: Pre-conversion to mono by averaging, which may reduce signal energy

### Improved Maximum Frequency Extractor

After running the professor's implementation, we decided to enhance our original `MaxFreqExtractor` based on the feedback and insights gained. The new version, `MaxFreqImproved`, incorporates down-sampling and uses the FFTW3 library for optimized performance, aligning with the professor's approach while maintaining our custom features.

#### Improvements in MaxFreqImproved

- **FFTW3 Integration**: Replaced custom FFT with industry-standard optimized library
- **Down-sampling Support**: Configurable sample reduction (default 4x) for faster processing
- **Memory Efficiency**: Lower memory footprint through intelligent sample reduction
- **Byte Compatibility**: Frequency indices truncated to 255 for compact storage
- **Unified Processing**: Combines mono conversion with down-sampling in single operation

#### Comparison Table

| Feature | MaxFreqExtractor (Original) | MaxFreqImproved | ProfMaxFreqExtractor |
|---------|----------------------------|-----------------|---------------------|
| **FFT Library** | Custom Cooley-Tukey | FFTW3 | FFTW3 |
| **Down-sampling** | None | 4x configurable | 4x fixed |
| **Performance** | Baseline | Improved (FFTW3 + down-sampling) | Improved (FFTW3 + down-sampling) |
| **Window Function** | Hann window | No windowing (matches professor) | No windowing |
| **Frequency Truncation** | None | To 255 (byte compatible) | To 255 |
| **Output Formats** | Text and binary | Text and binary | Binary only |
| **Channel Support** | Mono/Stereo | Mono/Stereo | Stereo only (44.1kHz) |

### Performance Impact
Both the professor's implementation and our improved version show significant performance improvements over the original `MaxFreqExtractor`. The new implementations achieve higher accuracy and lower computational overhead, making them more suitable for real-world music identification tasks.

#### 36 Songs Dataset

20 seconds samples, avoiding the first and last 10 seconds of each song

| Noise | Method | Type | Compressor | Top-1 Accuracy (%) |
|-------|--------|------|------------|----------------|
| Clean | ProfMaxfreq | Text | gzip | N/A |
| Clean | ProfMaxfreq | Text | bzip2 | N/A |
| Clean | ProfMaxfreq | Text | lzma | N/A |
| Clean | ProfMaxfreq | Text | zstd | N/A |
| Clean | ProfMaxfreq | Binary | gzip | 55.9 |
| Clean | ProfMaxfreq | Binary | bzip2 | 64.7 |
| Clean | ProfMaxfreq | Binary | lzma | 52.9 |
| Clean | ProfMaxfreq | Binary | zstd | 64.7 |
| Clean | Maxfreq | Text | gzip | 5.9 |
| Clean | Maxfreq | Text | bzip2 | 32.4 |
| Clean | Maxfreq | Text | lzma | 14.7 |
| Clean | Maxfreq | Text | zstd | 20.6 |
| Clean | Maxfreq | Binary | gzip | 8.8 |
| Clean | Maxfreq | Binary | bzip2 | 29.4 |
| Clean | Maxfreq | Binary | lzma | 8.8 |
| Clean | Maxfreq | Binary | zstd | 0.0 |
| Clean | Maxfreq Improved | Text | gzip | 50.0 |
| Clean | Maxfreq Improved | Text | bzip2 | 61.8 |
| Clean | Maxfreq Improved | Text | lzma | 55.9 |
| Clean | Maxfreq Improved | Text | zstd | 61.8 |
| Clean | Maxfreq Improved | Binary | gzip | 61.8 |
| Clean | Maxfreq Improved | Binary | bzip2 | 64.7 |
| Clean | Maxfreq Improved | Binary | lzma | 55.9 |
| Clean | Maxfreq Improved | Binary | zstd | 58.8 |
| Clean | Spectral | Text | gzip | 2.9 |
| Clean | Spectral | Text | bzip2 | 11.8 |
| Clean | Spectral | Text | lzma | 8.8 |
| Clean | Spectral | Text | zstd | 11.8 |
| Clean | Spectral | Binary | gzip | 5.9 |
| Clean | Spectral | Binary | bzip2 | 5.9 |
| Clean | Spectral | Binary | lzma | 8.8 |
| Clean | Spectral | Binary | zstd | 8.8 |
| Brown | ProfMaxfreq | Text | gzip | N/A |
| Brown | ProfMaxfreq | Text | bzip2 | N/A |
| Brown | ProfMaxfreq | Text | lzma | N/A |
| Brown | ProfMaxfreq | Text | zstd | N/A |
| Brown | ProfMaxfreq | Binary | gzip | 44.1 |
| Brown | ProfMaxfreq | Binary | bzip2 | 38.2 |
| Brown | ProfMaxfreq | Binary | lzma | 5.9 |
| Brown | ProfMaxfreq | Binary | zstd | 26.5 |
| Brown | Maxfreq | Text | gzip | 5.9 |
| Brown | Maxfreq | Text | bzip2 | 11.8 |
| Brown | Maxfreq | Text | lzma | 8.8 |
| Brown | Maxfreq | Text | zstd | 8.8 |
| Brown | Maxfreq | Binary | gzip | 5.9 |
| Brown | Maxfreq | Binary | bzip2 | 14.7 |
| Brown | Maxfreq | Binary | lzma | 2.9 |
| Brown | Maxfreq | Binary | zstd | 2.9 |
| Brown | Maxfreq Improved | Text | gzip | 5.9 |
| Brown | Maxfreq Improved | Text | bzip2 | 58.8 |
| Brown | Maxfreq Improved | Text | lzma | 17.6 |
| Brown | Maxfreq Improved | Text | zstd | 38.2 |
| Brown | Maxfreq Improved | Binary | gzip | 35.3 |
| Brown | Maxfreq Improved | Binary | bzip2 | 50.0 |
| Brown | Maxfreq Improved | Binary | lzma | 23.5 |
| Brown | Maxfreq Improved | Binary | zstd | 20.6 |
| Brown | Spectral | Text | gzip | 2.9 |
| Brown | Spectral | Text | bzip2 | 8.8 |
| Brown | Spectral | Text | lzma | 8.8 |
| Brown | Spectral | Text | zstd | 2.9 |
| Brown | Spectral | Binary | gzip | 5.9 |
| Brown | Spectral | Binary | bzip2 | 8.8 |
| Brown | Spectral | Binary | lzma | 2.9 |
| Brown | Spectral | Binary | zstd | 8.8 |
| Pink | ProfMaxfreq | Text | gzip | N/A |
| Pink | ProfMaxfreq | Text | bzip2 | N/A |
| Pink | ProfMaxfreq | Text | lzma | N/A |
| Pink | ProfMaxfreq | Text | zstd | N/A |
| Pink | ProfMaxfreq | Binary | gzip | 55.9 |
| Pink | ProfMaxfreq | Binary | bzip2 | 61.8 |
| Pink | ProfMaxfreq | Binary | lzma | 44.1 |
| Pink | ProfMaxfreq | Binary | zstd | 58.8 |
| Pink | Maxfreq | Text | gzip | 5.9 |
| Pink | Maxfreq | Text | bzip2 | 20.6 |
| Pink | Maxfreq | Text | lzma | 11.8 |
| Pink | Maxfreq | Text | zstd | 11.8 |
| Pink | Maxfreq | Binary | gzip | 11.8 |
| Pink | Maxfreq | Binary | bzip2 | 26.5 |
| Pink | Maxfreq | Binary | lzma | 8.8 |
| Pink | Maxfreq | Binary | zstd | 8.8 |
| Pink | Maxfreq Improved | Text | gzip | 38.2 |
| Pink | Maxfreq Improved | Text | bzip2 | 61.8 |
| Pink | Maxfreq Improved | Text | lzma | 47.1 |
| Pink | Maxfreq Improved | Text | zstd | 55.9 |
| Pink | Maxfreq Improved | Binary | gzip | 50.0 |
| Pink | Maxfreq Improved | Binary | bzip2 | 61.8 |
| Pink | Maxfreq Improved | Binary | lzma | 44.1 |
| Pink | Maxfreq Improved | Binary | zstd | 55.9 |
| Pink | Spectral | Text | gzip | 2.9 |
| Pink | Spectral | Text | bzip2 | 11.8 |
| Pink | Spectral | Text | lzma | 11.8 |
| Pink | Spectral | Text | zstd | 2.9 |
| Pink | Spectral | Binary | gzip | 8.8 |
| Pink | Spectral | Binary | bzip2 | 5.9 |
| Pink | Spectral | Binary | lzma | 8.8 |
| Pink | Spectral | Binary | zstd | 5.9 |
| White | ProfMaxfreq | Text | gzip | N/A |
| White | ProfMaxfreq | Text | bzip2 | N/A |
| White | ProfMaxfreq | Text | lzma | N/A |
| White | ProfMaxfreq | Text | zstd | N/A |
| White | ProfMaxfreq | Binary | gzip | 55.9 |
| White | ProfMaxfreq | Binary | bzip2 | 61.8 |
| White | ProfMaxfreq | Binary | lzma | 50.0 |
| White | ProfMaxfreq | Binary | zstd | 58.8 |
| White | Maxfreq | Text | gzip | 5.9 |
| White | Maxfreq | Text | bzip2 | 32.4 |
| White | Maxfreq | Text | lzma | 8.8 |
| White | Maxfreq | Text | zstd | 11.8 |
| White | Maxfreq | Binary | gzip | 5.9 |
| White | Maxfreq | Binary | bzip2 | 38.2 |
| White | Maxfreq | Binary | lzma | 2.9 |
| White | Maxfreq | Binary | zstd | 2.9 |
| White | Maxfreq Improved | Text | gzip | 38.2 |
| White | Maxfreq Improved | Text | bzip2 | 61.8 |
| White | Maxfreq Improved | Text | lzma | 50.0 |
| White | Maxfreq Improved | Text | zstd | 55.9 |
| White | Maxfreq Improved | Binary | gzip | 64.7 |
| White | Maxfreq Improved | Binary | bzip2 | 61.8 |
| White | Maxfreq Improved | Binary | lzma | 58.8 |
| White | Maxfreq Improved | Binary | zstd | 52.9 |
| White | Spectral | Text | gzip | 2.9 |
| White | Spectral | Text | bzip2 | 11.8 |
| White | Spectral | Text | lzma | 5.9 |
| White | Spectral | Text | zstd | 8.8 |
| White | Spectral | Binary | gzip | 5.9 |
| White | Spectral | Binary | bzip2 | 5.9 |
| White | Spectral | Binary | lzma | 5.9 |
| White | Spectral | Binary | zstd | 8.8 |


#### 100 Songs Dataset, reduced to 32 songs for lower computational cost

20 seconds samples, avoiding the first and last 10 seconds of each song
The spectral method was not used in this dataset, because of high running time.
Only 4 genres were used, to reduce the computational cost.

| Genre | Top-1 Accuracy | Top-5 Accuracy | Sample Count |
|-------|----------------|----------------|--------------|
| Hip-Hop / Rap | 45.2% | 63.0% | 640 |
| Pop | 39.1% | 63.0% | 640 |
| Blues | 34.1% | 57.5% | 640 |
| Rock | 32.5% | 49.8% | 640 |

| Noise | Method | Type | Compressor | Top-1 Accuracy (%) |
|-------|--------|------|------------|------------------|
| Clean | ProfMaxfreq | Text | gzip | N/A |
| Clean | ProfMaxfreq | Text | bzip2 | N/A |
| Clean | ProfMaxfreq | Text | lzma | N/A |
| Clean | ProfMaxfreq | Text | zstd | N/A |
| Clean | ProfMaxfreq | Binary | gzip | 46.9 |
| Clean | ProfMaxfreq | Binary | bzip2 | 93.8 |
| Clean | ProfMaxfreq | Binary | lzma | 31.2 |
| Clean | ProfMaxfreq | Binary | zstd | 84.4 |
| Clean | Maxfreq | Text | gzip | 9.4 |
| Clean | Maxfreq | Text | bzip2 | 34.4 |
| Clean | Maxfreq | Text | lzma | 34.4 |
| Clean | Maxfreq | Text | zstd | 37.5 |
| Clean | Maxfreq | Binary | gzip | 12.5 |
| Clean | Maxfreq | Binary | bzip2 | 25.0 |
| Clean | Maxfreq | Binary | lzma | 18.8 |
| Clean | Maxfreq | Binary | zstd | 12.5 |
| Clean | Maxfreq Improved | Text | gzip | 43.8 |
| Clean | Maxfreq Improved | Text | bzip2 | 87.5 |
| Clean | Maxfreq Improved | Text | lzma | 50.0 |
| Clean | Maxfreq Improved | Text | zstd | 56.2 |
| Clean | Maxfreq Improved | Binary | gzip | 46.9 |
| Clean | Maxfreq Improved | Binary | bzip2 | 93.8 |
| Clean | Maxfreq Improved | Binary | lzma | 43.8 |
| Clean | Maxfreq Improved | Binary | zstd | 15.6 |
| Brown | ProfMaxfreq | Text | gzip | N/A |
| Brown | ProfMaxfreq | Text | bzip2 | N/A |
| Brown | ProfMaxfreq | Text | lzma | N/A |
| Brown | ProfMaxfreq | Text | zstd | N/A |
| Brown | ProfMaxfreq | Binary | gzip | 28.1 |
| Brown | ProfMaxfreq | Binary | bzip2 | 56.2 |
| Brown | ProfMaxfreq | Binary | lzma | 9.4 |
| Brown | ProfMaxfreq | Binary | zstd | 46.9 |
| Brown | Maxfreq | Text | gzip | 6.2 |
| Brown | Maxfreq | Text | bzip2 | 25.0 |
| Brown | Maxfreq | Text | lzma | 21.9 |
| Brown | Maxfreq | Text | zstd | 21.9 |
| Brown | Maxfreq | Binary | gzip | 12.5 |
| Brown | Maxfreq | Binary | bzip2 | 15.6 |
| Brown | Maxfreq | Binary | lzma | 15.6 |
| Brown | Maxfreq | Binary | zstd | 3.1 |
| Brown | Maxfreq Improved | Text | gzip | 15.6 |
| Brown | Maxfreq Improved | Text | bzip2 | 65.6 |
| Brown | Maxfreq Improved | Text | lzma | 15.6 |
| Brown | Maxfreq Improved | Text | zstd | 18.8 |
| Brown | Maxfreq Improved | Binary | gzip | 18.8 |
| Brown | Maxfreq Improved | Binary | bzip2 | 50.0 |
| Brown | Maxfreq Improved | Binary | lzma | 21.9 |
| Brown | Maxfreq Improved | Binary | zstd | 9.4 |
| Pink | ProfMaxfreq | Text | gzip | N/A |
| Pink | ProfMaxfreq | Text | bzip2 | N/A |
| Pink | ProfMaxfreq | Text | lzma | N/A |
| Pink | ProfMaxfreq | Text | zstd | N/A |
| Pink | ProfMaxfreq | Binary | gzip | 34.4 |
| Pink | ProfMaxfreq | Binary | bzip2 | 93.8 |
| Pink | ProfMaxfreq | Binary | lzma | 28.1 |
| Pink | ProfMaxfreq | Binary | zstd | 81.2 |
| Pink | Maxfreq | Text | gzip | 12.5 |
| Pink | Maxfreq | Text | bzip2 | 34.4 |
| Pink | Maxfreq | Text | lzma | 25.0 |
| Pink | Maxfreq | Text | zstd | 25.0 |
| Pink | Maxfreq | Binary | gzip | 9.4 |
| Pink | Maxfreq | Binary | bzip2 | 25.0 |
| Pink | Maxfreq | Binary | lzma | 21.9 |
| Pink | Maxfreq | Binary | zstd | 12.5 |
| Pink | Maxfreq Improved | Text | gzip | 34.4 |
| Pink | Maxfreq Improved | Text | bzip2 | 90.6 |
| Pink | Maxfreq Improved | Text | lzma | 43.8 |
| Pink | Maxfreq Improved | Text | zstd | 53.1 |
| Pink | Maxfreq Improved | Binary | gzip | 28.1 |
| Pink | Maxfreq Improved | Binary | bzip2 | 87.5 |
| Pink | Maxfreq Improved | Binary | lzma | 50.0 |
| Pink | Maxfreq Improved | Binary | zstd | 25.0 |
| White | ProfMaxfreq | Text | gzip | N/A |
| White | ProfMaxfreq | Text | bzip2 | N/A |
| White | ProfMaxfreq | Text | lzma | N/A |
| White | ProfMaxfreq | Text | zstd | N/A |
| White | ProfMaxfreq | Binary | gzip | 50.0 |
| White | ProfMaxfreq | Binary | bzip2 | 93.8 |
| White | ProfMaxfreq | Binary | lzma | 40.6 |
| White | ProfMaxfreq | Binary | zstd | 93.8 |
| White | Maxfreq | Text | gzip | 6.2 |
| White | Maxfreq | Text | bzip2 | 21.9 |
| White | Maxfreq | Text | lzma | 15.6 |
| White | Maxfreq | Text | zstd | 15.6 |
| White | Maxfreq | Binary | gzip | 6.2 |
| White | Maxfreq | Binary | bzip2 | 21.9 |
| White | Maxfreq | Binary | lzma | 15.6 |
| White | Maxfreq | Binary | zstd | 12.5 |
| White | Maxfreq Improved | Text | gzip | 25.0 |
| White | Maxfreq Improved | Text | bzip2 | 87.5 |
| White | Maxfreq Improved | Text | lzma | 50.0 |
| White | Maxfreq Improved | Text | zstd | 56.2 |
| White | Maxfreq Improved | Binary | gzip | 40.6 |
| White | Maxfreq Improved | Binary | bzip2 | 93.8 |
| White | Maxfreq Improved | Binary | lzma | 46.9 |
| White | Maxfreq Improved | Binary | zstd | 18.8 |

### License

This project is released under the MIT License. See `LICENSE` file for details.

## Authors

<table>
  <tr>
    <td align="center">
        <a href="https://github.com/Sytuz">
            <img src="https://avatars0.githubusercontent.com/Sytuz?v=3" width="100px;" alt="Alexandre"/>
            <br />
            <sub>
                <b>Alexandre Ribeiro</b>
                <br>
                <i>108122</i>
            </sub>
        </a>
    </td>
    <td align="center">
        <a href="https://github.com/mariiajoao">
            <img src="https://avatars0.githubusercontent.com/mariiajoao?v=3" width="100px;" alt="Maria"/>
            <br />
            <sub>
                <b>Maria Sardinha</b>
                <br>
                <i>108756</i>
            </sub>
        </a>
    </td>
    <td align="center">
        <a href="https://github.com/miguel-silva48">
            <img src="https://avatars0.githubusercontent.com/miguel-silva48?v=3" width="100px;" alt="Miguel"/>
            <br />
            <sub>
                <b>Miguel Pinto</b>
                <br>
                <i>107449</i>
            </sub>
        </a>
    </td>
  </tr>
</table>