#!/bin/bash

#####################################################################
# Comprehensive Music Identification Test - Small Dataset (5 songs)
# Tests the complete pipeline with all four compressors
#####################################################################

set -e  # Exit on any error

# Configuration
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
TEST_NAME="small_dataset"
RESULTS_DIR="$SCRIPT_DIR/results_$TEST_NAME"
DATASET_SIZE=5
COMPRESSORS=("gzip" "bzip2" "lzma" "zstd")

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo -e "${BLUE}=============================================${NC}"
echo -e "${BLUE}  Music Identification Test - Small Dataset${NC}"
echo -e "${BLUE}  Dataset size: $DATASET_SIZE songs${NC}"
echo -e "${BLUE}  Compressors: ${COMPRESSORS[*]}${NC}"
echo -e "${BLUE}=============================================${NC}"

cd "$PROJECT_ROOT"

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
echo ""

#####################################################################
# STEP 1: Prepare Small Dataset
#####################################################################

echo -e "${GREEN}STEP 1: Preparing small dataset ($DATASET_SIZE songs)...${NC}"

SAMPLES_SMALL="$RESULTS_DIR/data/samples_small"
mkdir -p "$SAMPLES_SMALL"

# Select first 5 songs from samples directory
echo "Selecting $DATASET_SIZE songs from data/samples..."
song_count=0
for wav_file in data/samples/*.wav; do
    if [ $song_count -lt $DATASET_SIZE ]; then
        cp "$wav_file" "$SAMPLES_SMALL/"
        echo "  Added: $(basename "$wav_file")"
        song_count=$((song_count + 1))
    else
        break
    fi
done

echo "Small dataset prepared with $song_count songs"
echo ""

#####################################################################
# STEP 2: Extract Features (Text and Binary formats)
#####################################################################

echo -e "${GREEN}STEP 2: Extracting features...${NC}"

FEATURES_TEXT="$RESULTS_DIR/data/features_text"
FEATURES_BINARY="$RESULTS_DIR/data/features_binary"

echo "Extracting text format features..."
./apps/extract_features "$SAMPLES_SMALL" "$FEATURES_TEXT" spectral config/feature_extraction_spectral_default.json

echo "Extracting binary format features..."
./apps/extract_features "$SAMPLES_SMALL" "$FEATURES_BINARY" spectral config/feature_extraction_spectral_default.json --binary

echo "Feature extraction complete"
echo ""

#####################################################################
# STEP 3: Test All Compressors (Text Format)
#####################################################################

echo -e "${GREEN}STEP 3: Testing all compressors with text format...${NC}"

TEXT_RESULTS="$RESULTS_DIR/text_format_results"

./scripts/evaluate_system.sh \
    -q "$FEATURES_TEXT" \
    -d "$FEATURES_TEXT" \
    -o "$TEXT_RESULTS" \
    -c "gzip,bzip2,lzma,zstd"

echo "Text format testing complete"
echo ""

#####################################################################
# STEP 4: Test All Compressors (Binary Format)
#####################################################################

echo -e "${GREEN}STEP 4: Testing all compressors with binary format...${NC}"

BINARY_RESULTS="$RESULTS_DIR/binary_format_results"

./scripts/evaluate_system.sh \
    -q "$FEATURES_BINARY" \
    -d "$FEATURES_BINARY" \
    -o "$BINARY_RESULTS" \
    -c "gzip,bzip2,lzma,zstd" \
    --binary

echo "Binary format testing complete"
echo ""

#####################################################################
# STEP 5: Parameter Evaluation (Binary Format)
#####################################################################

echo -e "${GREEN}STEP 5: Parameter evaluation with all compressors...${NC}"

PARAM_EVAL_RESULTS="$RESULTS_DIR/parameter_evaluation"

./scripts/quick_param_eval.sh \
    --query-dir "$SAMPLES_SMALL" \
    --db-dir "$SAMPLES_SMALL" \
    --output-dir "$PARAM_EVAL_RESULTS" \
    --compressors "gzip,bzip2,lzma,zstd" \
    --binary

echo "Parameter evaluation complete"
echo ""

#####################################################################
# STEP 6: Generate Visualizations
#####################################################################

echo -e "${GREEN}STEP 6: Generating visualizations...${NC}"

# Use dedicated plotting script to generate all visualizations
bash scripts/generate_test_plots.sh \
    --results-dir "$RESULTS_DIR" \
    --output-dir "$RESULTS_DIR/plots" \
    --test-type "small_dataset" \
    --formats "text,binary" \
    --compressors "gzip,bzip2,lzma,zstd"

echo "Visualizations complete"
echo ""

#####################################################################
# STEP 7: Generate Comprehensive Report
#####################################################################

echo -e "${GREEN}STEP 7: Generating comprehensive report...${NC}"

REPORT_FILE="$RESULTS_DIR/comprehensive_report.md"

cat > "$REPORT_FILE" << EOF
# Music Identification Test Report - Small Dataset

**Test Date:** $(date)  
**Dataset:** Small dataset ($DATASET_SIZE songs)  
**Compressors Tested:** ${COMPRESSORS[*]}  

## Dataset Information

- **Source:** data/samples (first $DATASET_SIZE songs)
- **Songs tested:** $song_count
- **Feature extraction:** Spectral method
- **Formats tested:** Text (.feat) and Binary (.featbin)

## Test Results Summary

### Text Format Results
EOF

# Add text format results to report
for comp in "${COMPRESSORS[@]}"; do
    if [ -f "$TEXT_RESULTS/$comp/accuracy_metrics_$comp.json" ]; then
        echo "#### $comp Compressor (Text Format)" >> "$REPORT_FILE"
        if command -v python3 &> /dev/null; then
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
    fi
done

echo "### Binary Format Results" >> "$REPORT_FILE"

# Add binary format results to report
for comp in "${COMPRESSORS[@]}"; do
    if [ -f "$BINARY_RESULTS/$comp/accuracy_metrics_$comp.json" ]; then
        echo "#### $comp Compressor (Binary Format)" >> "$REPORT_FILE"
        if command -v python3 &> /dev/null; then
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
    fi
done

cat >> "$REPORT_FILE" << EOF

## Files Generated

### Data Files
- **Original Dataset:** \`$SAMPLES_SMALL\`
- **Text Features:** \`$FEATURES_TEXT\`
- **Binary Features:** \`$FEATURES_BINARY\`

### Results
- **Text Format Results:** \`$TEXT_RESULTS\`
- **Binary Format Results:** \`$BINARY_RESULTS\`
- **Parameter Evaluation:** \`$PARAM_EVAL_RESULTS\`

### Visualizations
- **Text Format Analysis:** \`$RESULTS_DIR/plots/text_format_analysis\`
- **Binary Format Analysis:** \`$RESULTS_DIR/plots/binary_format_analysis\`
- **Parameter Evaluation:** \`$RESULTS_DIR/plots/parameter_evaluation\`

## Test Log
See \`test_log.txt\` for complete execution details.
EOF

echo "Comprehensive report generated: $REPORT_FILE"
echo ""

#####################################################################
# STEP 8: Test Summary
#####################################################################

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
echo -e "${GREEN}Small dataset test completed successfully!${NC}"
