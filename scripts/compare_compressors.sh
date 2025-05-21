#!/bin/bash

# Default values
query_dir="data/queries"
db_dir="data/features/db"
output_dir="results"
compressors=("gzip" "bzip2" "lzma" "zstd")
parallel=false

# Display usage information
function show_help() {
    echo "Usage: $0 [OPTIONS]"
    echo "Options:"
    echo "  -q, --query <dir>           Directory with query feature files [default: data/queries]"
    echo "  -d, --db <dir>              Directory with database feature files [default: data/features/db]"
    echo "  -o, --output <dir>          Base output directory for results [default: results]"
    echo "  -c, --compressors <list>    Comma-separated list of compressors [default: gzip,bzip2,lzma,zstd]"
    echo "  -p, --parallel              Run compressor tests in parallel [default: false]"
    echo "  -h, --help                  Show this help message"
    echo
    echo "Compares multiple compressors for music identification."
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
        -c|--compressors)
            IFS=',' read -r -a compressors <<< "$2"
            shift 2
            ;;
        -p|--parallel)
            parallel=true
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

echo "Starting compressor comparison with the following settings:"
echo "Query directory: $query_dir"
echo "Database directory: $db_dir" 
echo "Base output directory: $output_dir"
echo "Compressors: ${compressors[*]}"
echo "Parallel execution: $parallel"
echo

# Track process IDs for parallel execution
pids=()

for comp in "${compressors[@]}"; do
    comp_output_dir="${output_dir}_${comp}"
    
    echo "========================================"
    echo "Testing compressor: $comp"
    echo "Output directory: $comp_output_dir"
    echo "========================================"
    
    if [ "$parallel" = true ]; then
        # Run in background for parallel execution
        ./scripts/batch_identify.sh -q "$query_dir" -d "$db_dir" -o "$comp_output_dir" -c "$comp" &
        pids+=($!)
        echo "Started process $! for $comp"
    else
        # Run sequentially
        ./scripts/batch_identify.sh -q "$query_dir" -d "$db_dir" -o "$comp_output_dir" -c "$comp"
    fi
    
    echo ""
done

# If running in parallel, wait for all processes to complete
if [ "$parallel" = true ]; then
    echo "Waiting for all compressor tests to complete..."
    
    # Wait for all background processes to finish
    for pid in "${pids[@]}"; do
        wait $pid
        echo "Process $pid completed"
    done
fi

echo "All compressor tests complete. Results are in:"
for comp in "${compressors[@]}"; do
    echo "- ${output_dir}_${comp}/"
done

# Create a combined summary if all tests are complete
summary_file="${output_dir}_comparison_summary.txt"
{
    echo "Music Identification Compressor Comparison"
    echo "========================================"
    echo "Date: $(date '+%Y-%m-%d %H:%M:%S')"
    echo "Query directory: $query_dir"
    echo "Database directory: $db_dir"
    echo "Compressors tested: ${compressors[*]}"
    echo
    echo "Summary of Results by Compressor:"
    echo "--------------------------------"
    
    for comp in "${compressors[@]}"; do
        comp_output_dir="${output_dir}_${comp}"
        summary_path="${comp_output_dir}/summary_${comp}.txt"
        
        echo "* Compressor: $comp"
        if [ -f "$summary_path" ]; then
            # Extract query count from summary file
            query_count=$(grep "Queries processed:" "$summary_path" | cut -d' ' -f3)
            echo "  Queries processed: $query_count"
            
            # Extract match rates if there's information about success/failure
            # For now, just note that detailed info is in the summary file
            echo "  See details in: $summary_path"
        else
            echo "  No summary file found"
        fi
        echo
    done
} > "$summary_file"

echo "Combined comparison summary saved to: $summary_file"