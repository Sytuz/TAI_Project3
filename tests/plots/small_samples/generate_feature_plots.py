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
