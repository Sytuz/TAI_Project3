#!/bin/bash

#####################################################################
# Test Results Visualization Generator
# 
# This script generates comprehensive visualizations using existing
# visualization infrastructure without creating custom Python files
#####################################################################

set -e

# Color codes for output
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Default values
RESULTS_DIR=""
OUTPUT_DIR=""
TEST_TYPE=""
FORMATS=""
COMPRESSORS=""

# Display usage information
function show_help() {
    echo "Usage: $0 [OPTIONS]"
    echo "Options:"
    echo "  --results-dir <dir>    Directory containing test results"
    echo "  --output-dir <dir>     Directory to save plots and visualizations"
    echo "  --test-type <type>     Type of test (small_dataset, samples_dataset, etc.)"
    echo "  --formats <list>       Comma-separated list of formats (text,binary)"
    echo "  --compressors <list>   Comma-separated list of compressors (gzip,bzip2,lzma,zstd)"
    echo "  -h, --help             Show this help message"
}

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        --results-dir)
            RESULTS_DIR="$2"
            shift 2
            ;;
        --output-dir)
            OUTPUT_DIR="$2"
            shift 2
            ;;
        --test-type)
            TEST_TYPE="$2"
            shift 2
            ;;
        --formats)
            FORMATS="$2"
            shift 2
            ;;
        --compressors)
            COMPRESSORS="$2"
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

# Validate required parameters
if [ -z "$RESULTS_DIR" ] || [ -z "$OUTPUT_DIR" ]; then
    echo "Error: --results-dir and --output-dir are required"
    show_help
    exit 1
fi

if [ ! -d "$RESULTS_DIR" ]; then
    echo "Error: Results directory does not exist: $RESULTS_DIR"
    exit 1
fi

# Create output directory
mkdir -p "$OUTPUT_DIR"

echo "============================================="
echo "  Test Results Visualization Generator"
echo "============================================="
echo "Results directory: $RESULTS_DIR"
echo "Output directory: $OUTPUT_DIR"
echo "Test type: $TEST_TYPE"
echo "Formats: $FORMATS"
echo "Compressors: $COMPRESSORS"
echo ""

# Activate virtual environment if it exists
echo -e "${GREEN}Activating virtual environment...${NC}"
if [ -f "venv/bin/activate" ]; then
    source venv/bin/activate
fi

echo -e "${GREEN}Generating visualizations...${NC}"

# Convert comma-separated lists to arrays
IFS=',' read -ra FORMAT_ARRAY <<< "$FORMATS"
IFS=',' read -ra COMPRESSOR_ARRAY <<< "$COMPRESSORS"

#####################################################################
# Generate individual compressor visualizations
#####################################################################

for format in "${FORMAT_ARRAY[@]}"; do
    for compressor in "${COMPRESSOR_ARRAY[@]}"; do
        # Determine the correct path to results
        if [ "$format" = "text" ]; then
            format_dir="$RESULTS_DIR/text_format_results"
        elif [ "$format" = "binary" ]; then
            format_dir="$RESULTS_DIR/binary_format_results"
        else
            # Try direct path structure
            format_dir="$RESULTS_DIR"
        fi
        
        result_file="$format_dir/$compressor/accuracy_metrics_${compressor}.json"
        
        if [ -f "$result_file" ]; then
            echo "Creating visualization for $format format, $compressor compressor..."
            output_subdir="$OUTPUT_DIR/${format}_${compressor}"
            mkdir -p "$output_subdir"
            
            # Create individual compressor analysis using inline Python
            python3 << EOF
import json
import matplotlib.pyplot as plt
import numpy as np
from pathlib import Path

def create_individual_analysis(result_file, output_dir, format_name, compressor):
    """Create analysis plots for individual compressor results."""
    with open(result_file, 'r') as f:
        data = json.load(f)
    
    output_dir = Path(output_dir)
    output_dir.mkdir(exist_ok=True)
    
    # Extract accuracy data
    top1 = data['top1_accuracy']
    top5 = data['top5_accuracy']
    top10 = data['top10_accuracy']
    total = data['total_queries']
    
    # Create accuracy bar chart
    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(12, 5))
    
    # Accuracy percentages
    accuracies = [top1, top5, top10]
    labels = ['Top-1', 'Top-5', 'Top-10']
    colors = ['#ff7f7f', '#7fbf7f', '#7f7fff']
    
    bars = ax1.bar(labels, accuracies, color=colors, alpha=0.8)
    ax1.set_ylabel('Accuracy (%)')
    ax1.set_title(f'{format_name.title()} Format - {compressor.title()} Compressor')
    ax1.set_ylim(0, 100)
    ax1.grid(True, alpha=0.3)
    
    # Add value labels on bars
    for bar, acc in zip(bars, accuracies):
        ax1.text(bar.get_x() + bar.get_width()/2, bar.get_height() + 1, 
                f'{acc:.1f}%', ha='center', va='bottom', fontweight='bold')
    
    # Correct vs incorrect breakdown
    correct_counts = [data['top1_correct'], data['top5_correct'], data['top10_correct']]
    incorrect_counts = [total - c for c in correct_counts]
    
    x = np.arange(len(labels))
    width = 0.6
    
    ax2.bar(x, correct_counts, width, label='Correct', color='green', alpha=0.8)
    ax2.bar(x, incorrect_counts, width, bottom=correct_counts, label='Incorrect', color='red', alpha=0.8)
    
    ax2.set_ylabel('Number of Queries')
    ax2.set_title('Correct vs Incorrect Identifications')
    ax2.set_xticks(x)
    ax2.set_xticklabels(labels)
    ax2.legend()
    ax2.grid(True, alpha=0.3)
    
    # Add count labels
    for i, (correct, incorrect) in enumerate(zip(correct_counts, incorrect_counts)):
        ax2.text(i, correct/2, str(correct), ha='center', va='center', fontweight='bold', color='white')
        if incorrect > 0:
            ax2.text(i, correct + incorrect/2, str(incorrect), ha='center', va='center', fontweight='bold', color='white')
    
    plt.tight_layout()
    plt.savefig(output_dir / f'{format_name}_{compressor}_analysis.png', dpi=300, bbox_inches='tight')
    plt.close()
    
    # Create detailed results analysis if available
    if 'detailed_results' in data and data['detailed_results']:
        fig, ax = plt.subplots(figsize=(12, 8))
        
        ranks = []
        for result in data['detailed_results']:
            if result['correct']:
                ranks.append(result['found_at_rank'])
            else:
                ranks.append(11)  # Beyond top-10
        
        # Create histogram of ranks
        bins = list(range(1, 13))
        counts, _, _ = ax.hist(ranks, bins=bins, alpha=0.7, color='skyblue', edgecolor='black')
        
        ax.set_xlabel('Rank of Correct Match')
        ax.set_ylabel('Number of Queries')
        ax.set_title(f'Distribution of Correct Match Rankings - {format_name.title()} Format, {compressor.title()}')
        ax.set_xticks(range(1, 12))
        ax.set_xticklabels([str(i) for i in range(1, 11)] + ['Not Found'])
        ax.grid(True, alpha=0.3)
        
        plt.tight_layout()
        plt.savefig(output_dir / f'{format_name}_{compressor}_rank_distribution.png', dpi=300, bbox_inches='tight')
        plt.close()
    
    # Create summary report
    with open(output_dir / f'{format_name}_{compressor}_summary.txt', 'w') as f:
        f.write(f"Individual Analysis Summary\\n")
        f.write(f"={'='*30}\\n\\n")
        f.write(f"Format: {format_name.title()}\\n")
        f.write(f"Compressor: {compressor.title()}\\n")
        f.write(f"Total Queries: {total}\\n\\n")
        f.write(f"Accuracy Results:\\n")
        f.write(f"- Top-1: {top1:.1f}% ({data['top1_correct']}/{total})\\n")
        f.write(f"- Top-5: {top5:.1f}% ({data['top5_correct']}/{total})\\n")
        f.write(f"- Top-10: {top10:.1f}% ({data['top10_correct']}/{total})\\n\\n")
        
        if 'detailed_results' in data:
            f.write("Detailed Results:\\n")
            f.write("-" * 50 + "\\n")
            for i, result in enumerate(data['detailed_results'], 1):
                status = "✓" if result['correct'] else "✗"
                rank = result['found_at_rank'] if result['correct'] else "Not found"
                f.write(f"{i:2d}. {status} {result['query'][:30]:30s} -> Rank: {rank}\\n")
    
    print(f"Created individual analysis for {format_name} format, {compressor} compressor")

create_individual_analysis('$result_file', '$output_subdir', '$format', '$compressor')
EOF
        else
            echo "Warning: Result file not found: $result_file"
        fi
    done
done

#####################################################################
# Generate format comparison plots using inline Python
#####################################################################

echo "Creating format comparison plots..."

python3 << EOF
import json
import matplotlib.pyplot as plt
import numpy as np
from pathlib import Path
import sys

def load_compressor_results(results_dir, format_name, compressors):
    """Load results from all compressors for a given format."""
    results = {}
    
    if format_name == 'text':
        format_dir = Path(results_dir) / 'text_format_results'
    elif format_name == 'binary':
        format_dir = Path(results_dir) / 'binary_format_results'
    else:
        format_dir = Path(results_dir)
    
    for comp in compressors:
        json_file = format_dir / comp / f'accuracy_metrics_{comp}.json'
        if json_file.exists():
            try:
                with open(json_file, 'r') as f:
                    results[comp] = json.load(f)
                print(f"Loaded {comp} results for {format_name} format")
            except Exception as e:
                print(f"Error loading {comp} results: {e}")
        else:
            print(f"Warning: Could not find {json_file}")
    
    return results

def create_comparison_plots():
    """Create comparison plots between formats."""
    results_dir = '$RESULTS_DIR'
    output_dir = Path('$OUTPUT_DIR')
    formats = [f.strip() for f in '$FORMATS'.split(',')]
    compressors = [c.strip() for c in '$COMPRESSORS'.split(',')]
    
    # Load results for each format
    format_results = {}
    for fmt in formats:
        format_results[fmt] = load_compressor_results(results_dir, fmt, compressors)
    
    # Create individual format plots
    for fmt, results in format_results.items():
        if results:
            create_single_format_plot(results, fmt, output_dir)
    
    # Create format comparison if we have multiple formats
    if len(formats) > 1 and 'text' in format_results and 'binary' in format_results:
        text_results = format_results['text']
        binary_results = format_results['binary']
        if text_results and binary_results:
            create_format_comparison_plot(text_results, binary_results, output_dir)

def create_single_format_plot(results, format_name, output_dir):
    """Create plot for a single format."""
    if not results:
        return
    
    compressors = list(results.keys())
    top1_scores = [results[comp]['top1_accuracy'] for comp in compressors]
    top5_scores = [results[comp]['top5_accuracy'] for comp in compressors]
    top10_scores = [results[comp]['top10_accuracy'] for comp in compressors]
    
    fig, ax = plt.subplots(figsize=(10, 6))
    
    x = np.arange(len(compressors))
    width = 0.25
    
    ax.bar(x - width, top1_scores, width, label='Top-1', alpha=0.8)
    ax.bar(x, top5_scores, width, label='Top-5', alpha=0.8)
    ax.bar(x + width, top10_scores, width, label='Top-10', alpha=0.8)
    
    ax.set_xlabel('Compressor')
    ax.set_ylabel('Accuracy (%)')
    ax.set_title(f'Accuracy Comparison - {format_name.title()} Format')
    ax.set_xticks(x)
    ax.set_xticklabels(compressors)
    ax.legend()
    ax.grid(True, alpha=0.3)
    
    # Add value labels on bars
    for i, comp in enumerate(compressors):
        ax.text(i - width, top1_scores[i] + 1, f'{top1_scores[i]:.1f}%', 
                ha='center', va='bottom', fontsize=8)
        ax.text(i, top5_scores[i] + 1, f'{top5_scores[i]:.1f}%', 
                ha='center', va='bottom', fontsize=8)
        ax.text(i + width, top10_scores[i] + 1, f'{top10_scores[i]:.1f}%', 
                ha='center', va='bottom', fontsize=8)
    
    plt.tight_layout()
    plt.savefig(output_dir / f'{format_name}_accuracy_comparison.png', dpi=300, bbox_inches='tight')
    plt.close()
    print(f"Created {format_name} accuracy comparison plot")

def create_format_comparison_plot(text_results, binary_results, output_dir):
    """Create comparison plot between text and binary formats."""
    common_compressors = set(text_results.keys()) & set(binary_results.keys())
    if not common_compressors:
        print("No common compressors found between formats")
        return
    
    compressors = sorted(list(common_compressors))
    
    # Create side-by-side comparison
    fig, ax = plt.subplots(figsize=(12, 8))
    
    x = np.arange(len(compressors))
    width = 0.35
    
    text_top1 = [text_results[comp]['top1_accuracy'] for comp in compressors]
    binary_top1 = [binary_results[comp]['top1_accuracy'] for comp in compressors]
    
    ax.bar(x - width/2, text_top1, width, label='Text Format', alpha=0.8)
    ax.bar(x + width/2, binary_top1, width, label='Binary Format', alpha=0.8)
    
    ax.set_xlabel('Compressor')
    ax.set_ylabel('Top-1 Accuracy (%)')
    ax.set_title('Format Comparison: Text vs Binary')
    ax.set_xticks(x)
    ax.set_xticklabels(compressors)
    ax.legend()
    ax.grid(True, alpha=0.3)
    
    # Add value labels
    for i, (t, b) in enumerate(zip(text_top1, binary_top1)):
        ax.text(i - width/2, t + 1, f'{t:.1f}%', ha='center', va='bottom', fontsize=9)
        ax.text(i + width/2, b + 1, f'{b:.1f}%', ha='center', va='bottom', fontsize=9)
    
    plt.tight_layout()
    plt.savefig(output_dir / 'format_comparison.png', dpi=300, bbox_inches='tight')
    plt.close()
    
    # Create heatmap
    fig, ax = plt.subplots(figsize=(10, 6))
    
    data = []
    for comp in compressors:
        data.append([
            text_results[comp]['top1_accuracy'],
            text_results[comp]['top5_accuracy'],
            binary_results[comp]['top1_accuracy'],
            binary_results[comp]['top5_accuracy']
        ])
    
    im = ax.imshow(data, cmap='RdYlGn', aspect='auto', vmin=0, vmax=100)
    
    ax.set_xticks(range(4))
    ax.set_xticklabels(['Text Top-1', 'Text Top-5', 'Binary Top-1', 'Binary Top-5'])
    ax.set_yticks(range(len(compressors)))
    ax.set_yticklabels(compressors)
    
    # Add text annotations
    for i in range(len(compressors)):
        for j in range(4):
            ax.text(j, i, f'{data[i][j]:.1f}%', 
                   ha="center", va="center", color="black", fontweight='bold')
    
    ax.set_title('Accuracy Heatmap: Text vs Binary Formats')
    plt.colorbar(im, ax=ax, label='Accuracy (%)')
    
    plt.tight_layout()
    plt.savefig(output_dir / 'accuracy_heatmap.png', dpi=300, bbox_inches='tight')
    plt.close()
    
    print("Created format comparison plots")

if __name__ == "__main__":
    create_comparison_plots()
EOF

#####################################################################
# Handle parameter evaluation results if present
#####################################################################

param_eval_file="$RESULTS_DIR/parameter_evaluation/parameter_evaluation_results.json"
if [ -f "$param_eval_file" ]; then
    echo "Creating parameter evaluation visualizations..."
    param_output_dir="$OUTPUT_DIR/parameter_evaluation"
    mkdir -p "$param_output_dir"
    
    python3 scripts/visualize_parameters.py \
        "$param_eval_file" \
        -o "$param_output_dir"
fi

#####################################################################
# Generate summary report
#####################################################################

echo "Generating summary report..."

cat > "$OUTPUT_DIR/visualization_summary.md" << EOF
# Test Results Visualization Summary

**Generated:** $(date)  
**Test Type:** $TEST_TYPE  
**Results Directory:** $RESULTS_DIR  

## Visualizations Created

### Individual Compressor Analysis
EOF

for format in "${FORMAT_ARRAY[@]}"; do
    for compressor in "${COMPRESSOR_ARRAY[@]}"; do
        output_subdir="$OUTPUT_DIR/${format}_${compressor}"
        if [ -d "$output_subdir" ]; then
            echo "- **${format^} Format, ${compressor^} Compressor:** \`${format}_${compressor}/\`" >> "$OUTPUT_DIR/visualization_summary.md"
        fi
    done
done

cat >> "$OUTPUT_DIR/visualization_summary.md" << EOF

### Comparative Analysis
- **Format Comparison:** \`format_comparison.png\`
- **Accuracy Heatmap:** \`accuracy_heatmap.png\` (if multiple formats available)

### Parameter Evaluation
EOF

if [ -f "$param_eval_file" ]; then
    echo "- **Parameter Analysis:** \`parameter_evaluation/\`" >> "$OUTPUT_DIR/visualization_summary.md"
else
    echo "- No parameter evaluation results found" >> "$OUTPUT_DIR/visualization_summary.md"
fi

echo ""
echo -e "${GREEN}Visualization generation complete!${NC}"
echo "All plots and reports saved to: $OUTPUT_DIR"
echo "Summary report: $OUTPUT_DIR/visualization_summary.md"
