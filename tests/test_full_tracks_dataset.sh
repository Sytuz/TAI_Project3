#!/bin/bash

#####################################################################
# Comprehensive Music Identification Test - Full Tracks Dataset (YouTube songs)
# Tests the complete pipeline with all four compressors
# Note: This is a comprehensive test that may take significant time
#####################################################################

set -e  # Exit on any error

# Configuration
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
TEST_NAME="full_tracks_dataset"
RESULTS_DIR="$SCRIPT_DIR/results_$TEST_NAME"
COMPRESSORS=("gzip" "bzip2" "lzma" "zstd")

# Performance optimization: Use subset for initial tests
USE_SUBSET=${USE_SUBSET:-true}
SUBSET_SIZE=${SUBSET_SIZE:-20}

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo -e "${BLUE}=============================================${NC}"
echo -e "${BLUE}  Music Identification Test - Full Tracks Dataset${NC}"
echo -e "${BLUE}  Compressors: ${COMPRESSORS[*]}${NC}"
if [ "$USE_SUBSET" = true ]; then
    echo -e "${BLUE}  Using subset: $SUBSET_SIZE songs (for performance)${NC}"
fi
echo -e "${BLUE}=============================================${NC}"

cd "$PROJECT_ROOT"

# Count songs in full_tracks
TOTAL_SONG_COUNT=$(find data/full_tracks -name "*.wav" | wc -l)
SONG_COUNT=$TOTAL_SONG_COUNT

if [ "$USE_SUBSET" = true ]; then
    SONG_COUNT=$SUBSET_SIZE
fi

echo -e "${BLUE}  Total available songs: $TOTAL_SONG_COUNT${NC}"
echo -e "${BLUE}  Songs to process: $SONG_COUNT${NC}"

# Clean up previous results
if [ -d "$RESULTS_DIR" ]; then
    echo -e "${YELLOW}Cleaning up previous results...${NC}"
    rm -rf "$RESULTS_DIR"
fi

mkdir -p "$RESULTS_DIR"
mkdir -p "$RESULTS_DIR/plots"
mkdir -p "$RESULTS_DIR/data"

# Log file for the complete test
LOG_FILE="$RESULTS_DIR/test_log.txt"
exec > >(tee -a "$LOG_FILE") 2>&1

echo "Test started at: $(date)"
echo "Project root: $PROJECT_ROOT"
echo "Results directory: $RESULTS_DIR"
echo "Songs to process: $SONG_COUNT (subset: $USE_SUBSET)"
echo ""

#####################################################################
# STEP 1: Prepare Dataset
#####################################################################

echo -e "${GREEN}STEP 1: Preparing full tracks dataset...${NC}"

if [ ! -d "data/full_tracks" ] || [ $TOTAL_SONG_COUNT -eq 0 ]; then
    echo -e "${RED}Error: No WAV files found in data/full_tracks${NC}"
    exit 1
fi

DATASET_DIR="data/full_tracks"

# Create subset if requested
if [ "$USE_SUBSET" = true ]; then
    DATASET_DIR="$RESULTS_DIR/data/full_tracks_subset"
    mkdir -p "$DATASET_DIR"
    
    echo "Creating subset of $SUBSET_SIZE songs..."
    subset_count=0
    for wav_file in data/full_tracks/*.wav; do
        if [ $subset_count -lt $SUBSET_SIZE ]; then
            cp "$wav_file" "$DATASET_DIR/"
            echo "  Added: $(basename "$wav_file")"
            ((subset_count++))
        else
            break
        fi
    done
    SONG_COUNT=$subset_count
fi

echo "Dataset prepared with $SONG_COUNT songs"
echo "First 10 files:"
find "$DATASET_DIR" -name "*.wav" | head -10 | while read file; do
    echo "  $(basename "$file")"
done
echo ""

#####################################################################
# STEP 2: Extract Features (Text and Binary formats)
#####################################################################

echo -e "${GREEN}STEP 2: Extracting features...${NC}"

FEATURES_TEXT="$RESULTS_DIR/data/features_text"
FEATURES_BINARY="$RESULTS_DIR/data/features_binary"

echo "Extracting text format features..."
echo "This may take a while for $SONG_COUNT songs..."
start_time=$(date +%s)
./apps/extract_features "$DATASET_DIR" "$FEATURES_TEXT" spectral config/feature_extraction_spectral_default.json
text_time=$(($(date +%s) - start_time))

echo "Extracting binary format features..."
start_time=$(date +%s)
./apps/extract_features "$DATASET_DIR" "$FEATURES_BINARY" spectral config/feature_extraction_spectral_default.json --binary
binary_time=$(($(date +%s) - start_time))

echo "Feature extraction complete"
echo "Text format extraction time: ${text_time}s ($(($text_time / 60))m $(($text_time % 60))s)"
echo "Binary format extraction time: ${binary_time}s ($(($binary_time / 60))m $(($binary_time % 60))s)"
echo ""

#####################################################################
# STEP 3: Test Individual Compressors (Binary Format for Speed)
#####################################################################

echo -e "${GREEN}STEP 3: Testing individual compressors (binary format)...${NC}"

BINARY_RESULTS="$RESULTS_DIR/binary_format_results"

# Test each compressor individually to monitor progress
for comp in "${COMPRESSORS[@]}"; do
    echo "Testing $comp compressor..."
    start_time=$(date +%s)
    
    ./scripts/evaluate_system.sh \
        -q "$FEATURES_BINARY" \
        -d "$FEATURES_BINARY" \
        -o "${BINARY_RESULTS}_${comp}" \
        -c "$comp" \
        --binary
    
    comp_time=$(($(date +%s) - start_time))
    echo "$comp testing complete in ${comp_time}s ($(($comp_time / 60))m $(($comp_time % 60))s)"
done

# Combine results into single directory structure
mkdir -p "$BINARY_RESULTS"
for comp in "${COMPRESSORS[@]}"; do
    if [ -d "${BINARY_RESULTS}_${comp}/$comp" ]; then
        cp -r "${BINARY_RESULTS}_${comp}/$comp" "$BINARY_RESULTS/"
    fi
done

echo "All compressor testing complete"
echo ""

#####################################################################
# STEP 4: Quick Text Format Test (Single Compressor)
#####################################################################

echo -e "${GREEN}STEP 4: Quick text format test (LZMA only)...${NC}"

TEXT_RESULTS="$RESULTS_DIR/text_format_results"

echo "Testing text format with LZMA compressor only (for comparison)..."
start_time=$(date +%s)

./scripts/evaluate_system.sh \
    -q "$FEATURES_TEXT" \
    -d "$FEATURES_TEXT" \
    -o "$TEXT_RESULTS" \
    -c "lzma"

text_eval_time=$(($(date +%s) - start_time))
echo "Text format testing complete in ${text_eval_time}s ($(($text_eval_time / 60))m $(($text_eval_time % 60))s)"
echo ""

#####################################################################
# STEP 5: Parameter Evaluation (Small Subset)
#####################################################################

echo -e "${GREEN}STEP 5: Parameter evaluation (subset of 5 songs)...${NC}"

# Create very small subset for parameter evaluation
PARAM_SUBSET_DIR="$RESULTS_DIR/data/param_subset"
mkdir -p "$PARAM_SUBSET_DIR"

echo "Creating subset of 5 songs for parameter evaluation..."
param_subset_count=0
for wav_file in "$DATASET_DIR"/*.wav; do
    if [ $param_subset_count -lt 5 ]; then
        cp "$wav_file" "$PARAM_SUBSET_DIR/"
        ((param_subset_count++))
    else
        break
    fi
done

PARAM_EVAL_RESULTS="$RESULTS_DIR/parameter_evaluation"

echo "Starting parameter evaluation..."
start_time=$(date +%s)

./scripts/quick_param_eval.sh \
    --query-dir "$PARAM_SUBSET_DIR" \
    --db-dir "$PARAM_SUBSET_DIR" \
    --output-dir "$PARAM_EVAL_RESULTS" \
    --compressors "gzip,lzma,zstd" \
    --binary

param_eval_time=$(($(date +%s) - start_time))
echo "Parameter evaluation complete in ${param_eval_time}s"
echo ""

#####################################################################
# STEP 6: Performance Analysis
#####################################################################

echo -e "${GREEN}STEP 6: Performance analysis...${NC}"

PERF_ANALYSIS="$RESULTS_DIR/performance_analysis"
mkdir -p "$PERF_ANALYSIS"

# Analyze file sizes and compression ratios
echo "Analyzing feature file sizes..."

cat > "$PERF_ANALYSIS/file_size_analysis.txt" << EOF
File Size Analysis - Full Tracks Dataset
========================================

Dataset Information:
- Total songs processed: $SONG_COUNT
- Text format extraction time: ${text_time}s
- Binary format extraction time: ${binary_time}s

Feature File Sizes:
EOF

if [ -d "$FEATURES_TEXT" ]; then
    text_total_size=$(du -sh "$FEATURES_TEXT" | cut -f1)
    text_file_count=$(find "$FEATURES_TEXT" -name "*.feat" | wc -l)
    echo "- Text format total size: $text_total_size ($text_file_count files)" >> "$PERF_ANALYSIS/file_size_analysis.txt"
fi

if [ -d "$FEATURES_BINARY" ]; then
    binary_total_size=$(du -sh "$FEATURES_BINARY" | cut -f1)
    binary_file_count=$(find "$FEATURES_BINARY" -name "*.featbin" | wc -l)
    echo "- Binary format total size: $binary_total_size ($binary_file_count files)" >> "$PERF_ANALYSIS/file_size_analysis.txt"
fi

# Test compression ratios with a sample file
if [ -f "$FEATURES_BINARY"/*.featbin ]; then
    sample_file=$(find "$FEATURES_BINARY" -name "*.featbin" | head -1)
    echo "" >> "$PERF_ANALYSIS/file_size_analysis.txt"
    echo "Sample Compression Ratios (file: $(basename "$sample_file")):" >> "$PERF_ANALYSIS/file_size_analysis.txt"
    
    original_size=$(stat -f%z "$sample_file" 2>/dev/null || stat -c%s "$sample_file")
    
    for comp in "${COMPRESSORS[@]}"; do
        if command -v "$comp" &> /dev/null; then
            if [ "$comp" = "gzip" ]; then
                compressed_size=$(gzip -c "$sample_file" | wc -c)
            elif [ "$comp" = "bzip2" ]; then
                compressed_size=$(bzip2 -c "$sample_file" | wc -c)
            elif [ "$comp" = "lzma" ]; then
                compressed_size=$(lzma -c "$sample_file" | wc -c)
            elif [ "$comp" = "zstd" ]; then
                compressed_size=$(zstd -c "$sample_file" | wc -c)
            fi
            ratio=$(echo "scale=2; $compressed_size * 100 / $original_size" | bc -l 2>/dev/null || echo "N/A")
            echo "- $comp: ${ratio}% of original size" >> "$PERF_ANALYSIS/file_size_analysis.txt"
        fi
    done
fi

echo "Performance analysis complete"
echo ""

#####################################################################
# STEP 7: Generate Visualizations
#####################################################################

echo -e "${GREEN}STEP 7: Generating visualizations...${NC}"

# Use dedicated plotting script to generate all visualizations
bash scripts/generate_test_plots.sh \
    --results-dir "$RESULTS_DIR" \
    --output-dir "$RESULTS_DIR/plots" \
    --test-type "full_tracks_dataset" \
    --formats "binary" \
    --compressors "gzip,bzip2,lzma,zstd"

echo "Visualizations complete"
echo ""

#####################################################################
# STEP 8: Generate Comprehensive Report
#####################################################################

echo -e "${GREEN}STEP 8: Generating comprehensive report...${NC}"

REPORT_FILE="$RESULTS_DIR/comprehensive_report.md"

total_test_time=$((text_time + binary_time + text_eval_time))

cat > "$REPORT_FILE" << EOF
# Music Identification Test Report - Full Tracks Dataset

**Test Date:** $(date)  
**Dataset:** data/full_tracks  
**Songs Processed:** $SONG_COUNT (Total available: $TOTAL_SONG_COUNT)  
**Compressors Tested:** ${COMPRESSORS[*]}  
**Subset Mode:** $USE_SUBSET

## Performance Summary

- **Text format extraction time:** ${text_time}s ($(($text_time / 60))m $(($text_time % 60))s)
- **Binary format extraction time:** ${binary_time}s ($(($binary_time / 60))m $(($binary_time % 60))s)
- **Text format evaluation time:** ${text_eval_time}s ($(($text_eval_time / 60))m $(($text_eval_time % 60))s)
- **Parameter evaluation time:** ${param_eval_time}s
- **Total test time:** Approximately $((total_test_time / 3600))h $(((total_test_time % 3600) / 60))m

## Binary Format Results (All Compressors)
EOF

# Add binary format results
for comp in "${COMPRESSORS[@]}"; do
    if [ -f "$BINARY_RESULTS/$comp/accuracy_metrics_$comp.json" ]; then
        echo "### $comp Compressor" >> "$REPORT_FILE"
        python3 -c "
import json
try:
    with open('$BINARY_RESULTS/$comp/accuracy_metrics_$comp.json', 'r') as f:
        data = json.load(f)
    print(f'- **Top-1 Accuracy:** {data[\"top1_accuracy\"]:.1f}%')
    print(f'- **Top-5 Accuracy:** {data[\"top5_accuracy\"]:.1f}%')
    print(f'- **Top-10 Accuracy:** {data[\"top10_accuracy\"]:.1f}%')
    print(f'- **Total Queries:** {data[\"total_queries\"]}')
    print()
except:
    print('- Results not available')
    print()
" >> "$REPORT_FILE"
    fi
done

echo "## Text Format Results (LZMA only)" >> "$REPORT_FILE"

if [ -f "$TEXT_RESULTS/lzma/accuracy_metrics_lzma.json" ]; then
    python3 -c "
import json
try:
    with open('$TEXT_RESULTS/lzma/accuracy_metrics_lzma.json', 'r') as f:
        data = json.load(f)
    print(f'- **Top-1 Accuracy:** {data[\"top1_accuracy\"]:.1f}%')
    print(f'- **Top-5 Accuracy:** {data[\"top5_accuracy\"]:.1f}%')
    print(f'- **Top-10 Accuracy:** {data[\"top10_accuracy\"]:.1f}%')
    print(f'- **Total Queries:** {data[\"total_queries\"]}')
    print()
except:
    print('- Results not available')
    print()
" >> "$REPORT_FILE"
fi

cat >> "$REPORT_FILE" << EOF

## Files Generated

### Dataset
- **Source:** $DATASET_DIR ($SONG_COUNT songs)
- **Text Features:** \`$FEATURES_TEXT\`
- **Binary Features:** \`$FEATURES_BINARY\`

### Results
- **Binary Format Results:** \`$BINARY_RESULTS\`
- **Text Format Results:** \`$TEXT_RESULTS\`
- **Parameter Evaluation:** \`$PARAM_EVAL_RESULTS\`
- **Performance Analysis:** \`$PERF_ANALYSIS\`

### Visualizations
- **Comprehensive Analysis:** \`$RESULTS_DIR/plots/comprehensive_analysis\`
- **Parameter Evaluation:** \`$RESULTS_DIR/plots/parameter_evaluation\`

## Performance Notes

This test was conducted on the full tracks dataset (YouTube songs). 
$(if [ "$USE_SUBSET" = true ]; then echo "A subset of $SUBSET_SIZE songs was used for performance reasons."; fi)

For the complete dataset test, consider running:
\`\`\`bash
USE_SUBSET=false SUBSET_SIZE=$TOTAL_SONG_COUNT ./tests/test_full_tracks_dataset.sh
\`\`\`

## Test Log
See \`test_log.txt\` for complete execution details.
EOF

echo "Comprehensive report generated: $REPORT_FILE"

#####################################################################
# STEP 9: Test Summary
#####################################################################

echo ""
echo -e "${GREEN}=============================================${NC}"
echo -e "${GREEN}  TEST COMPLETED SUCCESSFULLY${NC}"
echo -e "${GREEN}=============================================${NC}"
echo ""
echo "Test completed at: $(date)"
echo ""
echo -e "${YELLOW}Results saved to:${NC} $RESULTS_DIR"
echo -e "${YELLOW}Comprehensive report:${NC} $REPORT_FILE"
echo ""
echo -e "${BLUE}Generated files:${NC}"
echo "  📁 Data files: $RESULTS_DIR/data/"
echo "  📊 Test results: $RESULTS_DIR/*_results/"
echo "  📈 Visualizations: $RESULTS_DIR/plots/"
echo "  📋 Performance analysis: $PERF_ANALYSIS/"
echo "  📋 Report: $REPORT_FILE"
echo "  📝 Log: $LOG_FILE"
echo ""
if [ "$USE_SUBSET" = true ]; then
    echo -e "${YELLOW}Note:${NC} This test used a subset of $SONG_COUNT songs."
    echo -e "${YELLOW}To test the complete dataset ($TOTAL_SONG_COUNT songs), run:${NC}"
    echo "USE_SUBSET=false SUBSET_SIZE=$TOTAL_SONG_COUNT ./tests/test_full_tracks_dataset.sh"
    echo ""
fi
echo -e "${GREEN}Full tracks dataset test completed successfully!${NC}"
