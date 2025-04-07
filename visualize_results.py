import json
import matplotlib.pyplot as plt
import seaborn as sns
import pandas as pd
import numpy as np
from collections import Counter
import os
import argparse
from matplotlib import cm
from scipy import signal
import datetime
import shutil

def load_data(filepath):
    """Load the JSON results file"""
    with open(filepath, 'r') as file:
        data = json.load(file)
    return data

def load_organism_symbols(filepath):
    """Load organism symbol information from CSV files"""
    organism_data = {}
    
    # Get list of all symbol info files
    symbol_info_dir = os.path.join(os.path.dirname(filepath), 'symbol_info')
    if os.path.exists(symbol_info_dir):
        # Iterate through each CSV file in the directory
        for filename in os.listdir(symbol_info_dir):
            if filename.endswith('.csv'):
                file_path = os.path.join(symbol_info_dir, filename)
                try:
                    # Extract organism name from filename (remove .csv extension)
                    # Example filename: rank1_OR353425.1_Octopus_vulgaris_mitochondrion
                    # Extract only the name, which is: OR353425.1_Octopus_vulgaris_mitochondrion
                    organism_name = '_'.join(filename.split('_')[1:]).split(',')[0].replace('.csv', '')
                    # Load CSV into dataframe
                    organism_df = pd.read_csv(file_path)
                    
                    # Store dataframe in dictionary with organism name as key
                    organism_data[organism_name] = organism_df
                    print(f"Loaded symbol data for {organism_name}")
                except Exception as e:
                    print(f"Error loading {filename}: {e}")
    else:
        print(f"Warning: Symbol info directory '{symbol_info_dir}' not found")
    
    return organism_data

def process_data(data):
    """Convert JSON data to pandas DataFrame"""
    results = []
    for experiment in data:
        alpha = experiment['alpha']
        k = experiment['k']
        exec_time = experiment['execTime_ms']
        
        for ref in experiment['references']:
            # Create a shorter name for display purposes
            full_name = ref['name']
            if "|" in full_name:
                parts = full_name.split("|")
                if len(parts) >= 3:
                    short_name = parts[2].strip()
                else:
                    short_name = parts[0].strip()
            else:
                name_parts = full_name.split(" ")
                if len(name_parts) > 2:
                    short_name = " ".join(name_parts[0:2])
                else:
                    short_name = full_name
            
            results.append({
                'alpha': alpha,
                'k': k,
                'execTime_ms': exec_time,
                'name': ref['name'],
                'short_name': short_name,
                'kld': ref['kld'],
                'nrc': ref['nrc'],
                'rank': ref['rank']
            })
    
    return pd.DataFrame(results)
        
def plot_top_organisms_info_profile(organisms_data, output_dir):
    """Plot information profile of top organisms with various filtering techniques"""
    
    for organism in organisms_data:
        org_df = organisms_data[organism]
        
        # Extract data
        positions = org_df['Position'].values
        information = org_df['Information'].values
        
        # Apply low-pass filtering using averaging with a Blackman window of size 21
        window_size = 21
        blackman_window = np.blackman(window_size)
        blackman_window = blackman_window / sum(blackman_window)  # Normalize for averaging
        
        # We need to handle edge effects, so pad the signal
        padded_info = np.pad(information, (window_size//2, window_size//2), mode='edge')
        
        # Apply the filter
        filtered_info = signal.convolve(padded_info, blackman_window, mode='valid')
        
        # Process in reverse direction (for comparison)
        reversed_info = np.flip(information)
        padded_reversed = np.pad(reversed_info, (window_size//2, window_size//2), mode='edge')
        filtered_reversed = signal.convolve(padded_reversed, blackman_window, mode='valid')
        filtered_reversed = np.flip(filtered_reversed)  # Flip back to original direction
        
        # Combine according to minimum value
        combined_info = np.minimum(filtered_info, filtered_reversed)
        
        # Create plots
        plt.figure(figsize=(15, 10))
        
        # Original data
        plt.subplot(3, 1, 1)
        plt.plot(positions, information, linestyle='-', color='green')
        plt.title(f'Original Information Profile for {organism}')
        plt.ylabel('Information')
        plt.grid(True, linestyle='--', alpha=0.7)
        
        # Filtered data
        plt.subplot(3, 1, 2)
        plt.plot(positions, filtered_info, linestyle='-', color='blue')
        plt.title('Low-pass Filtered with Blackman Window (size 21)')
        plt.ylabel('Information')
        plt.grid(True, linestyle='--', alpha=0.7)
        
        # Combined minimum from both directions
        plt.subplot(3, 1, 3)
        plt.plot(positions, combined_info, linestyle='-', color='black')
        plt.title('Combined Minimum from Both Directions')
        plt.xlabel('Position')
        plt.ylabel('Information')
        plt.grid(True, linestyle='--', alpha=0.7)
        
        plt.xticks(rotation=45)
        plt.tight_layout()
        plt.savefig(os.path.join(output_dir, f'info_profile_{organism}.png'))
        plt.close()
        
        # Also save a version with just the final processed data for cleaner visualization
        plt.figure(figsize=(12, 6))
        plt.plot(positions, combined_info, linestyle='-', color='black')
        plt.title(f'Processed Information Profile for {organism}')
        plt.xlabel('Position')
        plt.ylabel('Information')
        plt.grid(True, linestyle='--', alpha=0.7)
        plt.xticks(rotation=45)
        plt.tight_layout()
        plt.savefig(os.path.join(output_dir, f'info_profile_processed_{organism}.png'))
        plt.close()

def plot_top_organisms_nrc(df, output_dir):
    """Plot NRC values of top-ranked organisms as a 3D mesh surface"""
    # Filter for only rank 1 data
    top_rank_data = df[df['rank'] == 1]
    
    # Create 3D figure
    fig = plt.figure(figsize=(14, 12))
    ax = fig.add_subplot(111, projection='3d')
    
    # Get unique k and alpha values, sorted
    k_values = sorted(top_rank_data['k'].unique())
    alpha_values = sorted(top_rank_data['alpha'].unique())
    
    # Create a 2D grid of k and alpha values
    k_grid, alpha_grid = np.meshgrid(k_values, alpha_values)
    
    # Initialize NRC values array with NaN
    nrc_values = np.zeros((len(alpha_values), len(k_values)))
    nrc_values.fill(np.nan)
    
    # Fill NRC values grid
    for i, alpha in enumerate(alpha_values):
        for j, k in enumerate(k_values):
            subset = top_rank_data[(top_rank_data['alpha'] == alpha) & (top_rank_data['k'] == k)]
            if not subset.empty:
                nrc_values[i, j] = subset.iloc[0]['nrc']
                # Annotate with organism name
                organism_name = subset.iloc[0]['short_name']
                ax.text(k, alpha, subset.iloc[0]['nrc'], 
                       f"{organism_name}", fontsize=8)
    
    # Create the surface plot with a colormap
    surf = ax.plot_surface(k_grid, alpha_grid, nrc_values, 
                          cmap=cm.viridis, edgecolor='none', alpha=0.8)
    
    # Add color bar
    cbar = fig.colorbar(surf, ax=ax, shrink=0.5, aspect=5)
    cbar.set_label('NRC Value')
    
    # Setup axes and labels
    ax.set_xlabel('k value')
    ax.set_ylabel('Alpha value')
    ax.set_zlabel('NRC')
    ax.set_title('3D Surface Plot of Top-Ranked Organism NRC Values')
    
    # Set better viewing angle
    ax.view_init(elev=30, azim=45)
    
    plt.tight_layout()
    plt.savefig(os.path.join(output_dir, 'nrc_3d_surface.png'))
    plt.close()
    
    # Also create a 2D heatmap for better readability
    plt.figure(figsize=(12, 8))
    heatmap = sns.heatmap(nrc_values, annot=True, fmt='.3f', 
                         xticklabels=k_values, yticklabels=alpha_values, 
                         cmap='viridis')
    plt.title('NRC Values of Top-Ranked Organisms (Rank 1)')
    plt.xlabel('k value')
    plt.ylabel('Alpha value')
    plt.tight_layout()
    plt.savefig(os.path.join(output_dir, 'top_organism_nrc_heatmap.png'))
    plt.close()

def plot_execution_time(df, output_dir):
    """Plot execution time analysis"""
    # Extract unique combinations
    unique_combos = df.drop_duplicates(subset=['alpha', 'k', 'execTime_ms'])
    
    plt.figure(figsize=(12, 5))
    
    # Plot 1: Time vs Alpha for different k
    plt.subplot(1, 2, 1)
    for k in sorted(unique_combos['k'].unique()):
        subset = unique_combos[unique_combos['k'] == k]
        plt.plot(subset['alpha'], subset['execTime_ms']/1000, 'o-', label=f'k={k}')
    
    plt.title('Execution Time vs Alpha')
    plt.xlabel('Alpha')
    plt.ylabel('Time (seconds)')
    plt.legend()
    plt.grid(True)
    
    # Plot 2: Time vs k for different alpha
    plt.subplot(1, 2, 2)
    for alpha in sorted(unique_combos['alpha'].unique()):
        subset = unique_combos[unique_combos['alpha'] == alpha]
        plt.plot(subset['k'], subset['execTime_ms']/1000, 'o-', label=f'α={alpha}')
    
    plt.title('Execution Time vs k')
    plt.xlabel('k')
    plt.ylabel('Time (seconds)')
    plt.legend()
    plt.grid(True)
    
    plt.tight_layout()
    plt.savefig(os.path.join(output_dir, 'execution_time.png'))
    plt.close()

def plot_rank_stability(df, output_dir):
    """Create a heatmap showing rank stability across parameters, ordered by overall performance"""
    plt.figure(figsize=(16, 10))
    
    # Find organisms that appear in top 5 most frequently
    top_ranks = df[df['rank'] <= 5]
    organism_counts = Counter(top_ranks['name'])
    
    # Calculate a score for each organism (sum of all ranks)
    organism_scores = {}
    for organism in organism_counts.keys():
        # Get all ranks for this organism
        organism_data = df[df['name'] == organism]
        # Calculate score as sum of ranks (lower is better)
        organism_scores[organism] = organism_data['rank'].sum()
    
    # Sort organisms by their score (ascending - lower is better)
    sorted_organisms = sorted(organism_scores.items(), key=lambda x: x[1])
    
    # Take the top 10 organisms (lowest scores)
    common_top_organisms = [organism for organism, score in sorted_organisms[:10]]
    
    # Create a matrix of ranks
    rank_matrix = np.zeros((len(common_top_organisms), len(df['k'].unique()) * len(df['alpha'].unique())))
    row_labels = []
    
    for i, organism in enumerate(common_top_organisms):
        # Create shorter row labels for display
        org_parts = organism.split(' ')
        if len(org_parts) > 2:
            row_labels.append(" ".join(org_parts[0:2]))
        else:
            row_labels.append(organism.split(' ')[0])
        
        # Add the score to the label
        row_labels[i] = f"{row_labels[i]} (score: {organism_scores[organism]:.1f})"
        
        col_idx = 0
        
        for k in sorted(df['k'].unique()):
            for alpha in sorted(df['alpha'].unique()):
                subset = df[(df['name'] == organism) & (df['k'] == k) & (df['alpha'] == alpha)]
                if not subset.empty:
                    rank_matrix[i, col_idx] = subset.iloc[0]['rank']
                else:
                    rank_matrix[i, col_idx] = np.nan
                
                col_idx += 1
    
    # Create heatmap
    ax = sns.heatmap(rank_matrix, annot=True, cmap='YlGnBu_r', cbar_kws={'label': 'Rank'}, 
                   yticklabels=row_labels, xticklabels=[])
    plt.title('Rank Stability Across Parameters (Ordered by Overall Performance)')
    plt.ylabel('Organism')
    plt.xlabel('Parameter Combinations (k,α)')
    
    # Add custom x-ticks at the middle of each k group
    alpha_vals = len(df['alpha'].unique())
    ticks = []
    tick_labels = []
    
    for i, k in enumerate(sorted(df['k'].unique())):
        ticks.append(i * alpha_vals + alpha_vals // 2)
        tick_labels.append(f'k={k}')
    
    plt.xticks(ticks, tick_labels)
    
    plt.tight_layout()
    plt.savefig(os.path.join(output_dir, 'rank_stability.png'))
    plt.close()

def plot_parameter_influence(df, output_dir):
    """Create heatmap showing how parameters influence NRC for top organisms"""
    # Get top 3 most consistent organisms
    top_ranks = df[df['rank'] <= 3]
    top_organisms = [name for name, _ in Counter(top_ranks['name']).most_common(3)]
    
    for organism in top_organisms:
        plt.figure(figsize=(10, 6))
        org_data = df[df['name'] == organism]
        
        # Create pivot table for heatmap
        pivot_data = org_data.pivot_table(
            values='nrc', index='k', columns='alpha', aggfunc='mean'
        )
        
        # Plot heatmap
        sns.heatmap(pivot_data, annot=True, cmap='YlGnBu', fmt='.3f')
        
        short_name = org_data.iloc[0]['short_name']
        plt.title(f'NRC Values for {short_name} Across Parameters')
        plt.ylabel('k value')
        plt.xlabel('Alpha value')
        
        plt.tight_layout()
        safe_name = short_name.replace('|', '_').replace(' ', '_')
        plt.savefig(os.path.join(output_dir, f'parameter_influence_{safe_name}.png'))
        plt.close()

def plot_nrc_boxplot_by_k(df, output_dir):
    """Create boxplots showing the distribution of NRC values for each k value with outlier tracking"""
    plt.figure(figsize=(14, 8))
    
    # First create a boxplot for all data
    ax1 = plt.subplot(1, 2, 1)
    box_plot = sns.boxplot(x='k', y='nrc', data=df, palette='viridis', showfliers=True)
    plt.title('Distribution of NRC Values by k')
    plt.xlabel('k value')
    plt.ylabel('NRC')
    plt.grid(True, linestyle='--', alpha=0.7)
    
    # Then create a boxplot for only top ranking (rank <= 5) organisms
    ax2 = plt.subplot(1, 2, 2)
    top_data = df[df['rank'] <= 5]
    sns.boxplot(x='k', y='nrc', data=top_data, palette='viridis', showfliers=True)
    plt.title('NRC Values for Top 5 Ranking Organisms by k')
    plt.xlabel('k value')
    plt.ylabel('NRC')
    plt.grid(True, linestyle='--', alpha=0.7)
    
    # Add annotations for median NRC values
    for ax, data in [(ax1, df), (ax2, top_data)]:
        # Calculate median NRC for each k value
        k_values = sorted(data['k'].unique())
        for i, k in enumerate(k_values):
            k_data = data[data['k'] == k]
            median_nrc = k_data['nrc'].median()
            ax.text(i, median_nrc + 0.02, f'{median_nrc:.3f}', 
                    horizontalalignment='center', size='small', 
                    color='black', weight='semibold')
    
    plt.tight_layout()
    plt.savefig(os.path.join(output_dir, 'nrc_boxplot_by_k.png'))
    plt.close()
    
    # Create a more detailed plot that shows the effect of alpha for each k
    plt.figure(figsize=(15, 10))
    # Use different colors for different alpha values
    sns.boxplot(x='k', y='nrc', hue='alpha', data=df, palette='viridis', showfliers=True)
    plt.title('Distribution of NRC Values by k and Alpha')
    plt.xlabel('k value')
    plt.ylabel('NRC')
    plt.grid(True, linestyle='--', alpha=0.7)
    plt.legend(title='Alpha value')
    
    plt.tight_layout()
    plt.savefig(os.path.join(output_dir, 'nrc_boxplot_by_k_and_alpha.png'))
    plt.close()
    
    # Generate outlier statistics and tracking
    track_outliers(df, output_dir)

def track_outliers(df, output_dir):
    """Track and identify outliers in the NRC values across different k values"""
    outliers_data = []
    
    # For each k value, identify outliers
    for k in sorted(df['k'].unique()):
        k_data = df[df['k'] == k]
        
        # Calculate IQR for this k value
        q1 = k_data['nrc'].quantile(0.25)
        q3 = k_data['nrc'].quantile(0.75)
        iqr = q3 - q1
        
        # Define outlier bounds (standard 1.5*IQR rule)
        lower_bound = q1 - 1.5 * iqr
        upper_bound = q3 + 1.5 * iqr
        
        # Find outliers
        outliers = k_data[(k_data['nrc'] < lower_bound) | (k_data['nrc'] > upper_bound)]
        
        # Calculate median for calculating deviation
        median = k_data['nrc'].median()
        
        # Add outlier details to our list
        for _, row in outliers.iterrows():
            deviation = row['nrc'] - median
            percent_deviation = (deviation / median) * 100 if median != 0 else float('inf')
            
            outliers_data.append({
                'k': k,
                'alpha': row['alpha'],
                'organism': row['short_name'],
                'full_name': row['name'],
                'nrc': row['nrc'],
                'median_nrc': median,
                'deviation': deviation,
                'percent_deviation': percent_deviation,
                'rank': row['rank']
            })
    
    # If we found outliers, save them to CSV
    if outliers_data:
        outliers_df = pd.DataFrame(outliers_data)
        outliers_df.sort_values(by=['k', 'percent_deviation'], ascending=[True, False], inplace=True)
        outliers_df.to_csv(os.path.join(output_dir, 'nrc_outliers.csv'), index=False)
        
        # Create a summary plot of the most frequent outlier organisms
        plt.figure(figsize=(12, 6))
        outlier_counts = Counter(outliers_df['organism'])
        top_outliers = pd.DataFrame({
            'Organism': [name for name, _ in outlier_counts.most_common(10)],
            'Count': [count for _, count in outlier_counts.most_common(10)]
        })
        
        if not top_outliers.empty:
            sns.barplot(x='Count', y='Organism', data=top_outliers, palette='viridis')
            plt.title('Most Frequent NRC Outlier Organisms')
            plt.xlabel('Number of times identified as outlier')
            plt.tight_layout()
            plt.savefig(os.path.join(output_dir, 'nrc_outliers_summary.png'))
            plt.close()
            
            # Also create a visualization showing the distribution of outlier deviations
            plt.figure(figsize=(12, 6))
            sns.histplot(outliers_df['percent_deviation'], bins=20, kde=True)
            plt.title('Distribution of Outlier Deviations from Median NRC')
            plt.xlabel('Percent Deviation from Median (%)')
            plt.ylabel('Frequency')
            plt.axvline(x=0, color='red', linestyle='--')
            plt.grid(True, linestyle='--', alpha=0.7)
            plt.tight_layout()
            plt.savefig(os.path.join(output_dir, 'nrc_outliers_deviation.png'))
            plt.close()
        
        # Add summary to the summary.txt file
        with open(os.path.join(output_dir, 'outliers_summary.txt'), 'w') as f:
            f.write("=== OUTLIER ANALYSIS ===\n")
            f.write(f"Total outliers detected: {len(outliers_data)}\n")
            f.write(f"Number of unique outlier organisms: {len(outlier_counts)}\n\n")
            f.write("TOP OUTLIER ORGANISMS:\n")
            for organism, count in outlier_counts.most_common(10):
                f.write(f"{organism}: {count} occurrences\n")
            
            # Also add statistics by k value
            f.write("\nOUTLIERS BY K VALUE:\n")
            for k in sorted(outliers_df['k'].unique()):
                k_outliers = outliers_df[outliers_df['k'] == k]
                f.write(f"k={k}: {len(k_outliers)} outliers\n")

def get_timestamp_from_info(input_folder):
    """Extract timestamp from info.txt file in the test results folder"""
    info_file_path = os.path.join(input_folder, 'info.txt')
    
    if os.path.exists(info_file_path):
        with open(info_file_path, 'r') as f:
            for line in f:
                if line.startswith('Date and Time:'):
                    # Extract timestamp from the line
                    timestamp = line.split(':', 1)[1].strip()
                    return timestamp
    
    # If we can't find the timestamp in the info file, generate a current timestamp as fallback
    current_time = datetime.datetime.now()
    timestamp = current_time.strftime("%Y%m%d_%H%M%S")
    print(f"Warning: Could not find timestamp in info.txt, using current time: {timestamp}")
    return timestamp

def main():
    # Set up command line argument parsing
    parser = argparse.ArgumentParser(description='Visualize NRC results from JSON data.')
    parser.add_argument('--input', '-i', type=str, default='results/latest',
                        help='Path to the test results folder (default: results/latest)')
    parser.add_argument('--output', '-o', type=str, default='visualization_results',
                        help='Base directory for visualization results (default: visualization_results)')
    args = parser.parse_args()
    
    # Find and load data
    input_folder = args.input
    
    # Get timestamp from info.txt in the input folder
    timestamp = get_timestamp_from_info(input_folder)
    
    # Create both output directories
    base_output_dir = args.output
    if not os.path.exists(base_output_dir):
        os.makedirs(base_output_dir)
    
    # Create timestamped directory
    timestamp_dir = os.path.join(base_output_dir, timestamp)
    os.makedirs(timestamp_dir, exist_ok=True)
    
    # Create/recreate latest directory
    latest_dir = os.path.join(base_output_dir, "latest")
    if os.path.exists(latest_dir):
        shutil.rmtree(latest_dir)
    os.makedirs(latest_dir, exist_ok=True)
    
    print(f"Visualization outputs will be saved to:")
    print(f"  - {os.path.abspath(timestamp_dir)}")
    print(f"  - {os.path.abspath(latest_dir)}")
    
    # Find test results file
    test_results_filepath = os.path.join(input_folder, 'test_results.json')
    
    # If the file doesn't exist at specified path, try some common relative paths
    if not os.path.exists(test_results_filepath):
        possible_paths = [
            'results/latest/test_results.json',
            '../results/latest/test_results.json',
            'results/test_results.json'
        ]
        
        for path in possible_paths:
            if os.path.exists(path):
                test_results_filepath = path
                print(f"Found input file at: {path}")
                break
    
    if not os.path.exists(test_results_filepath):
        print(f"Error: Could not find input file at {test_results_filepath}")
        print("Please provide a valid file path using the --input argument.")
        return
    
    data = load_data(test_results_filepath)
    df = process_data(data)
    
    # Load organism symbol data
    organism_data = load_organism_symbols(os.path.join(input_folder, 'symbol_info'))
    
    # Generate plots to both directories
    for output_dir in [timestamp_dir, latest_dir]:
        if organism_data:
            plot_top_organisms_info_profile(organism_data, output_dir)
        plot_top_organisms_nrc(df, output_dir)
        plot_execution_time(df, output_dir)
        plot_rank_stability(df, output_dir)
        plot_parameter_influence(df, output_dir)
        plot_nrc_boxplot_by_k(df, output_dir)
        
        # Generate summary statistics
        top_organism = Counter(df[df['rank'] == 1]['name']).most_common(1)[0][0]
        best_nrc = df[df['rank'] == 1]['nrc'].min()
        best_row = df.loc[df[df['rank'] == 1]['nrc'].idxmin()]
        
        # Write the summary to a file in the output directory
        with open(os.path.join(output_dir, 'summary.txt'), 'w') as f:
            f.write("=== SUMMARY STATISTICS ===\n")
            f.write(f"Input file: {test_results_filepath}\n")
            f.write(f"Timestamp: {timestamp}\n")
            f.write(f"Most frequent top-ranking organism: {top_organism}\n")
            f.write(f"Best NRC value: {best_nrc:.6f}\n")
            f.write(f"Best parameters: k={best_row['k']}, alpha={best_row['alpha']}\n")
    
    print("=== SUMMARY STATISTICS ===")
    print(f"Most frequent top-ranking organism: {top_organism}")
    print(f"Best NRC value: {best_nrc:.6f}")
    print(f"Best parameters: k={best_row['k']}, alpha={best_row['alpha']}")
    
    print(f"\nVisualization complete. Results saved to both directories.")

if __name__ == "__main__":
    main()
