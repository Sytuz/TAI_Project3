#!/bin/bash

set -e  # Exit on any error

echo "========================================="
echo "FULL YOUTUBE MUSIC IDENTIFICATION TEST"
echo "========================================="

# Configuration - Use relative paths from script location
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
YOUTUBE_DIR="data/full_tracks"
DATABASE_DIR="data/generated/youtube_db"
TEST_SAMPLES_DIR="data/test_samples/youtube"
TEST_FEATURES_DIR="data/test_features/youtube"
RESULTS_DIR="results/youtube_full_test"
SAMPLE_LENGTH=5
NUM_TEST_SAMPLES=10

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

print_step() {
    echo -e "${BLUE}[STEP $1]${NC} $2"
}

print_success() {
    echo -e "${GREEN}✓${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}⚠${NC} $1"
}

print_error() {
    echo -e "${RED}✗${NC} $1"
}

# Function to extract features one by one (more memory efficient)
extract_features_individually() {
    local input_dir="$1"
    local output_dir="$2"
    local method="$3"
    local config="$4"
    
    local total_files=$(ls "$input_dir"/*.wav 2>/dev/null | wc -l)
    local current=1
    local failed=0
    
    echo "Processing $total_files files individually..."
    
    for file in "$input_dir"/*.wav; do
        if [ -f "$file" ]; then
            local filename=$(basename "$file" .wav)
            local output_file="$output_dir/${filename}_${method}.feat"
            
            echo "[$current/$total_files] Processing: $(basename "$file")"
            
            # Check if feature file already exists
            if [ -f "$output_file" ]; then
                print_warning "Feature file already exists: $(basename "$output_file")"
                ((current++))
                continue
            fi
            
            # Extract features for this single file
            if timeout 300 ./apps/extract_features "$file" "$output_dir/" "$method" "$config" 2>/dev/null; then
                print_success "Extracted: $(basename "$output_file")"
            else
                print_error "Failed to extract features from: $(basename "$file")"
                ((failed++))
            fi
            
            ((current++))
            
            # Small delay to prevent memory issues
            sleep 1
        fi
    done
    
    echo "Feature extraction completed: $((total_files - failed))/$total_files successful"
    return 0
}

# Change to project directory
echo "Project root detected: $PROJECT_ROOT"
cd "$PROJECT_ROOT"

# Step 1: Prepare directories
print_step "1" "Preparing directories..."
mkdir -p "$DATABASE_DIR"
mkdir -p "$TEST_SAMPLES_DIR"
mkdir -p "$TEST_FEATURES_DIR"
mkdir -p "$RESULTS_DIR"
print_success "Directories created"

# Step 2: Check if YouTube songs exist
print_step "2" "Checking YouTube songs..."
if [ ! -d "$YOUTUBE_DIR" ] || [ -z "$(ls -A $YOUTUBE_DIR/*.wav 2>/dev/null)" ]; then
    print_error "No WAV files found in $YOUTUBE_DIR"
    echo "Please ensure you have YouTube songs in WAV format in $YOUTUBE_DIR"
    exit 1
fi

TOTAL_SONGS=$(ls "$YOUTUBE_DIR"/*.wav | wc -l)
print_success "Found $TOTAL_SONGS YouTube songs"

# Step 3: Extract features from all YouTube songs (DATABASE) - One by one
print_step "3" "Extracting features from all YouTube songs for database..."
echo "Processing files individually to avoid memory issues..."

if [ ! -f "$DATABASE_DIR"/.features_extracted ]; then
    extract_features_individually "$YOUTUBE_DIR" "$DATABASE_DIR" "spectral" "config/feature_extraction_spectral_default.json"
    
    # Check if we have at least some feature files
    FEATURE_COUNT=$(find "$DATABASE_DIR" -name "*.feat" | wc -l)
    if [ "$FEATURE_COUNT" -gt 0 ]; then
        touch "$DATABASE_DIR"/.features_extracted
        print_success "Database features extracted successfully ($FEATURE_COUNT files)"
    else
        print_error "No feature files were created"
        exit 1
    fi
else
    FEATURE_COUNT=$(find "$DATABASE_DIR" -name "*.feat" | wc -l)
    print_warning "Database features already exist ($FEATURE_COUNT files), skipping extraction"
fi

# Step 4: Create random test samples
print_step "4" "Creating random test samples from YouTube songs..."

# Get random songs for testing (up to NUM_TEST_SAMPLES) - with proper quoting
mapfile -t SONG_LIST < <(find "$YOUTUBE_DIR" -name "*.wav" -type f | shuf | head -n "$NUM_TEST_SAMPLES")

echo "Creating $SAMPLE_LENGTH-second random samples from ${#SONG_LIST[@]} songs..."

for song in "${SONG_LIST[@]}"; do
    if [ -f "$song" ]; then
        song_name=$(basename "$song")
        echo "Creating sample from: $song_name"

        # Use proper quoting to handle special characters
        if ./scripts/extract_sample.sh -i "$song" -o "$TEST_SAMPLES_DIR" -l "$SAMPLE_LENGTH"; then
            print_success "Sample created from $song_name"
        else
            print_warning "Failed to create sample from $song_name"
        fi
    else
        print_warning "File not found: $song"
    fi
done
# Step 5: Extract features from test samples (one by one)
print_step "5" "Extracting features from test samples..."

SAMPLE_COUNT=$(ls "$TEST_SAMPLES_DIR"/*.wav 2>/dev/null | wc -l)
if [ "$SAMPLE_COUNT" -eq 0 ]; then
    print_error "No test samples found!"
    exit 1
fi

extract_features_individually "$TEST_SAMPLES_DIR" "$TEST_FEATURES_DIR" "spectral" "config/feature_extraction_spectral_default.json"

TEST_FEATURE_COUNT=$(find "$TEST_FEATURES_DIR" -name "*.feat" | wc -l)
if [ "$TEST_FEATURE_COUNT" -gt 0 ]; then
    print_success "Test sample features extracted ($TEST_FEATURE_COUNT files)"
else
    print_error "Failed to extract test sample features"
    exit 1
fi

# Step 6: Run batch identification
print_step "6" "Running batch music identification..."

./scripts/batch_identify.sh -q "$TEST_FEATURES_DIR/" -d "$DATABASE_DIR/" -o "$RESULTS_DIR/batch_results/" -c gzip

if [ $? -eq 0 ]; then
    print_success "Batch identification completed"
else
    print_error "Batch identification failed"
    exit 1
fi

# Step 7: Calculate accuracy metrics
print_step "7" "Calculating accuracy metrics..."

python3 scripts/calculate_accuracy.py "$RESULTS_DIR/batch_results/" -o "$RESULTS_DIR/accuracy_report.json"

if [ $? -eq 0 ]; then
    print_success "Accuracy metrics calculated"
else
    print_error "Failed to calculate accuracy metrics"
    exit 1
fi

# Step 8: Test different compressors (only if we have time and resources)
print_step "8" "Testing different compressors..."

COMPRESSORS=("bzip2" "lzma")  # Reduced list to avoid too much time

for comp in "${COMPRESSORS[@]}"; do
    echo "Testing with $comp compressor..."
    
    mkdir -p "$RESULTS_DIR/${comp}_results"
    
    ./scripts/batch_identify.sh -q "$TEST_FEATURES_DIR/" -d "$DATABASE_DIR/" -o "$RESULTS_DIR/${comp}_results/" -c "$comp"
    
    if [ $? -eq 0 ]; then
        python3 scripts/calculate_accuracy.py "$RESULTS_DIR/${comp}_results/" -o "$RESULTS_DIR/${comp}_accuracy.json"
        print_success "$comp compressor test completed"
    else
        print_warning "$comp compressor test failed"
    fi
done

# # Step 9: Test individual samples directly with WAV support (limited to 3 samples)
# print_step "9" "Testing direct WAV identification (new feature)..."

# mkdir -p "$RESULTS_DIR/individual_tests"

# counter=1
# max_individual_tests=3
# for sample in "$TEST_SAMPLES_DIR"/*.wav; do
#     if [ -f "$sample" ] && [ $counter -le $max_individual_tests ]; then
#         sample_name=$(basename "$sample" .wav)
#         echo "Testing individual sample: $sample_name"
        
#         ./apps/music_id "$sample" "$DATABASE_DIR/" --compressor gzip --top 10 > "$RESULTS_DIR/individual_tests/${sample_name}_result.txt"
        
#         if [ $? -eq 0 ]; then
#             print_success "Individual test $counter completed"
#         else
#             print_warning "Individual test $counter failed"
#         fi
        
#         ((counter++))
#     fi
# done

# # Step 10: Generate summary report
# print_step "10" "Generating summary report..."

# SUMMARY_FILE="$RESULTS_DIR/test_summary.txt"

# cat > "$SUMMARY_FILE" << EOF
# ========================================
# YOUTUBE MUSIC IDENTIFICATION TEST SUMMARY
# ========================================
# Test Date: $(date)
# Project Root: $PROJECT_ROOT

# DATABASE INFORMATION:
# - Total songs in database: $TOTAL_SONGS
# - Feature files created: $(find "$DATABASE_DIR" -name "*.feat" 2>/dev/null | wc -l)
# - Database directory: $DATABASE_DIR
# - Feature extraction method: spectral

# TEST SAMPLES:
# - Number of test samples: $(ls "$TEST_SAMPLES_DIR"/*.wav 2>/dev/null | wc -l)
# - Test feature files: $(find "$TEST_FEATURES_DIR" -name "*.feat" 2>/dev/null | wc -l)
# - Sample length: ${SAMPLE_LENGTH} seconds
# - Test samples directory: $TEST_SAMPLES_DIR

# TESTS PERFORMED:
# - Individual feature extraction (memory efficient)
# - Batch identification with multiple compressors
# - Individual WAV file identification (limited)
# - Accuracy metric calculations

# COMPRESSORS TESTED:
# - gzip
# $(for comp in "${COMPRESSORS[@]}"; do echo "- $comp"; done)

# RESULTS LOCATION:
# - Main results directory: $RESULTS_DIR
# - Batch results: $RESULTS_DIR/batch_results/
# - Individual tests: $RESULTS_DIR/individual_tests/
# - Accuracy reports: $RESULTS_DIR/*_accuracy.json

# FILES CREATED:
# $(find "$RESULTS_DIR" -type f 2>/dev/null | wc -l) result files

# To view accuracy results:
# cat $RESULTS_DIR/accuracy_report.json

# To compare compressor performance:
# cat $RESULTS_DIR/gzip_accuracy.json
# $(for comp in "${COMPRESSORS[@]}"; do echo "cat $RESULTS_DIR/${comp}_accuracy.json"; done)
# EOF

# print_success "Summary report generated: $SUMMARY_FILE"

# # Step 11: Quick analysis
# print_step "11" "Quick Results Analysis..."

# echo ""
# echo "=== QUICK RESULTS PREVIEW ==="

# if [ -f "$RESULTS_DIR/accuracy_report.json" ]; then
#     echo ""
#     echo "Main Accuracy Results (gzip):"
#     python3 -c "
# import json
# try:
#     with open('$RESULTS_DIR/accuracy_report.json', 'r') as f:
#         data = json.load(f)
#     print(f\"Top-1 Accuracy: {data.get('top1_accuracy', 'N/A')}%\")
#     print(f\"Top-5 Accuracy: {data.get('top5_accuracy', 'N/A')}%\")
#     print(f\"Top-10 Accuracy: {data.get('top10_accuracy', 'N/A')}%\")
#     print(f\"Total Queries: {data.get('total_queries', 'N/A')}\")
# except:
#     print('Error reading accuracy report')
# "
# fi

# echo ""
# echo "=== DATABASE STATISTICS ==="
# echo "Total songs processed: $TOTAL_SONGS"
# echo "Feature files created: $(find "$DATABASE_DIR" -name "*.feat" 2>/dev/null | wc -l)"
# echo "Test samples created: $(ls "$TEST_SAMPLES_DIR"/*.wav 2>/dev/null | wc -l)"
# echo "Test features extracted: $(find "$TEST_FEATURES_DIR" -name "*.feat" 2>/dev/null | wc -l)"

# echo ""
# echo "========================================="
# echo -e "${GREEN}🎉 YOUTUBE TEST COMPLETED! 🎉${NC}"
# echo "========================================="
# echo ""
# echo "Results summary:"
# echo "1. Database: $TOTAL_SONGS songs → $(find "$DATABASE_DIR" -name "*.feat" 2>/dev/null | wc -l) feature files"
# echo "2. Test samples: $(ls "$TEST_SAMPLES_DIR"/*.wav 2>/dev/null | wc -l) samples → $(find "$TEST_FEATURES_DIR" -name "*.feat" 2>/dev/null | wc -l) features"
# echo "3. All results saved in: $RESULTS_DIR"
# echo ""
# echo "To view detailed results:"
# echo "cat $SUMMARY_FILE"
# echo ""