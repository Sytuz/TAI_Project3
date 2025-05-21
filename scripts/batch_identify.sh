#!/bin/bash

# Default values
query_dir="data/queries"
db_dir="data/features/db"
output_dir="output"
compressor="gzip"

# Display usage information
function show_help() {
    echo "Usage: $0 [OPTIONS]"
    echo "Options:"
    echo "  -q, --query <dir>     Directory with query feature files [default: data/queries]"
    echo "  -d, --db <dir>        Directory with database feature files [default: data/features/db]"
    echo "  -o, --output <dir>    Output directory for results [default: output]"
    echo "  -c, --compressor <c>  Compressor to use (gzip, bzip2, lzma, zstd) [default: gzip]"
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
query_count=0

for query_file in "${query_dir}"/*.feat; do
    if [ ! -f "$query_file" ]; then
        echo "No feature files found in $query_dir"
        exit 1
    fi
    
    query_name=$(basename "$query_file")
    result_file="${output_dir}/${query_name%.feat}_results.csv"
    
    echo "Processing query: $query_name"
    ./apps/music_id --compressor "$compressor" "$query_file" "$db_dir" "$result_file"
    
    ((query_count++))
done

echo "Batch processing complete. Processed $query_count queries."
echo "Results saved to $output_dir"

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