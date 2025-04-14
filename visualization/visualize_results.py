import os
import argparse
import shutil

# Import our modules
from data_handler import (
    load_data, load_organism_symbols, process_data, 
    get_timestamp_from_info, generate_summary_statistics
)
from plot_organism_info import (plot_info_profile
                                , plot_cross_comparison
                                , plot_complexity_profile)
from plot_nrc import (
    plot_top_organisms_nrc, plot_rank_stability, 
    plot_parameter_influence, plot_nrc_boxplot_by_k
)
from plot_performance import plot_execution_time
from plot_chunk_analysis import plot_chunk_analysis

def main():
    # Set up command line argument parsing
    parser = argparse.ArgumentParser(description='Visualize NRC results from JSON data.')
    parser.add_argument('--input', '-i', type=str, default='../build/results/latest',
                        help='Path to the test results folder (default: ../build/results/latest)')
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
    top_organisms_results_filepath = os.path.join(input_folder, 'top_organisms_results.json')
    
    # If the file doesn't exist at specified path, try some common relative paths
    if not os.path.exists(top_organisms_results_filepath):
        possible_paths = [
            'results/latest/top_organisms_results.json',
            '../results/latest/top_organisms_results.json',
            'results/top_organisms_results.json'
        ]
        
        for path in possible_paths:
            if os.path.exists(path):
                top_organisms_results_filepath = path
                print(f"Found input file at: {path}")
                break
    
    if not os.path.exists(top_organisms_results_filepath):
        print(f"Error: Could not find input file at {top_organisms_results_filepath}")
        print("Please provide a valid file path using the --input argument.")
        return
    
    data = load_data(top_organisms_results_filepath)
    df = process_data(data)
    
    # Load organism symbol data
    organism_data = load_organism_symbols(os.path.join(input_folder, 'symbol_info'))
    
    # Generate plots to both directories
    for output_dir in [timestamp_dir, latest_dir]:
        if organism_data:
            plot_info_profile(organism_data, output_dir)
            plot_complexity_profile(organism_data, output_dir, nrc_threshold=0.05)
        plot_top_organisms_nrc(df, output_dir)
        plot_execution_time(df, output_dir)
        plot_rank_stability(df, output_dir)
        plot_parameter_influence(df, output_dir)
        plot_nrc_boxplot_by_k(df, output_dir)
        plot_cross_comparison(input_folder, output_dir)

        # Add chunk analysis visualization if available
        chunk_data_file = os.path.join(input_folder, 'chunk_analysis.json')
        if os.path.exists(chunk_data_file):
            plot_chunk_analysis(chunk_data_file, output_dir)

        # Generate summary statistics
        stats = generate_summary_statistics(df, top_organisms_results_filepath, timestamp, output_dir)
    
    print("\n=== SUMMARY STATISTICS ===")
    print(f"Most frequent top-ranking organism: {stats['top_organism']}")
    print(f"Best NRC value: {stats['best_nrc']:.6f}")
    print(f"Best parameters: k={stats['best_k']}, alpha={stats['best_alpha']}")
    
    print(f"\nVisualization complete. Results saved to both directories.")

if __name__ == "__main__":
    main()
