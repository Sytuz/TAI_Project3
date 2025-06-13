#!/usr/bin/env python3
"""
Visualization script for music identification test results.
"""

import json
import numpy as np
import matplotlib.pyplot as plt
import seaborn as sns
import pandas as pd
from pathlib import Path
import os
import csv
from collections import defaultdict
import glob

class MusicIdentificationVisualizer:
    def __init__(self, results_dir, output_dir):
        self.results_dir = Path(results_dir)
        self.output_dir = Path(output_dir)
        self.output_dir.mkdir(parents=True, exist_ok=True)
        
        # Configuration
        self.methods = ["maxfreq", "spectral"]
        self.formats = ["text", "binary"]
        self.noises = ["clean", "brown", "pink", "white"]
        self.compressors = ["gzip", "bzip2", "lzma", "zstd"]
        self.dataset = "youtube"
        
        # Data storage
        self.results_data = []
        
    def load_all_results(self):
        """Load all accuracy results from the directory structure."""
        print("Loading results from all combinations...")
        
        pattern = str(self.results_dir / "compressors" / self.dataset / "*" / "*" / "*" / "accuracy_metrics_*.json")
        result_files = glob.glob(pattern)
        
        print(pattern)
        print(result_files)
        
        print(f"Found {len(result_files)} result files")
        
        for result_file in result_files:
            try:
                # Parse path to extract configuration
                parts = Path(result_file).parts
                if len(parts) >= 8:
                    method = parts[-5]
                    format_type = parts[-4]
                    noise = parts[-3]
                    filename = Path(result_file).name
                    compressor = filename.replace("accuracy_metrics_", "").replace(".json", "")
                    
                    # Load the JSON data
                    with open(result_file, 'r') as f:
                        data = json.load(f)
                    
                    # Store the result
                    result_entry = {
                        'method': method,
                        'format': format_type,
                        'noise': noise,
                        'compressor': compressor,
                        'top1_accuracy': data.get('top1_accuracy', 0),
                        'top5_accuracy': data.get('top5_accuracy', 0),
                        'top10_accuracy': data.get('top10_accuracy', 0),
                        'total_queries': data.get('total_queries', 0),
                        'top1_correct': data.get('top1_correct', 0),
                        'top5_correct': data.get('top5_correct', 0),
                        'top10_correct': data.get('top10_correct', 0)
                    }
                    
                    self.results_data.append(result_entry)
                    print(f"Loaded: {method}/{format_type}/{noise}/{compressor} - Top-1: {result_entry['top1_accuracy']:.1f}%")
                    
            except Exception as e:
                print(f"Error loading {result_file}: {e}")
        
        print(f"Successfully loaded {len(self.results_data)} results")
        return len(self.results_data) > 0

    def create_accuracy_heatmaps(self):
        """Create accuracy heatmaps for each noise type."""
        print("Creating accuracy heatmaps...")
        
        fig, axes = plt.subplots(2, 2, figsize=(16, 12))
        axes = axes.flatten()
        
        for i, noise in enumerate(self.noises):
            ax = axes[i]
            
            # Filter data for this noise type
            noise_data = [r for r in self.results_data if r['noise'] == noise]
            
            if not noise_data:
                ax.set_title(f'{noise.title()} Noise - No Data')
                continue
            
            # Create matrix: rows = method_format combinations, cols = compressors
            combinations = []
            for method in self.methods:
                for format_type in self.formats:
                    combinations.append(f"{method}_{format_type}")
            
            matrix = np.zeros((len(combinations), len(self.compressors)))
            
            for j, combo in enumerate(combinations):
                method, format_type = combo.split('_')
                for k, compressor in enumerate(self.compressors):
                    # Find matching result
                    matches = [r for r in noise_data 
                             if r['method'] == method and r['format'] == format_type and r['compressor'] == compressor]
                    if matches:
                        matrix[j, k] = matches[0]['top1_accuracy']
            
            # Create heatmap
            im = ax.imshow(matrix, cmap='RdYlGn', aspect='auto', vmin=0, vmax=100)
            
            # Set labels
            ax.set_xticks(range(len(self.compressors)))
            ax.set_xticklabels(self.compressors)
            ax.set_yticks(range(len(combinations)))
            ax.set_yticklabels([c.replace('_', ' ') for c in combinations])
            ax.set_title(f'{noise.title()} Noise - Top-1 Accuracy (%)')
            
            # Add text annotations
            for row in range(len(combinations)):
                for col in range(len(self.compressors)):
                    if matrix[row, col] > 0:
                        ax.text(col, row, f'{matrix[row, col]:.1f}', 
                               ha="center", va="center", color="black", fontweight='bold')
        
        plt.tight_layout()
        plt.savefig(self.output_dir / 'accuracy_heatmap.png', dpi=300, bbox_inches='tight')
        plt.close()
        print("✓ Created accuracy_heatmap.png")

    def create_compressor_ranking(self):
        """Create compressor ranking with min/max bars."""
        print("Creating compressor ranking...")
        
        df = pd.DataFrame(self.results_data)
        
        fig, axes = plt.subplots(1, 3, figsize=(15, 5))
        
        metrics = ['top1_accuracy', 'top5_accuracy', 'top10_accuracy']
        titles = ['Top-1 Accuracy', 'Top-5 Accuracy', 'Top-10 Accuracy']
        
        for i, (metric, title) in enumerate(zip(metrics, titles)):
            ax = axes[i]
            
            # Calculate statistics by compressor
            stats = df.groupby('compressor')[metric].agg(['mean', 'min', 'max']).reset_index()
            stats = stats.sort_values('mean', ascending=True)
            
            # Create bar plot
            bars = ax.barh(range(len(stats)), stats['mean'], color='skyblue', alpha=0.8)
            
            # Add min/max lines
            for j, (_, row) in enumerate(stats.iterrows()):
                ax.plot([row['min'], row['max']], [j, j], 'k-', linewidth=2)
                ax.plot([row['min'], row['min']], [j-0.1, j+0.1], 'k-', linewidth=2)
                ax.plot([row['max'], row['max']], [j-0.1, j+0.1], 'k-', linewidth=2)
            
            ax.set_yticks(range(len(stats)))
            ax.set_yticklabels(stats['compressor'])
            ax.set_xlabel('Accuracy (%)')
            ax.set_title(title)
            ax.grid(True, alpha=0.3)
            
            # Add value labels
            for j, bar in enumerate(bars):
                width = bar.get_width()
                ax.text(width + 1, bar.get_y() + bar.get_height()/2, 
                       f'{width:.1f}%', ha='left', va='center')
        
        plt.tight_layout()
        plt.savefig(self.output_dir / 'compressor_ranking.png', dpi=300, bbox_inches='tight')
        plt.close()
        print("✓ Created compressor_ranking.png")

    def create_format_comparison(self):
        """Create format comparison plots."""
        print("Creating format comparison...")
        
        df = pd.DataFrame(self.results_data)
        
        fig, axes = plt.subplots(2, 2, figsize=(16, 12))
        
        plot_configs = [
            ('spectral', 'top1_accuracy', 'Spectral - Top-1 Accuracy'),
            ('spectral', 'top5_accuracy', 'Spectral - Top-5 Accuracy'),
            ('maxfreq', 'top1_accuracy', 'MaxFreq - Top-1 Accuracy'),
            ('maxfreq', 'top5_accuracy', 'MaxFreq - Top-5 Accuracy')
        ]
        
        for idx, (method, metric, title) in enumerate(plot_configs):
            ax = axes[idx // 2, idx % 2]
            
            # Filter data for this method
            method_data = df[df['method'] == method]
            
            if method_data.empty:
                ax.set_title(f'{title} - No Data')
                continue
            
            # Group by format and compressor
            grouped = method_data.groupby(['format', 'compressor'])[metric].mean().reset_index()
            
            # Pivot for plotting
            pivot_data = grouped.pivot(index='compressor', columns='format', values=metric)
            
            # Create grouped bar plot
            x = np.arange(len(pivot_data.index))
            width = 0.35
            
            if 'text' in pivot_data.columns:
                ax.bar(x - width/2, pivot_data['text'], width, label='Text', alpha=0.8)
            if 'binary' in pivot_data.columns:
                ax.bar(x + width/2, pivot_data['binary'], width, label='Binary', alpha=0.8)
            
            ax.set_xlabel('Compressor')
            ax.set_ylabel('Accuracy (%)')
            ax.set_title(title)
            ax.set_xticks(x)
            ax.set_xticklabels(pivot_data.index)
            ax.legend()
            ax.grid(True, alpha=0.3)
        
        plt.tight_layout()
        plt.savefig(self.output_dir / 'format_comparison.png', dpi=300, bbox_inches='tight')
        plt.close()
        print("✓ Created format_comparison.png")

    def create_method_comparison(self):
        """Create method comparison plots."""
        print("Creating method comparison...")
        
        df = pd.DataFrame(self.results_data)
        
        fig, axes = plt.subplots(2, 2, figsize=(16, 12))
        
        plot_configs = [
            ('text', 'top1_accuracy', 'Text - Top-1 Accuracy'),
            ('text', 'top5_accuracy', 'Text - Top-5 Accuracy'),
            ('binary', 'top1_accuracy', 'Binary - Top-1 Accuracy'),
            ('binary', 'top5_accuracy', 'Binary - Top-5 Accuracy')
        ]
        
        for idx, (format_type, metric, title) in enumerate(plot_configs):
            ax = axes[idx // 2, idx % 2]
            
            # Filter data for this format
            format_data = df[df['format'] == format_type]
            
            if format_data.empty:
                ax.set_title(f'{title} - No Data')
                continue
            
            # Group by method and compressor
            grouped = format_data.groupby(['method', 'compressor'])[metric].mean().reset_index()
            
            # Pivot for plotting
            pivot_data = grouped.pivot(index='compressor', columns='method', values=metric)
            
            # Create grouped bar plot
            x = np.arange(len(pivot_data.index))
            width = 0.35
            
            if 'maxfreq' in pivot_data.columns:
                ax.bar(x - width/2, pivot_data['maxfreq'], width, label='MaxFreq', alpha=0.8)
            if 'spectral' in pivot_data.columns:
                ax.bar(x + width/2, pivot_data['spectral'], width, label='Spectral', alpha=0.8)
            
            ax.set_xlabel('Compressor')
            ax.set_ylabel('Accuracy (%)')
            ax.set_title(title)
            ax.set_xticks(x)
            ax.set_xticklabels(pivot_data.index)
            ax.legend()
            ax.grid(True, alpha=0.3)
        
        plt.tight_layout()
        plt.savefig(self.output_dir / 'method_comparison.png', dpi=300, bbox_inches='tight')
        plt.close()
        print("✓ Created method_comparison.png")

    def create_noise_comparison(self):
        """Create noise comparison plots."""
        print("Creating noise comparison...")
        
        df = pd.DataFrame(self.results_data)
        
        fig, axes = plt.subplots(2, 2, figsize=(16, 12))
        
        # Plot 1: Average top-1 accuracy by noise with min/max
        ax = axes[0, 0]
        noise_stats = df.groupby('noise')['top1_accuracy'].agg(['mean', 'min', 'max']).reset_index()
        
        bars = ax.bar(range(len(noise_stats)), noise_stats['mean'], color='lightblue', alpha=0.8)
        
        for i, (_, row) in enumerate(noise_stats.iterrows()):
            ax.plot([i, i], [row['min'], row['max']], 'k-', linewidth=2)
            ax.plot([i-0.1, i+0.1], [row['min'], row['min']], 'k-', linewidth=2)
            ax.plot([i-0.1, i+0.1], [row['max'], row['max']], 'k-', linewidth=2)
        
        ax.set_xticks(range(len(noise_stats)))
        ax.set_xticklabels(noise_stats['noise'])
        ax.set_ylabel('Top-1 Accuracy (%)')
        ax.set_title('Average Top-1 Accuracy by Noise Type')
        ax.grid(True, alpha=0.3)
        
        # Plot 2: Noise impact by method
        ax = axes[0, 1]
        method_noise = df.groupby(['noise', 'method'])['top1_accuracy'].mean().reset_index()
        method_pivot = method_noise.pivot(index='noise', columns='method', values='top1_accuracy')
        
        x = np.arange(len(method_pivot.index))
        width = 0.35
        
        if 'maxfreq' in method_pivot.columns:
            ax.bar(x - width/2, method_pivot['maxfreq'], width, label='MaxFreq', alpha=0.8)
        if 'spectral' in method_pivot.columns:
            ax.bar(x + width/2, method_pivot['spectral'], width, label='Spectral', alpha=0.8)
        
        ax.set_xticks(x)
        ax.set_xticklabels(method_pivot.index)
        ax.set_ylabel('Top-1 Accuracy (%)')
        ax.set_title('Noise Impact by Method')
        ax.legend()
        ax.grid(True, alpha=0.3)
        
        # Plot 3: Noise impact by compressor
        ax = axes[1, 0]
        comp_noise = df.groupby(['noise', 'compressor'])['top1_accuracy'].mean().reset_index()
        comp_pivot = comp_noise.pivot(index='noise', columns='compressor', values='top1_accuracy')
        
        comp_pivot.plot(kind='bar', ax=ax, alpha=0.8)
        ax.set_ylabel('Top-1 Accuracy (%)')
        ax.set_title('Noise Impact by Compressor')
        ax.legend(title='Compressor')
        ax.grid(True, alpha=0.3)
        plt.setp(ax.get_xticklabels(), rotation=45)
        
        # Plot 4: Configuration vs noise heatmap
        ax = axes[1, 1]
        config_noise = df.groupby(['noise', 'method', 'format'])['top1_accuracy'].mean().reset_index()
        
        # Create configuration labels
        configs = []
        for method in self.methods:
            for format_type in self.formats:
                configs.append(f"{method}_{format_type}")
        
        matrix = np.zeros((len(self.noises), len(configs)))
        
        for i, noise in enumerate(self.noises):
            for j, config in enumerate(configs):
                method, format_type = config.split('_')
                matches = config_noise[
                    (config_noise['noise'] == noise) & 
                    (config_noise['method'] == method) & 
                    (config_noise['format'] == format_type)
                ]
                if not matches.empty:
                    matrix[i, j] = matches['top1_accuracy'].iloc[0]
        
        im = ax.imshow(matrix, cmap='RdYlGn', aspect='auto')
        ax.set_xticks(range(len(configs)))
        ax.set_xticklabels([c.replace('_', '\n') for c in configs])
        ax.set_yticks(range(len(self.noises)))
        ax.set_yticklabels(self.noises)
        ax.set_title('Configuration vs Noise Heatmap')
        
        # Add text annotations
        for i in range(len(self.noises)):
            for j in range(len(configs)):
                if matrix[i, j] > 0:
                    ax.text(j, i, f'{matrix[i, j]:.1f}', 
                           ha="center", va="center", color="black", fontweight='bold')
        
        plt.tight_layout()
        plt.savefig(self.output_dir / 'noise_comparison.png', dpi=300, bbox_inches='tight')
        plt.close()
        print("✓ Created noise_comparison.png")

    def create_performance_overview(self):
        """Create performance overview plots."""
        print("Creating performance overview...")
        
        df = pd.DataFrame(self.results_data)
        
        fig, axes = plt.subplots(2, 2, figsize=(16, 12))
        
        # Plot 1: Distribution of top-1 accuracy scores
        ax = axes[0, 0]
        ax.hist(df['top1_accuracy'], bins=20, alpha=0.7, color='skyblue', edgecolor='black')
        ax.set_xlabel('Top-1 Accuracy (%)')
        ax.set_ylabel('Frequency')
        ax.set_title('Distribution of Top-1 Accuracy Scores')
        ax.grid(True, alpha=0.3)
        
        # Plot 2: Top-8 configurations
        ax = axes[0, 1]
        df['config'] = df['method'] + '_' + df['format'] + '_' + df['noise'] + '_' + df['compressor']
        top_configs = df.nlargest(8, 'top1_accuracy')[['config', 'top1_accuracy']]
        
        bars = ax.barh(range(len(top_configs)), top_configs['top1_accuracy'], color='lightgreen', alpha=0.8)
        ax.set_yticks(range(len(top_configs)))
        ax.set_yticklabels([c.replace('_', '\n') for c in top_configs['config']], fontsize=8)
        ax.set_xlabel('Top-1 Accuracy (%)')
        ax.set_title('Top-8 Configurations by Accuracy')
        ax.grid(True, alpha=0.3)
        
        # Add value labels
        for i, bar in enumerate(bars):
            width = bar.get_width()
            ax.text(width + 0.5, bar.get_y() + bar.get_height()/2, 
                   f'{width:.1f}%', ha='left', va='center')
        
        # Plot 3: Method vs Compressor heatmap
        ax = axes[1, 0]
        method_comp = df.groupby(['method', 'compressor'])['top1_accuracy'].mean().reset_index()
        method_pivot = method_comp.pivot(index='method', columns='compressor', values='top1_accuracy')
        
        im = ax.imshow(method_pivot.values, cmap='RdYlGn', aspect='auto')
        ax.set_xticks(range(len(method_pivot.columns)))
        ax.set_xticklabels(method_pivot.columns)
        ax.set_yticks(range(len(method_pivot.index)))
        ax.set_yticklabels(method_pivot.index)
        ax.set_title('Average Top-1 Accuracy: Method vs Compressor')
        
        # Add text annotations
        for i in range(len(method_pivot.index)):
            for j in range(len(method_pivot.columns)):
                value = method_pivot.iloc[i, j]
                if not np.isnan(value):
                    ax.text(j, i, f'{value:.1f}', ha="center", va="center", 
                           color="black", fontweight='bold')
        
        # Plot 4: Format vs Compressor heatmap
        ax = axes[1, 1]
        format_comp = df.groupby(['format', 'compressor'])['top1_accuracy'].mean().reset_index()
        format_pivot = format_comp.pivot(index='format', columns='compressor', values='top1_accuracy')
        
        im = ax.imshow(format_pivot.values, cmap='RdYlGn', aspect='auto')
        ax.set_xticks(range(len(format_pivot.columns)))
        ax.set_xticklabels(format_pivot.columns)
        ax.set_yticks(range(len(format_pivot.index)))
        ax.set_yticklabels(format_pivot.index)
        ax.set_title('Average Top-1 Accuracy: Format vs Compressor')
        
        # Add text annotations
        for i in range(len(format_pivot.index)):
            for j in range(len(format_pivot.columns)):
                value = format_pivot.iloc[i, j]
                if not np.isnan(value):
                    ax.text(j, i, f'{value:.1f}', ha="center", va="center", 
                           color="black", fontweight='bold')
        
        plt.tight_layout()
        plt.savefig(self.output_dir / 'performance_overview.png', dpi=300, bbox_inches='tight')
        plt.close()
        print("✓ Created performance_overview.png")

    def generate_summary_report(self):
        """Generate a comprehensive summary report."""
        print("Generating summary report...")
        
        df = pd.DataFrame(self.results_data)
        
        report_path = self.output_dir / 'visualization_summary.md'
        
        with open(report_path, 'w') as f:
            f.write("# Music Identification Test Results - Visualization Summary\n\n")
            f.write(f"**Generated:** {pd.Timestamp.now().strftime('%Y-%m-%d %H:%M:%S')}\n\n")
            
            f.write("## Dataset Summary\n\n")
            f.write(f"- **Total Configurations Tested:** {len(df)}\n")
            f.write(f"- **Methods:** {', '.join(self.methods)}\n")
            f.write(f"- **Formats:** {', '.join(self.formats)}\n")
            f.write(f"- **Noise Types:** {', '.join(self.noises)}\n")
            f.write(f"- **Compressors:** {', '.join(self.compressors)}\n\n")
            
            f.write("## Performance Summary\n\n")
            f.write(f"- **Best Top-1 Accuracy:** {df['top1_accuracy'].max():.1f}%\n")
            f.write(f"- **Average Top-1 Accuracy:** {df['top1_accuracy'].mean():.1f}%\n")
            f.write(f"- **Best Top-5 Accuracy:** {df['top5_accuracy'].max():.1f}%\n")
            f.write(f"- **Average Top-5 Accuracy:** {df['top5_accuracy'].mean():.1f}%\n")
            f.write(f"- **Best Top-10 Accuracy:** {df['top10_accuracy'].max():.1f}%\n")
            f.write(f"- **Average Top-10 Accuracy:** {df['top10_accuracy'].mean():.1f}%\n\n")
            
            # Best configuration
            best_config = df.loc[df['top1_accuracy'].idxmax()]
            f.write("## Best Configuration\n\n")
            f.write(f"- **Method:** {best_config['method']}\n")
            f.write(f"- **Format:** {best_config['format']}\n")
            f.write(f"- **Noise:** {best_config['noise']}\n")
            f.write(f"- **Compressor:** {best_config['compressor']}\n")
            f.write(f"- **Top-1 Accuracy:** {best_config['top1_accuracy']:.1f}%\n")
            f.write(f"- **Top-5 Accuracy:** {best_config['top5_accuracy']:.1f}%\n")
            f.write(f"- **Top-10 Accuracy:** {best_config['top10_accuracy']:.1f}%\n\n")
            
            f.write("## Generated Visualizations\n\n")
            f.write("1. **accuracy_heatmap.png** - Accuracy heatmaps for each noise type\n")
            f.write("2. **compressor_ranking.png** - Compressor performance comparison with min/max ranges\n")
            f.write("3. **format_comparison.png** - Text vs Binary format comparison by method\n")
            f.write("4. **method_comparison.png** - MaxFreq vs Spectral method comparison by format\n")
            f.write("5. **noise_comparison.png** - Noise impact analysis\n")
            f.write("6. **performance_overview.png** - Overall performance analysis\n\n")
            
            f.write("## Analysis Notes\n\n")
            f.write("- All plots show Top-1 accuracy unless otherwise specified\n")
            f.write("- Heatmaps use a red-yellow-green color scale (red=low, green=high)\n")
            f.write("- Error bars on bar plots indicate minimum and maximum values\n")
            f.write("- Results are averaged across all noise types unless analyzing noise impact specifically\n")
        
        print(f"✓ Created summary report: {report_path}")

    def create_all_visualizations(self):
        """Create all requested visualizations."""
        print("Starting comprehensive visualization generation...")
        
        if not self.load_all_results():
            print("Error: No results found to visualize")
            return False
        
        try:
            self.create_accuracy_heatmaps()
            self.create_compressor_ranking()
            self.create_format_comparison()
            self.create_method_comparison()
            self.create_noise_comparison()
            self.create_performance_overview()
            self.generate_summary_report()
            
            print(f"\n{'='*50}")
            print("✅ ALL VISUALIZATIONS COMPLETED SUCCESSFULLY!")
            print(f"{'='*50}")
            print(f"Output directory: {self.output_dir}")
            print("Generated files:")
            for plot_file in self.output_dir.glob("*.png"):
                print(f"  - {plot_file.name}")
            print(f"  - visualization_summary.md")
            
            return True
            
        except Exception as e:
            print(f"Error creating visualizations: {e}")
            import traceback
            traceback.print_exc()
            return False

def main():
    dataset_name = "youtube"
    results_dir = os.path.join(os.path.dirname(__file__), '..', 'results')
    output_dir = os.path.join(os.path.dirname(__file__), '..', 'results', 'visualizations')

    # Create visualizer and generate all plots
    visualizer = MusicIdentificationVisualizer(results_dir, output_dir)
    success = visualizer.create_all_visualizations()

    if success:
        print(f"\nVisualization generation completed! Check {output_dir}/ for all plots.")
    else:
        print("\nVisualization generation failed. Check the error messages above.")
        exit(1)

if __name__ == "__main__":
    main()