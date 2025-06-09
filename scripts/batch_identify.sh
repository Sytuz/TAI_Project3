#!/bin/bash

# Default values
query_dir="data/queries"
db_dir="data/features/db"
output_dir="output"
compressor="gzip"
use_binary=false

# Display usage information
function show_help() {
    echo "Usage: $0 [OPTIONS]"
    echo "Options:"
    echo "  -q, --query <dir>     Directory with query feature files [default: data/queries]"
    echo "  -d, --db <dir>        Directory with database feature files [default: data/features/db]"
    echo "  -o, --output <dir>    Output directory for results [default: output]"
    echo "  -c, --compressor <c>  Compressor to use (gzip, bzip2, lzma, zstd) [default: gzip]"
    echo "  --binary              Use binary feature files (.featbin) instead of text (.feat)"
    echo "  -h, --help            Show this help message"
    echo
    echo "Processes a batch of queries against the database for music identification."
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
            output_dir="$2"
            shift 2
            ;;
        -c|--compressor)
            compressor="$2"
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

# Create output directory
mkdir -p "$output_dir"

# Look for query config file if it exists
if [ -f "${query_dir}/extraction_config.txt" ]; then
    echo "Query configuration found. Copying to output directory..."
    cp "${query_dir}/extraction_config.txt" "${output_dir}/"
fi

# Process each query file
echo "Processing queries using ${compressor} compressor..."
if [ "$use_binary" = true ]; then
    echo "Using binary feature format (.featbin files)"
    file_extension="*.featbin"
else
    echo "Using text feature format (.feat files)"
    file_extension="*.feat"
fi

query_count=0

for query_file in "${query_dir}"/${file_extension}; do
    if [ ! -f "$query_file" ]; then
        if [ "$use_binary" = true ]; then
            echo "No .featbin files found in $query_dir"
        else
            echo "No .feat files found in $query_dir"
        fi
        exit 1
    fi
    
    query_name=$(basename "$query_file")
    if [ "$use_binary" = true ]; then
        result_file="${output_dir}/${query_name%.featbin}_results.csv"
    else
        result_file="${output_dir}/${query_name%.feat}_results.csv"
    fi
    
    echo "Processing query: $query_name"
    if [ "$use_binary" = true ]; then
        ./apps/music_id --compressor "$compressor" --binary "$query_file" "$db_dir" "$result_file"
    else
        ./apps/music_id --compressor "$compressor" "$query_file" "$db_dir" "$result_file"
    fi
    
    ((query_count++))
done

echo "Batch processing complete. Processed $query_count queries."
echo "Results saved to $output_dir"

# Calculate accuracy metrics
echo ""
echo "Calculating accuracy metrics..."
if command -v python3 &> /dev/null; then
    accuracy_output="${output_dir}/accuracy_metrics_${compressor}.json"
    if python3 scripts/calculate_accuracy.py "$output_dir" -o "$accuracy_output"; then
        echo "Accuracy metrics calculated and saved to $accuracy_output"
    else
        echo "Warning: Failed to calculate accuracy metrics"
    fi
else
    echo "Warning: python3 not available, skipping accuracy calculation"
    echo "To calculate accuracy manually, run:"
    echo "python3 scripts/calculate_accuracy.py $output_dir"
fi

# Generate a summary file
summary_file="${output_dir}/summary_${compressor}.txt"
{
    echo "Music Identification Summary"
    echo "=========================="
    echo "Date: $(date '+%Y-%m-%d %H:%M:%S')"
    echo "Compressor: $compressor"
    echo "Queries processed: $query_count"
    echo "Database directory: $db_dir"
    echo ""
    echo "Query Results:"
    echo "-------------"
    
    for result_file in "${output_dir}"/*_results.csv; do
        if [ -f "$result_file" ]; then
            echo "$(basename "$result_file"):"
            # Extract and show the top match for each query
            query_name=$(head -n 1 "$result_file" | cut -d' ' -f2-)
            top_match=$(grep -v "^Query\|^Compressor\|^$\|^Rank" "$result_file" | head -n 1)
            echo "  Query: $query_name"
            echo "  Top match: $top_match"
            echo ""
        fi
    done
} > "$summary_file"

echo "Summary saved to $summary_file"