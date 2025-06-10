#!/bin/bash

# Parameter Evaluation and Performance Analysis Test Script
# Analyzes results from test_feature_extraction.sh and test_compression.sh

# Color codes for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Configuration
PROJECT_ROOT="$(dirname "$(realpath "$0")")/.."
DATASET="full_tracks"  # Change to: samples or full_tracks for larger datasets
FEATURES_BASE_DIR="tests/feature_extraction_results/$DATASET"
COMPRESSION_BASE_DIR="tests/compression_results/$DATASET"
OUTPUT_BASE_DIR="tests/parameter_analysis"
METHODS=("spectral" "maxfreq")
FORMATS=("text" "binary")
COMPRESSORS=("gzip" "bzip2" "lzma" "zstd")
NOISE_TYPES=("clean" "white" "brown" "pink")  # Include noise types

# Print colored output
print_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

print_step() {
    echo -e "${BLUE}==================== STEP $1: $2 ====================${NC}"
}

# Change to project directory
cd "$PROJECT_ROOT" || {
    print_error "Could not change to project directory: $PROJECT_ROOT"
    exit 1
}

# Pre checks
print_step "1" "Pre checks"

if [ ! -d "$FEATURES_BASE_DIR" ]; then
    print_error "Features directory not found: $FEATURES_BASE_DIR"
    print_error "Please run test_feature_extraction.sh first"
    exit 1
fi

if [ ! -d "$COMPRESSION_BASE_DIR" ]; then
    print_error "Compression results directory not found: $COMPRESSION_BASE_DIR"
    print_error "Please run test_compression.sh first"
    exit 1
fi

print_success "Found feature extraction results in: $FEATURES_BASE_DIR"
print_success "Found compression results in: $COMPRESSION_BASE_DIR"

# Create output directory
TEST_OUTPUT_DIR="$OUTPUT_BASE_DIR/$DATASET"
mkdir -p "$TEST_OUTPUT_DIR"
print_info "Analysis output directory: $TEST_OUTPUT_DIR"

# Function to analyze feature extraction performance
analyze_feature_extraction() {
    print_step "2" "Feature extraction performance analysis"
    
    local analysis_file="$TEST_OUTPUT_DIR/feature_extraction_analysis.txt"
    
    cat > "$analysis_file" << EOF
========================================
FEATURE EXTRACTION PERFORMANCE ANALYSIS
========================================
Analysis Date: $(date)

DATASET: $DATASET
METHODS ANALYZED: ${METHODS[*]}
FORMATS ANALYZED: ${FORMATS[*]}
NOISE TYPES ANALYZED: ${NOISE_TYPES[*]}

EOF

    # Analyze each noise type, method and format combination
    for noise_type in "${NOISE_TYPES[@]}"; do
        cat >> "$analysis_file" << EOF

=== $noise_type SAMPLES ===
EOF
        
        for method in "${METHODS[@]}"; do
            for format in "${FORMATS[@]}"; do
                local features_dir="$FEATURES_BASE_DIR/${noise_type}/${method}/${format}"
            
                if [ -d "$features_dir" ]; then
                    cat >> "$analysis_file" << EOF

$noise_type/$method METHOD - $format FORMAT:
EOF
                
                # Count files
                if [ "$format" = "binary" ]; then
                    extension="featbin"
                else
                    extension="feat"
                fi
                
                local file_count=$(find "$features_dir" -name "*.${extension}" | wc -l)
                local total_size=$(du -sh "$features_dir" 2>/dev/null | cut -f1)
                
                cat >> "$analysis_file" << EOF
- Feature files generated: $file_count
- Total storage size: $total_size
EOF
                
                # Calculate average file size
                if [ "$file_count" -gt 0 ]; then
                    local avg_size_bytes=$(find "$features_dir" -name "*.${extension}" -exec du -b {} + | awk '{sum+=$1} END {print int(sum/NR)}')
                    local avg_size_human=$(numfmt --to=iec --suffix=B "$avg_size_bytes" 2>/dev/null || echo "${avg_size_bytes}B")
                    
                    cat >> "$analysis_file" << EOF
- Average file size: $avg_size_human
EOF
                fi
                
                # Sample file analysis
                local sample_file=$(find "$features_dir" -name "*.${extension}" | head -1)
                if [ -n "$sample_file" ] && [ "$format" = "text" ]; then
                    local line_count=$(wc -l < "$sample_file" 2>/dev/null || echo "0")
                    local char_count=$(wc -c < "$sample_file" 2>/dev/null || echo "0")
                    
                    cat >> "$analysis_file" << EOF
- Sample file lines: $line_count
- Sample file characters: $char_count
EOF
                fi
                else
                    cat >> "$analysis_file" << EOF

$noise_type/$method METHOD - $format FORMAT:
- Status: NOT FOUND
EOF
                fi
            done
        done
    done
    
    print_success "Feature extraction analysis saved to: $analysis_file"
}

# Function to analyze compression performance
analyze_compression_performance() {
    print_step "3" "Compression performance analysis"
    
    local analysis_file="$TEST_OUTPUT_DIR/compression_analysis.txt"
    
    cat > "$analysis_file" << EOF
========================================
COMPRESSION PERFORMANCE ANALYSIS
========================================
Analysis Date: $(date)

DATASET: $DATASET
METHODS ANALYZED: ${METHODS[*]}
FORMATS ANALYZED: ${FORMATS[*]}
COMPRESSORS ANALYZED: ${COMPRESSORS[*]}
NOISE TYPES ANALYZED: ${NOISE_TYPES[*]}

PERFORMANCE SUMMARY:
EOF

    # Track best performers
    local best_top1_accuracy=0
    local best_top1_config=""
    local best_top5_accuracy=0
    local best_top5_config=""
    
    # Analyze each combination
    for noise_type in "${NOISE_TYPES[@]}"; do
        cat >> "$analysis_file" << EOF

=== $noise_type SAMPLES ===
EOF
        
        for method in "${METHODS[@]}"; do
            for format in "${FORMATS[@]}"; do
                cat >> "$analysis_file" << EOF

$noise_type/$method METHOD - $format FORMAT:
EOF
                for compressor in "${COMPRESSORS[@]}"; do
                    local results_dir="$COMPRESSION_BASE_DIR/${noise_type}/${method}/${format}/${compressor}"
                    local accuracy_file="$results_dir/accuracy_metrics_${compressor}.json"
                
                if [ -f "$accuracy_file" ]; then
                    if command -v jq > /dev/null; then
                        local top1_accuracy=$(jq -r '.top1_accuracy // 0' "$accuracy_file" 2>/dev/null)
                        local top5_accuracy=$(jq -r '.top5_accuracy // 0' "$accuracy_file" 2>/dev/null)
                        local total_queries=$(jq -r '.total_queries // 0' "$accuracy_file" 2>/dev/null)
                        
                        cat >> "$analysis_file" << EOF
  $compressor: Top-1: ${top1_accuracy}%, Top-5: ${top5_accuracy}%, Queries: $total_queries
EOF
                        
                        # Track best performers
                        if (( $(echo "$top1_accuracy > $best_top1_accuracy" | bc -l 2>/dev/null || echo "0") )); then
                            best_top1_accuracy="$top1_accuracy"
                            best_top1_config="$noise_type/$method/$format/$compressor"
                        fi
                        
                        if (( $(echo "$top5_accuracy > $best_top5_accuracy" | bc -l 2>/dev/null || echo "0") )); then
                            best_top5_accuracy="$top5_accuracy"
                            best_top5_config="$noise_type/$method/$format/$compressor"
                        fi
                    else
                        cat >> "$analysis_file" << EOF
  $compressor: Results available (install jq for detailed metrics)
EOF
                    fi
                else
                    cat >> "$analysis_file" << EOF
  $compressor: NO RESULTS
EOF
                fi
            done
        done
    done
    done
    
    # Add best performers summary
    cat >> "$analysis_file" << EOF

BEST PERFORMERS:
- Best Top-1 Accuracy: ${best_top1_accuracy}% (${best_top1_config})
- Best Top-5 Accuracy: ${best_top5_accuracy}% (${best_top5_config})
EOF
    
    print_success "Compression analysis saved to: $analysis_file"
    
    if [ -n "$best_top1_config" ]; then
        print_success "Best Top-1 accuracy: ${best_top1_accuracy}% with ${best_top1_config}"
    fi
    
    if [ -n "$best_top5_config" ]; then
        print_success "Best Top-5 accuracy: ${best_top5_accuracy}% with ${best_top5_config}"
    fi
}

# Function to compare methods
compare_methods() {
    print_step "4" "Method comparison analysis"
    
    local comparison_file="$TEST_OUTPUT_DIR/method_comparison.txt"
    
    cat > "$comparison_file" << EOF
========================================
METHOD COMPARISON ANALYSIS
========================================
Analysis Date: $(date)

Comparing SPECTRAL vs MAXFREQ methods across all formats and compressors:

EOF

    for format in "${FORMATS[@]}"; do
        cat >> "$comparison_file" << EOF

$format FORMAT COMPARISON:
EOF
        
        # Calculate averages for each method
        local spectral_total=0
        local spectral_count=0
        local maxfreq_total=0
        local maxfreq_count=0
        
        for noise_type in "${NOISE_TYPES[@]}"; do
            for compressor in "${COMPRESSORS[@]}"; do
                # Spectral method
                local spectral_file="$COMPRESSION_BASE_DIR/${noise_type}/spectral/${format}/${compressor}/accuracy_metrics_${compressor}.json"
                if [ -f "$spectral_file" ] && command -v jq > /dev/null; then
                    local spectral_top1=$(jq -r '.top1_accuracy // 0' "$spectral_file" 2>/dev/null)
                    spectral_total=$(echo "$spectral_total + $spectral_top1" | bc -l 2>/dev/null || echo "$spectral_total")
                    ((spectral_count++))
                fi
                
                # MaxFreq method
                local maxfreq_file="$COMPRESSION_BASE_DIR/${noise_type}/maxfreq/${format}/${compressor}/accuracy_metrics_${compressor}.json"
                if [ -f "$maxfreq_file" ] && command -v jq > /dev/null; then
                    local maxfreq_top1=$(jq -r '.top1_accuracy // 0' "$maxfreq_file" 2>/dev/null)
                    maxfreq_total=$(echo "$maxfreq_total + $maxfreq_top1" | bc -l 2>/dev/null || echo "$maxfreq_total")
                    ((maxfreq_count++))
                fi
            done
        done
        
        # Calculate averages
        if [ "$spectral_count" -gt 0 ]; then
            local spectral_avg=$(echo "scale=2; $spectral_total / $spectral_count" | bc -l 2>/dev/null || echo "0")
        else
            local spectral_avg="0"
        fi
        
        if [ "$maxfreq_count" -gt 0 ]; then
            local maxfreq_avg=$(echo "scale=2; $maxfreq_total / $maxfreq_count" | bc -l 2>/dev/null || echo "0")
        else
            local maxfreq_avg="0"
        fi
        
        cat >> "$comparison_file" << EOF
- Spectral average Top-1 accuracy: ${spectral_avg}% (${spectral_count} tests)
- MaxFreq average Top-1 accuracy: ${maxfreq_avg}% (${maxfreq_count} tests)
EOF
        
        # Determine winner
        if (( $(echo "$spectral_avg > $maxfreq_avg" | bc -l 2>/dev/null || echo "0") )); then
            cat >> "$comparison_file" << EOF
- Winner: SPECTRAL method
EOF
        elif (( $(echo "$maxfreq_avg > $spectral_avg" | bc -l 2>/dev/null || echo "0") )); then
            cat >> "$comparison_file" << EOF
- Winner: MAXFREQ method
EOF
        else
            cat >> "$comparison_file" << EOF
- Result: TIE
EOF
        fi
    done
    
    print_success "Method comparison saved to: $comparison_file"
}

# Function to compare formats
compare_formats() {
    print_step "5" "Format comparison analysis"
    
    local comparison_file="$TEST_OUTPUT_DIR/format_comparison.txt"
    
    cat > "$comparison_file" << EOF
========================================
FORMAT COMPARISON ANALYSIS
========================================
Analysis Date: $(date)

Comparing TEXT vs BINARY formats across all methods and compressors:

EOF

    for method in "${METHODS[@]}"; do
        cat >> "$comparison_file" << EOF

$method METHOD COMPARISON:
EOF
        
        # Calculate averages for each format
        local text_total=0
        local text_count=0
        local binary_total=0
        local binary_count=0
        
        for noise_type in "${NOISE_TYPES[@]}"; do
            for compressor in "${COMPRESSORS[@]}"; do
                # Text format
                local text_file="$COMPRESSION_BASE_DIR/${noise_type}/${method}/text/${compressor}/accuracy_metrics_${compressor}.json"
                if [ -f "$text_file" ] && command -v jq > /dev/null; then
                    local text_top1=$(jq -r '.top1_accuracy // 0' "$text_file" 2>/dev/null)
                    text_total=$(echo "$text_total + $text_top1" | bc -l 2>/dev/null || echo "$text_total")
                    ((text_count++))
                fi
                
                # Binary format
                local binary_file="$COMPRESSION_BASE_DIR/${noise_type}/${method}/binary/${compressor}/accuracy_metrics_${compressor}.json"
                if [ -f "$binary_file" ] && command -v jq > /dev/null; then
                    local binary_top1=$(jq -r '.top1_accuracy // 0' "$binary_file" 2>/dev/null)
                    binary_total=$(echo "$binary_total + $binary_top1" | bc -l 2>/dev/null || echo "$binary_total")
                    ((binary_count++))
                fi
            done
        done
        
        # Calculate averages
        if [ "$text_count" -gt 0 ]; then
            local text_avg=$(echo "scale=2; $text_total / $text_count" | bc -l 2>/dev/null || echo "0")
        else
            local text_avg="0"
        fi
        
        if [ "$binary_count" -gt 0 ]; then
            local binary_avg=$(echo "scale=2; $binary_total / $binary_count" | bc -l 2>/dev/null || echo "0")
        else
            local binary_avg="0"
        fi
        
        cat >> "$comparison_file" << EOF
- Text format average Top-1 accuracy: ${text_avg}% (${text_count} tests)
- Binary format average Top-1 accuracy: ${binary_avg}% (${binary_count} tests)
EOF
        
        # Determine winner
        if (( $(echo "$text_avg > $binary_avg" | bc -l 2>/dev/null || echo "0") )); then
            cat >> "$comparison_file" << EOF
- Winner: TEXT format
EOF
        elif (( $(echo "$binary_avg > $text_avg" | bc -l 2>/dev/null || echo "0") )); then
            cat >> "$comparison_file" << EOF
- Winner: BINARY format
EOF
        else
            cat >> "$comparison_file" << EOF
- Result: TIE
EOF
        fi
    done
    
    print_success "Format comparison saved to: $comparison_file"
}

# Function to analyze compressor performance
compare_compressors() {
    print_step "6" "Compressor comparison analysis"
    
    local comparison_file="$TEST_OUTPUT_DIR/compressor_comparison.txt"
    
    cat > "$comparison_file" << EOF
========================================
COMPRESSOR COMPARISON ANALYSIS
========================================
Analysis Date: $(date)

Comparing all compressors across methods and formats:

EOF

    # Calculate overall averages for each compressor
    for compressor in "${COMPRESSORS[@]}"; do
        local total_accuracy=0
        local test_count=0
        
        for noise_type in "${NOISE_TYPES[@]}"; do
            for method in "${METHODS[@]}"; do
                for format in "${FORMATS[@]}"; do
                    local accuracy_file="$COMPRESSION_BASE_DIR/${noise_type}/${method}/${format}/${compressor}/accuracy_metrics_${compressor}.json"
                    if [ -f "$accuracy_file" ] && command -v jq > /dev/null; then
                        local top1_accuracy=$(jq -r '.top1_accuracy // 0' "$accuracy_file" 2>/dev/null)
                        total_accuracy=$(echo "$total_accuracy + $top1_accuracy" | bc -l 2>/dev/null || echo "$total_accuracy")
                        ((test_count++))
                    fi
                done
            done
        done
        
        if [ "$test_count" -gt 0 ]; then
            local avg_accuracy=$(echo "scale=2; $total_accuracy / $test_count" | bc -l 2>/dev/null || echo "0")
            cat >> "$comparison_file" << EOF
$compressor: ${avg_accuracy}% average Top-1 accuracy (${test_count} tests)
EOF
        else
            cat >> "$comparison_file" << EOF
$compressor: NO RESULTS
EOF
        fi
    done
    
    print_success "Compressor comparison saved to: $comparison_file"
}

# Function to generate comprehensive report
generate_comprehensive_report() {
    print_step "7" "Generating comprehensive analysis report"
    
    local report_file="$TEST_OUTPUT_DIR/comprehensive_analysis_report.txt"
    
    cat > "$report_file" << EOF
========================================
COMPREHENSIVE PARAMETER ANALYSIS REPORT
========================================
Analysis Date: $(date)
Dataset: $DATASET

This report analyzes the performance of different parameter combinations
for the music identification system using Normalized Compression Distance (NCD).

ANALYSIS INCLUDES:
- Feature extraction performance (file sizes, generation success)
- Compression algorithm accuracy comparison
- Method comparison (Spectral vs MaxFreq)
- Format comparison (Text vs Binary)
- Compressor performance ranking

METHODOLOGY:
- Default parameters used for both methods:
  * Spectral: 32 bins, 1024 frame size, 512 hop size
  * MaxFreq: 4 frequencies, 1024 frame size, 512 hop size
- All 4 compressors tested: ${COMPRESSORS[*]}
- Both text (.feat) and binary (.featbin) formats tested

DETAILED RESULTS:
EOF

    # Include summaries from other analysis files
    for analysis_file in "$TEST_OUTPUT_DIR"/*.txt; do
        if [ -f "$analysis_file" ] && [ "$analysis_file" != "$report_file" ]; then
            local filename=$(basename "$analysis_file")
            cat >> "$report_file" << EOF

--- INCLUDED: $filename ---
EOF
            cat "$analysis_file" >> "$report_file"
            cat >> "$report_file" << EOF

EOF
        fi
    done
    
    cat >> "$report_file" << EOF

ANALYSIS COMPLETED: $(date)

For more detailed results, check the individual analysis files in:
$TEST_OUTPUT_DIR/
EOF

    print_success "Comprehensive report generated: $report_file"
    
    echo
    print_info "=== ANALYSIS SUMMARY ==="
    echo
    
    # Show best configurations
    local best_config_file=$(mktemp)
    
    # Find best Top-1 accuracy
    local best_accuracy=0
    local best_config=""
    
    for noise_type in "${NOISE_TYPES[@]}"; do
        for method in "${METHODS[@]}"; do
            for format in "${FORMATS[@]}"; do
                for compressor in "${COMPRESSORS[@]}"; do
                    local accuracy_file="$COMPRESSION_BASE_DIR/${noise_type}/${method}/${format}/${compressor}/accuracy_metrics_${compressor}.json"
                    if [ -f "$accuracy_file" ] && command -v jq > /dev/null; then
                        local top1_accuracy=$(jq -r '.top1_accuracy // 0' "$accuracy_file" 2>/dev/null)
                        if (( $(echo "$top1_accuracy > $best_accuracy" | bc -l 2>/dev/null || echo "0") )); then
                            best_accuracy="$top1_accuracy"
                            best_config="$noise_type/$method/$format/$compressor"
                        fi
                    fi
                done
            done
        done
    done
    
    if [ -n "$best_config" ]; then
        print_success "BEST CONFIGURATION: $best_config with ${best_accuracy}% Top-1 accuracy"
    else
        print_warning "No valid results found - check if compression tests completed successfully"
    fi
    
    rm -f "$best_config_file"
}

# Main execution
print_info "Starting parameter evaluation and performance analysis for dataset: $DATASET"

# Run all analyses
analyze_feature_extraction
analyze_compression_performance
compare_methods
compare_formats
compare_compressors
generate_comprehensive_report

print_info "All analysis files saved in: $TEST_OUTPUT_DIR"
print_success "Parameter evaluation and performance analysis completed successfully!"
