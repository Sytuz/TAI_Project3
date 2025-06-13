#!/bin/bash

# Comprehensive Music Identification Testing Script
# This script automates testing of all combinations of methods, formats, noises, and compressors

# set -e  # Exit on any error

# Configuration
DATASET_NAME="supersmall"
METHODS=("maxfreq" "spectral")
FORMATS=("text" "binary")
NOISES=("clean" "brown" "pink" "white")
COMPRESSORS=("gzip" "bzip2" "lzma" "zstd")

# Base directories
PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
DATA_DIR="data"
FULL_TRACKS_DIR="${DATA_DIR}/full_tracks/${DATASET_NAME}"
FEATURES_DIR="${DATA_DIR}/features"
SAMPLES_DIR="${DATA_DIR}/samples"
QUERIES_DIR="${FEATURES_DIR}/queries"
RESULTS_DIR="results"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Logging
LOG_FILE="automation_$(date +%Y%m%d_%H%M%S).log"

# Helper functions
log() {
    echo -e "$1" | tee -a "$LOG_FILE"
}

print_step() {
    log "${BLUE}[STEP $1]${NC} $2"
}

print_success() {
    log "${GREEN}✓${NC} $1"
}

print_warning() {
    log "${YELLOW}⚠${NC} $1"
}

print_error() {
    log "${RED}✗${NC} $1"
}

# Check if directory exists and create if needed
ensure_directory() {
    local dir="$1"
    if [ ! -d "$dir" ]; then
        mkdir -p "$dir"
        log "Created directory: $dir"
    fi
}

# Check prerequisites
check_prerequisites() {
    print_step "0" "Checking prerequisites..."
    
    # Check if dataset exists
    if [ ! -d "$FULL_TRACKS_DIR" ]; then
        print_error "Dataset directory not found: $FULL_TRACKS_DIR"
        exit 1
    fi
    
    # Check if applications exist
    for app in extract_features music_id; do
        if [ ! -f "./apps/$app" ]; then
            print_error "Application not found: ./apps/$app"
            exit 1
        fi
    done
    
    # Check if scripts exist
    for script in extract_sample.sh compare_compressors.sh; do
        if [ ! -f "./scripts/$script" ]; then
            print_error "Script not found: ./scripts/$script"
            exit 1
        fi
    done
    
    print_success "Prerequisites check passed"
}

# Extract features from database
extract_db_features() {
    local method="$1"
    local format="$2"
    
    print_step "1" "Extracting database features - Method: $method, Format: $format"
    
    local output_dir="${FEATURES_DIR}/db/${DATASET_NAME}/${method}/${format}"
    ensure_directory "$output_dir"
    
    local binary_flag=""
    if [ "$format" = "binary" ]; then
        binary_flag="--binary"
    fi
    
    # Check if features already exist
    if [ -f "$output_dir/.extraction_complete" ]; then
        print_warning "Database features already exist for $method/$format, skipping"
        return 0
    fi
    
    log "Command: ./apps/extract_features --method $method $binary_flag -i $FULL_TRACKS_DIR -o $output_dir"
    
    if ./apps/extract_features --method "$method" $binary_flag -i "$FULL_TRACKS_DIR" -o "$output_dir"; then
        touch "$output_dir/.extraction_complete"
        print_success "Database features extracted: $method/$format"
    else
        print_error "Failed to extract database features: $method/$format"
        return 1
    fi
}

# Extract samples with different noise types
extract_samples() {
    local method="$1"
    local format="$2"
    local noise="$3"
    
    print_step "2" "Extracting samples - Method: $method, Format: $format, Noise: $noise"
    
    local output_dir="${SAMPLES_DIR}/${DATASET_NAME}/${method}/${format}/${noise}"
    ensure_directory "$output_dir"
    
    # Check if samples already exist
    if [ -f "$output_dir/.extraction_complete" ]; then
        print_warning "Samples already exist for $method/$format/$noise, skipping"
        return 0
    fi
    
    local noise_flags=""
    if [ "$noise" != "clean" ]; then
        noise_flags="-n $noise -m 0.1"  # 10% noise level
    fi
    
    log "Command: ./scripts/extract_sample.sh -i $FULL_TRACKS_DIR -o $output_dir $noise_flags"
    
    if ./scripts/extract_sample.sh -i "$FULL_TRACKS_DIR" -o "$output_dir" $noise_flags; then
        touch "$output_dir/.extraction_complete"
        print_success "Samples extracted: $method/$format/$noise"
    else
        print_error "Failed to extract samples: $method/$format/$noise"
        return 1
    fi
}

# Extract features from query samples
extract_query_features() {
    local method="$1"
    local format="$2"
    local noise="$3"
    
    print_step "3" "Extracting query features - Method: $method, Format: $format, Noise: $noise"
    
    local input_dir="${SAMPLES_DIR}/${DATASET_NAME}/${method}/${format}/${noise}"
    local output_dir="${QUERIES_DIR}/${DATASET_NAME}/${method}/${format}/${noise}"
    ensure_directory "$output_dir"
    
    # Check if input samples exist
    if [ ! -d "$input_dir" ] || [ -z "$(ls -A $input_dir/*.wav 2>/dev/null)" ]; then
        print_error "No samples found in: $input_dir"
        return 1
    fi
    
    # Check if query features already exist
    if [ -f "$output_dir/.extraction_complete" ]; then
        print_warning "Query features already exist for $method/$format/$noise, skipping"
        return 0
    fi
    
    local binary_flag=""
    if [ "$format" = "binary" ]; then
        binary_flag="--binary"
    fi
    
    log "Command: ./apps/extract_features --method $method $binary_flag -i $input_dir -o $output_dir"
    
    if ./apps/extract_features --method "$method" $binary_flag -i "$input_dir" -o "$output_dir"; then
        touch "$output_dir/.extraction_complete"
        print_success "Query features extracted: $method/$format/$noise"
    else
        print_error "Failed to extract query features: $method/$format/$noise"
        return 1
    fi
}

# Run compressor comparison
run_compressor_comparison() {
    local method="$1"
    local format="$2"
    local noise="$3"
    
    print_step "4" "Running compressor comparison - Method: $method, Format: $format, Noise: $noise"
    
    local query_dir="${QUERIES_DIR}/${DATASET_NAME}/${method}/${format}/${noise}"
    local db_dir="${FEATURES_DIR}/db/${DATASET_NAME}/${method}/${format}"
    local output_dir="${RESULTS_DIR}/compressors/${DATASET_NAME}/${method}/${format}/${noise}"
    ensure_directory "$output_dir"
    
    # Check if directories exist
    if [ ! -d "$query_dir" ] || [ ! -d "$db_dir" ]; then
        print_error "Required directories not found: $query_dir or $db_dir"
        return 1
    fi
    
    # Check if comparison already completed
    if [ -f "$output_dir/.comparison_complete" ]; then
        print_warning "Compressor comparison already completed for $method/$format/$noise, skipping"
        return 0
    fi
    
    local binary_flag=""
    if [ "$format" = "binary" ]; then
        binary_flag="--binary"
    fi
    
    # Create compressor list
    local compressor_list=$(IFS=,; echo "${COMPRESSORS[*]}")
    
    log "Command: ./scripts/compare_compressors.sh -q $query_dir -d $db_dir -o $output_dir -c $compressor_list -p $binary_flag"
    
    if ./scripts/compare_compressors.sh -q "$query_dir" -d "$db_dir" -o "$output_dir" -c "$compressor_list" -p $binary_flag; then
        touch "$output_dir/.comparison_complete"
        print_success "Compressor comparison completed: $method/$format/$noise"
    else
        print_error "Failed to run compressor comparison: $method/$format/$noise"
        return 1
    fi
}

# Generate summary report
generate_summary() {
    print_step "5" "Generating summary report..."
    
    local summary_file="${RESULTS_DIR}/automation_summary_$(date +%Y%m%d_%H%M%S).md"
    
    cat > "$summary_file" << EOF
# Automated Music Identification Testing Summary

**Generated:** $(date)  
**Dataset:** $DATASET_NAME  
**Project Root:** $PROJECT_ROOT  

## Configuration

- **Methods:** ${METHODS[*]}
- **Formats:** ${FORMATS[*]}
- **Noise Types:** ${NOISES[*]}
- **Compressors:** ${COMPRESSORS[*]}

## Test Combinations

Total combinations tested: $((${#METHODS[@]} * ${#FORMATS[@]} * ${#NOISES[@]}))

## Results Structure

```
results/compressors/${DATASET_NAME}/
├── maxfreq/
│   ├── text/
│   │   ├── clean/
│   │   ├── brown/
│   │   ├── pink/
│   │   └── white/
│   └── binary/
│       ├── clean/
│       ├── brown/
│       ├── pink/
│       └── white/
└── spectral/
    ├── text/
    │   ├── clean/
    │   ├── brown/
    │   ├── pink/
    │   └── white/
    └── binary/
        ├── clean/
        ├── brown/
        ├── pink/
        └── white/
```

## Log File

Complete execution log: $LOG_FILE

EOF

    print_success "Summary report generated: $summary_file"
}

# Main execution function
main() {
    log "Starting automated music identification testing at $(date)"
    log "Dataset: $DATASET_NAME"
    log "Total combinations: $((${#METHODS[@]} * ${#FORMATS[@]} * ${#NOISES[@]}))"
    
    # Change to project directory
    cd "$PROJECT_ROOT"
    
    # Check prerequisites
    check_prerequisites
    
    local total_combinations=0
    local completed_combinations=0
    local failed_combinations=0
    
    # Calculate total combinations
    total_combinations=$((${#METHODS[@]} * ${#FORMATS[@]} * ${#NOISES[@]}))
    
    log "\n=========================================="
    log "Starting main testing loop"
    log "Total combinations to process: $total_combinations"
    log "=========================================="
    
    # Main testing loop
    for method in "${METHODS[@]}"; do
        log "\n>>> Starting method: $method"
        for format in "${FORMATS[@]}"; do
            log "\n>> Starting format: $format for method: $method"
            
            # Step 1: Extract database features (once per method/format combination)
            if ! extract_db_features "$method" "$format"; then
                print_error "Failed to extract database features for $method/$format"
                # Don't continue with this method/format combination, but continue with others
                for noise in "${NOISES[@]}"; do
                    ((failed_combinations++))
                done
                continue
            fi
            
            for noise in "${NOISES[@]}"; do
                local combination="$method/$format/$noise"
                log "\n==========================================\nProcessing combination: $combination\n=========================================="
                
                local combination_failed=false
                
                # Step 2: Extract samples
                log "> Calling extract_samples for $combination"
                if ! extract_samples "$method" "$format" "$noise"; then
                    print_error "Failed in step 2 for $combination"
                    combination_failed=true
                fi
                
                # Step 3: Extract query features
                log "> Calling extract_query_features for $combination"
                if [ "$combination_failed" = false ] && ! extract_query_features "$method" "$format" "$noise"; then
                    print_error "Failed in step 3 for $combination"
                    combination_failed=true
                fi
                
                # Step 4: Run compressor comparison
                log "> Calling run_compressor_comparison for $combination"
                if [ "$combination_failed" = false ] && ! run_compressor_comparison "$method" "$format" "$noise"; then
                    print_error "Failed in step 4 for $combination"
                    combination_failed=true
                fi
                
                if [ "$combination_failed" = true ]; then
                    ((failed_combinations++))
                    print_error "Combination failed: $combination"
                else
                    ((completed_combinations++))
                    print_success "Combination completed: $combination"
                fi
                
                log "Progress: $((completed_combinations + failed_combinations))/$total_combinations combinations processed"
                log "> Finished processing $combination, continuing to next..."
            done
            log "<< Finished format: $format for method: $method"
        done
        log "<<< Finished method: $method"
    done
    
    # Generate summary
    generate_summary
    
    # Final report
    log "\n=========================================="
    log "AUTOMATION COMPLETE"
    log "=========================================="
    log "Total combinations: $total_combinations"
    log "Completed successfully: $completed_combinations"
    log "Failed: $failed_combinations"
    if [ $total_combinations -gt 0 ]; then
        log "Success rate: $(( (completed_combinations * 100) / total_combinations ))%"
    else
        log "Success rate: 0%"
    fi
    log "Log file: $LOG_FILE"
    log "End time: $(date)"
    
    if [ $failed_combinations -eq 0 ]; then
        print_success "All combinations completed successfully!"
        exit 0
    else
        print_warning "Some combinations failed. Check the log for details."
        exit 1
    fi
}

# Script help
show_help() {
    echo "Automated Music Identification Testing Script"
    echo ""
    echo "This script runs comprehensive testing of all combinations of:"
    echo "- Methods: ${METHODS[*]}"
    echo "- Formats: ${FORMATS[*]}"
    echo "- Noise types: ${NOISES[*]}"
    echo "- Compressors: ${COMPRESSORS[*]}"
    echo ""
    echo "Usage: $0 [OPTIONS]"
    echo "Options:"
    echo "  -h, --help    Show this help message"
    echo "  --dry-run     Show what would be executed without running"
    echo ""
    echo "The script will create a complete directory structure and run all tests automatically."
}

# Dry run function
dry_run() {
    echo "DRY RUN - Commands that would be executed:"
    echo ""
    
    for method in "${METHODS[@]}"; do
        for format in "${FORMATS[@]}"; do
            local binary_flag=""
            if [ "$format" = "binary" ]; then
                binary_flag="--binary"
            fi
            
            echo "# Database features for $method/$format"
            echo "./apps/extract_features --method $method $binary_flag -i $FULL_TRACKS_DIR -o ${FEATURES_DIR}/db/${DATASET_NAME}/${method}/${format}"
            echo ""
            
            for noise in "${NOISES[@]}"; do
                local noise_flags=""
                if [ "$noise" != "clean" ]; then
                    noise_flags="-n $noise -m 0.1"
                fi
                
                echo "# Samples for $method/$format/$noise"
                echo "./scripts/extract_sample.sh -i $FULL_TRACKS_DIR -o ${SAMPLES_DIR}/${DATASET_NAME}/${method}/${format}/${noise} $noise_flags"
                echo ""
                
                echo "# Query features for $method/$format/$noise"
                echo "./apps/extract_features --method $method $binary_flag -i ${SAMPLES_DIR}/${DATASET_NAME}/${method}/${format}/${noise} -o ${QUERIES_DIR}/${DATASET_NAME}/${method}/${format}/${noise}"
                echo ""
                
                local compressor_list=$(IFS=,; echo "${COMPRESSORS[*]}")
                echo "# Compressor comparison for $method/$format/$noise"
                echo "./scripts/compare_compressors.sh -q ${QUERIES_DIR}/${DATASET_NAME}/${method}/${format}/${noise} -d ${FEATURES_DIR}/db/${DATASET_NAME}/${method}/${format} -o ${RESULTS_DIR}/compressors/${DATASET_NAME}/${method}/${format}/${noise} -c $compressor_list -p $binary_flag"
                echo ""
            done
        done
    done
}

# Parse command line arguments
case "${1:-}" in
    -h|--help)
        show_help
        exit 0
        ;;
    --dry-run)
        dry_run
        exit 0
        ;;
    "")
        main
        ;;
    *)
        echo "Unknown option: $1"
        show_help
        exit 1
        ;;
esac
