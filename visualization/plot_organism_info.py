import matplotlib.pyplot as plt
import numpy as np
from scipy import signal
import os
import json
import seaborn as sns

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