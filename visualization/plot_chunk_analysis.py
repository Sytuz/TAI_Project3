import json
import matplotlib.pyplot as plt
import numpy as np
from matplotlib.patches import Rectangle
import os

def plot_chunk_analysis(chunk_file, output_dir):
    """
    Create visualization of best matching organisms across sequence positions 
    from chunk analysis data.
    """
    print(f"Creating chunk analysis visualization...")
    
    # Set a reliable font family
    plt.rcParams['font.family'] = 'Arial'
    
    # Load chunk analysis data
    with open(chunk_file, 'r') as f:
        chunk_data = json.load(f)
    
    # Extract chunks
    chunks = chunk_data.get('chunks', [])
    if not chunks:
        print("No chunk data found.")
        return
    
    # Get unique organisms
    organisms = sorted(list(set(chunk['best_match'] for chunk in chunks)))
    organism_mapping = {org: i for i, org in enumerate(organisms)}
    
    # Create positions and values for plotting
    positions = []
    end_positions = []
    org_indices = []
    nrc_values = []
    
    for chunk in chunks:
        positions.append(chunk['position'])
        end_positions.append(chunk['position'] + chunk['length'])
        org_indices.append(organism_mapping[chunk['best_match']])
        nrc_values.append(chunk['best_nrc'])
    
    # Find min and max NRC for normalization
    min_nrc = min(nrc_values)
    max_nrc = max(nrc_values)
    
    create_standard_visualization(positions, end_positions, org_indices, nrc_values, 
                              min_nrc, max_nrc, organisms, chunk_data, output_dir)
    
    create_highres_visualization(positions, end_positions, org_indices, nrc_values, 
                             min_nrc, max_nrc, organisms, chunk_data, output_dir)
    
    print(f"Chunk analysis visualizations saved to {output_dir}")

def create_standard_visualization(positions, end_positions, org_indices, nrc_values, 
                               min_nrc, max_nrc, organisms, chunk_data, output_dir):
    """Create standard resolution visualization"""
    # Create plot
    plt.figure(figsize=(15, 8))
    
    # Use only one colormap for NRC intensity
    nrc_cmap = plt.cm.get_cmap('viridis')
    
    # Create a scatter for NRC colorbar
    scatter = plt.scatter([], [], c=[], cmap=nrc_cmap, vmin=min_nrc, vmax=max_nrc)
    
    # Plot each chunk with NRC value represented by color
    for i in range(len(positions)):
        # Normalize NRC value to 0-1 range for color mapping
        norm_nrc = (nrc_values[i] - min_nrc) / (max_nrc - min_nrc) if max_nrc > min_nrc else 0.5
        
        # Create a patch for this chunk
        chunk_width = end_positions[i] - positions[i]
        rect = Rectangle(
            (positions[i], org_indices[i] - 0.4),
            chunk_width,
            0.8,
            color=nrc_cmap(norm_nrc),
            alpha=0.8  # Fixed alpha since color already represents NRC
        )
        
        plt.gca().add_patch(rect)
        
        # Add NRC value text if chunk is wide enough
        if chunk_width > (max(positions) - min(positions)) * 0.05:  # Only add text if chunk is wide enough
            plt.text(
                positions[i] + chunk_width/2,
                org_indices[i],
                f"{nrc_values[i]:.3f}",
                ha='center',
                va='center',
                fontsize=8,
                color='black',
                bbox=dict(boxstyle="round,pad=0.1", fc="white", ec="gray", alpha=0.7)
            )
    
    # Add a colorbar for NRC values
    cbar = plt.colorbar(scatter)
    cbar.set_label('NRC Score')
    
    # Compress long organism names
    compressed_labels = prepare_compressed_labels(organisms)
    
    format_chunk_plot(plt, positions, end_positions, compressed_labels, organisms, chunk_data, 
                   'Best Matching Organisms Across Sequence Positions')
    
    # Save figure
    plt.savefig(os.path.join(output_dir, 'chunk_analysis_visualization.png'), dpi=300, bbox_inches='tight')
    plt.close()

def create_highres_visualization(positions, end_positions, org_indices, nrc_values, 
                              min_nrc, max_nrc, organisms, chunk_data, output_dir):
    """Create high resolution visualization"""
    # Create a higher-resolution version for detailed viewing
    plt.figure(figsize=(20, 10))
    
    # Use only one colormap for NRC intensity
    nrc_cmap = plt.cm.get_cmap('viridis')
    
    # Plot each chunk in high-res version
    for i in range(len(positions)):
        # Normalize NRC value to 0-1 range for color mapping
        norm_nrc = (nrc_values[i] - min_nrc) / (max_nrc - min_nrc) if max_nrc > min_nrc else 0.5
        
        # Create a patch for this chunk
        chunk_width = end_positions[i] - positions[i]
        rect = Rectangle(
            (positions[i], org_indices[i] - 0.4),
            chunk_width,
            0.8,
            color=nrc_cmap(norm_nrc),
            alpha=0.8  # Fixed alpha since color already represents NRC
        )
        
        plt.gca().add_patch(rect)
        
        # Add NRC value text
        plt.text(
            positions[i] + chunk_width/2,
            org_indices[i],
            f"{nrc_values[i]:.3f}",
            ha='center',
            va='center',
            fontsize=8,
            color='black',
            bbox=dict(boxstyle="round,pad=0.1", fc="white", ec="gray", alpha=0.7)
        )
    
    # Add colorbar for high-res version
    scatter = plt.scatter([], [], c=[], cmap=nrc_cmap, vmin=min_nrc, vmax=max_nrc)
    cbar = plt.colorbar(scatter)
    cbar.set_label('NRC Score')
    
    # Compress long organism names
    compressed_labels = prepare_compressed_labels(organisms)
    
    format_chunk_plot(plt, positions, end_positions, compressed_labels, organisms, chunk_data,
                   'Best Matching Organisms Across Sequence Positions (High Resolution)')
    
    plt.savefig(os.path.join(output_dir, 'chunk_analysis_visualization_hires.png'), dpi=600, bbox_inches='tight')
    plt.close()

def prepare_compressed_labels(organisms):
    """Compress long organism names for readability"""
    compressed_labels = []
    for org in organisms:
        if len(org) > 50:
            parts = org.split(' ')
            if len(parts) > 2:
                compressed_labels.append(f"{parts[0]} {parts[1]}...")
            else:
                compressed_labels.append(org[:47] + "...")
        else:
            compressed_labels.append(org)
    return compressed_labels

def format_chunk_plot(plt_obj, positions, end_positions, compressed_labels, organisms, chunk_data, title):
    """Format the chunk analysis plot with consistent styling"""
    plt_obj.yticks(range(len(organisms)), compressed_labels, fontsize=8)
    plt_obj.xlabel('Sequence Position (nucleotides)')
    plt_obj.ylabel('Best Matching Organism')
    plt_obj.title(title)
    plt_obj.grid(True, axis='y', linestyle='--', alpha=0.7)
    
    # Set appropriate axis limits
    plt_obj.xlim(min(positions), max(end_positions))
    plt_obj.ylim(-0.5, len(organisms) - 0.5)
    
    # Add chunk size and overlap information in the corner
    plt_obj.annotate(
        f"Chunk size: {chunk_data.get('chunk_size', 'N/A')}, "
        f"Overlap: {chunk_data.get('overlap', 'N/A')} nucleotides", 
        xy=(0.02, 0.02), 
        xycoords='axes fraction', 
        fontsize=8, 
        bbox=dict(boxstyle="round,pad=0.3", fc="white", ec="gray", alpha=0.8)
    )
    
    # Update explanation of visualization
    plt_obj.annotate(
        f"Color represents NRC score (higher score = better match)", 
        xy=(0.98, 0.02), 
        xycoords='axes fraction', 
        fontsize=8,
        ha='right',
        bbox=dict(boxstyle="round,pad=0.3", fc="white", ec="gray", alpha=0.8)
    )
    
    plt_obj.tight_layout()
