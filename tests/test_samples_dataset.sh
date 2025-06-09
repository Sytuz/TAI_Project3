#!/bin/bash

#####################################################################
# Comprehensive Music Identification Test - Samples Dataset (36 songs)
# Tests the complete pipeline with all four compressors
#####################################################################

set -e  # Exit on any error

# Configuration
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
TEST_NAME="samples_dataset"
RESULTS_DIR="$SCRIPT_DIR/results_$TEST_NAME"
COMPRESSORS=("gzip" "bzip2" "lzma" "zstd")

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo -e "${BLUE}=============================================${NC}"
echo -e "${BLUE}  Music Identification Test - Samples Dataset${NC}"
echo -e "${BLUE}  Compressors: ${COMPRESSORS[*]}${NC}"
echo -e "${BLUE}=============================================${NC}"

cd "$PROJECT_ROOT"

# Count songs in samples
SONG_COUNT=$(find data/samples -name "*.wav" | wc -l)
echo -e "${BLUE}  Dataset size: $SONG_COUNT songs${NC}"

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
echo "Songs to process: $SONG_COUNT"
echo ""

#####################################################################
# STEP 1: Verify Dataset
#####################################################################

echo -e "${GREEN}STEP 1: Verifying samples dataset...${NC}"

if [ ! -d "data/samples" ] || [ $SONG_COUNT -eq 0 ]; then
    echo -e "${RED}Error: No WAV files found in data/samples${NC}"
    exit 1
fi

echo "Found $SONG_COUNT WAV files in data/samples"
echo "First 10 files:"
find data/samples -name "*.wav" | head -10 | while read file; do
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
start_time=$(date +%s)
./apps/extract_features data/samples "$FEATURES_TEXT" spectral config/feature_extraction_spectral_default.json
text_time=$(($(date +%s) - start_time))

echo "Extracting binary format features..."
start_time=$(date +%s)
./apps/extract_features data/samples "$FEATURES_BINARY" spectral config/feature_extraction_spectral_default.json --binary
binary_time=$(($(date +%s) - start_time))

echo "Feature extraction complete"
echo "Text format extraction time: ${text_time}s"
echo "Binary format extraction time: ${binary_time}s"
echo ""

#####################################################################
# STEP 3: Test All Compressors (Text Format)
#####################################################################

echo -e "${GREEN}STEP 3: Testing all compressors with text format...${NC}"

TEXT_RESULTS="$RESULTS_DIR/text_format_results"

echo "Starting text format evaluation..."
start_time=$(date +%s)

./scripts/evaluate_system.sh \
    -q "$FEATURES_TEXT" \
    -d "$FEATURES_TEXT" \
    -o "$TEXT_RESULTS" \
    -c "gzip,bzip2,lzma,zstd"

text_eval_time=$(($(date +%s) - start_time))
echo "Text format testing complete in ${text_eval_time}s"
echo ""

#####################################################################
# STEP 4: Test All Compressors (Binary Format)
#####################################################################

echo -e "${GREEN}STEP 4: Testing all compressors with binary format...${NC}"

BINARY_RESULTS="$RESULTS_DIR/binary_format_results"

echo "Starting binary format evaluation..."
start_time=$(date +%s)

./scripts/evaluate_system.sh \
    -q "$FEATURES_BINARY" \
    -d "$FEATURES_BINARY" \
    -o "$BINARY_RESULTS" \
    -c "gzip,bzip2,lzma,zstd" \
    --binary

binary_eval_time=$(($(date +%s) - start_time))
echo "Binary format testing complete in ${binary_eval_time}s"
echo ""

#####################################################################
# STEP 5: Parameter Evaluation (Subset for efficiency)
#####################################################################

echo -e "${GREEN}STEP 5: Parameter evaluation with subset (first 10 songs)...${NC}"

# Create subset for parameter evaluation to save time
SUBSET_DIR="$RESULTS_DIR/data/samples_subset"
mkdir -p "$SUBSET_DIR"

echo "Creating subset of 10 songs for parameter evaluation..."
subset_count=0
for wav_file in data/samples/*.wav; do
    if [ $subset_count -lt 10 ]; then
        cp "$wav_file" "$SUBSET_DIR/"
        ((subset_count++))
    else
        break
    fi
done

PARAM_EVAL_RESULTS="$RESULTS_DIR/parameter_evaluation"

echo "Starting parameter evaluation..."
start_time=$(date +%s)

./scripts/quick_param_eval.sh \
    --query-dir "$SUBSET_DIR" \
    --db-dir "$SUBSET_DIR" \
    --output-dir "$PARAM_EVAL_RESULTS" \
    --compressors "gzip,bzip2,lzma,zstd" \
    --binary

param_eval_time=$(($(date +%s) - start_time))
echo "Parameter evaluation complete in ${param_eval_time}s"
echo ""

#####################################################################
# STEP 6: Individual Song Tests
#####################################################################

echo -e "${GREEN}STEP 6: Individual song tests with each compressor...${NC}"

INDIVIDUAL_TESTS="$RESULTS_DIR/individual_tests"
mkdir -p "$INDIVIDUAL_TESTS"

# Test first song with each compressor
test_song=$(find "$FEATURES_BINARY" -name "*.featbin" | head -1)
test_song_name=$(basename "$test_song" .featbin)

echo "Testing song: $test_song_name"

for comp in "${COMPRESSORS[@]}"; do
    echo "  Testing with $comp compressor..."
    ./apps/music_id \
        --compressor "$comp" \
        --binary \
        "$test_song" \
        "$FEATURES_BINARY" \
        "$INDIVIDUAL_TESTS/${test_song_name}_${comp}_results.csv"
done

echo "Individual song tests complete"
echo ""

#####################################################################
# STEP 7: Generate Visualizations
#####################################################################

echo -e "${GREEN}STEP 7: Generating visualizations...${NC}"

# Use dedicated plotting script to generate all visualizations
bash scripts/generate_test_plots.sh \
    --results-dir "$RESULTS_DIR" \
    --output-dir "$RESULTS_DIR/plots" \
    --test-type "samples_dataset" \
    --formats "text,binary" \
    --compressors "gzip,bzip2,lzma,zstd"

echo "Visualizations complete"
echo ""

#####################################################################
# STEP 8: Generate Comprehensive Report
#####################################################################

echo -e "${GREEN}STEP 8: Generating comprehensive report...${NC}"

REPORT_FILE="$RESULTS_DIR/comprehensive_report.md"

cat > "$REPORT_FILE" << EOF
# Music Identification Test Report - Samples Dataset

**Test Date:** $(date)  
**Dataset:** data/samples ($SONG_COUNT songs)  
**Compressors Tested:** ${COMPRESSORS[*]}  

## Performance Summary

- **Text format extraction time:** ${text_time}s
- **Binary format extraction time:** ${binary_time}s
- **Text format evaluation time:** ${text_eval_time}s  
- **Binary format evaluation time:** ${binary_eval_time}s
- **Parameter evaluation time:** ${param_eval_time}s

## Test Results Summary

### Text Format Results
EOF

# Add results to report
for comp in "${COMPRESSORS[@]}"; do
    if [ -f "$TEXT_RESULTS/$comp/accuracy_metrics_$comp.json" ]; then
        echo "#### $comp Compressor (Text Format)" >> "$REPORT_FILE"
        python3 -c "
import json
try:
    with open('$TEXT_RESULTS/$comp/accuracy_metrics_$comp.json', 'r') as f:
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

echo "### Binary Format Results" >> "$REPORT_FILE"

for comp in "${COMPRESSORS[@]}"; do
    if [ -f "$BINARY_RESULTS/$comp/accuracy_metrics_$comp.json" ]; then
        echo "#### $comp Compressor (Binary Format)" >> "$REPORT_FILE"
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

cat >> "$REPORT_FILE" << EOF

## Files Generated

### Results
- **Text Format Results:** \`$TEXT_RESULTS\`
- **Binary Format Results:** \`$BINARY_RESULTS\`
- **Parameter Evaluation:** \`$PARAM_EVAL_RESULTS\`
- **Individual Tests:** \`$INDIVIDUAL_TESTS\`

### Visualizations
- **Format Comparison:** \`$RESULTS_DIR/plots/format_comparison\`
- **Parameter Evaluation:** \`$RESULTS_DIR/plots/parameter_evaluation\`

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
echo "  📋 Report: $REPORT_FILE"
echo "  📝 Log: $LOG_FILE"
echo ""
echo -e "${GREEN}Samples dataset test completed successfully!${NC}"
