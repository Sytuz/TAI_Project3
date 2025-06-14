"""
Music Identification Results Visualization
Generates multiple types of plots for analyzing accuracy across different configurations.
"""

import os
import json
import numpy as np
import matplotlib.pyplot as plt
import seaborn as sns
from pathlib import Path
import argparse
from collections import defaultdict
import warnings
warnings.filterwarnings('ignore')

# Set style
plt.style.use('default')
sns.set_palette("husl")

class ResultsAnalyzer:
    def __init__(self, results_dir, output_dir, dataset='youtube'):
        self.results_dir = Path(results_dir)
        self.output_dir = Path(output_dir)
        self.output_dir.mkdir(exist_ok=True)
        self.dataset = dataset  # 'youtube', 'small', or 'both'
        
        # Configuration
        self.methods = ["maxfreq", "spectral"]
        self.formats = ["text", "binary"]
        self.noises = ["clean", "brown", "pink", "white"]
        self.compressors = ["gzip", "bzip2", "lzma", "zstd"]
        
        # Load all data
        self.data = self.load_all_data()
        
    def load_all_data(self):
        """Load all accuracy data from result files."""
        data = {}
        missing_combinations = []
        
        print("Loading data from results directory...")
        
        # Determine which datasets to load
        if self.dataset == 'both':
            datasets = ["youtube", "small"]
        else:
            datasets = [self.dataset]
        
        print(f"Loading data for dataset(s): {datasets}")
        
        for dataset in datasets:
            for method in self.methods:
                for format_type in self.formats:
                    for noise in self.noises:
                        for compressor in self.compressors:
                            key = (method, format_type, noise, compressor, dataset)
                            
                            # The actual path structure is:
                            # results/compressors/{dataset}/{method}/{format}/{noise}_{compressor}/accuracy_metrics_{compressor}.json
                            path = self.results_dir / "compressors" / dataset / method / format_type / f"{noise}_{compressor}" / f"accuracy_metrics_{compressor}.json"
                            
                            if path.exists():
                                try:
                                    with open(path, 'r') as f:
                                        data[key] = json.load(f)
                                except Exception as e:
                                    print(f"Error loading {path}: {e}")
                                    missing_combinations.append(key)
                            else:
                                missing_combinations.append(key)
        
        print(f"Loaded {len(data)} combinations out of {len(datasets) * len(self.methods) * len(self.formats) * len(self.noises) * len(self.compressors)} possible")
        if missing_combinations:
            print(f"Missing combinations: {len(missing_combinations)}")
            
        return data
    
    def get_accuracy(self, method, format_type, noise, compressor, metric='top1_accuracy'):
        """Get accuracy for specific combination, combining datasets if needed."""
        if self.dataset == 'both':
            # Combine results from both datasets
            youtube_key = (method, format_type, noise, compressor, 'youtube')
            small_key = (method, format_type, noise, compressor, 'small')
            
            youtube_acc = self.data.get(youtube_key, {}).get(metric)
            small_acc = self.data.get(small_key, {}).get(metric)
            
            # Return average if both exist, otherwise return the one that exists
            if youtube_acc is not None and small_acc is not None:
                return (youtube_acc + small_acc) / 2
            elif youtube_acc is not None:
                return youtube_acc
            elif small_acc is not None:
                return small_acc
            else:
                return None
        else:
            # Single dataset
            key = (method, format_type, noise, compressor, self.dataset)
            if key in self.data:
                return self.data[key].get(metric, 0.0)
            return None
    
    def get_missing_data_message(self, available_count, total_count, context=""):
        """Generate a message about missing data."""
        if available_count == 0:
            return f"No data available{' for ' + context if context else ''}"
        elif available_count < total_count:
            missing = total_count - available_count
            return f"Showing {available_count}/{total_count} combinations{' for ' + context if context else ''} ({missing} missing)"
        else:
            return ""
    def create_accuracy_heatmap(self):
        """Create 4 heatmaps - one for each noise type."""
        fig, axes = plt.subplots(2, 2, figsize=(16, 12))
        axes = axes.flatten()
        
        for idx, noise in enumerate(self.noises):
            if idx >= 4:
                break
                
            ax = axes[idx]
            
            # Create matrix: rows = method+format combinations, cols = compressors
            combinations = []
            for method in self.methods:
                for format_type in self.formats:
                    combinations.append(f"{method}_{format_type}")
            
            matrix = np.zeros((len(combinations), len(self.compressors)))
            available_data_count = 0
            total_possible = len(combinations) * len(self.compressors)
            
            for i, combo in enumerate(combinations):
                method, format_type = combo.split('_')
                for j, compressor in enumerate(self.compressors):
                    acc = self.get_accuracy(method, format_type, noise, compressor)
                    if acc is not None:
                        matrix[i, j] = acc
                        available_data_count += 1
                    else:
                        matrix[i, j] = np.nan
            
            # Always create the plot if we have any data
            if available_data_count > 0:
                im = ax.imshow(matrix, cmap='RdYlGn', aspect='auto', vmin=0, vmax=100)
                
                # Add text annotations only for available data
                for i in range(len(combinations)):
                    for j in range(len(self.compressors)):
                        if not np.isnan(matrix[i, j]):
                            ax.text(j, i, f'{matrix[i, j]:.1f}%', 
                                ha="center", va="center", color="black", fontweight='bold')
                        else:
                            # Mark missing data
                            ax.text(j, i, 'N/A', 
                                ha="center", va="center", color="gray", fontsize=8)
                
                ax.set_xticks(range(len(self.compressors)))
                ax.set_xticklabels(self.compressors)
                ax.set_yticks(range(len(combinations)))
                ax.set_yticklabels(combinations)
                
                # Add data availability message
                title = f'Top-1 Accuracy - {noise.title()} Noise'
                missing_msg = self.get_missing_data_message(available_data_count, total_possible)
                if missing_msg:
                    title += f'\n({missing_msg})'
                ax.set_title(title, fontsize=10)
                
                # Add colorbar for each heatmap
                plt.colorbar(im, ax=ax, label='Accuracy (%)')
            else:
                ax.text(0.5, 0.5, f'No data available\nfor {noise} noise', 
                    ha='center', va='center', transform=ax.transAxes, fontsize=12)
                ax.set_title(f'Top-1 Accuracy - {noise.title()} Noise')
                ax.set_xticks([])
                ax.set_yticks([])
        
        dataset_info = f" ({self.dataset} dataset)" if self.dataset != 'both' else " (combined datasets)"
        fig.suptitle(f'Accuracy Heatmaps by Noise Type{dataset_info}', fontsize=14, y=0.98)
        
        plt.tight_layout()
        plt.savefig(self.output_dir / f'accuracy_heatmap_{self.dataset}.png', dpi=300, bbox_inches='tight')
        plt.close()
        print(f"Created accuracy_heatmap_{self.dataset}.png")

    def create_compressor_ranking(self):
        """Create bar plots comparing compressors across all metrics."""
        fig, axes = plt.subplots(1, 3, figsize=(18, 6))
        metrics = ['top1_accuracy', 'top5_accuracy', 'top10_accuracy']
        titles = ['Top-1 Accuracy', 'Top-5 Accuracy', 'Top-10 Accuracy']
        
        for idx, (metric, title) in enumerate(zip(metrics, titles)):
            ax = axes[idx]
            
            compressor_scores = defaultdict(list)
            total_possible = len(self.methods) * len(self.formats) * len(self.noises)
            
            # Collect all scores for each compressor
            for method in self.methods:
                for format_type in self.formats:
                    for noise in self.noises:
                        for compressor in self.compressors:
                            acc = self.get_accuracy(method, format_type, noise, compressor, metric)
                            if acc is not None:
                                compressor_scores[compressor].append(acc)
            
            # Always create plot if we have any data
            available_compressors = [comp for comp in self.compressors if compressor_scores[comp]]
            
            if available_compressors:
                # Calculate statistics
                means = []
                mins = []
                maxs = []
                labels = []
                data_counts = []
                
                for compressor in self.compressors:
                    if compressor_scores[compressor]:
                        scores = compressor_scores[compressor]
                        means.append(np.mean(scores))
                        mins.append(np.min(scores))
                        maxs.append(np.max(scores))
                        labels.append(compressor)
                        data_counts.append(len(scores))
                
                x = np.arange(len(labels))
                bars = ax.bar(x, means, alpha=0.8, capsize=5)
                
                # Add min-max lines
                for i, (mean, min_val, max_val) in enumerate(zip(means, mins, maxs)):
                    ax.plot([i, i], [min_val, max_val], 'k-', alpha=0.7, linewidth=2)
                    ax.plot([i-0.1, i+0.1], [min_val, min_val], 'k-', alpha=0.7, linewidth=2)
                    ax.plot([i-0.1, i+0.1], [max_val, max_val], 'k-', alpha=0.7, linewidth=2)
                
                # Add value labels with data count - position them above the max line
                for i, (mean, count, max_val) in enumerate(zip(means, data_counts, maxs)):
                    y_pos = max_val + 3  # Position above the max line
                    ax.text(i, y_pos, f'{mean:.1f}%\n(n={count})', 
                        ha='center', va='bottom', fontweight='bold', fontsize=8)
                
                ax.set_xlabel('Compressor')
                ax.set_ylabel('Accuracy (%)')
                ax.set_xticks(x)
                ax.set_xticklabels(labels)
                ax.grid(True, alpha=0.3)
                ax.set_ylim(0, 100)
                
                # Add missing data info
                total_data_available = sum(data_counts)
                total_data_possible = len(self.compressors) * total_possible
                missing_msg = self.get_missing_data_message(total_data_available, total_data_possible)
                if missing_msg:
                    title += f'\n({missing_msg})'
                ax.set_title(title, fontsize=10)
            else:
                ax.text(0.5, 0.5, f'No data available\nfor {title}', 
                    ha='center', va='center', transform=ax.transAxes, fontsize=12)
                ax.set_title(title)
        
        plt.tight_layout()
        plt.savefig(self.output_dir / 'compressor_ranking.png', dpi=300, bbox_inches='tight')
        plt.close()
        print("Created compressor_ranking.png")
    
    def create_format_comparison(self):
        """Create 4 plots comparing text vs binary for each method and metric."""
        fig, axes = plt.subplots(2, 2, figsize=(16, 12))
        
        plot_configs = [
            ('spectral', 'top1_accuracy', 'Spectral - Top-1 Accuracy'),
            ('spectral', 'top5_accuracy', 'Spectral - Top-5 Accuracy'),
            ('maxfreq', 'top1_accuracy', 'MaxFreq - Top-1 Accuracy'),
            ('maxfreq', 'top5_accuracy', 'MaxFreq - Top-5 Accuracy')
        ]
        
        for idx, (method, metric, title) in enumerate(plot_configs):
            ax = axes[idx // 2, idx % 2]
            
            # Collect data for text vs binary
            text_data = defaultdict(list)
            binary_data = defaultdict(list)
            total_possible_per_format = len(self.noises)
            
            for noise in self.noises:
                for compressor in self.compressors:
                    text_acc = self.get_accuracy(method, 'text', noise, compressor, metric)
                    binary_acc = self.get_accuracy(method, 'binary', noise, compressor, metric)
                    
                    if text_acc is not None:
                        text_data[compressor].append(text_acc)
                    if binary_acc is not None:
                        binary_data[compressor].append(binary_acc)
            
            # Create plot if we have any data
            has_text_data = any(text_data.values())
            has_binary_data = any(binary_data.values())
            
            if has_text_data or has_binary_data:
                x = np.arange(len(self.compressors))
                width = 0.35
                
                text_means = []
                binary_means = []
                text_counts = []
                binary_counts = []
                
                for compressor in self.compressors:
                    text_mean = np.mean(text_data[compressor]) if text_data[compressor] else 0
                    binary_mean = np.mean(binary_data[compressor]) if binary_data[compressor] else 0
                    text_means.append(text_mean)
                    binary_means.append(binary_mean)
                    text_counts.append(len(text_data[compressor]))
                    binary_counts.append(len(binary_data[compressor]))
                
                # Only plot bars for formats that have data
                if has_text_data:
                    bars1 = ax.bar(x - width/2, text_means, width, label='Text', alpha=0.8)
                if has_binary_data:
                    bars2 = ax.bar(x + width/2, binary_means, width, label='Binary', alpha=0.8)
                
                # Add value labels
                for i, (text, binary, text_c, binary_c) in enumerate(zip(text_means, binary_means, text_counts, binary_counts)):
                    if text > 0 and has_text_data:
                        ax.text(i - width/2, text + 1, f'{text:.1f}%\n(n={text_c})', 
                               ha='center', va='bottom', fontsize=7)
                    if binary > 0 and has_binary_data:
                        ax.text(i + width/2, binary + 1, f'{binary:.1f}%\n(n={binary_c})', 
                               ha='center', va='bottom', fontsize=7)
                
                ax.set_xlabel('Compressor')
                ax.set_ylabel('Accuracy (%)')
                ax.set_xticks(x)
                ax.set_xticklabels(self.compressors)
                ax.legend()
                ax.grid(True, alpha=0.3)
                ax.set_ylim(0, 100)
                
                # Add missing data info
                total_text = sum(text_counts)
                total_binary = sum(binary_counts)
                total_possible = len(self.compressors) * total_possible_per_format * 2
                total_available = total_text + total_binary
                missing_msg = self.get_missing_data_message(total_available, total_possible)
                if missing_msg:
                    title += f'\n({missing_msg})'
                ax.set_title(title, fontsize=10)
            else:
                ax.text(0.5, 0.5, f'No data available\nfor {title}', 
                       ha='center', va='center', transform=ax.transAxes, fontsize=12)
                ax.set_title(title)
        
        plt.tight_layout()
        plt.savefig(self.output_dir / 'format_comparison.png', dpi=300, bbox_inches='tight')
        plt.close()
        print("Created format_comparison.png")
    
    def create_method_comparison(self):
        """Create 4 plots comparing maxfreq vs spectral for each format and metric."""
        fig, axes = plt.subplots(2, 2, figsize=(16, 12))
        
        plot_configs = [
            ('text', 'top1_accuracy', 'Text - Top-1 Accuracy'),
            ('text', 'top5_accuracy', 'Text - Top-5 Accuracy'),
            ('binary', 'top1_accuracy', 'Binary - Top-1 Accuracy'),
            ('binary', 'top5_accuracy', 'Binary - Top-5 Accuracy')
        ]
        
        for idx, (format_type, metric, title) in enumerate(plot_configs):
            ax = axes[idx // 2, idx % 2]
            
            # Collect data for maxfreq vs spectral
            maxfreq_data = defaultdict(list)
            spectral_data = defaultdict(list)
            total_possible_per_method = len(self.noises)
            
            for noise in self.noises:
                for compressor in self.compressors:
                    maxfreq_acc = self.get_accuracy('maxfreq', format_type, noise, compressor, metric)
                    spectral_acc = self.get_accuracy('spectral', format_type, noise, compressor, metric)
                    
                    if maxfreq_acc is not None:
                        maxfreq_data[compressor].append(maxfreq_acc)
                    if spectral_acc is not None:
                        spectral_data[compressor].append(spectral_acc)
            
            # Create plot if we have any data
            has_maxfreq_data = any(maxfreq_data.values())
            has_spectral_data = any(spectral_data.values())
            
            if has_maxfreq_data or has_spectral_data:
                x = np.arange(len(self.compressors))
                width = 0.35
                
                maxfreq_means = []
                spectral_means = []
                maxfreq_counts = []
                spectral_counts = []
                
                for compressor in self.compressors:
                    maxfreq_mean = np.mean(maxfreq_data[compressor]) if maxfreq_data[compressor] else 0
                    spectral_mean = np.mean(spectral_data[compressor]) if spectral_data[compressor] else 0
                    maxfreq_means.append(maxfreq_mean)
                    spectral_means.append(spectral_mean)
                    maxfreq_counts.append(len(maxfreq_data[compressor]))
                    spectral_counts.append(len(spectral_data[compressor]))
                
                # Only plot bars for methods that have data
                if has_maxfreq_data:
                    ax.bar(x - width/2, maxfreq_means, width, label='MaxFreq', alpha=0.8)
                if has_spectral_data:
                    ax.bar(x + width/2, spectral_means, width, label='Spectral', alpha=0.8)
                
                # Add value labels
                for i, (maxfreq, spectral, maxfreq_c, spectral_c) in enumerate(zip(maxfreq_means, spectral_means, maxfreq_counts, spectral_counts)):
                    if maxfreq > 0 and has_maxfreq_data:
                        ax.text(i - width/2, maxfreq + 1, f'{maxfreq:.1f}%\n(n={maxfreq_c})', 
                               ha='center', va='bottom', fontsize=7)
                    if spectral > 0 and has_spectral_data:
                        ax.text(i + width/2, spectral + 1, f'{spectral:.1f}%\n(n={spectral_c})', 
                               ha='center', va='bottom', fontsize=7)
                
                ax.set_xlabel('Compressor')
                ax.set_ylabel('Accuracy (%)')
                ax.set_xticks(x)
                ax.set_xticklabels(self.compressors)
                ax.legend()
                ax.grid(True, alpha=0.3)
                ax.set_ylim(0, 100)
                
                # Add missing data info
                total_maxfreq = sum(maxfreq_counts)
                total_spectral = sum(spectral_counts)
                total_possible = len(self.compressors) * total_possible_per_method * 2
                total_available = total_maxfreq + total_spectral
                missing_msg = self.get_missing_data_message(total_available, total_possible)
                if missing_msg:
                    title += f'\n({missing_msg})'
                ax.set_title(title, fontsize=10)
            else:
                ax.text(0.5, 0.5, f'No data available\nfor {title}', 
                       ha='center', va='center', transform=ax.transAxes, fontsize=12)
                ax.set_title(title)
        
        plt.tight_layout()
        plt.savefig(self.output_dir / 'method_comparison.png', dpi=300, bbox_inches='tight')
        plt.close()
        print("Created method_comparison.png")
    
    def create_noise_comparison(self):
        """Create 4 plots analyzing noise impact."""
        fig, axes = plt.subplots(2, 2, figsize=(16, 12))
        
        # Plot 1: Average top-1 accuracy by noise
        ax1 = axes[0, 0]
        noise_scores = defaultdict(list)
        
        for noise in self.noises:
            for method in self.methods:
                for format_type in self.formats:
                    for compressor in self.compressors:
                        acc = self.get_accuracy(method, format_type, noise, compressor, 'top1_accuracy')
                        if acc is not None:
                            noise_scores[noise].append(acc)
        
        available_noises = [noise for noise in self.noises if noise_scores[noise]]
        
        if available_noises:
            means = []
            mins = []
            maxs = []
            labels = []
            counts = []
            
            for noise in self.noises:
                if noise_scores[noise]:
                    scores = noise_scores[noise]
                    means.append(np.mean(scores))
                    mins.append(np.min(scores))
                    maxs.append(np.max(scores))
                    labels.append(noise.title())
                    counts.append(len(scores))
            
            x = np.arange(len(labels))
            bars = ax1.bar(x, means, alpha=0.8)
            
            # Add min-max lines
            for i, (mean, min_val, max_val) in enumerate(zip(means, mins, maxs)):
                ax1.plot([i, i], [min_val, max_val], 'k-', alpha=0.7, linewidth=2)
                ax1.plot([i-0.1, i+0.1], [min_val, min_val], 'k-', alpha=0.7)
                ax1.plot([i-0.1, i+0.1], [max_val, max_val], 'k-', alpha=0.7)
            
            # Position labels above the max line
            for i, (mean, count, max_val) in enumerate(zip(means, counts, maxs)):
                y_pos = max_val + 2
                ax1.text(i, y_pos, f'{mean:.1f}%\n(n={count})', 
                        ha='center', va='bottom', fontweight='bold', fontsize=8)
            
            ax1.set_xlabel('Noise Type')
            ax1.set_ylabel('Top-1 Accuracy (%)')
            ax1.set_xticks(x)
            ax1.set_xticklabels(labels)
            ax1.grid(True, alpha=0.3)
            
            # Add missing data info
            total_available = sum(counts)
            total_possible = len(self.noises) * len(self.methods) * len(self.formats) * len(self.compressors)
            missing_msg = self.get_missing_data_message(total_available, total_possible)
            title = 'Average Top-1 Accuracy by Noise'
            if missing_msg:
                title += f'\n({missing_msg})'
            ax1.set_title(title, fontsize=10)
        else:
            ax1.text(0.5, 0.5, 'No data available', ha='center', va='center', transform=ax1.transAxes)
            ax1.set_title('Average Top-1 Accuracy by Noise')
        
        # Plot 2: Noise impact by method
        ax2 = axes[0, 1]
        method_noise_data = defaultdict(lambda: defaultdict(list))
        
        for noise in self.noises:
            for method in self.methods:
                for format_type in self.formats:
                    for compressor in self.compressors:
                        acc = self.get_accuracy(method, format_type, noise, compressor, 'top1_accuracy')
                        if acc is not None:
                            method_noise_data[noise][method].append(acc)
        
        has_method_data = any(any(methods.values()) for methods in method_noise_data.values())
        
        if has_method_data:
            x = np.arange(len(self.noises))
            width = 0.35
            
            maxfreq_means = []
            spectral_means = []
            maxfreq_counts = []
            spectral_counts = []
            
            for noise in self.noises:
                maxfreq_mean = np.mean(method_noise_data[noise]['maxfreq']) if method_noise_data[noise]['maxfreq'] else 0
                spectral_mean = np.mean(method_noise_data[noise]['spectral']) if method_noise_data[noise]['spectral'] else 0
                maxfreq_means.append(maxfreq_mean)
                spectral_means.append(spectral_mean)
                maxfreq_counts.append(len(method_noise_data[noise]['maxfreq']))
                spectral_counts.append(len(method_noise_data[noise]['spectral']))
            
            has_maxfreq = any(maxfreq_counts)
            has_spectral = any(spectral_counts)
            
            if has_maxfreq:
                bars1 = ax2.bar(x - width/2, maxfreq_means, width, label='MaxFreq', alpha=0.8)
            if has_spectral:
                bars2 = ax2.bar(x + width/2, spectral_means, width, label='Spectral', alpha=0.8)
            
            # Add value labels
            for i, (maxfreq, spectral, maxfreq_c, spectral_c) in enumerate(zip(maxfreq_means, spectral_means, maxfreq_counts, spectral_counts)):
                if maxfreq > 0 and has_maxfreq:
                    ax2.text(i - width/2, maxfreq + 1, f'{maxfreq:.1f}%\n(n={maxfreq_c})', 
                        ha='center', va='bottom', fontsize=7)
                if spectral > 0 and has_spectral:
                    ax2.text(i + width/2, spectral + 1, f'{spectral:.1f}%\n(n={spectral_c})', 
                        ha='center', va='bottom', fontsize=7)
            
            ax2.set_xlabel('Noise Type')
            ax2.set_ylabel('Top-1 Accuracy (%)')
            ax2.set_xticks(x)
            ax2.set_xticklabels([n.title() for n in self.noises])
            if has_maxfreq or has_spectral:
                ax2.legend()
            ax2.grid(True, alpha=0.3)
            
            # Add missing data info
            total_available = sum(maxfreq_counts) + sum(spectral_counts)
            total_possible = len(self.noises) * len(self.methods) * len(self.formats) * len(self.compressors)
            missing_msg = self.get_missing_data_message(total_available, total_possible)
            title = 'Noise Impact by Method'
            if missing_msg:
                title += f'\n({missing_msg})'
            ax2.set_title(title, fontsize=10)
        else:
            ax2.text(0.5, 0.5, 'No data available', ha='center', va='center', transform=ax2.transAxes)
            ax2.set_title('Noise Impact by Method')
        
        # Plot 3: Noise impact by compressor
        ax3 = axes[1, 0]
        comp_noise_data = defaultdict(lambda: defaultdict(list))
        
        for noise in self.noises:
            for compressor in self.compressors:
                for method in self.methods:
                    for format_type in self.formats:
                        acc = self.get_accuracy(method, format_type, noise, compressor, 'top1_accuracy')
                        if acc is not None:
                            comp_noise_data[noise][compressor].append(acc)
        
        has_comp_data = any(any(comps.values()) for comps in comp_noise_data.values())
        
        if has_comp_data:
            # Create grouped bar chart for compressors
            x = np.arange(len(self.noises))
            width = 0.2
            
            compressor_means = defaultdict(list)
            compressor_counts = defaultdict(list)
            
            for compressor in self.compressors:
                means = []
                counts = []
                for noise in self.noises:
                    mean = np.mean(comp_noise_data[noise][compressor]) if comp_noise_data[noise][compressor] else 0
                    count = len(comp_noise_data[noise][compressor])
                    means.append(mean)
                    counts.append(count)
                compressor_means[compressor] = means
                compressor_counts[compressor] = counts
            
            # Only plot compressors that have data
            available_compressors = [comp for comp in self.compressors if any(compressor_counts[comp])]
            
            for i, compressor in enumerate(available_compressors):
                offset = (i - len(available_compressors)/2 + 0.5) * width
                bars = ax3.bar(x + offset, compressor_means[compressor], width, 
                    label=compressor, alpha=0.8)
                
                # Add value labels
                for j, (mean, count) in enumerate(zip(compressor_means[compressor], compressor_counts[compressor])):
                    if mean > 0:
                        ax3.text(j + offset, mean + 1, f'{mean:.1f}%\n(n={count})', 
                            ha='center', va='bottom', fontsize=6)
            
            ax3.set_xlabel('Noise Type')
            ax3.set_ylabel('Top-1 Accuracy (%)')
            ax3.set_xticks(x)
            ax3.set_xticklabels([n.title() for n in self.noises])
            if available_compressors:
                ax3.legend()
            ax3.grid(True, alpha=0.3)
            
            # Add missing data info
            total_available = sum(sum(compressor_counts[comp]) for comp in self.compressors)
            total_possible = len(self.noises) * len(self.compressors) * len(self.methods) * len(self.formats)
            missing_msg = self.get_missing_data_message(total_available, total_possible)
            title = 'Noise Impact by Compressor'
            if missing_msg:
                title += f'\n({missing_msg})'
            ax3.set_title(title, fontsize=10)
        else:
            ax3.text(0.5, 0.5, 'No data available', ha='center', va='center', transform=ax3.transAxes)
            ax3.set_title('Noise Impact by Compressor')
        
        # Plot 4: Configuration vs noise heatmap (unchanged)
        ax4 = axes[1, 1]
        
        # Create matrix: rows = method+format combinations, cols = noise types
        combinations = []
        for method in self.methods:
            for format_type in self.formats:
                combinations.append(f"{method}_{format_type}")
        
        matrix = np.zeros((len(combinations), len(self.noises)))
        available_data_count = 0
        total_possible = len(combinations) * len(self.noises)
        
        for i, combo in enumerate(combinations):
            method, format_type = combo.split('_')
            for j, noise in enumerate(self.noises):
                scores = []
                for compressor in self.compressors:
                    acc = self.get_accuracy(method, format_type, noise, compressor, 'top1_accuracy')
                    if acc is not None:
                        scores.append(acc)
                
                if scores:
                    matrix[i, j] = np.mean(scores)
                    available_data_count += 1
                else:
                    matrix[i, j] = np.nan
        
        if available_data_count > 0:
            im = ax4.imshow(matrix, cmap='RdYlGn', aspect='auto', vmin=0, vmax=100)
            
            for i in range(len(combinations)):
                for j in range(len(self.noises)):
                    if not np.isnan(matrix[i, j]):
                        ax4.text(j, i, f'{matrix[i, j]:.1f}%', 
                            ha="center", va="center", color="black", fontweight='bold')
                    else:
                        ax4.text(j, i, 'N/A', 
                            ha="center", va="center", color="gray", fontsize=8)
            
            ax4.set_xticks(range(len(self.noises)))
            ax4.set_xticklabels([n.title() for n in self.noises])
            ax4.set_yticks(range(len(combinations)))
            ax4.set_yticklabels(combinations)
            plt.colorbar(im, ax=ax4, label='Accuracy (%)')
            
            # Add missing data info
            missing_msg = self.get_missing_data_message(available_data_count, total_possible)
            title = 'Configuration vs Noise'
            if missing_msg:
                title += f'\n({missing_msg})'
            ax4.set_title(title, fontsize=10)
        else:
            ax4.text(0.5, 0.5, 'No data available', ha='center', va='center', transform=ax4.transAxes)
            ax4.set_title('Configuration vs Noise')
        
        plt.tight_layout()
        plt.savefig(self.output_dir / 'noise_comparison.png', dpi=300, bbox_inches='tight')
        plt.close()
        print("Created noise_comparison.png")

    def create_performance_overview(self):
        """Create 4 plots for performance overview."""
        fig, axes = plt.subplots(2, 2, figsize=(16, 12))
        
        # Plot 1: Distribution of top-1 accuracy scores
        ax1 = axes[0, 0]
        all_scores = []
        
        for method in self.methods:
            for format_type in self.formats:
                for noise in self.noises:
                    for compressor in self.compressors:
                        acc = self.get_accuracy(method, format_type, noise, compressor, 'top1_accuracy')
                        if acc is not None:
                            all_scores.append(acc)
        
        if all_scores:
            n, bins, patches = ax1.hist(all_scores, bins=20, alpha=0.7, edgecolor='black')
            
            # Add frequency labels on top of bars
            for i in range(len(n)):
                if n[i] > 0:  # Only show label if there are values in this bin
                    ax1.text(bins[i] + (bins[i+1] - bins[i])/2, n[i] + 0.1, 
                            f'{int(n[i])}', ha='center', va='bottom', fontsize=8)
            
            ax1.set_xlabel('Top-1 Accuracy (%)')
            ax1.set_ylabel('Frequency')
            ax1.grid(True, alpha=0.3)
            ax1.axvline(np.mean(all_scores), color='red', linestyle='--', 
                    label=f'Mean: {np.mean(all_scores):.1f}%')
            ax1.legend()
            
            # Add missing data info
            total_possible = len(self.methods) * len(self.formats) * len(self.noises) * len(self.compressors)
            missing_msg = self.get_missing_data_message(len(all_scores), total_possible)
            title = 'Distribution of Top-1 Accuracy Scores'
            if missing_msg:
                title += f'\n({missing_msg})'
            ax1.set_title(title, fontsize=10)
        else:
            ax1.text(0.5, 0.5, 'No data available', ha='center', va='center', transform=ax1.transAxes)
            ax1.set_title('Distribution of Top-1 Accuracy Scores')
        
        def generate_all_plots(self):
            """Generate all requested visualizations."""
            print(f"Generating visualization plots for {self.dataset} dataset(s)...")
            
            self.create_accuracy_heatmap()
            self.create_compressor_ranking()
            self.create_format_comparison()
            self.create_method_comparison()
            self.create_noise_comparison()
            self.create_performance_overview()
            
            print(f"\nAll plots saved to: {self.output_dir}")
            print("Generated files:")
            for png_file in sorted(self.output_dir.glob("*.png")):
                print(f"  - {png_file.name}")

    def generate_all_plots(self):
        """Generate all requested visualizations."""
        print(f"Generating visualization plots for {self.dataset} dataset(s)...")
        
        self.create_accuracy_heatmap()
        self.create_compressor_ranking()
        self.create_format_comparison()
        self.create_method_comparison()
        self.create_noise_comparison()
        self.create_performance_overview()
        
        print(f"\nAll plots saved to: {self.output_dir}")
        print("Generated files:")
        for png_file in sorted(self.output_dir.glob("*.png")):
            print(f"  - {png_file.name}")

def main():    
    # Choose which dataset to analyze: 'youtube', 'small', or 'both'
    dataset = 'youtube'  # Change this to 'small' or 'both' as needed
    
    results_dir = Path("results/")
    output = Path(f"results/plots_{dataset}/")

    if not os.path.exists(results_dir):
        print(f"Error: Results directory does not exist: {results_dir}")
        return 1
    
    analyzer = ResultsAnalyzer(results_dir, output, dataset)
    analyzer.generate_all_plots()
    
    return 0


if __name__ == "__main__":
    exit(main())
