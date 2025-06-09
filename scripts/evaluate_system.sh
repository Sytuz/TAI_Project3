#!/bin/bash

# Evaluation script for music identification system
# This script runs batch identification with different compressors and calculates metrics

# Default values
query_dir="data/test_samples"
db_dir="data/features/db"
output_base_dir="evaluation_results"
compressors=("gzip" "bzip2" "lzma" "zstd")
config_file="config/feature_extraction_spectral_default.json"
use_binary=false

# Display usage information
function show_help() {
    echo "Usage: $0 [OPTIONS]"
    echo "Options:"
    echo "  -q, --query <dir>     Directory with query files (WAV or .feat) [default: data/test_samples]"
    echo "  -d, --db <dir>        Directory with database feature files [default: data/features/db]"
    echo "  -o, --output <dir>    Base output directory [default: evaluation_results]"
    echo "  -c, --compressors <list>  Comma-separated list of compressors [default: gzip,bzip2,lzma,zstd]"
    echo "  --config <file>       Config file for feature extraction [default: config/feature_extraction_spectral_default.json]"
    echo "  --binary              Use binary feature files (.featbin) instead of text (.feat)"
    echo "  -h, --help            Show this help message"
    echo
    echo "This script performs evaluation across multiple compressors."
    echo "It automatically extracts features from WAV files if needed and calculates accuracy metrics."
}

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        -q|--query)
            query_dir="$2"
            shift 2
            ;;
        -d|--db)
            db_dir="$2"
            shift 2
            ;;
        -o|--output)
            output_base_dir="$2"
            shift 2
            ;;
        -c|--compressors)
            IFS=',' read -ra compressors <<< "$2"
            shift 2
            ;;
        --config)
            config_file="$2"
            shift 2
            ;;
        --binary)
            use_binary=true
            shift
            ;;
        -h|--help)
            show_help
            exit 0
            ;;
        *)
            echo "Error: Unknown option $1"
            show_help
            exit 1
            ;;
    esac
done

# Check if directories exist
if [ ! -d "$query_dir" ]; then
    echo "Error: Query directory does not exist: $query_dir"
    exit 1
fi

if [ ! -d "$db_dir" ]; then
    echo "Error: Database directory does not exist: $db_dir"
    exit 1
fi

# Create base output directory
mkdir -p "$output_base_dir"

# Check if we need to extract features from WAV files
query_feat_dir="$query_dir"
temp_feat_dir=""

# Check if query directory contains WAV files
wav_count=$(find "$query_dir" -name "*.wav" | wc -l)
if [ "$use_binary" = true ]; then
    feat_count=$(find "$query_dir" -name "*.featbin" | wc -l)
    feat_extension="featbin"
else
    feat_count=$(find "$query_dir" -name "*.feat" | wc -l)
    feat_extension="feat"
fi

if [ $wav_count -gt 0 ] && [ $feat_count -eq 0 ]; then
    echo "Found $wav_count WAV files in query directory. Extracting features..."
    temp_feat_dir="${output_base_dir}/temp_query_features"
    mkdir -p "$temp_feat_dir"
    
    # Extract features from WAV files
    extract_cmd="./apps/extract_features --config $config_file -i $query_dir -o $temp_feat_dir"
    if [ "$use_binary" = true ]; then
        extract_cmd="$extract_cmd --binary"
    fi
    
    if ! eval "$extract_cmd"; then
        echo "Error: Failed to extract features from WAV files"
        exit 1
    fi
    
    query_feat_dir="$temp_feat_dir"
    echo "Features extracted to: $temp_feat_dir"
elif [ $feat_count -gt 0 ]; then
    echo "Found $feat_count .$feat_extension files in query directory. Using existing features."
else
    echo "Error: No WAV or .$feat_extension files found in query directory: $query_dir"
    exit 1
fi

echo "Starting evaluation..."
echo "Query directory: $query_feat_dir"
echo "Database directory: $db_dir"
echo "Compressors: ${compressors[*]}"
echo ""

# Run evaluation for each compressor
for compressor in "${compressors[@]}"; do
    echo "=========================================="
    echo "Evaluating with compressor: $compressor"
    echo "=========================================="
    
    # Create output directory for this compressor
    comp_output_dir="${output_base_dir}/${compressor}"
    mkdir -p "$comp_output_dir"
    
    # Run batch identification
    echo "Running batch identification..."
    batch_cmd="bash scripts/batch_identify.sh -q $query_feat_dir -d $db_dir -o $comp_output_dir -c $compressor"
    if [ "$use_binary" = true ]; then
        batch_cmd="$batch_cmd --binary"
    fi
    
    if ! eval "$batch_cmd"; then
        echo "Error: Batch identification failed for $compressor"
        continue
    fi
    
    echo "Compressor $compressor evaluation complete."
    echo ""
done

# Generate comparative summary
echo "=========================================="
echo "Generating comparative summary..."
echo "=========================================="

summary_file="${output_base_dir}/comparative_summary.txt"
{
    echo "Music Identification Evaluation Summary"
    echo "======================================"
    echo "Date: $(date '+%Y-%m-%d %H:%M:%S')"
    echo "Query directory: $query_dir"
    echo "Database directory: $db_dir"
    echo "Compressors evaluated: ${compressors[*]}"
    echo ""
    
    # Extract metrics from each compressor's results
    echo "Accuracy Comparison:"
    echo "-------------------"
    printf "%-12s %-10s %-10s %-10s\n" "Compressor" "Top-1" "Top-5" "Top-10"
    printf "%-12s %-10s %-10s %-10s\n" "----------" "-----" "-----" "------"
    
    for compressor in "${compressors[@]}"; do
        accuracy_file="${output_base_dir}/${compressor}/accuracy_metrics_${compressor}.json"
        if [ -f "$accuracy_file" ]; then
            # Extract accuracy values using python if available
            if command -v python3 &> /dev/null; then
                top1=$(python3 -c "import json; data=json.load(open('$accuracy_file')); print(f'{data[\"top1_accuracy\"]:.1f}%')" 2>/dev/null || echo "N/A")
                top5=$(python3 -c "import json; data=json.load(open('$accuracy_file')); print(f'{data[\"top5_accuracy\"]:.1f}%')" 2>/dev/null || echo "N/A")
                top10=$(python3 -c "import json; data=json.load(open('$accuracy_file')); print(f'{data[\"top10_accuracy\"]:.1f}%')" 2>/dev/null || echo "N/A")
                printf "%-12s %-10s %-10s %-10s\n" "$compressor" "$top1" "$top5" "$top10"
            else
                printf "%-12s %-10s %-10s %-10s\n" "$compressor" "N/A" "N/A" "N/A"
            fi
        else
            printf "%-12s %-10s %-10s %-10s\n" "$compressor" "ERROR" "ERROR" "ERROR"
        fi
    done
    
    echo ""
    echo "Individual Results:"
    echo "------------------"
    for compressor in "${compressors[@]}"; do
        echo "Results for $compressor: ${output_base_dir}/${compressor}/"
    done
    
} > "$summary_file"

echo "Comparative summary saved to: $summary_file"
cat "$summary_file"

# Clean up temporary feature directory if created
if [ -n "$temp_feat_dir" ] && [ -d "$temp_feat_dir" ]; then
    echo ""
    echo "Cleaning up temporary feature files..."
    rm -rf "$temp_feat_dir"
fi

echo ""
echo  evaluation complete!"
echo "Results saved to: $output_base_dir"
