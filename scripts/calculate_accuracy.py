"""
Calculate accuracy metrics for music identification results.
This script analyzes the output from batch music identification and calculates
various accuracy metrics, including the Top-K accuracy.
"""

import os
import sys
import csv
import re
import json
from pathlib import Path
from collections import defaultdict
import argparse

def extract_song_name(filename):
    """
    Extract the base song name from a filename with improved parsing.
    Handles various naming patterns more robustly.
    """
    # Remove file extension first
    name = Path(filename).stem
    
    # Remove common prefixes (sample_, etc.)
    if name.startswith('sample_'):
        name = name[7:]  # Remove 'sample_'
    
    # Remove feature extraction method suffixes FIRST (before other processing)
    name = re.sub(r'_(spectral|maxfreq)$', '', name, flags=re.IGNORECASE)
    
    # Remove noise indicators
    name = re.sub(r'_(white|pink|brown)_noise$', '', name, flags=re.IGNORECASE)
    
    # Remove timestamp indicators like _t30s, _t91s etc.
    name = re.sub(r'_t\d+s$', '', name, flags=re.IGNORECASE)
    
    # Remove -Main-version suffix (common in your dataset)
    name = re.sub(r'-Main-version$', '', name, flags=re.IGNORECASE)
    
    # Remove any trailing underscores or hyphens
    name = name.strip('_-')
    
    return name

def normalize_song_name(name):
    """
    Enhanced normalization for better matching.
    """
    if not name:
        return ""
        
    # Convert to lowercase
    name = name.lower()
    
    # Replace hyphens and underscores with spaces
    name = re.sub(r'[-_]+', ' ', name)
    
    # Replace special characters with spaces
    name = re.sub(r'[^a-z0-9\s]', ' ', name)
    
    # Normalize multiple spaces to single space
    name = re.sub(r'\s+', ' ', name)
    
    # Strip whitespace
    name = name.strip()
    
    return name

def fuzzy_match(name1, name2):
    """
    Check if two song names match with some fuzzy logic.
    Returns True if they match, False otherwise.
    """
    if name1 == name2:
        return True
    
    # Check if one is contained in the other (for partial matches)
    if len(name1) > 3 and len(name2) > 3:
        if name1 in name2 or name2 in name1:
            return True
    
    # Check word-level matching (all words from shorter name in longer name)
    words1 = set(name1.split())
    words2 = set(name2.split())
    
    if len(words1) == 0 or len(words2) == 0:
        return False
    
    # If shorter name's words are subset of longer name's words
    shorter_words = words1 if len(words1) <= len(words2) else words2
    longer_words = words2 if len(words1) <= len(words2) else words1
    
    if len(shorter_words) > 0 and shorter_words.issubset(longer_words):
        return True
    
    return False

def load_results_csv(filepath):
    """
    Load results from a CSV file with improved parsing.
    """
    query_name = ""
    results = []
    
    try:
        with open(filepath, 'r', encoding='utf-8') as f:
            content = f.read()
    except Exception as e:
        print(f"Error reading file {filepath}: {e}")
        return query_name, results
    
    lines = content.strip().split('\n')
    
    # Parse header information
    csv_section_started = False
    for i, line in enumerate(lines):
        line = line.strip()
        if line.startswith('Query:'):
            query_name = line.split(':', 1)[1].strip()
        elif line.startswith('Rank,') or (line and ',' in line and any(c.isdigit() for c in line.split(',')[0])):
            csv_section_started = True
            # Check if this line is already data (not header)
            if not line.startswith('Rank,'):
                # This is data, process it
                parts = line.split(',')
                if len(parts) >= 3:
                    try:
                        rank = int(parts[0])
                        filename = parts[1].strip()
                        ncd_score = float(parts[2])
                        results.append((rank, filename, ncd_score))
                    except (ValueError, IndexError):
                        continue
            # Parse remaining lines as CSV data
            for j in range(i + 1, len(lines)):
                data_line = lines[j].strip()
                if data_line and not data_line.startswith('Query') and not data_line.startswith('Compressor'):
                    parts = data_line.split(',')
                    if len(parts) >= 3:
                        try:
                            rank = int(parts[0])
                            filename = parts[1].strip()
                            ncd_score = float(parts[2])
                            results.append((rank, filename, ncd_score))
                        except (ValueError, IndexError):
                            continue
            break
    
    return query_name, results

def calculate_accuracy_metrics(results_dir, output_file=None):
    """
    Calculate various accuracy metrics from batch identification results.
    """
    print(f"Analyzing results in: {results_dir}")
    
    # Find all result CSV files
    result_files = list(Path(results_dir).glob("*_results.csv"))
    
    if not result_files:
        print("Error: No result CSV files found in the directory")
        return
    
    print(f"Found {len(result_files)} result files")
    
    # Metrics tracking
    total_queries = 0
    top1_correct = 0
    top5_correct = 0
    top10_correct = 0
    
    detailed_results = []
    
    # Process each result file
    for result_file in result_files:
        try:
            print(f"\nProcessing: {result_file.name}")
            query_name, results = load_results_csv(result_file)
            
            if not results:
                print(f"Warning: No results found in {result_file}")
                continue
            
            # Extract ground truth from query name
            extracted_name = extract_song_name(query_name)
            ground_truth = normalize_song_name(extracted_name)
            
            print(f"  Query: {query_name}")
            print(f"  Extracted: {extracted_name}")
            print(f"  Ground truth: '{ground_truth}'")
            
            if not ground_truth:
                print(f"Warning: Could not extract ground truth from query: {query_name}")
                continue
            
            total_queries += 1
            
            # Check if ground truth appears in top-K results
            found_at_rank = None
            
            for rank, filename, ncd_score in results[:10]:  # Only check top 10
                candidate_extracted = extract_song_name(filename)
                candidate = normalize_song_name(candidate_extracted)
                
                print(f"    Rank {rank}: {filename} -> '{candidate_extracted}' -> '{candidate}'")
                
                if fuzzy_match(ground_truth, candidate):
                    found_at_rank = rank
                    print(f"    *** MATCH found at rank {rank} ***")
                    break
            
            # Update metrics
            if found_at_rank is not None:
                if found_at_rank <= 1:
                    top1_correct += 1
                if found_at_rank <= 5:
                    top5_correct += 1
                if found_at_rank <= 10:
                    top10_correct += 1
            
            # Store detailed result
            top_match = results[0] if results else (0, "None", float('inf'))
            top_match_extracted = extract_song_name(top_match[1]) if results else ""
            top_match_normalized = normalize_song_name(top_match_extracted) if results else ""
            is_top_match_correct = fuzzy_match(ground_truth, top_match_normalized) if results else False
            
            detailed_results.append({
                'query': query_name,
                'extracted_query_name': extracted_name,
                'ground_truth': ground_truth,
                'top_match': top_match_extracted,
                'top_match_normalized': top_match_normalized,
                'top_match_ncd': top_match[2] if results else float('inf'),
                'found_at_rank': found_at_rank,
                'correct': is_top_match_correct
            })
            
            # Print progress
            status = "✓" if found_at_rank is not None else "✗"
            rank_info = f"(rank {found_at_rank})" if found_at_rank is not None else "(not found)"
            print(f"  {status} Result: {extracted_name} -> {ground_truth} {rank_info}")
            
        except Exception as e:
            print(f"Error processing {result_file}: {e}")
            import traceback
            traceback.print_exc()
            continue
    
    # Calculate final metrics
    if total_queries > 0:
        top1_accuracy = (top1_correct / total_queries) * 100
        top5_accuracy = (top5_correct / total_queries) * 100
        top10_accuracy = (top10_correct / total_queries) * 100
        
        # Print summary
        print("\n" + "="*50)
        print("ACCURACY METRICS SUMMARY")
        print("="*50)
        print(f"Total queries processed: {total_queries}")
        print(f"Top-1 Accuracy: {top1_correct}/{total_queries} ({top1_accuracy:.1f}%)")
        print(f"Top-5 Accuracy: {top5_correct}/{total_queries} ({top5_accuracy:.1f}%)")
        print(f"Top-10 Accuracy: {top10_correct}/{total_queries} ({top10_accuracy:.1f}%)")
        
        # Prepare output data
        summary = {
            'total_queries': total_queries,
            'top1_correct': top1_correct,
            'top5_correct': top5_correct,
            'top10_correct': top10_correct,
            'top1_accuracy': top1_accuracy,
            'top5_accuracy': top5_accuracy,
            'top10_accuracy': top10_accuracy,
            'detailed_results': detailed_results
        }
        
        # Save to file if specified
        if output_file:
            with open(output_file, 'w') as f:
                json.dump(summary, f, indent=2)
            print(f"\nDetailed results saved to: {output_file}")
        
        return summary
    else:
        print("Error: No valid queries were processed")
        return None

def main():
    parser = argparse.ArgumentParser(description='Calculate accuracy metrics for music identification results')
    parser.add_argument('results_dir', help='Directory containing *_results.csv files')
    parser.add_argument('-o', '--output', help='Output JSON file for detailed results')
    parser.add_argument('-v', '--verbose', action='store_true', help='Enable verbose output')
    
    args = parser.parse_args()
    
    if not os.path.exists(args.results_dir):
        print(f"Error: Results directory does not exist: {args.results_dir}")
        sys.exit(1)
    
    calculate_accuracy_metrics(args.results_dir, args.output)

if __name__ == "__main__":
    main()
