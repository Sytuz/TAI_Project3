#!/bin/bash

# Quick Parameter Evaluation Script
# This script runs a quick evaluation of different parameters to find optimal settings

# Default values
project_root="/home/maria/Desktop/TAI_Project3"
query_dir="data/test_samples"
db_dir="data/samples"
output_dir="quick_param_eval"
compressors="gzip,bzip2"
use_binary=false

# Display usage information
function show_help() {
    echo "Usage: $0 [OPTIONS]"
    echo "Options:"
    echo "  --project-root <dir>  Project root directory [default: /home/maria/Desktop/TAI_Project3]"
    echo "  --query-dir <dir>     Directory with query WAV files [default: data/test_samples]"
    echo "  --db-dir <dir>        Directory with database WAV files [default: data/samples]"
    echo "  --output-dir <dir>    Output directory [default: quick_param_eval]"
    echo "  --compressors <list>  Comma-separated compressors [default: gzip,bzip2]"
    echo "  --binary              Use binary format for feature files (.featbin)"
    echo "  --full                Run full evaluation (takes much longer)"
    echo "  -h, --help            Show this help message"
    echo
    echo "This script evaluates different feature extraction parameters to find optimal settings."
}

# Parse command line arguments
full_evaluation=false

while [[ $# -gt 0 ]]; do
    case $1 in
        --project-root)
            project_root="$2"
            shift 2
            ;;
        --query-dir)
            query_dir="$2"
            shift 2
            ;;
        --db-dir)
            db_dir="$2"
            shift 2
            ;;
        --output-dir)
            output_dir="$2"
            shift 2
            ;;
        --compressors)
            compressors="$2"
            shift 2
            ;;
        --binary)
            use_binary=true
            shift
            ;;
        --full)
            full_evaluation=true
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

# Change to project directory
cd "$project_root" || {
    echo "Error: Could not change to project directory: $project_root"
    exit 1
}

# Check if required directories exist
if [ ! -d "$query_dir" ]; then
    echo "Error: Query directory does not exist: $query_dir"
    exit 1
fi

if [ ! -d "$db_dir" ]; then
    echo "Error: Database directory does not exist: $db_dir"
    exit 1
fi

# Check if required applications exist
if [ ! -f "apps/extract_features" ]; then
    echo "Error: extract_features application not found. Please build the project first."
    echo "Run: bash scripts/rebuild.sh"
    exit 1
fi

echo "Starting parameter evaluation..."
echo "Project root: $project_root"
echo "Query directory: $query_dir"
echo "Database directory: $db_dir"
echo "Output directory: $output_dir"
echo "Compressors: $compressors"
echo "Binary format: $use_binary"

if [ "$full_evaluation" = true ]; then
    echo "Running FULL evaluation (this may take a long time)..."
    evaluation_type=""
else
    echo "Running QUICK evaluation..."
    evaluation_type="--quick"
fi

echo ""

# Run the parameter evaluation
binary_arg=""
if [ "$use_binary" = true ]; then
    binary_arg="--binary"
fi

if python3 scripts/evaluate_parameters.py \
    --project-root "$project_root" \
    --query-dir "$query_dir" \
    --db-dir "$db_dir" \
    --output-dir "$output_dir" \
    --compressors "$compressors" \
    $binary_arg \
    $evaluation_type; then
    
    echo ""
    echo "Parameter evaluation completed successfully!"
    
    # Check if results file exists and create visualizations
    results_file="$output_dir/parameter_evaluation_results.json"
    if [ -f "$results_file" ]; then
        echo "Creating visualizations..."
        plots_dir="$output_dir/plots"
        
        if python3 scripts/visualize_parameters.py "$results_file" -o "$plots_dir"; then
            echo "Visualizations created in: $plots_dir"
        else
            echo "Warning: Failed to create visualizations"
        fi
        
        # Display summary
        echo ""
        echo "=========================================="
        echo "EVALUATION SUMMARY"
        echo "=========================================="
        
        if command -v python3 &> /dev/null; then
            python3 -c "
import json
with open('$results_file', 'r') as f:
    data = json.load(f)
results = data['results']
if results:
    best = max(results, key=lambda x: x['top1_accuracy'])
    print(f'Total configurations tested: {len(results)}')
    print(f'Best configuration:')
    print(f'  Method: {best[\"method\"]}')
    print(f'  Frequencies: {best[\"numFrequencies\"]}')
    print(f'  Bins: {best[\"numBins\"]}')
    print(f'  Frame Size: {best[\"frameSize\"]}')
    print(f'  Hop Size: {best[\"hopSize\"]}')
    print(f'  Compressor: {best[\"compressor\"]}')
    print(f'  Top-1 Accuracy: {best[\"top1_accuracy\"]:.1f}%')
    print(f'  Top-5 Accuracy: {best[\"top5_accuracy\"]:.1f}%')
    print(f'  Top-10 Accuracy: {best[\"top10_accuracy\"]:.1f}%')
else:
    print('No results found')
"
        fi
        
        echo ""
        echo "Results saved to: $output_dir"
        echo "Detailed analysis: $plots_dir/detailed_analysis_report.txt"
        
    else
        echo "Warning: Results file not found: $results_file"
    fi
    
else
    echo "Error: Parameter evaluation failed"
    exit 1
fi
