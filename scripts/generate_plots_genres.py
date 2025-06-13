"""
Genre-based Music Identification Analysis for YouTube Dataset
Analyzes accuracy performance across different music genres.
"""

import os
import json
import numpy as np
import matplotlib.pyplot as plt
import seaborn as sns
from pathlib import Path
from collections import defaultdict
import re
import warnings
warnings.filterwarnings('ignore')

# Set style
plt.style.use('default')
sns.set_palette("Set3")

class GenreAnalyzer:
    def __init__(self, results_dir, output_dir, songs_genre_file):
        self.results_dir = Path(results_dir)
        self.output_dir = Path(output_dir)
        self.output_dir.mkdir(exist_ok=True)
        self.songs_genre_file = Path(songs_genre_file)
        
        # Configuration
        self.methods = ["maxfreq", "spectral"]
        self.formats = ["text", "binary"]
        self.noises = ["clean", "brown", "pink", "white"]
        self.compressors = ["gzip", "bzip2", "lzma", "zstd"]
        
        # Load genre mapping and results
        self.genre_mapping = self.load_genre_mapping()
        self.results_data = self.load_results_data()
        
        print(f"Loaded results for {len(self.results_data)} configurations")
        print(f"Loaded {len(self.genre_mapping)} songs across {len(set(self.genre_mapping.values()))} genres")
        
    def normalize_text(self, text):
        """Simple text normalization - keep all words, just clean up"""
        if not text:
            return ""
        
        # Convert to lowercase
        text = text.lower()
        
        # Remove common YouTube annotations
        text = re.sub(r'\(.*?(?:lyrics?|video|official|hd|hq|audio|letra).*?\)', '', text)
        text = re.sub(r'\[.*?(?:lyrics?|video|official|hd|hq).*?\]', '', text)
        text = re.sub(r'[🎵🎶🎤🎧｜]', '', text)
        
        # Replace special characters with spaces
        text = re.sub(r'[^a-z0-9\s]', ' ', text)
        
        # Normalize spaces
        text = re.sub(r'\s+', ' ', text)
        
        return text.strip()
    
    def load_genre_mapping(self):
        """Load the genre mapping from songs_genre.txt file."""
        genre_mapping = {}
        current_genre = None
        
        if not self.songs_genre_file.exists():
            print(f"Warning: Genre file not found: {self.songs_genre_file}")
            return {}
            
        with open(self.songs_genre_file, 'r', encoding='utf-8') as f:
            lines = f.readlines()
        
        for line_num, line in enumerate(lines, 1):
            original_line = line
            line = line.strip()
            if not line:
                continue
                
            # Check if it's a genre header
            if re.match(r'^\d+\.\s+', line):
                current_genre = re.sub(r'^\d+\.\s+', '', line).strip()
                continue
                
            # Check if it's a song entry
            if current_genre and original_line.startswith('    '):
                song_line = line.strip()
                if song_line:
                    # Keep the full song line (song + artist)
                    normalized_song = self.normalize_text(song_line)
                    if normalized_song:
                        genre_mapping[normalized_song] = current_genre
        
        return genre_mapping
    
    def extract_query_text(self, filename):
        """Extract clean text from query filename"""
        name = Path(filename).stem
        
        # Remove prefixes and suffixes
        if name.startswith('sample_'):
            name = name[7:]
        
        name = re.sub(r'_(spectral|maxfreq)$', '', name)
        name = re.sub(r'_(white|pink|brown|clean)_noise$', '', name)
        name = re.sub(r'_t\d+s$', '', name)
        name = name.strip('_-')
        
        return self.normalize_text(name)
    
    def word_similarity(self, text1, text2):
        """Calculate word overlap similarity between two texts"""
        if not text1 or not text2:
            return 0.0
        
        words1 = set(text1.split())
        words2 = set(text2.split())
        
        if not words1 or not words2:
            return 0.0
        
        # Calculate overlap ratio
        common_words = words1.intersection(words2)
        similarity = len(common_words) / min(len(words1), len(words2))
        
        return similarity
    
    def match_song_to_genre(self, query_filename):
        """Match a query filename to its genre using word similarity"""
        query_text = self.extract_query_text(query_filename)
        
        best_match = None
        best_similarity = 0.0
        
        for song_text, genre in self.genre_mapping.items():
            similarity = self.word_similarity(query_text, song_text)
            
            # Require at least 30% word overlap
            if similarity >= 0.3 and similarity > best_similarity:
                best_similarity = similarity
                best_match = genre
        
        return best_match if best_match else "Unknown"
    
    def load_results_data(self):
        """Load all accuracy results from the results directory."""
        results = {}
        
        for method in self.methods:
            for format_type in self.formats:
                for noise in self.noises:
                    for compressor in self.compressors:
                        metrics_file = (self.results_dir / "compressors" / "youtube" / method / 
                                       format_type / f"{noise}_{compressor}" / 
                                       f"accuracy_metrics_{compressor}.json")
                        
                        if metrics_file.exists():
                            try:
                                with open(metrics_file, 'r') as f:
                                    data = json.load(f)
                                results[(method, format_type, noise, compressor)] = data
                            except Exception as e:
                                print(f"Error loading {metrics_file}: {e}")
        
        return results
    
    def analyze_genre_accuracy(self):
        """Analyze accuracy by genre across all configurations."""
        genre_results = defaultdict(lambda: defaultdict(list))
        unmatched_queries = []
        
        for config, data in self.results_data.items():
            if 'detailed_results' not in data:
                continue
                
            for result in data['detailed_results']:
                query = result['query']
                genre = self.match_song_to_genre(query)
                
                if genre == "Unknown":
                    unmatched_queries.append(query)
                
                top1_correct = result.get('correct', False)
                top5_correct = result.get('found_at_rank') is not None and result.get('found_at_rank', 11) <= 5
                top10_correct = result.get('found_at_rank') is not None and result.get('found_at_rank', 11) <= 10
                
                genre_results[genre]['top1_correct'].append(1 if top1_correct else 0)
                genre_results[genre]['top5_correct'].append(1 if top5_correct else 0)
                genre_results[genre]['top10_correct'].append(1 if top10_correct else 0)
        
        # Calculate statistics
        total_queries = sum(len(results['top1_correct']) for results in genre_results.values())
        matched_queries = sum(len(results['top1_correct']) for genre, results in genre_results.items() if genre != "Unknown")
        match_rate = matched_queries / total_queries * 100 if total_queries > 0 else 0
        
        print(f"Genre matching: {matched_queries}/{total_queries} queries ({match_rate:.1f}%)")
        
        # Show unmatched queries
        if unmatched_queries:
            unique_unmatched = list(set(unmatched_queries))
            print(f"\nUnmatched queries ({len(unique_unmatched)}):")
            print("-" * 60)
            
            for i, query in enumerate(sorted(unique_unmatched)[:15], 1):
                query_text = self.extract_query_text(query)
                print(f"{i:2d}. {query}")
                print(f"    Normalized: '{query_text}'")
                
                # Find best partial matches
                best_matches = []
                for song_text in self.genre_mapping.keys():
                    similarity = self.word_similarity(query_text, song_text)
                    if similarity > 0.2:  # Show any reasonable similarity
                        best_matches.append((song_text, self.genre_mapping[song_text], similarity))
                
                if best_matches:
                    best_matches.sort(key=lambda x: x[2], reverse=True)
                    print(f"    Best matches:")
                    for song, genre, sim in best_matches[:3]:
                        print(f"      {sim:.2f}: '{song}' ({genre})")
                print()
            
            if len(unique_unmatched) > 15:
                print(f"... and {len(unique_unmatched) - 15} more unmatched queries")
        
        return genre_results
    
    def create_genre_plots(self):
        """Create main genre analysis plots"""
        genre_results = self.analyze_genre_accuracy()
        
        if not genre_results:
            print("No genre results found")
            return
            
        # Prepare data (excluding Unknown)
        genres = []
        top1_accs = []
        top5_accs = []
        sample_counts = []
        
        for genre, results in genre_results.items():
            if genre != "Unknown" and results['top1_correct']:
                genres.append(genre)
                top1_accs.append(np.mean(results['top1_correct']) * 100)
                top5_accs.append(np.mean(results['top5_correct']) * 100)
                sample_counts.append(len(results['top1_correct']))
        
        if not genres:
            print("No genre data to plot")
            return
            
        # Create plots
        fig, axes = plt.subplots(2, 2, figsize=(20, 16))
        
        # Plot 1: Accuracy by genre
        ax1 = axes[0, 0]
        x = np.arange(len(genres))
        width = 0.35
        
        ax1.bar(x - width/2, top1_accs, width, label='Top-1', alpha=0.8, color='lightcoral')
        ax1.bar(x + width/2, top5_accs, width, label='Top-5', alpha=0.8, color='lightgreen')
        
        ax1.set_xlabel('Genre')
        ax1.set_ylabel('Accuracy (%)')
        ax1.set_title('Accuracy by Music Genre')
        ax1.set_xticks(x)
        ax1.set_xticklabels(genres, rotation=45, ha='right')
        ax1.legend()
        ax1.grid(True, alpha=0.3)
        ax1.set_ylim(0, 100)
        
        # Plot 2: Genre ranking
        ax2 = axes[0, 1]
        sorted_data = sorted(zip(genres, top1_accs, sample_counts), key=lambda x: x[1], reverse=True)
        sorted_genres, sorted_accs, sorted_counts = zip(*sorted_data)
        
        colors = ['green' if acc >= 60 else 'orange' if acc >= 40 else 'red' for acc in sorted_accs]
        ax2.barh(range(len(sorted_genres)), sorted_accs, color=colors, alpha=0.8)
        
        for i, (acc, count) in enumerate(zip(sorted_accs, sorted_counts)):
            ax2.text(acc + 1, i, f'{acc:.1f}% (n={count})', va='center', fontsize=9)
        
        ax2.set_yticks(range(len(sorted_genres)))
        ax2.set_yticklabels(sorted_genres)
        ax2.set_xlabel('Top-1 Accuracy (%)')
        ax2.set_title('Genre Performance Ranking')
        ax2.grid(True, alpha=0.3)
        ax2.set_xlim(0, 100)
        
        # Plot 3: Sample distribution
        ax3 = axes[1, 0]
        ax3.pie(sample_counts, labels=genres, autopct='%1.1f%%', startangle=90)
        ax3.set_title('Sample Distribution by Genre')
        
        # Plot 4: Performance summary
        ax4 = axes[1, 1]
        known_count = sum(sample_counts)
        unknown_count = len(genre_results.get("Unknown", {}).get('top1_correct', []))
        
        if known_count > 0 or unknown_count > 0:
            labels = ['Matched', 'Unmatched']
            sizes = [known_count, unknown_count]
            colors = ['lightgreen', 'lightcoral']
            
            ax4.pie(sizes, labels=labels, autopct='%1.1f%%', colors=colors, startangle=90)
            ax4.set_title(f'Matching Success\n(Total: {known_count + unknown_count})')
        
        plt.tight_layout()
        plt.savefig(self.output_dir / 'genre_analysis.png', dpi=300, bbox_inches='tight')
        plt.close()
        print("Created genre_analysis.png")

    def create_method_format_plots(self):
        """Create method and format comparison plots"""
        fig, axes = plt.subplots(2, 2, figsize=(20, 16))
        
        # Plot 1: Method Performance by Genre
        self._plot_method_by_genre(axes[0, 0])
        
        # Plot 2: Format Performance by Genre
        self._plot_format_by_genre(axes[0, 1])
        
        # Plot 3: Accuracy Heatmap (Method+Format)
        self._plot_accuracy_heatmap(axes[1, 0])
        
        # Plot 4: Configuration Ranking (Top 15)
        self._plot_configuration_ranking(axes[1, 1])
        
        plt.tight_layout()
        plt.savefig(self.output_dir / 'method_format_analysis.png', dpi=300, bbox_inches='tight')
        plt.close()
        print("Created method_format_analysis.png")

    def create_noise_compressor_plots(self):
        """Create noise and compressor analysis plots"""
        fig, axes = plt.subplots(2, 2, figsize=(20, 16))
        
        # Plot 1: Noise Impact by Genre
        self._plot_noise_by_genre(axes[0, 0])
        
        # Plot 2: Compressor Performance by Genre
        self._plot_compressor_by_genre(axes[0, 1])
        
        # Plot 3: Performance Distribution
        self._plot_performance_distribution(axes[1, 0])
        
        # Plot 4: Error Analysis
        self._plot_error_analysis(axes[1, 1])
        
        plt.tight_layout()
        plt.savefig(self.output_dir / 'noise_compressor_analysis.png', dpi=300, bbox_inches='tight')
        plt.close()
        print("Created noise_compressor_analysis.png")

    def create_difficulty_analysis_plots(self):
        """Create genre difficulty and detailed analysis plots"""
        fig, axes = plt.subplots(1, 2, figsize=(20, 10))
        
        # Plot 1: Genre Difficulty Analysis
        self._plot_genre_difficulty(axes[0])
        
        # Plot 2: Top-5 vs Top-1 Comparison
        self._plot_top5_vs_top1_comparison(axes[1])
        
        plt.tight_layout()
        plt.savefig(self.output_dir / 'difficulty_analysis.png', dpi=300, bbox_inches='tight')
        plt.close()
        print("Created difficulty_analysis.png")

    def _plot_method_by_genre(self, ax):
        """Plot method effectiveness by genre"""
        method_genre_data = defaultdict(lambda: defaultdict(list))
        
        for config, data in self.results_data.items():
            if 'detailed_results' not in data:
                continue
            method, format_type, noise, compressor = config
            
            for result in data['detailed_results']:
                genre = self.match_song_to_genre(result['query'])
                if genre != "Unknown":
                    accuracy = 1 if result.get('correct', False) else 0
                    method_genre_data[method][genre].append(accuracy)
        
        # Get all genres and sort them
        all_genres = sorted(set().union(*[method_data.keys() for method_data in method_genre_data.values()]))
        methods = ['maxfreq', 'spectral']
        
        x = np.arange(len(all_genres))
        width = 0.35
        
        for i, method in enumerate(methods):
            accuracies = []
            for genre in all_genres:
                if genre in method_genre_data[method] and method_genre_data[method][genre]:
                    acc = np.mean(method_genre_data[method][genre]) * 100
                else:
                    acc = 0
                accuracies.append(acc)
            
            ax.bar(x + i*width, accuracies, width, label=method.title(), alpha=0.8)
        
        ax.set_xlabel('Genre')
        ax.set_ylabel('Top-1 Accuracy (%)')
        ax.set_title('Method Performance by Genre')
        ax.set_xticks(x + width/2)
        ax.set_xticklabels([g[:12] + '...' if len(g) > 12 else g for g in all_genres], rotation=45, ha='right')
        ax.legend()
        ax.grid(True, alpha=0.3)

    def _plot_format_by_genre(self, ax):
        """Plot format effectiveness by genre"""
        format_genre_data = defaultdict(lambda: defaultdict(list))
        
        for config, data in self.results_data.items():
            if 'detailed_results' not in data:
                continue
            method, format_type, noise, compressor = config
            
            for result in data['detailed_results']:
                genre = self.match_song_to_genre(result['query'])
                if genre != "Unknown":
                    accuracy = 1 if result.get('correct', False) else 0
                    format_genre_data[format_type][genre].append(accuracy)
        
        # Get all genres
        all_genres = sorted(set().union(*[format_data.keys() for format_data in format_genre_data.values()]))
        
        x = np.arange(len(all_genres))
        width = 0.35
        formats = ['text', 'binary']
        
        for i, fmt in enumerate(formats):
            accuracies = []
            for genre in all_genres:
                if genre in format_genre_data[fmt] and format_genre_data[fmt][genre]:
                    acc = np.mean(format_genre_data[fmt][genre]) * 100
                else:
                    acc = 0
                accuracies.append(acc)
            
            ax.bar(x + i*width, accuracies, width, label=fmt.title(), alpha=0.8)
        
        ax.set_xlabel('Genre')
        ax.set_ylabel('Top-1 Accuracy (%)')
        ax.set_title('Format Performance by Genre')
        ax.set_xticks(x + width/2)
        ax.set_xticklabels([g[:12] + '...' if len(g) > 12 else g for g in all_genres], rotation=45, ha='right')
        ax.legend()
        ax.grid(True, alpha=0.3)

    def _plot_noise_by_genre(self, ax):
        """Plot noise impact by genre"""
        noise_genre_data = defaultdict(lambda: defaultdict(list))
        
        for config, data in self.results_data.items():
            if 'detailed_results' not in data:
                continue
            method, format_type, noise, compressor = config
            
            for result in data['detailed_results']:
                genre = self.match_song_to_genre(result['query'])
                if genre != "Unknown":
                    accuracy = 1 if result.get('correct', False) else 0
                    noise_genre_data[noise][genre].append(accuracy)
        
        # Get all genres
        all_genres = sorted(set().union(*[noise_data.keys() for noise_data in noise_genre_data.values()]))
        
        x = np.arange(len(all_genres))
        width = 0.2
        noises = ['clean', 'brown', 'pink', 'white']
        colors = ['blue', 'brown', 'pink', 'gray']
        
        for i, (noise, color) in enumerate(zip(noises, colors)):
            accuracies = []
            for genre in all_genres:
                if genre in noise_genre_data[noise] and noise_genre_data[noise][genre]:
                    acc = np.mean(noise_genre_data[noise][genre]) * 100
                else:
                    acc = 0
                accuracies.append(acc)
            
            ax.bar(x + i*width, accuracies, width, label=noise.title(), color=color, alpha=0.7)
        
        ax.set_xlabel('Genre')
        ax.set_ylabel('Top-1 Accuracy (%)')
        ax.set_title('Noise Impact by Genre')
        ax.set_xticks(x + width*1.5)
        ax.set_xticklabels([g[:10] + '...' if len(g) > 10 else g for g in all_genres], rotation=45, ha='right')
        ax.legend()
        ax.grid(True, alpha=0.3)

    def _plot_compressor_by_genre(self, ax):
        """Plot compressor effectiveness by genre"""
        comp_genre_data = defaultdict(lambda: defaultdict(list))
        
        for config, data in self.results_data.items():
            if 'detailed_results' not in data:
                continue
            method, format_type, noise, compressor = config
            
            for result in data['detailed_results']:
                genre = self.match_song_to_genre(result['query'])
                if genre != "Unknown":
                    accuracy = 1 if result.get('correct', False) else 0
                    comp_genre_data[compressor][genre].append(accuracy)
        
        # Get all genres
        all_genres = sorted(set().union(*[comp_data.keys() for comp_data in comp_genre_data.values()]))
        
        x = np.arange(len(all_genres))
        width = 0.2
        compressors = ['gzip', 'bzip2', 'lzma', 'zstd']
        colors = ['red', 'orange', 'green', 'purple']
        
        for i, (comp, color) in enumerate(zip(compressors, colors)):
            accuracies = []
            for genre in all_genres:
                if genre in comp_genre_data[comp] and comp_genre_data[comp][genre]:
                    acc = np.mean(comp_genre_data[comp][genre]) * 100
                else:
                    acc = 0
                accuracies.append(acc)
            
            ax.bar(x + i*width, accuracies, width, label=comp.upper(), color=color, alpha=0.7)
        
        ax.set_xlabel('Genre')
        ax.set_ylabel('Top-1 Accuracy (%)')
        ax.set_title('Compressor Performance by Genre')
        ax.set_xticks(x + width*1.5)
        ax.set_xticklabels([g[:10] + '...' if len(g) > 10 else g for g in all_genres], rotation=45, ha='right')
        ax.legend()
        ax.grid(True, alpha=0.3)

    def _plot_accuracy_heatmap(self, ax):
        """Create accuracy heatmap across configurations"""
        config_genre_accuracy = defaultdict(lambda: defaultdict(list))
        
        for config, data in self.results_data.items():
            if 'detailed_results' not in data:
                continue
            method, format_type, noise, compressor = config
            config_name = f"{method}_{format_type}"
            
            for result in data['detailed_results']:
                genre = self.match_song_to_genre(result['query'])
                if genre != "Unknown":
                    accuracy = 1 if result.get('correct', False) else 0
                    config_genre_accuracy[config_name][genre].append(accuracy)
        
        # Create heatmap data
        genres = sorted(list(set().union(*[config_data.keys() for config_data in config_genre_accuracy.values()])))
        configs = sorted(config_genre_accuracy.keys())
        
        heatmap_data = []
        for config in configs:
            row = []
            for genre in genres:
                if genre in config_genre_accuracy[config] and config_genre_accuracy[config][genre]:
                    acc = np.mean(config_genre_accuracy[config][genre]) * 100
                else:
                    acc = 0
                row.append(acc)
            heatmap_data.append(row)
        
        im = ax.imshow(heatmap_data, cmap='RdYlGn', aspect='auto', vmin=0, vmax=100)
        
        # Add labels
        ax.set_xticks(np.arange(len(genres)))
        ax.set_yticks(np.arange(len(configs)))
        ax.set_xticklabels([g[:12] + '...' if len(g) > 12 else g for g in genres], rotation=45, ha='right')
        ax.set_yticklabels(configs)
        
        # Add text annotations
        for i in range(len(configs)):
            for j in range(len(genres)):
                text = ax.text(j, i, f'{heatmap_data[i][j]:.0f}%', ha="center", va="center", 
                             color="black", fontsize=10)
        
        ax.set_title('Accuracy Heatmap by Configuration')
        plt.colorbar(im, ax=ax, label='Accuracy (%)')

    def _plot_performance_distribution(self, ax):
        """Plot distribution of performance across genres"""
        genre_accuracies = []
        genre_names = []
        
        for genre, results in self.analyze_genre_accuracy().items():
            if genre != "Unknown" and results['top1_correct']:
                accuracies = [acc * 100 for acc in results['top1_correct']]
                genre_accuracies.extend(accuracies)
                genre_names.extend([genre] * len(accuracies))
        
        # Create violin plot for all genres
        if genre_accuracies:
            unique_genres = sorted(list(set(genre_names)))
            data_for_plot = []
            labels_for_plot = []
            
            for genre in unique_genres:
                genre_data = [acc for acc, name in zip(genre_accuracies, genre_names) if name == genre]
                if genre_data:
                    data_for_plot.append(genre_data)
                    labels_for_plot.append(genre[:12] + '...' if len(genre) > 12 else genre)
            
            if data_for_plot:
                ax.violinplot(data_for_plot, positions=range(len(data_for_plot)), showmeans=True)
                ax.set_xticks(range(len(labels_for_plot)))
                ax.set_xticklabels(labels_for_plot, rotation=45, ha='right')
                ax.set_ylabel('Accuracy (%)')
                ax.set_title('Performance Distribution by Genre')
                ax.grid(True, alpha=0.3)

    def _plot_configuration_ranking(self, ax):
        """Plot ranking of best configurations"""
        config_performance = defaultdict(list)
        
        for config, data in self.results_data.items():
            if 'detailed_results' not in data:
                continue
            method, format_type, noise, compressor = config
            config_name = f"{method}_{format_type}_{noise}_{compressor}"
            
            for result in data['detailed_results']:
                genre = self.match_song_to_genre(result['query'])
                if genre != "Unknown":
                    accuracy = 1 if result.get('correct', False) else 0
                    config_performance[config_name].append(accuracy)
        
        # Calculate average performance
        config_avg = [(config, np.mean(accuracies) * 100) for config, accuracies in config_performance.items() if accuracies]
        config_avg.sort(key=lambda x: x[1], reverse=True)
        
        # Show top 15 configurations
        top_configs = config_avg[:15]
        configs, accuracies = zip(*top_configs)
        
        y_pos = np.arange(len(configs))
        colors = ['green' if acc >= 70 else 'orange' if acc >= 50 else 'red' for acc in accuracies]
        
        ax.barh(y_pos, accuracies, color=colors, alpha=0.8)
        ax.set_yticks(y_pos)
        ax.set_yticklabels([c.replace('_', '\n') for c in configs], fontsize=9)
        ax.set_xlabel('Average Accuracy (%)')
        ax.set_title('Top 15 Configuration Rankings')
        ax.grid(True, alpha=0.3)

    def _plot_error_analysis(self, ax):
        """Analyze common error patterns"""
        top_rank_counts = defaultdict(int)
        
        for config, data in self.results_data.items():
            if 'detailed_results' not in data:
                continue
                
            for result in data['detailed_results']:
                genre = self.match_song_to_genre(result['query'])
                if genre != "Unknown":
                    rank = result.get('found_at_rank', 11)
                    if rank and rank <= 10:
                        top_rank_counts[rank] += 1
                    else:
                        top_rank_counts['Not Found'] += 1
        
        ranks = list(range(1, 11)) + ['Not Found']
        counts = [top_rank_counts[rank] for rank in ranks]
        
        colors = ['green'] + ['orange'] * 4 + ['red'] * 5 + ['darkred']
        ax.bar(range(len(ranks)), counts, color=colors, alpha=0.8)
        ax.set_xticks(range(len(ranks)))
        ax.set_xticklabels([str(r) for r in ranks], rotation=45)
        ax.set_xlabel('Found at Rank')
        ax.set_ylabel('Count')
        ax.set_title('Error Analysis: Where Songs Are Found')
        ax.grid(True, alpha=0.3)

    def _plot_genre_difficulty(self, ax):
        """Plot genre difficulty analysis"""
        genre_results = self.analyze_genre_accuracy()
        
        # Calculate metrics for each genre
        genre_metrics = []
        for genre, results in genre_results.items():
            if genre != "Unknown" and results['top1_correct']:
                top1 = np.mean(results['top1_correct']) * 100
                top5 = np.mean(results['top5_correct']) * 100
                samples = len(results['top1_correct'])
                genre_metrics.append((genre, top1, top5, samples))
        
        if not genre_metrics:
            ax.text(0.5, 0.5, 'No genre data available', ha='center', va='center', transform=ax.transAxes)
            ax.set_title('Genre Difficulty Analysis')
            return
        
        # Sort by difficulty (inverse of accuracy)
        genre_metrics.sort(key=lambda x: x[1])
        
        genres, top1s, top5s, samples = zip(*genre_metrics)
        
        # Create difficulty categories
        colors = []
        for acc in top1s:
            if acc >= 70:
                colors.append('green')
            elif acc >= 50:
                colors.append('orange')
            elif acc >= 30:
                colors.append('red')
            else:
                colors.append('darkred')
        
        y_pos = np.arange(len(genres))
        ax.barh(y_pos, top1s, color=colors, alpha=0.8)
        
        # Add sample count labels
        for i, (acc, sample) in enumerate(zip(top1s, samples)):
            ax.text(acc + 1, i, f'n={sample}', va='center', fontsize=10)
        
        ax.set_yticks(y_pos)
        ax.set_yticklabels([g[:20] + '...' if len(g) > 20 else g for g in genres])
        ax.set_xlabel('Top-1 Accuracy (%)')
        ax.set_title('Genre Difficulty Ranking')
        ax.grid(True, alpha=0.3)
        
        # Add legend for difficulty levels
        from matplotlib.patches import Patch
        legend_elements = [Patch(facecolor='green', alpha=0.8, label='Easy (≥70%)'),
                          Patch(facecolor='orange', alpha=0.8, label='Medium (50-70%)'),
                          Patch(facecolor='red', alpha=0.8, label='Hard (30-50%)'),
                          Patch(facecolor='darkred', alpha=0.8, label='Very Hard (<30%)')]
        ax.legend(handles=legend_elements, loc='lower right', fontsize=10)

    def _plot_top5_vs_top1_comparison(self, ax):
        """Plot Top-5 vs Top-1 accuracy comparison"""
        genre_results = self.analyze_genre_accuracy()
        
        genres = []
        top1_accs = []
        top5_accs = []
        
        for genre, results in genre_results.items():
            if genre != "Unknown" and results['top1_correct']:
                genres.append(genre)
                top1_accs.append(np.mean(results['top1_correct']) * 100)
                top5_accs.append(np.mean(results['top5_correct']) * 100)
        
        if not genres:
            ax.text(0.5, 0.5, 'No genre data available', ha='center', va='center', transform=ax.transAxes)
            ax.set_title('Top-5 vs Top-1 Comparison')
            return
        
        # Create scatter plot
        ax.scatter(top1_accs, top5_accs, s=100, alpha=0.7, c=range(len(genres)), cmap='tab10')
        
        # Add diagonal line
        max_acc = max(max(top1_accs), max(top5_accs))
        ax.plot([0, max_acc], [0, max_acc], 'k--', alpha=0.5, label='Top-1 = Top-5')
        
        # Add genre labels
        for i, genre in enumerate(genres):
            ax.annotate(genre[:10] + '...' if len(genre) > 10 else genre, 
                       (top1_accs[i], top5_accs[i]), 
                       xytext=(5, 5), textcoords='offset points', fontsize=9)
        
        ax.set_xlabel('Top-1 Accuracy (%)')
        ax.set_ylabel('Top-5 Accuracy (%)')
        ax.set_title('Top-5 vs Top-1 Accuracy Comparison')
        ax.grid(True, alpha=0.3)
        ax.legend()

    def create_report(self):
        """Create a summary report"""
        genre_results = self.analyze_genre_accuracy()
        
        report_file = self.output_dir / 'genre_report.txt'
        
        with open(report_file, 'w') as f:
            f.write("GENRE ANALYSIS REPORT\n")
            f.write("=" * 30 + "\n\n")
            
            # Summary statistics
            genre_stats = []
            for genre, results in genre_results.items():
                if genre != "Unknown" and results['top1_correct']:
                    top1_acc = np.mean(results['top1_correct']) * 100
                    top5_acc = np.mean(results['top5_correct']) * 100
                    sample_count = len(results['top1_correct'])
                    genre_stats.append((genre, top1_acc, top5_acc, sample_count))
            
            genre_stats.sort(key=lambda x: x[1], reverse=True)
            
            f.write(f"{'Genre':<25} {'Top-1%':<8} {'Top-5%':<8} {'Samples':<8}\n")
            f.write("-" * 55 + "\n")
            
            for genre, top1, top5, samples in genre_stats:
                f.write(f"{genre:<25} {top1:6.1f}% {top5:6.1f}% {samples:6d}\n")
            
            if genre_stats:
                avg_accuracy = np.mean([stat[1] for stat in genre_stats])
                f.write(f"\nAverage accuracy: {avg_accuracy:.1f}%\n")
                f.write(f"Best genre: {genre_stats[0][0]} ({genre_stats[0][1]:.1f}%)\n")
                f.write(f"Most challenging: {genre_stats[-1][0]} ({genre_stats[-1][1]:.1f}%)\n")
        
        print(f"Created genre_report.txt")
    
    def run_analysis(self):
        """Run the complete genre analysis"""
        print("Running genre-based music identification analysis...")
        
        if not self.genre_mapping:
            print("No genre mapping available")
            return
            
        if not self.results_data:
            print("No results data available")
            return
        
        self.create_genre_plots()
        self.create_method_format_plots()
        self.create_noise_compressor_plots()
        self.create_difficulty_analysis_plots()
        self.create_report()
        
        print(f"\nAnalysis complete. Files saved to: {self.output_dir}")
        print("Generated files:")
        for file in sorted(self.output_dir.glob("*.png")):
            print(f"  - {file.name}")
        for file in sorted(self.output_dir.glob("*.txt")):
            print(f"  - {file.name}")

def main():
    results_dir = Path("results/")
    songs_genre_file = Path("songs_genre.txt")
    output_dir = Path("results/youtube_genre_analysis")
    
    if not results_dir.exists():
        print(f"Error: Results directory not found: {results_dir}")
        return 1
    
    if not songs_genre_file.exists():
        print(f"Error: Genre file not found: {songs_genre_file}")
        return 1
    
    analyzer = GenreAnalyzer(results_dir, output_dir, songs_genre_file)
    analyzer.run_analysis()
    
    return 0

if __name__ == "__main__":
    exit(main())