#!/bin/bash

# Master Test Script - Runs all tests with noise combinations and threading
# This script runs feature extraction, compression, analysis, and plotting tests

# Color codes for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
PURPLE='\033[0;35m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

# Configuration
PROJECT_ROOT="$(dirname "$(realpath "$0")")/.."
DATASET="full_tracks"  # Change to: samples or full_tracks for larger datasets
NOISE_TYPES=("clean" "white" "brown" "pink")
METHODS=("spectral" "maxfreq")
FORMATS=("text" "binary")
COMPRESSORS=("gzip" "bzip2" "lzma" "zstd")

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
    echo -e "${PURPLE}==================== STEP $1: $2 ====================${NC}"
}

print_phase() {
    echo -e "${CYAN}#################### PHASE $1: $2 ####################${NC}"
}

# Change to project directory
cd "$PROJECT_ROOT" || {
    print_error "Could not change to project directory: $PROJECT_ROOT"
    exit 1
}

# Function to check prerequisites
check_prerequisites() {
    print_step "1" "Checking prerequisites"
    
    # Check if project is built
    if [ ! -f "apps/extract_features" ] || [ ! -f "apps/music_id" ]; then
        print_error "Project not built. Please run: bash scripts/rebuild.sh"
        exit 1
    fi
    
    # Check for required scripts
    local required_scripts=(
        "scripts/batch_identify.sh"
        "scripts/calculate_accuracy.py"
        "tests/test_feature_extraction.sh"
        "tests/test_compression.sh"
        "tests/test_parameter_analysis.sh"
        "tests/test_plots.sh"
    )
    
    for script in "${required_scripts[@]}"; do
        if [ ! -f "$script" ]; then
            print_error "Required script not found: $script"
            exit 1
        fi
    done
    
    # Check for sox (needed for noise generation)
    if ! command -v sox > /dev/null; then
        print_warning "sox not found - noise generation will be skipped"
        print_info "Install sox: sudo apt-get install sox"
    fi
    
    # Check for jq (needed for JSON parsing)
    if ! command -v jq > /dev/null; then
        print_warning "jq not found - some analysis features will be limited"
        print_info "Install jq: sudo apt-get install jq"
    fi
    
    # Check dataset
    if [ ! -d "data/$DATASET" ]; then
        print_error "Dataset directory not found: data/$DATASET"
        exit 1
    fi
    
    local wav_count=$(find "data/$DATASET" -name "*.wav" | wc -l)
    if [ "$wav_count" -eq 0 ]; then
        print_error "No WAV files found in data/$DATASET"
        exit 1
    fi
    
    print_success "Found $wav_count WAV files in data/$DATASET"
    print_success "All prerequisites met"
}

# Function to show test configuration
show_configuration() {
    print_step "2" "Test Configuration"
    
    echo "Dataset: $DATASET"
    echo "Noise types: ${NOISE_TYPES[*]}"
    echo "Methods: ${METHODS[*]}"
    echo "Formats: ${FORMATS[*]}"
    echo "Compressors: ${COMPRESSORS[*]}"
    echo ""
    echo "Total combinations:"
    echo "  Feature extraction: $((${#NOISE_TYPES[@]} * ${#METHODS[@]} * ${#FORMATS[@]})) combinations"
    echo "  Compression tests: $((${#NOISE_TYPES[@]} * ${#METHODS[@]} * ${#FORMATS[@]} * ${#COMPRESSORS[@]})) combinations"
    echo ""
}

# Function to run feature extraction tests
run_feature_extraction() {
    print_phase "1" "Feature Extraction Tests"
    
    chmod +x tests/test_feature_extraction.sh
    
    print_info "Starting feature extraction with parallel processing..."
    start_time=$(date +%s)
    
    if ! ./tests/test_feature_extraction.sh; then
        print_error "Feature extraction tests failed"
        return 1
    fi
    
    end_time=$(date +%s)
    duration=$((end_time - start_time))
    print_success "Feature extraction completed in ${duration}s"
    
    # Show summary
    local total_features=$(find tests/feature_extraction_results/$DATASET -name "*.feat" -o -name "*.featbin" | wc -l)
    print_info "Generated $total_features feature files"
    
    return 0
}

# Function to run compression tests
run_compression_tests() {
    print_phase "2" "Compression Tests"
    
    chmod +x tests/test_compression.sh
    
    print_info "Starting compression tests with parallel processing (up to 16 threads)..."
    start_time=$(date +%s)
    
    if ! ./tests/test_compression.sh; then
        print_error "Compression tests failed"
        return 1
    fi
    
    end_time=$(date +%s)
    duration=$((end_time - start_time))
    print_success "Compression tests completed in ${duration}s"
    
    # Show summary
    local total_results=$(find tests/compression_results/$DATASET -name "*.csv" | wc -l)
    local total_accuracy=$(find tests/compression_results/$DATASET -name "accuracy_metrics_*.json" | wc -l)
    print_info "Generated $total_results result files and $total_accuracy accuracy reports"
    
    return 0
}

# Function to run parameter analysis
run_parameter_analysis() {
    print_phase "3" "Parameter Analysis"
    
    chmod +x tests/test_parameter_analysis.sh
    
    print_info "Starting parameter analysis..."
    start_time=$(date +%s)
    
    if ! ./tests/test_parameter_analysis.sh; then
        print_error "Parameter analysis failed"
        return 1
    fi
    
    end_time=$(date +%s)
    duration=$((end_time - start_time))
    print_success "Parameter analysis completed in ${duration}s"
    
    # Show summary
    local analysis_dir="tests/parameter_analysis/$DATASET"
    if [ -d "$analysis_dir" ]; then
        local report_count=$(find "$analysis_dir" -name "*.txt" | wc -l)
        print_info "Generated $report_count analysis reports"
    fi
    
    return 0
}

# Function to run plot generation
run_plot_generation() {
    print_phase "4" "Plot Generation"
    
    chmod +x tests/test_plots.sh
    
    print_info "Starting plot generation..."
    start_time=$(date +%s)
    
    if ! ./tests/test_plots.sh; then
        print_error "Plot generation failed"
        return 1
    fi
    
    end_time=$(date +%s)
    duration=$((end_time - start_time))
    print_success "Plot generation completed in ${duration}s"
    
    # Show summary
    local plots_dir="tests/plots/$DATASET"
    if [ -d "$plots_dir" ]; then
        local plot_count=$(find "$plots_dir" -name "*.png" | wc -l)
        print_info "Generated $plot_count visualization plots"
        
        # Check for HTML summary
        if [ -f "$plots_dir/plot_summary.html" ]; then
            print_info "HTML summary available: $plots_dir/plot_summary.html"
        fi
    fi
    
    return 0
}

# Function to generate final summary
generate_final_summary() {
    print_phase "5" "Final Summary"
    
    local summary_file="tests/complete_test_summary.txt"
    
    cat > "$summary_file" << EOF
========================================
COMPLETE TEST SUITE SUMMARY
========================================
Test Date: $(date)
Dataset: $DATASET

CONFIGURATION:
- Noise Types: ${NOISE_TYPES[*]}
- Methods: ${METHODS[*]}
- Formats: ${FORMATS[*]}
- Compressors: ${COMPRESSORS[*]}

RESULTS:
EOF

    # Feature extraction results
    if [ -d "tests/feature_extraction_results/$DATASET" ]; then
        local total_features=$(find tests/feature_extraction_results/$DATASET -name "*.feat" -o -name "*.featbin" | wc -l)
        cat >> "$summary_file" << EOF

FEATURE EXTRACTION:
- Total feature files: $total_features
- Output directory: tests/feature_extraction_results/$DATASET
EOF
    fi
    
    # Compression results
    if [ -d "tests/compression_results/$DATASET" ]; then
        local total_results=$(find tests/compression_results/$DATASET -name "*.csv" | wc -l)
        local total_accuracy=$(find tests/compression_results/$DATASET -name "accuracy_metrics_*.json" | wc -l)
        cat >> "$summary_file" << EOF

COMPRESSION TESTS:
- Total result files: $total_results
- Accuracy reports: $total_accuracy
- Output directory: tests/compression_results/$DATASET
EOF
    fi
    
    # Analysis results
    if [ -d "tests/parameter_analysis/$DATASET" ]; then
        local report_count=$(find tests/parameter_analysis/$DATASET -name "*.txt" | wc -l)
        cat >> "$summary_file" << EOF

PARAMETER ANALYSIS:
- Analysis reports: $report_count
- Output directory: tests/parameter_analysis/$DATASET
EOF
    fi
    
    # Plot results
    if [ -d "tests/plots/$DATASET" ]; then
        local plot_count=$(find tests/plots/$DATASET -name "*.png" | wc -l)
        cat >> "$summary_file" << EOF

VISUALIZATIONS:
- Generated plots: $plot_count
- Output directory: tests/plots/$DATASET
EOF
        
        if [ -f "tests/plots/$DATASET/plot_summary.html" ]; then
            cat >> "$summary_file" << EOF
- HTML summary: tests/plots/$DATASET/plot_summary.html
EOF
        fi
    fi
    
    cat >> "$summary_file" << EOF

TEST COMPLETED: $(date)
EOF

    print_success "Complete summary saved to: $summary_file"
    echo
    cat "$summary_file"
}

# Function to cleanup on interrupt
cleanup() {
    print_warning "Test interrupted. Cleaning up..."
    # Kill any background jobs
    jobs -p | xargs -r kill
    exit 1
}

# Set up signal handlers
trap cleanup INT TERM

# Main execution

# Record start time
TOTAL_START_TIME=$(date +%s)

# Run all test phases
check_prerequisites
show_configuration

echo
read -p "Continue with test execution? (y/N): " -n 1 -r
echo
if [[ ! $REPLY =~ ^[Yy]$ ]]; then
    print_info "Test execution cancelled by user"
    exit 0
fi

# Run test phases
if run_feature_extraction; then
    if run_compression_tests; then
        if run_parameter_analysis; then
            if run_plot_generation; then
                # All tests completed successfully
                TOTAL_END_TIME=$(date +%s)
                TOTAL_DURATION=$((TOTAL_END_TIME - TOTAL_START_TIME))
                
                echo
                print_success "ALL TESTS COMPLETED SUCCESSFULLY!"
                print_info "Total execution time: ${TOTAL_DURATION}s ($((TOTAL_DURATION / 60))m $((TOTAL_DURATION % 60))s)"
                
                generate_final_summary
                
                exit 0
            else
                print_error "Plot generation failed - stopping test suite"
                exit 1
            fi
        else
            print_error "Parameter analysis failed - stopping test suite"
            exit 1
        fi
    else
        print_error "Compression tests failed - stopping test suite"
        exit 1
    fi
else
    print_error "Feature extraction failed - stopping test suite"
    exit 1
fi
