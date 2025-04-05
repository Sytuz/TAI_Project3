import json
import matplotlib.pyplot as plt
import seaborn as sns
import pandas as pd
import numpy as np
from collections import Counter
import os
import argparse
from matplotlib import cm

def load_data(filepath):
    """Load the JSON results file"""
    with open(filepath, 'r') as file:
        data = json.load(file)
    return data

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

def main():
    # Set up command line argument parsing
    parser = argparse.ArgumentParser(description='Visualize NRC results from JSON data.')
    parser.add_argument('--input', '-i', type=str, default='results/test_results_20250401_133812.json',
                        help='Path to the JSON results file (default: results/test_results_20250401_133812.json)')
    parser.add_argument('--output', '-o', type=str, default='visualization_results',
                        help='Directory to save visualization results (default: visualization_results)')
    args = parser.parse_args()
    
    # Create output directory if it doesn't exist
    output_dir = args.output
    os.makedirs(output_dir, exist_ok=True)
    
    # Find and load data
    input_filepath = args.input
    
    # If the file doesn't exist at specified path, try some common relative paths
    if not os.path.exists(input_filepath):
        possible_paths = [
            f'results/{os.path.basename(input_filepath)}',
            f'../results/{os.path.basename(input_filepath)}',
            f'../{input_filepath}'
        ]
        
        for path in possible_paths:
            if os.path.exists(path):
                input_filepath = path
                print(f"Found input file at: {path}")
                break
    
    if not os.path.exists(input_filepath):
        print(f"Error: Could not find input file at {input_filepath}")
        print("Please provide a valid file path using the --input argument.")
        return
    
    data = load_data(input_filepath)
    df = process_data(data)
    
    # Generate plots
    plot_top_organisms_nrc(df, output_dir)
    plot_execution_time(df, output_dir)
    plot_rank_stability(df, output_dir)
    plot_parameter_influence(df, output_dir)
    
    # Generate summary statistics
    top_organism = Counter(df[df['rank'] == 1]['name']).most_common(1)[0][0]
    best_nrc = df[df['rank'] == 1]['nrc'].min()
    best_row = df.loc[df[df['rank'] == 1]['nrc'].idxmin()]
    
    print("=== SUMMARY STATISTICS ===")
    print(f"Most frequent top-ranking organism: {top_organism}")
    print(f"Best NRC value: {best_nrc:.6f}")
    print(f"Best parameters: k={best_row['k']}, alpha={best_row['alpha']}")
    
    print(f"\nVisualization complete. Results saved to '{output_dir}' directory")
    
    # Also write the summary to a file in the output directory
    with open(os.path.join(output_dir, 'summary.txt'), 'w') as f:
        f.write("=== SUMMARY STATISTICS ===\n")
        f.write(f"Input file: {input_filepath}\n")
        f.write(f"Most frequent top-ranking organism: {top_organism}\n")
        f.write(f"Best NRC value: {best_nrc:.6f}\n")
        f.write(f"Best parameters: k={best_row['k']}, alpha={best_row['alpha']}\n")

if __name__ == "__main__":
    main()
