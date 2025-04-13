import matplotlib.pyplot as plt
from matplotlib.ticker import MaxNLocator
import numpy as np
from scipy import signal
import os
import json
import seaborn as sns
import pandas as pd

def plot_info_profile(organisms_data, output_dir):
    """Plot information profile of organisms with various filtering techniques"""
    
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
        
        # Clean up the organism name, by removing the last character
        organism = organism[:-1]
        
        # Create plot with just the final processed data
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

def plot_cross_comparison(input_folder, output_dir):
    """Create heatmap visualization of cross-comparison between top organisms."""
    cross_comparison_file = os.path.join(input_folder, 'cross_comparison.json')
    if not os.path.exists(cross_comparison_file):
        print(f"Cross-comparison file not found at {cross_comparison_file}")
        return

    print("Creating cross-comparison visualization...")

    # Load cross-comparison data
    with open(cross_comparison_file, 'r') as f:
        cross_data = json.load(f)

    if not cross_data or 'organisms' not in cross_data:
        print("No valid cross-comparison data found.")
        return

    # Extract data
    organisms = cross_data.get('organisms', [])
    nrc_matrix = cross_data.get('nrc_matrix', [])
    kld_matrix = cross_data.get('kld_matrix', [])
    k = cross_data.get('k', 'unknown')
    alpha = cross_data.get('alpha', 'unknown')

    if not organisms or not nrc_matrix:
        print("Missing required data in cross-comparison file.")
        return

    # Create shorter labels for the heatmap
    short_labels = []
    for name in organisms:
        if "|" in name:
            parts = name.split("|")
            if len(parts) >= 3:
                short_name = parts[2].strip()
            else:
                short_name = parts[0].strip()
        else:
            name_parts = name.split(" ")
            if len(name_parts) > 2:
                short_name = " ".join(name_parts[0:2])
            else:
                short_name = name
        short_labels.append(short_name)

    # Create NRC heatmap
    plt.figure(figsize=(14, 12))
    nrc_array = np.array(nrc_matrix)

    plt.figure(figsize=(14, 12))
    ax = sns.heatmap(
        nrc_array,
        annot=True,
        fmt='.3f',
        cmap='YlGnBu_r',
        xticklabels=short_labels,
        yticklabels=short_labels
    )
    plt.title(f'Cross-Comparison NRC Values (k={k}, α={alpha})')
    plt.xlabel('Target Organism (being compressed)')
    plt.ylabel('Reference Organism (providing model)')
    plt.xticks(rotation=45, ha='right')
    plt.yticks(rotation=0)
    plt.tight_layout()
    plt.savefig(os.path.join(output_dir, 'cross_comparison_nrc.png'))
    plt.close()

    # Create KLD heatmap if available
    if kld_matrix:
        plt.figure(figsize=(14, 12))
        kld_array = np.array(kld_matrix)

        # Create heatmap
        ax = sns.heatmap(
            kld_array,
            annot=True,
            fmt='.3f',
            cmap='YlOrRd',
            xticklabels=short_labels,
            yticklabels=short_labels
        )
        plt.title(f'Cross-Comparison KLD Values (k={k}, α={alpha})')
        plt.xlabel('Target Organism (being compressed)')
        plt.ylabel('Reference Organism (providing model)')
        plt.xticks(rotation=45, ha='right')
        plt.yticks(rotation=0)
        plt.tight_layout()
        plt.savefig(os.path.join(output_dir, 'cross_comparison_kld.png'))
        plt.close()

    print(f"Cross-comparison visualizations saved to {output_dir}")

def plot_complexity_profile(organism_data, output_dir, nrc_threshold=0.05):
    """
    Generate a complexity profile plot comparing two coronavirus genomes with NRC < threshold.
    """
    # Filter for coronavirus data files
    coronavirus_files = [f for f in organism_data.keys() if 'coronavirus' in f.lower()]

    if len(coronavirus_files) < 2:
        print("Not enough coronavirus files found to generate complexity profile")
        return

    # Get the first two coronavirus files
    organism1_key = coronavirus_files[0]
    organism2_key = coronavirus_files[1]

    # Get the data
    organism1_data = organism_data[organism1_key]
    organism2_data = organism_data[organism2_key]

    # Extract positions and complexity values
    positions1 = organism1_data.iloc[:, 0].astype(int).values
    complexity1 = organism1_data.iloc[:, 2].astype(float).values

    positions2 = organism2_data.iloc[:, 0].astype(int).values
    complexity2 = organism2_data.iloc[:, 2].astype(float).values

    # Create the plot
    plt.figure(figsize=(14, 7))

    # Plot both profiles
    plt.plot(positions1, complexity1, 'b-', alpha=0.7, label=organism1_key)
    plt.plot(positions2, complexity2, 'r-', alpha=0.7, label=organism2_key)

    plt.xlabel('Genomic Position', fontsize=12)
    plt.ylabel('Complexity Value', fontsize=12)
    plt.title('Coronavirus Genome Complexity Profile Comparison', fontsize=14)
    plt.legend(loc='best')
    plt.grid(True, alpha=0.3)
    plt.gca().xaxis.set_major_locator(MaxNLocator(integer=True))

    # Add annotations for high complexity regions
    threshold = np.percentile(np.concatenate([complexity1, complexity2]), 95)  # Top 5% of values

    high_complexity1 = positions1[complexity1 > threshold]
    if len(high_complexity1) > 0:
        plt.axvspan(min(high_complexity1), max(high_complexity1), alpha=0.2, color='blue')

    high_complexity2 = positions2[complexity2 > threshold]
    if len(high_complexity2) > 0:
        plt.axvspan(min(high_complexity2), max(high_complexity2), alpha=0.2, color='red')

    # Save the plot
    plt.tight_layout()
    output_path = os.path.join(output_dir, 'coronavirus_complexity_profile.png')
    plt.savefig(output_path, dpi=300)
    plt.close()

    print(f"Generated coronavirus complexity profile: {output_path}")

    # Create a second plot showing complexity difference
    plt.figure(figsize=(14, 7))

    # Interpolate to match positions if needed
    if len(positions1) != len(positions2) or not np.array_equal(positions1, positions2):
        # Use the shorter sequence length as reference
        max_pos = min(max(positions1), max(positions2))
        common_positions = np.arange(1, max_pos + 1)

        # Interpolate values for organism1
        complexity1_interp = np.interp(common_positions, positions1, complexity1)

        # Interpolate values for organism2
        complexity2_interp = np.interp(common_positions, positions2, complexity2)

        # Calculate difference
        diff = complexity1_interp - complexity2_interp
        plt.plot(common_positions, diff, 'g-', alpha=0.7)
    else:
        # Calculate difference directly
        diff = complexity1 - complexity2
        plt.plot(positions1, diff, 'g-', alpha=0.7)

    plt.axhline(y=0, color='r', linestyle='-', alpha=0.3)
    plt.xlabel('Genomic Position', fontsize=12)
    plt.ylabel('Complexity Difference', fontsize=12)
    plt.title('Coronavirus Complexity Difference Profile', fontsize=14)
    plt.grid(True, alpha=0.3)

    # Save the difference plot
    plt.tight_layout()
    diff_output_path = os.path.join(output_dir, 'coronavirus_complexity_difference.png')
    plt.savefig(diff_output_path, dpi=300)
    plt.close()

    print(f"Generated coronavirus complexity difference profile: {diff_output_path}")
