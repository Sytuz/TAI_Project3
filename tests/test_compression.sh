#!/bin/bash

# Compression Test Script
# Tests all compressors with both methods and formats using already extracted features

# Color codes for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Configuration
PROJECT_ROOT="$(dirname "$(realpath "$0")")/.."
DATASET="small_samples"  # Change to: samples or full_tracks for larger datasets
OUTPUT_BASE_DIR="tests/compression_results"
FEATURES_BASE_DIR="tests/feature_extraction_results/$DATASET"
COMPRESSORS=("gzip" "bzip2" "lzma" "zstd")
METHODS=("spectral" "maxfreq")
FORMATS=("text" "binary")
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

if [ ! -f "scripts/batch_identify.sh" ]; then
    print_error "batch_identify.sh script not found"
    exit 1
fi

if [ ! -f "scripts/calculate_accuracy.py" ]; then
    print_error "calculate_accuracy.py script not found"
    exit 1
fi

if [ ! -d "$FEATURES_BASE_DIR" ]; then
    print_error "Features directory not found: $FEATURES_BASE_DIR"
    print_error "Please run test_feature_extraction.sh first to extract features"
    exit 1
fi

print_success "Found feature extraction results in: $FEATURES_BASE_DIR"
print_info "Testing compressors: ${COMPRESSORS[*]}"
print_info "Testing methods: ${METHODS[*]}"
print_info "Testing formats: ${FORMATS[*]}"

# Create output directory
TEST_OUTPUT_DIR="$OUTPUT_BASE_DIR/$DATASET"
mkdir -p "$TEST_OUTPUT_DIR"
print_info "Test output directory: $TEST_OUTPUT_DIR"

# Function to validate feature files exist
validate_features() {
    local method=$1
    local format=$2
    local features_dir=$3
    
    if [ "$format" = "binary" ]; then
        extension="featbin"
    else
        extension="feat"
    fi
    
    feat_count=$(find "$features_dir" -name "*.${extension}" | wc -l)
    
    if [ "$feat_count" -eq 0 ]; then
        print_error "No $format feature files found in $features_dir"
        return 1
    fi
    
    print_success "Found $feat_count $format feature files for $method method"
    return 0
}

# Function to run batch identification with a specific compressor
run_batch_identification() {
    local method=$1
    local format=$2
    local compressor=$3
    local features_dir=$4
    local output_dir=$5
    
    print_step "2" "Batch identification - $method/$format with $compressor"
    
    mkdir -p "$output_dir"
    
    # Start timing
    start_time=$(date +%s)
    
    # Determine binary flag
    local binary_flag=""
    if [ "$format" = "binary" ]; then
        binary_flag="--binary"
    fi
    
    # Run batch identification
    cmd="./scripts/batch_identify.sh -q $features_dir -d $features_dir -o $output_dir -c $compressor $binary_flag"
    
    print_info "Executing: $cmd"
    
    if ! eval "$cmd"; then
        print_error "Batch identification failed for $method/$format with $compressor"
        return 1
    fi
    
    # End timing
    end_time=$(date +%s)
    duration=$((end_time - start_time))
    
    print_success "$method/$format with $compressor completed in ${duration}s"
    
    return 0
}

# Function to calculate accuracy metrics
calculate_accuracy() {
    local method=$1
    local format=$2
    local compressor=$3
    local results_dir=$4
    
    print_step "3" "Reading accuracy - $method/$format with $compressor"
    
    # The batch_identify.sh script already creates accuracy_metrics_${compressor}.json
    # We just need to read and display the results
    accuracy_file="$results_dir/accuracy_metrics_${compressor}.json"
    
    # Display results if accuracy file exists
    if [ -f "$accuracy_file" ]; then
        if command -v jq > /dev/null; then
            top1=$(jq -r '.top1_accuracy // "N/A"' "$accuracy_file" 2>/dev/null)
            top5=$(jq -r '.top5_accuracy // "N/A"' "$accuracy_file" 2>/dev/null)
            top10=$(jq -r '.top10_accuracy // "N/A"' "$accuracy_file" 2>/dev/null)
            total=$(jq -r '.total_queries // "N/A"' "$accuracy_file" 2>/dev/null)
            
            print_success "Total queries: $total"
            print_success "Top-1 accuracy: $top1%"
            print_info "Top-5 accuracy: $top5%"
            print_info "Top-10 accuracy: $top10%"
        else
            print_info "Accuracy file found: $accuracy_file (install jq for detailed results)"
        fi
    else
        print_warning "Accuracy file not found: $accuracy_file"
    fi
    
    return 0
}

# Function to generate summary report
generate_report() {
    print_step "4" "Generating test report"
    
    report_file="$TEST_OUTPUT_DIR/compression_test_report.txt"
    
    cat > "$report_file" << EOF
========================================
COMPRESSION TEST REPORT
========================================
Test Date: $(date)

CONFIGURATION:
- Dataset: $DATASET
- Compressors Tested: ${COMPRESSORS[*]}
- Methods Tested: ${METHODS[*]}
- Formats Tested: ${FORMATS[*]}
- Noise Types: ${NOISE_TYPES[*]}
- Features Directory: $FEATURES_BASE_DIR
- Output Directory: $TEST_OUTPUT_DIR

TESTS PERFORMED:
EOF

    for noise_type in "${NOISE_TYPES[@]}"; do
        cat >> "$report_file" << EOF

=== $noise_type SAMPLES ===
EOF
        
        for method in "${METHODS[@]}"; do
            for format in "${FORMATS[@]}"; do
                cat >> "$report_file" << EOF

$noise_type/$method METHOD - $format FORMAT:
EOF
                for compressor in "${COMPRESSORS[@]}"; do
                    results_dir="$TEST_OUTPUT_DIR/${noise_type}/${method}/${format}/${compressor}"
                    if [ -d "$results_dir" ]; then
                        results_count=$(find "$results_dir" -name "*.csv" | wc -l)
                        accuracy_file="$results_dir/accuracy_metrics_${compressor}.json"
                        
                        if [ -f "$accuracy_file" ] && command -v jq > /dev/null; then
                            top1_accuracy=$(jq -r '.top1_accuracy // "N/A"' "$accuracy_file" 2>/dev/null)
                            top5_accuracy=$(jq -r '.top5_accuracy // "N/A"' "$accuracy_file" 2>/dev/null)
                            total_queries=$(jq -r '.total_queries // "N/A"' "$accuracy_file" 2>/dev/null)
                            cat >> "$report_file" << EOF
- $compressor: $results_count result files, $total_queries queries, Top-1: $top1_accuracy%, Top-5: $top5_accuracy%
EOF
                        else
                            cat >> "$report_file" << EOF
- $compressor: $results_count result files
EOF
                    fi
                else
                    cat >> "$report_file" << EOF
- $compressor: FAILED
EOF
                fi
            done
        done
    done

    cat >> "$report_file" << EOF

TEST COMPLETED: $(date)
EOF

    print_success "Report generated: $report_file"
    
    echo
    cat "$report_file"
}

# Main execution
print_info "Starting compression tests for dataset: $DATASET"
print_info "Testing noise types: ${NOISE_TYPES[*]}"

# Test each combination of noise type, method, format, and compressor
for noise_type in "${NOISE_TYPES[@]}"; do
    print_info "=== Testing with $noise_type samples ==="
    
    for method in "${METHODS[@]}"; do
        for format in "${FORMATS[@]}"; do
            features_dir="$FEATURES_BASE_DIR/${noise_type}/${method}/${format}"
            
            print_info "Testing $method method with $format format ($noise_type samples)..."
            
            # Validate that features exist
            if ! validate_features "$method" "$format" "$features_dir"; then
                print_warning "Skipping $noise_type/$method/$format - no features found"
                continue
            fi
            
            # Test each compressor
            for compressor in "${COMPRESSORS[@]}"; do
                print_info "Testing $compressor compressor..."
                
                output_dir="$TEST_OUTPUT_DIR/${noise_type}/${method}/${format}/${compressor}"
                
                if run_batch_identification "$method" "$format" "$compressor" "$features_dir" "$output_dir"; then
                    calculate_accuracy "$method" "$format" "$compressor" "$output_dir"
                fi
            done
        done
    done
done

# Generate summary report
generate_report

print_info "Test files preserved in: $TEST_OUTPUT_DIR"
print_success "Compression tests completed successfully!"
