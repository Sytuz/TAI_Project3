#!/bin/bash

# Plot Generation Test Script
# Creates visualizations from test_feature_extraction, test_compression, and test_parameter_analysis results

# Color codes for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Configuration
PROJECT_ROOT="$(dirname "$(realpath "$0")")/.."
DATASET="small_samples"  # Change to: samples or full_tracks for larger datasets
FEATURES_BASE_DIR="tests/feature_extraction_results/$DATASET"
COMPRESSION_BASE_DIR="tests/compression_results/$DATASET"
ANALYSIS_BASE_DIR="tests/parameter_analysis/$DATASET"
OUTPUT_BASE_DIR="tests/plots"
METHODS=("spectral" "maxfreq")
FORMATS=("text" "binary")
COMPRESSORS=("gzip" "bzip2" "lzma" "zstd")
NOISE_TYPES=("clean" "white" "brown" "pink")  # Include noise types

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

# Pre checks
print_step "1" "Pre checks"

# Check if Python and required packages are available
if ! command -v python3 > /dev/null; then
    print_error "Python3 not found. Please install Python3."
    exit 1
fi

# Check for required directories
if [ ! -d "$COMPRESSION_BASE_DIR" ]; then
    print_error "Compression results not found: $COMPRESSION_BASE_DIR"
    print_error "Please run test_compression.sh first"
    exit 1
fi

if [ ! -d "$FEATURES_BASE_DIR" ]; then
    print_error "Feature extraction results not found: $FEATURES_BASE_DIR"
    print_error "Please run test_feature_extraction.sh first"
    exit 1
fi

print_success "Found required test results"

# Create output directory
TEST_OUTPUT_DIR="$OUTPUT_BASE_DIR/$DATASET"
mkdir -p "$TEST_OUTPUT_DIR"
print_info "Plot output directory: $TEST_OUTPUT_DIR"

# Function to check and install Python packages
check_python_packages() {
    print_step "2" "Checking Python dependencies"
    
    local packages=("matplotlib" "numpy" "pandas" "seaborn")
    local missing_packages=()
    
    for package in "${packages[@]}"; do
        if ! python3 -c "import $package" 2>/dev/null; then
            missing_packages+=("$package")
        fi
    done
    
    if [ ${#missing_packages[@]} -gt 0 ]; then
        print_warning "Missing Python packages: ${missing_packages[*]}"
        print_info "Installing missing packages..."
        
        if ! pip3 install "${missing_packages[@]}"; then
            print_error "Failed to install Python packages"
            print_warning "You can install them manually: pip3 install ${missing_packages[*]}"
            return 1
        fi
    fi
    
    print_success "All Python dependencies available"
    return 0
}

# Function to generate accuracy comparison plots
generate_accuracy_plots() {
    print_step "3" "Generating accuracy comparison plots"
    
    local plot_script="$TEST_OUTPUT_DIR/generate_accuracy_plots.py"
    
    cat > "$plot_script" << 'EOF'
#!/usr/bin/env python3
import json
import matplotlib.pyplot as plt
import numpy as np
import pandas as pd
import seaborn as sns
from pathlib import Path
import sys
import os

# Configuration
dataset = "small_samples"  # Change to: samples or full_tracks for larger datasets
compression_base = f"tests/compression_results/{dataset}"
output_dir = f"tests/plots/{dataset}"
methods = ["spectral", "maxfreq"]
formats = ["text", "binary"]
compressors = ["gzip", "bzip2", "lzma", "zstd"]
noise_types = ["clean", "white", "brown", "pink"]

# Create output directory
Path(output_dir).mkdir(parents=True, exist_ok=True)

def load_accuracy_data():
    """Load all accuracy data from JSON files"""
    data = []
    
    for noise_type in noise_types:
        for method in methods:
            for format_type in formats:
                for compressor in compressors:
                    json_file = Path(f"{compression_base}/{noise_type}/{method}/{format_type}/{compressor}/accuracy_metrics_{compressor}.json")
                    
                    if json_file.exists():
                        try:
                            with open(json_file, 'r') as f:
                                accuracy_data = json.load(f)
                            
                            data.append({
                                'Noise_Type': noise_type.capitalize(),
                                'Method': method.capitalize(),
                                'Format': format_type.capitalize(),
                                'Compressor': compressor.upper(),
                                'Top1_Accuracy': accuracy_data.get('top1_accuracy', 0),
                                'Top5_Accuracy': accuracy_data.get('top5_accuracy', 0),
                                'Top10_Accuracy': accuracy_data.get('top10_accuracy', 0),
                                'Total_Queries': accuracy_data.get('total_queries', 0),
                                'Configuration': f"{noise_type}/{method}/{format_type}/{compressor}"
                            })
                        except Exception as e:
                            print(f"Error loading {json_file}: {e}")
    
    return pd.DataFrame(data)

def plot_accuracy_heatmap(df):
    """Create accuracy heatmap"""
    print("Generating accuracy heatmap...")
    
    # Create separate heatmaps for each noise type
    noise_types_list = df['Noise_Type'].unique()
    
    # Calculate global min/max for consistent color scale
    vmin = df['Top1_Accuracy'].min()
    vmax = df['Top1_Accuracy'].max()
    
    fig, axes = plt.subplots(2, 2, figsize=(20, 16))
    fig.suptitle('Top-1 Accuracy by Method, Format, and Compressor (by Noise Type)', fontsize=16)
    
    for i, noise_type in enumerate(noise_types_list):
        row = i // 2
        col = i % 2
        ax = axes[row, col]
        
        noise_data = df[df['Noise_Type'] == noise_type]
        
        # Create pivot table for heatmap
        pivot_data = noise_data.pivot_table(
            values='Top1_Accuracy', 
            index=['Method', 'Format'], 
            columns='Compressor', 
            aggfunc='mean'
        )
        
        # Use consistent color scale across all subplots
        sns.heatmap(pivot_data, annot=True, fmt='.1f', cmap='YlOrRd', 
                    vmin=vmin, vmax=vmax,
                    cbar_kws={'label': 'Top-1 Accuracy (%)'}, ax=ax)
        ax.set_title(f'{noise_type} Samples')
        ax.set_xlabel('Compressor')
        ax.set_ylabel('Method / Format')
    
    plt.tight_layout()
    plt.savefig(f'{output_dir}/accuracy_heatmap.png', dpi=300, bbox_inches='tight')
    plt.close()

def plot_method_comparison(df):
    """Compare methods across formats and compressors"""
    print("Generating method comparison plots...")
    
    fig, axes = plt.subplots(2, 2, figsize=(15, 12))
    fig.suptitle('Method Comparison: Spectral vs MaxFreq', fontsize=16)
    
    metrics = ['Top1_Accuracy', 'Top5_Accuracy']
    metric_names = ['Top-1 Accuracy', 'Top-5 Accuracy']
    
    for i, (metric, name) in enumerate(zip(metrics, metric_names)):
        for j, format_type in enumerate(['Text', 'Binary']):
            ax = axes[i, j]
            
            format_data = df[df['Format'] == format_type]
            
            # Group by method and compressor
            grouped = format_data.groupby(['Method', 'Compressor'])[metric].mean().unstack()
            
            grouped.plot(kind='bar', ax=ax, width=0.8)
            ax.set_title(f'{name} - {format_type} Format')
            ax.set_ylabel('Accuracy (%)')
            ax.set_xlabel('Method')
            ax.legend(title='Compressor', bbox_to_anchor=(1.05, 1), loc='upper left')
            ax.tick_params(axis='x', rotation=45)
    
    plt.tight_layout()
    plt.savefig(f'{output_dir}/method_comparison.png', dpi=300, bbox_inches='tight')
    plt.close()

def plot_compressor_ranking(df):
    """Rank compressors by average performance"""
    print("Generating compressor ranking plot...")
    
    # Calculate average accuracy for each compressor
    compressor_stats = df.groupby('Compressor').agg({
        'Top1_Accuracy': ['mean', 'std'],
        'Top5_Accuracy': ['mean', 'std']
    }).round(2)
    
    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(15, 6))
    
    # Top-1 accuracy ranking
    top1_means = compressor_stats['Top1_Accuracy']['mean']
    top1_stds = compressor_stats['Top1_Accuracy']['std']
    
    bars1 = ax1.bar(top1_means.index, top1_means.values, 
                    yerr=top1_stds.values, capsize=5, alpha=0.7)
    ax1.set_title('Average Top-1 Accuracy by Compressor')
    ax1.set_ylabel('Top-1 Accuracy (%)')
    ax1.set_xlabel('Compressor')
    
    # Add value labels on bars
    for bar, mean_val in zip(bars1, top1_means.values):
        ax1.text(bar.get_x() + bar.get_width()/2, bar.get_height() + 1,
                f'{mean_val:.1f}%', ha='center', va='bottom')
    
    # Top-5 accuracy ranking
    top5_means = compressor_stats['Top5_Accuracy']['mean']
    top5_stds = compressor_stats['Top5_Accuracy']['std']
    
    bars2 = ax2.bar(top5_means.index, top5_means.values,
                    yerr=top5_stds.values, capsize=5, alpha=0.7, color='orange')
    ax2.set_title('Average Top-5 Accuracy by Compressor')
    ax2.set_ylabel('Top-5 Accuracy (%)')
    ax2.set_xlabel('Compressor')
    
    # Add value labels on bars
    for bar, mean_val in zip(bars2, top5_means.values):
        ax2.text(bar.get_x() + bar.get_width()/2, bar.get_height() + 1,
                f'{mean_val:.1f}%', ha='center', va='bottom')
    
    plt.tight_layout()
    plt.savefig(f'{output_dir}/compressor_ranking.png', dpi=300, bbox_inches='tight')
    plt.close()

def plot_format_comparison(df):
    """Compare text vs binary formats"""
    print("Generating format comparison plot...")
    
    fig, axes = plt.subplots(2, 2, figsize=(15, 12))
    fig.suptitle('Format Comparison: Text vs Binary', fontsize=16)
    
    for i, method in enumerate(['Spectral', 'Maxfreq']):
        method_data = df[df['Method'] == method]
        
        # Top-1 accuracy comparison
        ax1 = axes[i, 0]
        format_means = method_data.groupby(['Format', 'Compressor'])['Top1_Accuracy'].mean().unstack()
        format_means.plot(kind='bar', ax=ax1, width=0.8)
        ax1.set_title(f'{method} - Top-1 Accuracy')
        ax1.set_ylabel('Top-1 Accuracy (%)')
        ax1.set_xlabel('Format')
        ax1.legend(title='Compressor')
        ax1.tick_params(axis='x', rotation=45)
        
        # Top-5 accuracy comparison
        ax2 = axes[i, 1]
        format_means_top5 = method_data.groupby(['Format', 'Compressor'])['Top5_Accuracy'].mean().unstack()
        format_means_top5.plot(kind='bar', ax=ax2, width=0.8)
        ax2.set_title(f'{method} - Top-5 Accuracy')
        ax2.set_ylabel('Top-5 Accuracy (%)')
        ax2.set_xlabel('Format')
        ax2.legend(title='Compressor')
        ax2.tick_params(axis='x', rotation=45)
    
    plt.tight_layout()
    plt.savefig(f'{output_dir}/format_comparison.png', dpi=300, bbox_inches='tight')
    plt.close()

def plot_performance_overview(df):
    """Create overview dashboard"""
    print("Generating performance overview...")
    
    fig = plt.figure(figsize=(20, 12))
    gs = fig.add_gridspec(3, 4, hspace=0.3, wspace=0.3)
    
    # Overall accuracy distribution
    ax1 = fig.add_subplot(gs[0, :2])
    df['Top1_Accuracy'].hist(bins=10, alpha=0.7, ax=ax1)
    ax1.set_title('Distribution of Top-1 Accuracy Scores')
    ax1.set_xlabel('Top-1 Accuracy (%)')
    ax1.set_ylabel('Frequency')
    ax1.axvline(df['Top1_Accuracy'].mean(), color='red', linestyle='--', 
                label=f'Mean: {df["Top1_Accuracy"].mean():.1f}%')
    ax1.legend()
    
    # Best configurations
    ax2 = fig.add_subplot(gs[0, 2:])
    top_configs = df.nlargest(8, 'Top1_Accuracy')[['Configuration', 'Top1_Accuracy']]
    bars = ax2.barh(range(len(top_configs)), top_configs['Top1_Accuracy'])
    ax2.set_yticks(range(len(top_configs)))
    ax2.set_yticklabels(top_configs['Configuration'], fontsize=8)
    ax2.set_xlabel('Top-1 Accuracy (%)')
    ax2.set_title('Top 8 Configurations')
    
    # Add value labels
    for i, (bar, val) in enumerate(zip(bars, top_configs['Top1_Accuracy'])):
        ax2.text(val + 0.5, i, f'{val:.1f}%', va='center')
    
    # Method performance by compressor
    ax3 = fig.add_subplot(gs[1, :2])
    method_comp_pivot = df.pivot_table(values='Top1_Accuracy', index='Method', columns='Compressor', aggfunc='mean')
    sns.heatmap(method_comp_pivot, annot=True, fmt='.1f', cmap='Blues', ax=ax3)
    ax3.set_title('Average Top-1 Accuracy: Method vs Compressor')
    
    # Format performance by compressor
    ax4 = fig.add_subplot(gs[1, 2:])
    format_comp_pivot = df.pivot_table(values='Top1_Accuracy', index='Format', columns='Compressor', aggfunc='mean')
    sns.heatmap(format_comp_pivot, annot=True, fmt='.1f', cmap='Greens', ax=ax4)
    ax4.set_title('Average Top-1 Accuracy: Format vs Compressor')
    
    # Summary statistics
    ax5 = fig.add_subplot(gs[2, :])
    summary_stats = df.groupby('Compressor').agg({
        'Top1_Accuracy': ['mean', 'std', 'min', 'max'],
        'Top5_Accuracy': ['mean', 'std', 'min', 'max']
    }).round(2)
    
    # Create a text summary
    summary_text = "PERFORMANCE SUMMARY:\n\n"
    for comp in compressors:
        comp_upper = comp.upper()
        top1_mean = summary_stats.loc[comp_upper, ('Top1_Accuracy', 'mean')]
        top1_std = summary_stats.loc[comp_upper, ('Top1_Accuracy', 'std')]
        top5_mean = summary_stats.loc[comp_upper, ('Top5_Accuracy', 'mean')]
        
        summary_text += f"{comp_upper}: Top-1: {top1_mean:.1f}±{top1_std:.1f}%, Top-5: {top5_mean:.1f}%\n"
    
    # Find best overall configuration
    best_config = df.loc[df['Top1_Accuracy'].idxmax()]
    summary_text += f"\nBEST CONFIGURATION:\n{best_config['Configuration']}\n"
    summary_text += f"Top-1: {best_config['Top1_Accuracy']:.1f}%, Top-5: {best_config['Top5_Accuracy']:.1f}%"
    
    ax5.text(0.05, 0.95, summary_text, transform=ax5.transAxes, fontsize=10,
             verticalalignment='top', fontfamily='monospace',
             bbox=dict(boxstyle='round', facecolor='lightgray', alpha=0.5))
    ax5.set_xlim(0, 1)
    ax5.set_ylim(0, 1)
    ax5.axis('off')
    ax5.set_title('Summary Statistics')
    
    plt.suptitle('Music Identification System - Performance Overview', fontsize=16)
    plt.savefig(f'{output_dir}/performance_overview.png', dpi=300, bbox_inches='tight')
    plt.close()

def plot_noise_comparison(df):
    """Compare performance across different noise types"""
    print("Generating noise comparison plot...")
    
    fig, axes = plt.subplots(2, 2, figsize=(16, 12))
    fig.suptitle('Noise Type Impact Analysis', fontsize=16)
    
    # Overall noise impact on Top-1 accuracy
    ax1 = axes[0, 0]
    noise_means = df.groupby('Noise_Type')['Top1_Accuracy'].mean().sort_values(ascending=False)
    noise_stds = df.groupby('Noise_Type')['Top1_Accuracy'].std()
    
    bars1 = ax1.bar(noise_means.index, noise_means.values, 
                    yerr=noise_stds.values, capsize=5, alpha=0.7,
                    color=['green', 'orange', 'red', 'purple'])
    ax1.set_title('Average Top-1 Accuracy by Noise Type')
    ax1.set_ylabel('Top-1 Accuracy (%)')
    ax1.set_xlabel('Noise Type')
    
    # Add value labels on bars
    for bar, mean_val in zip(bars1, noise_means.values):
        ax1.text(bar.get_x() + bar.get_width()/2, bar.get_height() + 1,
                f'{mean_val:.1f}%', ha='center', va='bottom')
    
    # Noise impact by method
    ax2 = axes[0, 1]
    method_noise_pivot = df.pivot_table(values='Top1_Accuracy', index='Noise_Type', columns='Method', aggfunc='mean')
    method_noise_pivot.plot(kind='bar', ax=ax2, width=0.8)
    ax2.set_title('Noise Impact by Method')
    ax2.set_ylabel('Top-1 Accuracy (%)')
    ax2.set_xlabel('Noise Type')
    ax2.legend(title='Method')
    ax2.tick_params(axis='x', rotation=45)
    
    # Noise impact by compressor
    ax3 = axes[1, 0]
    comp_noise_pivot = df.pivot_table(values='Top1_Accuracy', index='Noise_Type', columns='Compressor', aggfunc='mean')
    comp_noise_pivot.plot(kind='bar', ax=ax3, width=0.8)
    ax3.set_title('Noise Impact by Compressor')
    ax3.set_ylabel('Top-1 Accuracy (%)')
    ax3.set_xlabel('Noise Type')
    ax3.legend(title='Compressor', bbox_to_anchor=(1.05, 1), loc='upper left')
    ax3.tick_params(axis='x', rotation=45)
    
    # Detailed heatmap: Noise vs Configuration
    ax4 = axes[1, 1]
    # Create a simplified configuration label
    df_config = df.copy()
    df_config['Config'] = df_config['Method'] + '/' + df_config['Format']
    config_noise_pivot = df_config.pivot_table(values='Top1_Accuracy', index='Config', columns='Noise_Type', aggfunc='mean')
    
    sns.heatmap(config_noise_pivot, annot=True, fmt='.1f', cmap='RdYlGn', 
                vmin=df['Top1_Accuracy'].min(), vmax=df['Top1_Accuracy'].max(),
                ax=ax4)
    ax4.set_title('Configuration vs Noise Type')
    ax4.set_xlabel('Noise Type')
    ax4.set_ylabel('Method/Format')
    
    plt.tight_layout()
    plt.savefig(f'{output_dir}/noise_comparison.png', dpi=300, bbox_inches='tight')
    plt.close()

def main():
    print("Loading accuracy data...")
    df = load_accuracy_data()
    
    if df.empty:
        print("No accuracy data found. Please run compression tests first.")
        return 1
    
    print(f"Loaded {len(df)} data points")
    print(f"Methods: {df['Method'].unique()}")
    print(f"Formats: {df['Format'].unique()}")
    print(f"Compressors: {df['Compressor'].unique()}")
    
    # Generate all plots
    plot_accuracy_heatmap(df)
    plot_method_comparison(df)
    plot_compressor_ranking(df)
    plot_format_comparison(df)
    plot_performance_overview(df)
    plot_noise_comparison(df)
    
    print(f"\nAll plots saved to: {output_dir}/")
    return 0

if __name__ == "__main__":
    sys.exit(main())
EOF

    python3 "$plot_script"
    
    if [ $? -eq 0 ]; then
        print_success "Accuracy plots generated successfully"
    else
        print_error "Failed to generate accuracy plots"
        return 1
    fi
}

# Function to generate feature extraction analysis plots
generate_feature_plots() {
    print_step "4" "Generating feature extraction analysis plots"
    
    local plot_script="$TEST_OUTPUT_DIR/generate_feature_plots.py"
    
    cat > "$plot_script" << 'EOF'
#!/usr/bin/env python3
import matplotlib.pyplot as plt
import numpy as np
import pandas as pd
import os
from pathlib import Path

# Configuration
dataset = "small_samples"  # Change to: samples or full_tracks for larger datasets
features_base = f"tests/feature_extraction_results/{dataset}"
output_dir = f"tests/plots/{dataset}"
methods = ["spectral", "maxfreq"]
formats = ["text", "binary"]

def analyze_feature_files():
    """Analyze feature file sizes and characteristics"""
    data = []
    
    for method in methods:
        for format_type in formats:
            features_dir = Path(f"{features_base}/{method}/{format_type}")
            
            if not features_dir.exists():
                continue
                
            extension = "featbin" if format_type == "binary" else "feat"
            feature_files = list(features_dir.glob(f"*.{extension}"))
            
            if not feature_files:
                continue
            
            # Calculate file sizes
            file_sizes = [f.stat().st_size for f in feature_files]
            total_size = sum(file_sizes)
            avg_size = total_size / len(file_sizes) if file_sizes else 0
            
            # For text files, count lines
            if format_type == "text" and feature_files:
                sample_file = feature_files[0]
                try:
                    with open(sample_file, 'r') as f:
                        lines = len(f.readlines())
                except:
                    lines = 0
            else:
                lines = 0
            
            data.append({
                'Method': method.capitalize(),
                'Format': format_type.capitalize(),
                'File_Count': len(feature_files),
                'Total_Size_MB': total_size / (1024 * 1024),
                'Avg_Size_KB': avg_size / 1024,
                'Sample_Lines': lines
            })
    
    return pd.DataFrame(data)

def plot_file_sizes(df):
    """Plot feature file size comparison"""
    print("Generating file size comparison plot...")
    
    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(15, 6))
    
    # Total storage size
    pivot_total = df.pivot(index='Method', columns='Format', values='Total_Size_MB')
    pivot_total.plot(kind='bar', ax=ax1, width=0.7)
    ax1.set_title('Total Storage Size by Method and Format')
    ax1.set_ylabel('Total Size (MB)')
    ax1.set_xlabel('Method')
    ax1.legend(title='Format')
    ax1.tick_params(axis='x', rotation=45)
    
    # Average file size
    pivot_avg = df.pivot(index='Method', columns='Format', values='Avg_Size_KB')
    pivot_avg.plot(kind='bar', ax=ax2, width=0.7, color=['orange', 'red'])
    ax2.set_title('Average File Size by Method and Format')
    ax2.set_ylabel('Average Size (KB)')
    ax2.set_xlabel('Method')
    ax2.legend(title='Format')
    ax2.tick_params(axis='x', rotation=45)
    
    plt.tight_layout()
    plt.savefig(f'{output_dir}/feature_file_sizes.png', dpi=300, bbox_inches='tight')
    plt.close()

def plot_storage_efficiency(df):
    """Plot storage efficiency comparison"""
    print("Generating storage efficiency plot...")
    
    # Calculate compression ratio (text vs binary)
    text_data = df[df['Format'] == 'Text'].set_index('Method')
    binary_data = df[df['Format'] == 'Binary'].set_index('Method')
    
    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(15, 6))
    
    # Compression ratio
    compression_ratios = []
    methods_list = []
    
    for method in text_data.index:
        if method in binary_data.index:
            text_size = text_data.loc[method, 'Total_Size_MB']
            binary_size = binary_data.loc[method, 'Total_Size_MB']
            
            if binary_size > 0:
                ratio = text_size / binary_size
                compression_ratios.append(ratio)
                methods_list.append(method)
    
    if compression_ratios:
        bars = ax1.bar(methods_list, compression_ratios, alpha=0.7)
        ax1.set_title('Text vs Binary Storage Ratio')
        ax1.set_ylabel('Ratio (Text Size / Binary Size)')
        ax1.set_xlabel('Method')
        ax1.axhline(y=1, color='red', linestyle='--', alpha=0.7, label='Equal size')
        ax1.legend()
        
        # Add value labels
        for bar, ratio in zip(bars, compression_ratios):
            ax1.text(bar.get_x() + bar.get_width()/2, bar.get_height() + 0.05,
                    f'{ratio:.2f}x', ha='center', va='bottom')
    
    # File count comparison
    file_counts = df.pivot(index='Method', columns='Format', values='File_Count')
    file_counts.plot(kind='bar', ax=ax2, width=0.7)
    ax2.set_title('Number of Feature Files Generated')
    ax2.set_ylabel('File Count')
    ax2.set_xlabel('Method')
    ax2.legend(title='Format')
    ax2.tick_params(axis='x', rotation=45)
    
    plt.tight_layout()
    plt.savefig(f'{output_dir}/storage_efficiency.png', dpi=300, bbox_inches='tight')
    plt.close()

def main():
    print("Analyzing feature extraction results...")
    df = analyze_feature_files()
    
    if df.empty:
        print("No feature extraction data found.")
        return 1
    
    print(f"Found feature data for {len(df)} configurations")
    print(df.to_string(index=False))
    
    plot_file_sizes(df)
    plot_storage_efficiency(df)
    
    print(f"\nFeature analysis plots saved to: {output_dir}/")
    return 0

if __name__ == "__main__":
    main()
EOF

    python3 "$plot_script"
    
    if [ $? -eq 0 ]; then
        print_success "Feature analysis plots generated successfully"
    else
        print_error "Failed to generate feature analysis plots"
        return 1
    fi
}

# Function to generate comprehensive summary report
generate_summary_report() {
    print_step "5" "Generating comprehensive summary report"
    
    local report_file="$TEST_OUTPUT_DIR/plot_generation_report.txt"
    
    cat > "$report_file" << EOF
========================================
PLOT GENERATION SUMMARY REPORT
========================================
Generated: $(date)
Dataset: $DATASET

PLOTS CREATED:
1. Accuracy Analysis Plots:
   - accuracy_heatmap.png: Heatmap showing Top-1 accuracy across all combinations
   - method_comparison.png: Detailed comparison of Spectral vs MaxFreq methods
   - compressor_ranking.png: Ranking of compressors by average performance
   - format_comparison.png: Text vs Binary format performance analysis
   - noise_comparison.png: Analysis of noise type impact on identification accuracy
   - performance_overview.png: Comprehensive dashboard with all key metrics

2. Feature Analysis Plots:
   - feature_file_sizes.png: Storage size comparison by method and format
   - storage_efficiency.png: Efficiency analysis and compression ratios

PLOT DESCRIPTIONS:

Accuracy Heatmap:
- Shows Top-1 accuracy percentages in a grid format
- Rows: Method/Format combinations
- Columns: Compressors
- Colors: Red indicates higher accuracy

Method Comparison:
- Compares Spectral vs MaxFreq methods
- Separate plots for Text and Binary formats
- Shows both Top-1 and Top-5 accuracy metrics
- Grouped bar charts for easy comparison

Compressor Ranking:
- Bar charts showing average performance of each compressor
- Error bars indicate standard deviation
- Separate charts for Top-1 and Top-5 accuracy
- Value labels on bars for precise readings

Format Comparison:
- Analyzes Text vs Binary format performance
- Separate analysis for each method
- Side-by-side comparison of accuracy metrics
- Helps identify optimal format for each method

Performance Overview:
- Comprehensive dashboard with multiple visualizations
- Accuracy distribution histogram
- Top 8 performing configurations
- Heatmaps for method and format comparisons
- Summary statistics and best configuration

Noise Impact Analysis:
- Average accuracy by noise type (clean, white, brown, pink)
- Noise impact broken down by method and compressor
- Configuration vs noise type heatmap
- Helps understand system robustness to audio quality

Feature File Analysis:
- Storage size comparison between methods and formats
- Average file size analysis
- Compression ratio calculation (Text vs Binary)
- File count verification

USAGE:
All plots are saved as high-resolution PNG files (300 DPI) in:
$TEST_OUTPUT_DIR/

EOF

    print_success "Summary report saved to: $report_file"
    
    # List all generated plots
    echo
    print_info "=== GENERATED PLOTS ==="
    if [ -d "$TEST_OUTPUT_DIR" ]; then
        find "$TEST_OUTPUT_DIR" -name "*.png" -type f | while read -r plot_file; do
            local filename=$(basename "$plot_file")
            local filesize=$(du -h "$plot_file" | cut -f1)
            print_success "$filename ($filesize)"
        done
    fi
}

# Function to create an HTML summary page
create_html_summary() {
    print_step "6" "Creating HTML summary page"
    
    local html_file="$TEST_OUTPUT_DIR/plot_summary.html"
    
    cat > "$html_file" << EOF
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Music Identification System - Test Results</title>
    <style>
        body {
            font-family: Arial, sans-serif;
            margin: 20px;
            background-color: #f5f5f5;
        }
        .container {
            max-width: 1200px;
            margin: 0 auto;
            background-color: white;
            padding: 20px;
            border-radius: 10px;
            box-shadow: 0 0 10px rgba(0,0,0,0.1);
        }
        h1 {
            color: #333;
            text-align: center;
            border-bottom: 2px solid #4CAF50;
            padding-bottom: 10px;
        }
        h2 {
            color: #4CAF50;
            border-left: 4px solid #4CAF50;
            padding-left: 10px;
        }
        .plot-section {
            margin: 30px 0;
            padding: 20px;
            border: 1px solid #ddd;
            border-radius: 5px;
            background-color: #fafafa;
        }
        .plot-grid {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(400px, 1fr));
            gap: 20px;
            margin: 20px 0;
        }
        .plot-item {
            text-align: center;
            background-color: white;
            padding: 15px;
            border-radius: 5px;
            box-shadow: 0 2px 5px rgba(0,0,0,0.1);
        }
        .plot-item img {
            max-width: 100%;
            height: auto;
            border: 1px solid #ddd;
            border-radius: 5px;
        }
        .plot-item h3 {
            margin-top: 10px;
            color: #333;
        }
        .info-box {
            background-color: #e7f3ff;
            border: 1px solid #b3d9ff;
            border-radius: 5px;
            padding: 15px;
            margin: 20px 0;
        }
        .generated-date {
            text-align: center;
            color: #666;
            font-style: italic;
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>Music Identification System - Test Results</h1>
        <p class="generated-date">Generated on $(date)</p>
        
        <div class="info-box">
            <strong>Dataset:</strong> $DATASET<br>
            <strong>Methods tested:</strong> ${METHODS[*]}<br>
            <strong>Formats tested:</strong> ${FORMATS[*]}<br>
            <strong>Compressors tested:</strong> ${COMPRESSORS[*]}
        </div>

        <div class="plot-section">
            <h2>🎯 Accuracy Analysis</h2>
            <p>These plots show the performance of different parameter combinations in terms of music identification accuracy.</p>
            
            <div class="plot-grid">
                <div class="plot-item">
                    <img src="performance_overview.png" alt="Performance Overview">
                    <h3>Performance Overview</h3>
                    <p>Comprehensive dashboard showing all key metrics and best configurations.</p>
                </div>
                
                <div class="plot-item">
                    <img src="accuracy_heatmap.png" alt="Accuracy Heatmap">
                    <h3>Accuracy Heatmap</h3>
                    <p>Top-1 accuracy across all method/format/compressor combinations.</p>
                </div>
                
                <div class="plot-item">
                    <img src="method_comparison.png" alt="Method Comparison">
                    <h3>Method Comparison</h3>
                    <p>Spectral vs MaxFreq method performance comparison.</p>
                </div>
                
                <div class="plot-item">
                    <img src="compressor_ranking.png" alt="Compressor Ranking">
                    <h3>Compressor Ranking</h3>
                    <p>Average performance ranking of compression algorithms.</p>
                </div>
                
                <div class="plot-item">
                    <img src="format_comparison.png" alt="Format Comparison">
                    <h3>Format Comparison</h3>
                    <p>Text vs Binary format performance analysis.</p>
                </div>
                
                <div class="plot-item">
                    <img src="noise_comparison.png" alt="Noise Comparison">
                    <h3>Noise Impact Analysis</h3>
                    <p>Analysis of how different noise types affect identification accuracy.</p>
                </div>
            </div>
        </div>

        <div class="plot-section">
            <h2>💾 Storage Analysis</h2>
            <p>These plots analyze the storage requirements and efficiency of different feature extraction methods.</p>
            
            <div class="plot-grid">
                <div class="plot-item">
                    <img src="feature_file_sizes.png" alt="Feature File Sizes">
                    <h3>Feature File Sizes</h3>
                    <p>Storage size comparison between methods and formats.</p>
                </div>
                
                <div class="plot-item">
                    <img src="storage_efficiency.png" alt="Storage Efficiency">
                    <h3>Storage Efficiency</h3>
                    <p>Compression ratios and storage efficiency analysis.</p>
                </div>
            </div>
        </div>

        <div class="info-box">
            <h3>📊 Key Insights</h3>
            <p>Use these visualizations to:</p>
            <ul>
                <li>Identify the best performing method/format/compressor combination</li>
                <li>Understand trade-offs between accuracy and storage requirements</li>
                <li>Compare different feature extraction approaches</li>
                <li>Make informed decisions for production deployment</li>
            </ul>
        </div>
    </div>
</body>
</html>
EOF

    print_success "HTML summary created: $html_file"
    print_info "Open $html_file in a web browser to view all plots"
}

# Main execution
print_info "Starting plot generation for dataset: $DATASET"

# Run all plot generation steps
if check_python_packages; then
    generate_accuracy_plots
    generate_feature_plots
    generate_summary_report
    create_html_summary
    
    echo
    print_info "=== PLOT GENERATION COMPLETE ==="
    print_success "All plots saved in: $TEST_OUTPUT_DIR"
    print_info "View HTML summary: $TEST_OUTPUT_DIR/plot_summary.html"
    print_success "Plot generation completed successfully!"
else
    print_error "Cannot generate plots without required Python packages"
    exit 1
fi
