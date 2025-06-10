#!/bin/bash

# Feature Extraction Test Script
# Tests both spectral and maxfreq methods in text and binary formats

# Color codes for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Configuration
PROJECT_ROOT="$(dirname "$(realpath "$0")")/.."
DATASET="small_samples"  # Change to: samples or full_tracks for larger datasets
OUTPUT_BASE_DIR="tests/feature_extraction_results"
TEST_METHODS=("spectral" "maxfreq")  # Both methods
FORMATS=("text" "binary")            # Both formats

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

# Check if required files exist
print_step "1" "Pre-flight checks"

if [ ! -f "apps/extract_features" ]; then
    print_error "extract_features application not found. Please build the project first."
    print_info "Run: bash scripts/rebuild.sh"
    exit 1
fi

if [ ! -d "data/$DATASET" ]; then
    print_error "Dataset directory does not exist: data/$DATASET"
    exit 1
fi

# Count input files
INPUT_DIR="data/$DATASET"
WAV_COUNT=$(find "$INPUT_DIR" -name "*.wav" | wc -l)

if [ "$WAV_COUNT" -eq 0 ]; then
    print_error "No WAV files found in $INPUT_DIR"
    exit 1
fi

print_success "Found $WAV_COUNT WAV files in $INPUT_DIR"
print_info "Testing methods: ${TEST_METHODS[*]}"
print_info "Testing formats: text and binary"

# Create output directories
TEST_OUTPUT_DIR="$OUTPUT_BASE_DIR/$DATASET"

print_info "Test output directory: $TEST_OUTPUT_DIR"

# Function to run feature extraction and measure performance
run_feature_extraction() {
    local method=$1
    local format=$2
    local binary_flag=$3
    local output_dir=$4
    
    print_step "2" "Feature extraction - $method method, $format format"
    
    mkdir -p "$output_dir"
    
    # Start timing
    start_time=$(date +%s)
    
    # Run feature extraction using default parameters (no config file needed)
    if [[ "$method" == "spectral" ]]; then
        # Default spectral parameters: 32 bins, 1024 frame size, 512 hop size
        cmd="./apps/extract_features --method spectral --bins 32 --frame-size 1024 --hop-size 512 -i $INPUT_DIR -o $output_dir"
    else
        # Default maxfreq parameters: 4 frequencies, 1024 frame size, 512 hop size
        cmd="./apps/extract_features --method maxfreq --frequencies 4 --frame-size 1024 --hop-size 512 -i $INPUT_DIR -o $output_dir"
    fi
    if [ "$binary_flag" = "true" ]; then
        cmd="$cmd --binary"
    fi
    
    print_info "Executing: $cmd"
    
    if ! eval "$cmd"; then
        print_error "Feature extraction failed for $method method, $format format"
        return 1
    fi
    
    # End timing
    end_time=$(date +%s)
    duration=$((end_time - start_time))
    
    # Count output files
    if [ "$binary_flag" = "true" ]; then
        feat_count=$(find "$output_dir" -name "*.featbin" | wc -l)
        extension="featbin"
    else
        feat_count=$(find "$output_dir" -name "*.feat" | wc -l)
        extension="feat"
    fi
    
    print_success "$method/$format extraction completed in ${duration}s"
    print_success "Generated $feat_count .$extension files"
    
    # Calculate file sizes
    total_size=$(du -sh "$output_dir" | cut -f1)
    print_info "Total size of $method/$format features: $total_size"
    
    # Sample a feature file to show content
    sample_file=$(find "$output_dir" -name "*.$extension" | head -1)
    if [ -n "$sample_file" ]; then
        print_info "Sample $format feature file: $(basename "$sample_file")"
        if [ "$binary_flag" = "true" ]; then
            print_info "Binary file size: $(du -h "$sample_file" | cut -f1)"
        else
            print_info "First 5 lines of text feature file:"
            head -5 "$sample_file" | sed 's/^/  /'
        fi
    fi
    
    return 0
}

# Function to validate feature files
validate_features() {
    local method=$1
    local format=$2
    local binary_flag=$3
    local output_dir=$4
    
    print_step "3" "Validation - $method method, $format format"
    
    if [ "$binary_flag" = "true" ]; then
        extension="featbin"
    else
        extension="feat"
    fi
    
    # Check if all input files have corresponding feature files
    missing_count=0
    for wav_file in "$INPUT_DIR"/*.wav; do
        if [ -f "$wav_file" ]; then
            base_name=$(basename "$wav_file" .wav)
            expected_feat="${output_dir}/${base_name}_${method}.${extension}"
            
            if [ ! -f "$expected_feat" ]; then
                print_warning "Missing feature file for: $base_name"
                ((missing_count++))
            fi
        fi
    done
    
    if [ "$missing_count" -eq 0 ]; then
        print_success "All WAV files have corresponding $method/$format feature files"
    else
        print_warning "$missing_count WAV files are missing $method/$format feature files"
    fi
    
    # Check for empty files
    empty_count=0
    while IFS= read -r -d '' feat_file; do
        if [ ! -s "$feat_file" ]; then
            print_warning "Empty feature file: $(basename "$feat_file")"
            ((empty_count++))
        fi
    done < <(find "$output_dir" -name "*.$extension" -print0)
    
    if [ "$empty_count" -eq 0 ]; then
        print_success "No empty $method/$format feature files found"
    else
        print_warning "$empty_count empty $method/$format feature files found"
    fi
}

# Function to compare text vs binary features
compare_formats() {
    print_step "4" "Comparing text vs binary formats"
    
    for method in "${TEST_METHODS[@]}"; do
        text_dir="$TEST_OUTPUT_DIR/${method}/text"
        binary_dir="$TEST_OUTPUT_DIR/${method}/binary"
        
        if [ -d "$text_dir" ] && [ -d "$binary_dir" ]; then
            text_size=$(du -s "$text_dir" | cut -f1)
            binary_size=$(du -s "$binary_dir" | cut -f1)
            
            if [ "$binary_size" -lt "$text_size" ]; then
                compression_ratio=$(echo "scale=2; $text_size / $binary_size" | bc)
                print_success "$method: Binary format is ${compression_ratio}x smaller than text format"
            else
                expansion_ratio=$(echo "scale=2; $binary_size / $text_size" | bc)
                print_info "$method: Binary format is ${expansion_ratio}x larger than text format"
            fi
            
            text_count=$(find "$text_dir" -name "*.feat" | wc -l)
            binary_count=$(find "$binary_dir" -name "*.featbin" | wc -l)
            
            if [ "$text_count" -eq "$binary_count" ]; then
                print_success "$method: Same number of files in both formats ($text_count)"
            else
                print_warning "$method: Different number of files: text=$text_count, binary=$binary_count"
            fi
        fi
    done
}

# Function to generate summary report
generate_report() {
    print_step "5" "Generating test report"
    
    report_file="$TEST_OUTPUT_DIR/test_report.txt"
    
    cat > "$report_file" << EOF
========================================
FEATURE EXTRACTION TEST REPORT
========================================
Test Date: $(date)

CONFIGURATION:
- Dataset: $DATASET
- Methods Tested: ${TEST_METHODS[*]}
- Input Directory: $INPUT_DIR
- Output Directory: $TEST_OUTPUT_DIR

PARAMETERS USED:
- Spectral method: 32 bins, 1024 frame size, 512 hop size
- MaxFreq method: 4 frequencies, 1024 frame size, 512 hop size

INPUT INFORMATION:
- Total WAV files: $WAV_COUNT
- Input directory size: $(du -sh "$INPUT_DIR" | cut -f1)

TESTS PERFORMED:
EOF

    for method in "${TEST_METHODS[@]}"; do
        cat >> "$report_file" << EOF

$method METHOD:
EOF
        
        text_dir="$TEST_OUTPUT_DIR/${method}/text"
        if [ -d "$text_dir" ]; then
            text_count=$(find "$text_dir" -name "*.feat" | wc -l)
            text_size=$(du -sh "$text_dir" | cut -f1)
            cat >> "$report_file" << EOF
- Text format (.feat): $text_count files, $text_size total
EOF
        fi

        binary_dir="$TEST_OUTPUT_DIR/${method}/binary"
        if [ -d "$binary_dir" ]; then
            binary_count=$(find "$binary_dir" -name "*.featbin" | wc -l)
            binary_size=$(du -sh "$binary_dir" | cut -f1)
            cat >> "$report_file" << EOF
- Binary format (.featbin): $binary_count files, $binary_size total
EOF
        fi
    done

    cat >> "$report_file" << EOF

TEST COMPLETED: $(date)
EOF

    print_success "Report generated: $report_file"
    
    echo
    cat "$report_file"
}

# Main execution
print_info "Starting feature extraction test for dataset: $DATASET"
print_info "Testing methods: ${TEST_METHODS[*]}"
print_info "Testing formats: text and binary"

# Loop through each method
for method in "${TEST_METHODS[@]}"; do
    print_info "Testing $method method..."
    
    # Test text format
    text_output_dir="$TEST_OUTPUT_DIR/${method}/text"
    if run_feature_extraction "$method" "text" "false" "$text_output_dir"; then
        validate_features "$method" "text" "false" "$text_output_dir"
    fi

    # Test binary format
    binary_output_dir="$TEST_OUTPUT_DIR/${method}/binary"
    if run_feature_extraction "$method" "binary" "true" "$binary_output_dir"; then
        validate_features "$method" "binary" "true" "$binary_output_dir"
    fi
done

# Generate summary report
generate_report

print_info "Test files preserved in: $TEST_OUTPUT_DIR"
print_success "Feature extraction test completed successfully!"
