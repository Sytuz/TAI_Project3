"""
Visualization script for parameter evaluation results.
Creates plots and heatmaps to analyze the impact of different parameters.
"""

import json
import numpy as np
import matplotlib.pyplot as plt
import seaborn as sns
import pandas as pd
import argparse
from pathlib import Path

def load_results(results_file):
    """Load evaluation results from JSON file."""
    with open(results_file, 'r') as f:
        data = json.load(f)
    return data['results']

def create_accuracy_comparison_plot(results, output_dir):
    """Create bar plot comparing Top-1, Top-5, and Top-10 accuracies."""
    # Convert to DataFrame
    df = pd.DataFrame(results)
    
    # Sort by Top-1 accuracy
    df = df.sort_values('top1_accuracy', ascending=True)
    
    # Take top 15 configurations for readability
    df_top = df.tail(15)
    
    fig, ax = plt.subplots(figsize=(12, 8))
    
    x = np.arange(len(df_top))
    width = 0.25
    
    ax.barh(x - width, df_top['top1_accuracy'], width, label='Top-1', alpha=0.8)
    ax.barh(x, df_top['top5_accuracy'], width, label='Top-5', alpha=0.8)
    ax.barh(x + width, df_top['top10_accuracy'], width, label='Top-10', alpha=0.8)
    
    # Create labels with configuration info
    labels = []
    for _, row in df_top.iterrows():
        label = f"{row['method']} (f:{row['numFrequencies']}, b:{row['numBins']}, fs:{row['frameSize']})"
        labels.append(label)
    
    ax.set_yticks(x)
    ax.set_yticklabels(labels, fontsize=8)
    ax.set_xlabel('Accuracy (%)')
    ax.set_title('Top Performing Configurations - Accuracy Comparison')
    ax.legend()
    ax.grid(True, alpha=0.3)
    
    plt.tight_layout()
    plt.savefig(output_dir / 'accuracy_comparison.png', dpi=300, bbox_inches='tight')
    plt.close()

def create_method_comparison_plot(results, output_dir):
    """Create box plot comparing methods."""
    df = pd.DataFrame(results)
    
    fig, axes = plt.subplots(1, 3, figsize=(15, 5))
    
    metrics = ['top1_accuracy', 'top5_accuracy', 'top10_accuracy']
    titles = ['Top-1 Accuracy', 'Top-5 Accuracy', 'Top-10 Accuracy']
    
    for i, (metric, title) in enumerate(zip(metrics, titles)):
        sns.boxplot(data=df, x='method', y=metric, ax=axes[i])
        axes[i].set_title(title)
        axes[i].set_ylabel('Accuracy (%)')
        axes[i].grid(True, alpha=0.3)
    
    plt.tight_layout()
    plt.savefig(output_dir / 'method_comparison.png', dpi=300, bbox_inches='tight')
    plt.close()

def create_parameter_heatmap(results, parameter, output_dir):
    """Create heatmap for a specific parameter's impact."""
    df = pd.DataFrame(results)
    
    if parameter not in df.columns:
        print(f"Parameter {parameter} not found in results")
        return
    
    # Create pivot table
    pivot = df.pivot_table(values='top1_accuracy', 
                          index='method', 
                          columns=parameter, 
                          aggfunc='mean')
    
    plt.figure(figsize=(10, 6))
    sns.heatmap(pivot, annot=True, fmt='.1f', cmap='viridis', 
                cbar_kws={'label': 'Top-1 Accuracy (%)'})
    plt.title(f'Top-1 Accuracy vs {parameter}')
    plt.tight_layout()
    plt.savefig(output_dir / f'heatmap_{parameter}.png', dpi=300, bbox_inches='tight')
    plt.close()

def create_frame_size_analysis(results, output_dir):
    """Analyze impact of frame size and hop size."""
    df = pd.DataFrame(results)
    
    fig, axes = plt.subplots(2, 2, figsize=(15, 10))
    
    # Frame size vs accuracy
    for method in df['method'].unique():
        method_data = df[df['method'] == method]
        frame_acc = method_data.groupby('frameSize')['top1_accuracy'].mean()
        axes[0,0].plot(frame_acc.index, frame_acc.values, marker='o', label=method)
    
    axes[0,0].set_xlabel('Frame Size')
    axes[0,0].set_ylabel('Top-1 Accuracy (%)')
    axes[0,0].set_title('Frame Size vs Accuracy')
    axes[0,0].legend()
    axes[0,0].grid(True, alpha=0.3)
    
    # Hop size vs accuracy
    for method in df['method'].unique():
        method_data = df[df['method'] == method]
        hop_acc = method_data.groupby('hopSize')['top1_accuracy'].mean()
        axes[0,1].plot(hop_acc.index, hop_acc.values, marker='o', label=method)
    
    axes[0,1].set_xlabel('Hop Size')
    axes[0,1].set_ylabel('Top-1 Accuracy (%)')
    axes[0,1].set_title('Hop Size vs Accuracy')
    axes[0,1].legend()
    axes[0,1].grid(True, alpha=0.3)
    
    # Frequencies vs accuracy (MaxFreq method)
    maxfreq_data = df[df['method'] == 'maxfreq']
    if not maxfreq_data.empty:
        freq_acc = maxfreq_data.groupby('numFrequencies')['top1_accuracy'].mean()
        axes[1,0].plot(freq_acc.index, freq_acc.values, marker='o', color='blue')
        axes[1,0].set_xlabel('Number of Frequencies')
        axes[1,0].set_ylabel('Top-1 Accuracy (%)')
        axes[1,0].set_title('Number of Frequencies vs Accuracy (MaxFreq)')
        axes[1,0].grid(True, alpha=0.3)
    
    # Bins vs accuracy (Spectral method)
    spectral_data = df[df['method'] == 'spectral']
    if not spectral_data.empty:
        bins_acc = spectral_data.groupby('numBins')['top1_accuracy'].mean()
        axes[1,1].plot(bins_acc.index, bins_acc.values, marker='o', color='red')
        axes[1,1].set_xlabel('Number of Bins')
        axes[1,1].set_ylabel('Top-1 Accuracy (%)')
        axes[1,1].set_title('Number of Bins vs Accuracy (Spectral)')
        axes[1,1].grid(True, alpha=0.3)
    
    plt.tight_layout()
    plt.savefig(output_dir / 'parameter_analysis.png', dpi=300, bbox_inches='tight')
    plt.close()

def create_compressor_comparison(results, output_dir):
    """Compare different compressors."""
    df = pd.DataFrame(results)
    
    if 'compressor' not in df.columns or len(df['compressor'].unique()) < 2:
        print("Not enough compressor data for comparison")
        return
    
    fig, axes = plt.subplots(1, 2, figsize=(12, 5))
    
    # Box plot
    sns.boxplot(data=df, x='compressor', y='top1_accuracy', ax=axes[0])
    axes[0].set_title('Compressor Performance Comparison')
    axes[0].set_ylabel('Top-1 Accuracy (%)')
    axes[0].grid(True, alpha=0.3)
    
    # Average accuracy by compressor
    comp_avg = df.groupby('compressor')['top1_accuracy'].mean().sort_values(ascending=True)
    axes[1].barh(range(len(comp_avg)), comp_avg.values)
    axes[1].set_yticks(range(len(comp_avg)))
    axes[1].set_yticklabels(comp_avg.index)
    axes[1].set_xlabel('Average Top-1 Accuracy (%)')
    axes[1].set_title('Average Performance by Compressor')
    axes[1].grid(True, alpha=0.3)
    
    plt.tight_layout()
    plt.savefig(output_dir / 'compressor_comparison.png', dpi=300, bbox_inches='tight')
    plt.close()

def generate_detailed_report(results, output_dir):
    """Generate a detailed text report."""
    df = pd.DataFrame(results)
    
    report_file = output_dir / 'detailed_analysis_report.txt'
    
    with open(report_file, 'w') as f:
        f.write("DETAILED PARAMETER EVALUATION REPORT\n")
        f.write("="*50 + "\n\n")
        
        # Overall statistics
        f.write("OVERALL STATISTICS:\n")
        f.write("-"*20 + "\n")
        f.write(f"Total configurations tested: {len(df)}\n")
        f.write(f"Methods tested: {', '.join(df['method'].unique())}\n")
        f.write(f"Compressors tested: {', '.join(df['compressor'].unique())}\n")
        f.write(f"Best Top-1 accuracy: {df['top1_accuracy'].max():.1f}%\n")
        f.write(f"Average Top-1 accuracy: {df['top1_accuracy'].mean():.1f}%\n")
        f.write(f"Standard deviation: {df['top1_accuracy'].std():.1f}%\n\n")
        
        # Best configurations
        f.write("TOP 10 CONFIGURATIONS:\n")
        f.write("-"*25 + "\n")
        top_configs = df.nlargest(10, 'top1_accuracy')
        for i, (_, config) in enumerate(top_configs.iterrows(), 1):
            f.write(f"{i}. {config['method']} - ")
            f.write(f"Freq: {config['numFrequencies']}, Bins: {config['numBins']}, ")
            f.write(f"Frame: {config['frameSize']}, Hop: {config['hopSize']}, ")
            f.write(f"Comp: {config['compressor']} - ")
            f.write(f"Acc: {config['top1_accuracy']:.1f}%\n")
        f.write("\n")
        
        # Method comparison
        f.write("METHOD COMPARISON:\n")
        f.write("-"*18 + "\n")
        method_stats = df.groupby('method')[['top1_accuracy', 'top5_accuracy', 'top10_accuracy']].agg(['mean', 'std'])
        for method in df['method'].unique():
            stats = method_stats.loc[method]
            f.write(f"{method.upper()}:\n")
            f.write(f"  Top-1: {stats[('top1_accuracy', 'mean')]:.1f}% ± {stats[('top1_accuracy', 'std')]:.1f}%\n")
            f.write(f"  Top-5: {stats[('top5_accuracy', 'mean')]:.1f}% ± {stats[('top5_accuracy', 'std')]:.1f}%\n")
            f.write(f"  Top-10: {stats[('top10_accuracy', 'mean')]:.1f}% ± {stats[('top10_accuracy', 'std')]:.1f}%\n\n")
        
        # Parameter recommendations
        f.write("PARAMETER RECOMMENDATIONS:\n")
        f.write("-"*26 + "\n")
        
        best_overall = df.loc[df['top1_accuracy'].idxmax()]
        f.write("Best overall configuration:\n")
        f.write(f"  Method: {best_overall['method']}\n")
        f.write(f"  Frequencies: {best_overall['numFrequencies']}\n")
        f.write(f"  Bins: {best_overall['numBins']}\n")
        f.write(f"  Frame Size: {best_overall['frameSize']}\n")
        f.write(f"  Hop Size: {best_overall['hopSize']}\n")
        f.write(f"  Compressor: {best_overall['compressor']}\n")
        f.write(f"  Accuracy: {best_overall['top1_accuracy']:.1f}%\n\n")
        
        # Best by method
        for method in df['method'].unique():
            method_best = df[df['method'] == method].loc[df[df['method'] == method]['top1_accuracy'].idxmax()]
            f.write(f"Best {method} configuration:\n")
            f.write(f"  Frequencies: {method_best['numFrequencies']}\n")
            f.write(f"  Bins: {method_best['numBins']}\n")
            f.write(f"  Frame Size: {method_best['frameSize']}\n")
            f.write(f"  Hop Size: {method_best['hopSize']}\n")
            f.write(f"  Accuracy: {method_best['top1_accuracy']:.1f}%\n\n")
    
    print(f"Detailed report saved to: {report_file}")

def main():
    parser = argparse.ArgumentParser(description='Visualize parameter evaluation results')
    parser.add_argument('results_file', help='JSON file with evaluation results')
    parser.add_argument('-o', '--output-dir', default='parameter_plots',
                       help='Output directory for plots')
    
    args = parser.parse_args()
    
    if not Path(args.results_file).exists():
        print(f"Error: Results file not found: {args.results_file}")
        return
    
    output_dir = Path(args.output_dir)
    output_dir.mkdir(exist_ok=True)
    
    print(f"Loading results from: {args.results_file}")
    results = load_results(args.results_file)
    
    if not results:
        print("No results found in file")
        return
    
    print(f"Creating visualizations for {len(results)} configurations...")
    
    # Set style for better plots
    plt.style.use('seaborn-v0_8')
    
    # Create all visualizations
    create_accuracy_comparison_plot(results, output_dir)
    print("- Created accuracy comparison plot")
    
    create_method_comparison_plot(results, output_dir)
    print("- Created method comparison plot")
    
    create_frame_size_analysis(results, output_dir)
    print("- Created parameter analysis plot")
    
    create_compressor_comparison(results, output_dir)
    print("- Created compressor comparison plot")
    
    # Create heatmaps for key parameters
    for param in ['numFrequencies', 'numBins', 'frameSize']:
        create_parameter_heatmap(results, param, output_dir)
        print(f"- Created {param} heatmap")
    
    # Generate detailed report
    generate_detailed_report(results, output_dir)
    print("- Generated detailed analysis report")
    
    print(f"\nAll visualizations saved to: {output_dir}")

if __name__ == "__main__":
    main()
