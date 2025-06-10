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
        
        sns.heatmap(pivot_data, annot=True, fmt='.1f', cmap='YlOrRd', 
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
    
    print(f"\nAll plots saved to: {output_dir}/")
    return 0

if __name__ == "__main__":
    sys.exit(main())
