import json
import os
import pandas as pd
import numpy as np
from collections import Counter

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
                    organism_name = '_'.join(filename.split('_')[1:]).split(',')[0].replace('.csv', '')
                    # Load CSV into dataframe
                    organism_df = pd.read_csv(file_path)
                    
                    # Store dataframe in dictionary with organism name as key
                    organism_data[organism_name] = organism_df
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
    import datetime
    current_time = datetime.datetime.now()
    timestamp = current_time.strftime("%Y%m%d_%H%M%S")
    print(f"Warning: Could not find timestamp in info.txt, using current time: {timestamp}")
    return timestamp

def generate_summary_statistics(df, filepath, timestamp, output_dir):
    """Generate summary statistics and save to file"""
    top_organism = Counter(df[df['rank'] == 1]['name']).most_common(1)[0][0]
    best_nrc = df[df['rank'] == 1]['nrc'].min()
    best_row = df.loc[df[df['rank'] == 1]['nrc'].idxmin()]
    
    # Write the summary to a file in the output directory
    with open(os.path.join(output_dir, 'summary.txt'), 'w') as f:
        f.write("=== SUMMARY STATISTICS ===\n")
        f.write(f"Input file: {filepath}\n")
        f.write(f"Timestamp: {timestamp}\n")
        f.write(f"Most frequent top-ranking organism: {top_organism}\n")
        f.write(f"Best NRC value: {best_nrc:.6f}\n")
        f.write(f"Best parameters: k={best_row['k']}, alpha={best_row['alpha']}\n")
    
    return {
        "top_organism": top_organism,
        "best_nrc": best_nrc,
        "best_k": best_row['k'],
        "best_alpha": best_row['alpha']
    }
