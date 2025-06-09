# Music Identification Test Report - Samples Dataset

**Test Date:** June 9, 2025  
**Dataset:** data/samples (36 songs)  
**Compressors Tested:** gzip bzip2 lzma zstd  

## Performance Summary

- **Text format extraction time:** 8s
- **Binary format extraction time:** 7s
- **Text format evaluation time:** 6042s  
- **Binary format evaluation time:** 2651s
- **Parameter evaluation time:** 120s

## Test Results Summary

### Text Format Results
#### gzip Compressor (Text Format)
- **Top-1 Accuracy:** 8.8%
- **Top-5 Accuracy:** 44.1%
- **Top-10 Accuracy:** 73.5%
- **Total Queries:** 34

#### bzip2 Compressor (Text Format)
- **Top-1 Accuracy:** 0.0%
- **Top-5 Accuracy:** 23.5%
- **Top-10 Accuracy:** 52.9%
- **Total Queries:** 34

#### lzma Compressor (Text Format)
- **Top-1 Accuracy:** 100.0%
- **Top-5 Accuracy:** 100.0%
- **Top-10 Accuracy:** 100.0%
- **Total Queries:** 34

#### zstd Compressor (Text Format)
- **Top-1 Accuracy:** 100.0%
- **Top-5 Accuracy:** 100.0%
- **Top-10 Accuracy:** 100.0%
- **Total Queries:** 34


### Binary Format Results

#### gzip Compressor (Binary Format)
- **Top-1 Accuracy:** 5.9%
- **Top-5 Accuracy:** 20.6%
- **Top-10 Accuracy:** 55.9%
- **Total Queries:** 34

#### bzip2 Compressor (Binary Format)
- **Top-1 Accuracy:** 11.8%
- **Top-5 Accuracy:** 44.1%
- **Top-10 Accuracy:** 61.8%
- **Total Queries:** 34

#### lzma Compressor (Binary Format)
- **Top-1 Accuracy:** 100.0%
- **Top-5 Accuracy:** 100.0%
- **Top-10 Accuracy:** 100.0%
- **Total Queries:** 34

#### zstd Compressor (Binary Format)
- **Top-1 Accuracy:** 100.0%
- **Top-5 Accuracy:** 100.0%
- **Top-10 Accuracy:** 100.0%
- **Total Queries:** 34

## Files Generated

### Results
- **Text Format Results:** `tests/results_samples_dataset/text_format_results`
- **Binary Format Results:** `tests/results_samples_dataset/binary_format_results`
- **Parameter Evaluation:** `tests/results_samples_dataset/parameter_evaluation`
- **Individual Tests:** `tests/results_samples_dataset/individual_tests`

### Visualizations
- **Format Comparison:** `tests/results_samples_dataset/plots/format_comparison`
- **Parameter Evaluation:** `tests/results_samples_dataset/plots/parameter_evaluation`
- **Individual Analysis:** `tests/results_samples_dataset/plots/individual_analysis`
