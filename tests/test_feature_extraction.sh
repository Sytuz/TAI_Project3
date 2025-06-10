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
DATASET="full_tracks"  # Change to: samples or full_tracks for larger datasets
OUTPUT_BASE_DIR="tests/feature_extraction_results"
TEST_METHODS=("spectral" "maxfreq")  # Both methods
FORMATS=("text" "binary")            # Both formats
NOISE_TYPES=("clean" "white" "brown" "pink")  # Include clean (no noise) plus noise types
NOISE_LEVEL=0.1  # 10% noise level for testing
MAX_THREADS=8  # Maximum number of parallel threads for feature extraction

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
print_step "1" "Pre checks"

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
print_info "Testing noise types: ${NOISE_TYPES[*]}"

# Create output directories
TEST_OUTPUT_DIR="$OUTPUT_BASE_DIR/$DATASET"

print_info "Test output directory: $TEST_OUTPUT_DIR"

# Thread management variables for feature extraction
declare -a fe_running_jobs=()
declare -a fe_job_logs=()

# Function to manage feature extraction thread pool
wait_for_fe_slot() {
    while [ ${#fe_running_jobs[@]} -ge $MAX_THREADS ]; do
        # Check for completed jobs
        local new_running_jobs=()
        local new_job_logs=()
        
        for i in "${!fe_running_jobs[@]}"; do
            local pid=${fe_running_jobs[$i]}
            local log_file=${fe_job_logs[$i]}
            
            if kill -0 "$pid" 2>/dev/null; then
                # Job still running
                new_running_jobs+=("$pid")
                new_job_logs+=("$log_file")
            else
                # Job completed
                wait "$pid"
                local exit_code=$?
                
                # Show job completion
                local job_info=$(basename "$log_file" .log)
                if [ $exit_code -eq 0 ]; then
                    print_success "Feature extraction completed: $job_info"
                else
                    print_error "Feature extraction failed: $job_info (exit code: $exit_code)"
                fi
                
                # Show brief job output
                if [ -f "$log_file" ]; then
                    local last_lines=$(tail -3 "$log_file" | grep -E "(SUCCESS|ERROR|Generated)" || echo "")
                    if [ -n "$last_lines" ]; then
                        echo "$last_lines"
                    fi
                    rm -f "$log_file"
                fi
            fi
        done
        
        fe_running_jobs=("${new_running_jobs[@]}")
        fe_job_logs=("${new_job_logs[@]}")
        
        sleep 0.1
    done
}

# Function to wait for all feature extraction jobs
wait_for_all_fe_jobs() {
    print_info "Waiting for ${#fe_running_jobs[@]} feature extraction jobs to complete..."
    
    while [ ${#fe_running_jobs[@]} -gt 0 ]; do
        local new_running_jobs=()
        local new_job_logs=()
        
        for i in "${!fe_running_jobs[@]}"; do
            local pid=${fe_running_jobs[$i]}
            local log_file=${fe_job_logs[$i]}
            
            if kill -0 "$pid" 2>/dev/null; then
                new_running_jobs+=("$pid")
                new_job_logs+=("$log_file")
            else
                wait "$pid"
                local exit_code=$?
                
                local job_info=$(basename "$log_file" .log)
                if [ $exit_code -eq 0 ]; then
                    print_success "Feature extraction completed: $job_info"
                else
                    print_error "Feature extraction failed: $job_info (exit code: $exit_code)"
                fi
                
                if [ -f "$log_file" ]; then
                    local last_lines=$(tail -3 "$log_file" | grep -E "(SUCCESS|ERROR|Generated)" || echo "")
                    if [ -n "$last_lines" ]; then
                        echo "$last_lines"
                    fi
                    rm -f "$log_file"
                fi
            fi
        done
        
        fe_running_jobs=("${new_running_jobs[@]}")
        fe_job_logs=("${new_job_logs[@]}")
        
        sleep 0.5
    done
    
    print_success "All feature extraction jobs completed!"
}

# Function to generate noisy samples
generate_noisy_samples() {
    local noise_type=$1
    local input_dir=$2
    local output_dir=$3
    
    if [ "$noise_type" = "clean" ]; then
        # For clean samples, just copy the original files
        mkdir -p "$output_dir"
        cp "$input_dir"/*.wav "$output_dir/"
        return 0
    fi
    
    print_info "Generating $noise_type noise samples..."
    mkdir -p "$output_dir"
    
    # Check if sox is available
    if ! command -v sox > /dev/null; then
        print_error "sox is required for noise generation but not found"
        return 1
    fi
    
    for wav_file in "$input_dir"/*.wav; do
        if [ -f "$wav_file" ]; then
            base_name=$(basename "$wav_file" .wav)
            output_file="$output_dir/${base_name}_${noise_type}_noise.wav"
            
            # Generate temporary noise file
            temp_noise=$(mktemp --suffix=.wav)
            
            # Get audio properties
            channels=$(soxi -c "$wav_file" 2>/dev/null || echo "2")
            rate=$(soxi -r "$wav_file" 2>/dev/null || echo "44100")
            duration=$(soxi -D "$wav_file" 2>/dev/null || echo "10")
            
            # Generate noise with same properties
            sox -n -r "$rate" -c "$channels" "$temp_noise" synth "$duration" \
                "$noise_type" vol "$NOISE_LEVEL" 2>/dev/null
            
            # Mix original with noise
            sox -m "$wav_file" "$temp_noise" "$output_file" 2>/dev/null
            
            # Clean up
            rm -f "$temp_noise"
            
            if [ -f "$output_file" ]; then
                print_info "Created noisy sample: $(basename "$output_file")"
            else
                print_warning "Failed to create noisy sample for: $base_name"
            fi
        fi
    done
    
    print_success "Generated $noise_type noise samples"
    return 0
}

# Function to run a single feature extraction job
run_feature_extraction_job() {
    local method=$1
    local format=$2
    local binary_flag=$3
    local output_dir=$4
    local input_dir=$5
    local noise_type=$6
    local log_file=$7
    
    # Redirect output to log file
    exec > "$log_file" 2>&1
    
    echo "Starting feature extraction job: $noise_type/$method/$format"
    echo "Input dir: $input_dir"
    echo "Output dir: $output_dir"
    echo "---"
    
    # Run feature extraction
    if run_feature_extraction "$method" "$format" "$binary_flag" "$output_dir" "$input_dir" "$noise_type"; then
        validate_features "$method" "$format" "$binary_flag" "$output_dir" "$input_dir" "$noise_type"
        echo "Feature extraction job completed successfully"
        exit 0
    else
        echo "Feature extraction job failed"
        exit 1
    fi
}

# Function to run feature extraction and measure performance
run_feature_extraction() {
    local method=$1
    local format=$2
    local binary_flag=$3
    local output_dir=$4
    local input_dir=$5
    local noise_type=$6
    
    
    print_step "2" "Feature extraction - $method method, $format format, $noise_type samples"
    
    mkdir -p "$output_dir"
    
    # Start timing
    start_time=$(date +%s)
    
    # Run feature extraction using default parameters (no config file needed)
    if [[ "$method" == "spectral" ]]; then
        # Default spectral parameters: 32 bins, 1024 frame size, 512 hop size
        cmd="./apps/extract_features --method spectral --bins 32 --frame-size 1024 --hop-size 512 -i $input_dir -o $output_dir"
    else
        # Default maxfreq parameters: 4 frequencies, 1024 frame size, 512 hop size
        cmd="./apps/extract_features --method maxfreq --frequencies 4 --frame-size 1024 --hop-size 512 -i $input_dir -o $output_dir"
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
    local input_dir=$5
    local noise_type=$6
    
    print_step "3" "Validation - $method method, $format format, $noise_type samples"
    
    if [ "$binary_flag" = "true" ]; then
        extension="featbin"
    else
        extension="feat"
    fi
    
    # Check if all input files have corresponding feature files
    missing_count=0
    for wav_file in "$input_dir"/*.wav; do
        if [ -f "$wav_file" ]; then
            base_name=$(basename "$wav_file" .wav)
            # Handle noise suffix in filename
            if [ "$noise_type" != "clean" ]; then
                expected_feat="${output_dir}/${base_name}_${method}.${extension}"
            else
                expected_feat="${output_dir}/${base_name}_${method}.${extension}"
            fi
            
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
    print_step "4" "Comparing text vs binary formats across noise types"
    
    for noise_type in "${NOISE_TYPES[@]}"; do
        print_info "=== Comparing formats for $noise_type samples ==="
        
        for method in "${TEST_METHODS[@]}"; do
            text_dir="$TEST_OUTPUT_DIR/${noise_type}/${method}/text"
            binary_dir="$TEST_OUTPUT_DIR/${noise_type}/${method}/binary"
            
            if [ -d "$text_dir" ] && [ -d "$binary_dir" ]; then
                text_size=$(du -s "$text_dir" | cut -f1)
                binary_size=$(du -s "$binary_dir" | cut -f1)
                
                if [ "$binary_size" -lt "$text_size" ]; then
                    compression_ratio=$(echo "scale=2; $text_size / $binary_size" | bc)
                    print_success "$noise_type/$method: Binary format is ${compression_ratio}x smaller than text format"
                else
                    expansion_ratio=$(echo "scale=2; $binary_size / $text_size" | bc)
                    print_info "$noise_type/$method: Binary format is ${expansion_ratio}x larger than text format"
                fi
                
                text_count=$(find "$text_dir" -name "*.feat" | wc -l)
                binary_count=$(find "$binary_dir" -name "*.featbin" | wc -l)
                
                if [ "$text_count" -eq "$binary_count" ]; then
                    print_success "$noise_type/$method: Same number of files in both formats ($text_count)"
                    print_success "$noise_type/$method: Same number of files in both formats ($text_count)"
                else
                    print_warning "$noise_type/$method: Different number of files: text=$text_count, binary=$binary_count"
                fi
            fi
        done
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
- Noise Types: ${NOISE_TYPES[*]}
- Noise Level: $NOISE_LEVEL
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

    for noise_type in "${NOISE_TYPES[@]}"; do
        cat >> "$report_file" << EOF

=== $noise_type SAMPLES ===
EOF
        
        for method in "${TEST_METHODS[@]}"; do
            cat >> "$report_file" << EOF

$noise_type/$method METHOD:
EOF
            
            text_dir="$TEST_OUTPUT_DIR/${noise_type}/${method}/text"
            if [ -d "$text_dir" ]; then
                text_count=$(find "$text_dir" -name "*.feat" | wc -l)
                text_size=$(du -sh "$text_dir" | cut -f1)
                cat >> "$report_file" << EOF
- Text format (.feat): $text_count files, $text_size total
EOF
            fi

            binary_dir="$TEST_OUTPUT_DIR/${noise_type}/${method}/binary"
            if [ -d "$binary_dir" ]; then
                binary_count=$(find "$binary_dir" -name "*.featbin" | wc -l)
                binary_size=$(du -sh "$binary_dir" | cut -f1)
                cat >> "$report_file" << EOF
- Binary format (.featbin): $binary_count files, $binary_size total
EOF
            fi
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
print_info "Starting feature extraction test for dataset: $DATASET"
print_info "Testing methods: ${TEST_METHODS[*]}"
print_info "Testing formats: text and binary"
print_info "Testing noise types: ${NOISE_TYPES[*]}"
print_info "Using parallel processing with up to $MAX_THREADS threads"

# Create logs directory for job outputs
FE_LOGS_DIR="$TEST_OUTPUT_DIR/fe_logs"
mkdir -p "$FE_LOGS_DIR"

# First, generate all noise samples sequentially (to avoid conflicts)
print_info "=== Generating noise samples ==="
for noise_type in "${NOISE_TYPES[@]}"; do
    if [ "$noise_type" != "clean" ]; then
        CURRENT_INPUT_DIR="$TEST_OUTPUT_DIR/noisy_samples/$noise_type"
        generate_noisy_samples "$noise_type" "$INPUT_DIR" "$CURRENT_INPUT_DIR"
    fi
done

# Now run feature extraction jobs in parallel
total_fe_jobs=0

for noise_type in "${NOISE_TYPES[@]}"; do
    print_info "=== Launching feature extraction jobs for $noise_type samples ==="
    
    # Determine input directory
    if [ "$noise_type" = "clean" ]; then
        CURRENT_INPUT_DIR="$INPUT_DIR"
    else
        CURRENT_INPUT_DIR="$TEST_OUTPUT_DIR/noisy_samples/$noise_type"
    fi
    
    for method in "${TEST_METHODS[@]}"; do
        for format in "${FORMATS[@]}"; do
            # Wait for available slot
            wait_for_fe_slot
            
            # Determine binary flag
            if [ "$format" = "binary" ]; then
                binary_flag="true"
            else
                binary_flag="false"
            fi
            
            output_dir="$TEST_OUTPUT_DIR/${noise_type}/${method}/${format}"
            log_file="$FE_LOGS_DIR/${noise_type}_${method}_${format}.log"
            
            print_info "Launching feature extraction: $noise_type/$method/$format"
            
            # Launch job in background using a simpler approach
            {
                run_feature_extraction_job "$method" "$format" "$binary_flag" "$output_dir" "$CURRENT_INPUT_DIR" "$noise_type" "$log_file"
            } &
            job_pid=$!
            
            # Validate PID before tracking
            if [ -n "$job_pid" ] && [ "$job_pid" != "0" ]; then
                fe_running_jobs+=("$job_pid")
                fe_job_logs+=("$log_file")
            else
                print_error "Failed to launch job for $noise_type/$method/$format"
            fi
            
            ((total_fe_jobs++))
        done
    done
done

print_info "Launched $total_fe_jobs feature extraction jobs"

# Wait for all feature extraction jobs to complete
wait_for_all_fe_jobs

# Clean up logs directory if empty
if [ -d "$FE_LOGS_DIR" ] && [ -z "$(ls -A "$FE_LOGS_DIR")" ]; then
    rmdir "$FE_LOGS_DIR"
fi

# Compare formats across noise types
compare_formats

# Generate summary report
generate_report

print_info "Test files preserved in: $TEST_OUTPUT_DIR"
print_success "Feature extraction test completed successfully!"
print_info "Total feature extraction jobs processed: $total_fe_jobs"
