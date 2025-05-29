"""
Parameter Evaluation Script for Music Identification System
This script evaluates different feature extraction parameters and their impact on accuracy.
"""

import os
import sys
import json
import subprocess
import shutil
from pathlib import Path
import itertools
import argparse
from datetime import datetime

class ParameterEvaluator:
    def __init__(self, project_root, query_dir, db_dir, output_dir):
        self.project_root = Path(project_root)
        self.query_dir = Path(query_dir)
        self.db_dir = Path(db_dir)
        self.output_dir = Path(output_dir)
        self.output_dir.mkdir(parents=True, exist_ok=True)
        
        # Ensure we're in the project directory
        os.chdir(self.project_root)
        
    def create_config(self, method, num_frequencies, num_bins, frame_size, hop_size):
        """Create a configuration file with specified parameters."""
        config = {
            "method": method,
            "numFrequencies": num_frequencies,
            "numBins": num_bins,
            "frameSize": frame_size,
            "hopSize": hop_size
        }
        
        config_name = f"{method}_f{num_frequencies}_b{num_bins}_fs{frame_size}_hs{hop_size}"
        config_file = self.output_dir / f"config_{config_name}.json"
        
        with open(config_file, 'w') as f:
            json.dump(config, f, indent=2)
            
        return config_file, config_name
    
    def extract_features(self, config_file, input_dir, output_dir, config_name):
        """Extract features using the specified configuration."""
        print(f"Extracting features with config: {config_name}")
        
        cmd = [
            "./apps/extract_features",
            "--config", str(config_file),
            "-i", str(input_dir),
            "-o", str(output_dir)
        ]
        
        try:
            result = subprocess.run(cmd, capture_output=True, text=True, timeout=300)
            if result.returncode != 0:
                print(f"Error extracting features: {result.stderr}")
                return False
            return True
        except subprocess.TimeoutExpired:
            print(f"Timeout during feature extraction for {config_name}")
            return False
        except Exception as e:
            print(f"Exception during feature extraction: {e}")
            return False
    
    def run_identification(self, query_feat_dir, db_feat_dir, output_file, compressor="gzip"):
        """Run music identification and return accuracy metrics."""
        # Run batch identification
        cmd = [
            "bash", "scripts/batch_identify.sh",
            "-q", str(query_feat_dir),
            "-d", str(db_feat_dir),
            "-o", str(output_file.parent),
            "-c", compressor
        ]
        
        try:
            result = subprocess.run(cmd, capture_output=True, text=True, timeout=600)
            if result.returncode != 0:
                print(f"Error in batch identification: {result.stderr}")
                return None
                
            # Load accuracy metrics
            accuracy_file = output_file.parent / f"accuracy_metrics_{compressor}.json"
            if accuracy_file.exists():
                with open(accuracy_file, 'r') as f:
                    return json.load(f)
            else:
                print(f"Accuracy file not found: {accuracy_file}")
                return None
                
        except subprocess.TimeoutExpired:
            print("Timeout during identification")
            return None
        except Exception as e:
            print(f"Exception during identification: {e}")
            return None
    
    def evaluate_parameters(self, parameter_sets, compressors=["gzip"]):
        """Evaluate multiple parameter configurations."""
        results = []
        total_configs = len(parameter_sets) * len(compressors)
        current_config = 0
        
        print(f"Starting evaluation of {total_configs} configurations...")
        
        for params in parameter_sets:
            method = params["method"]
            num_frequencies = params["numFrequencies"]
            num_bins = params["numBins"]
            frame_size = params["frameSize"]
            hop_size = params["hopSize"]
            
            # Create configuration
            config_file, config_name = self.create_config(
                method, num_frequencies, num_bins, frame_size, hop_size
            )
            
            print(f"\n{'='*60}")
            print(f"Configuration: {config_name}")
            print(f"Method: {method}, Frequencies: {num_frequencies}, Bins: {num_bins}")
            print(f"Frame Size: {frame_size}, Hop Size: {hop_size}")
            print(f"{'='*60}")
            
            # Extract features for queries
            query_feat_dir = self.output_dir / f"query_features_{config_name}"
            if not self.extract_features(config_file, self.query_dir, query_feat_dir, config_name):
                print(f"Skipping {config_name} due to feature extraction failure")
                continue
            
            # Extract features for database (if not already done)
            db_feat_dir = self.output_dir / f"db_features_{config_name}"
            if not db_feat_dir.exists():
                if not self.extract_features(config_file, self.db_dir, db_feat_dir, config_name):
                    print(f"Skipping {config_name} due to database feature extraction failure")
                    continue
            
            # Test with each compressor
            for compressor in compressors:
                current_config += 1
                print(f"\n[{current_config}/{total_configs}] Testing {config_name} with {compressor}")
                
                # Run identification
                id_output_dir = self.output_dir / f"results_{config_name}_{compressor}"
                id_output_dir.mkdir(exist_ok=True)
                
                metrics = self.run_identification(
                    query_feat_dir, db_feat_dir, 
                    id_output_dir / "results.txt", compressor
                )
                
                if metrics:
                    result = {
                        "config_name": config_name,
                        "method": method,
                        "numFrequencies": num_frequencies,
                        "numBins": num_bins,
                        "frameSize": frame_size,
                        "hopSize": hop_size,
                        "compressor": compressor,
                        "top1_accuracy": metrics["top1_accuracy"],
                        "top5_accuracy": metrics["top5_accuracy"],
                        "top10_accuracy": metrics["top10_accuracy"],
                        "total_queries": metrics["total_queries"]
                    }
                    results.append(result)
                    
                    print(f"- Top-1: {metrics['top1_accuracy']:.1f}%, "
                          f"Top-5: {metrics['top5_accuracy']:.1f}%, "
                          f"Top-10: {metrics['top10_accuracy']:.1f}%")
                else:
                    print(f"✗ Failed to get metrics for {config_name} with {compressor}")
        
        return results
    
    def save_results(self, results, filename="parameter_evaluation_results.json"):
        """Save evaluation results to file."""
        output_file = self.output_dir / filename
        
        # Add metadata
        evaluation_data = {
            "timestamp": datetime.now().isoformat(),
            "query_dir": str(self.query_dir),
            "db_dir": str(self.db_dir),
            "total_configurations": len(results),
            "results": results
        }
        
        with open(output_file, 'w') as f:
            json.dump(evaluation_data, f, indent=2)
        
        print(f"\nResults saved to: {output_file}")
        return output_file
    
    def generate_summary(self, results):
        """Generate a human-readable summary of results."""
        if not results:
            print("No results to summarize")
            return
        
        print(f"\n{'='*80}")
        print("PARAMETER EVALUATION SUMMARY")
        print(f"{'='*80}")
        
        # Sort by top-1 accuracy
        sorted_results = sorted(results, key=lambda x: x["top1_accuracy"], reverse=True)
        
        print(f"{'Rank':<4} {'Method':<8} {'Freq':<4} {'Bins':<4} {'Frame':<5} {'Hop':<4} {'Comp':<6} {'Top-1':<6} {'Top-5':<6} {'Top-10':<7}")
        print("-" * 80)
        
        for i, result in enumerate(sorted_results[:20], 1):  # Show top 20
            print(f"{i:<4} {result['method']:<8} {result['numFrequencies']:<4} "
                  f"{result['numBins']:<4} {result['frameSize']:<5} {result['hopSize']:<4} "
                  f"{result['compressor']:<6} {result['top1_accuracy']:<6.1f} "
                  f"{result['top5_accuracy']:<6.1f} {result['top10_accuracy']:<7.1f}")
        
        # Best configuration
        best = sorted_results[0]
        print(f"\nBest Configuration:")
        print(f"Method: {best['method']}")
        print(f"Frequencies: {best['numFrequencies']}")
        print(f"Bins: {best['numBins']}")
        print(f"Frame Size: {best['frameSize']}")
        print(f"Hop Size: {best['hopSize']}")
        print(f"Compressor: {best['compressor']}")
        print(f"Top-1 Accuracy: {best['top1_accuracy']:.1f}%")

def generate_parameter_sets():
    """Generate comprehensive parameter sets for evaluation."""
    
    # Define parameter ranges
    methods = ["maxfreq", "spectral"]
    frequencies_range = [2, 4, 6, 8, 12]  # For maxfreq method
    bins_range = [16, 32, 64, 128]  # For spectral method
    frame_sizes = [512, 1024, 2048]
    hop_ratios = [0.25, 0.5, 0.75]  # Hop size as fraction of frame size
    
    parameter_sets = []
    
    for method in methods:
        for frame_size in frame_sizes:
            for hop_ratio in hop_ratios:
                hop_size = int(frame_size * hop_ratio)
                
                if method == "maxfreq":
                    for num_freq in frequencies_range:
                        parameter_sets.append({
                            "method": method,
                            "numFrequencies": num_freq,
                            "numBins": 32,  # Default for maxfreq
                            "frameSize": frame_size,
                            "hopSize": hop_size
                        })
                
                elif method == "spectral":
                    for num_bins in bins_range:
                        parameter_sets.append({
                            "method": method,
                            "numFrequencies": 4,  # Default for spectral
                            "numBins": num_bins,
                            "frameSize": frame_size,
                            "hopSize": hop_size
                        })
    
    return parameter_sets

def main():
    parser = argparse.ArgumentParser(description='Evaluate feature extraction parameters')
    parser.add_argument('--project-root', default='/home/maria/Desktop/TAI_Project3',
                       help='Project root directory')
    parser.add_argument('--query-dir', default='data/test_samples',
                       help='Directory with query WAV files')
    parser.add_argument('--db-dir', default='data/samples',
                       help='Directory with database WAV files')
    parser.add_argument('--output-dir', default='parameter_evaluation',
                       help='Output directory for evaluation results')
    parser.add_argument('--compressors', default='gzip,bzip2',
                       help='Comma-separated list of compressors to test')
    parser.add_argument('--quick', action='store_true',
                       help='Run quick evaluation with fewer parameters')
    
    args = parser.parse_args()
    
    # Parse compressors
    compressors = [c.strip() for c in args.compressors.split(',')]
    
    # Initialize evaluator
    evaluator = ParameterEvaluator(
        args.project_root, args.query_dir, args.db_dir, args.output_dir
    )
    
    # Generate parameter sets
    if args.quick:
        # Quick evaluation - fewer parameters
        parameter_sets = [
            {"method": "maxfreq", "numFrequencies": 4, "numBins": 32, "frameSize": 1024, "hopSize": 512},
            {"method": "maxfreq", "numFrequencies": 6, "numBins": 32, "frameSize": 1024, "hopSize": 512},
            {"method": "maxfreq", "numFrequencies": 8, "numBins": 32, "frameSize": 1024, "hopSize": 512},
            {"method": "spectral", "numFrequencies": 4, "numBins": 32, "frameSize": 1024, "hopSize": 512},
            {"method": "spectral", "numFrequencies": 4, "numBins": 64, "frameSize": 1024, "hopSize": 512},
            {"method": "spectral", "numFrequencies": 4, "numBins": 128, "frameSize": 1024, "hopSize": 512},
        ]
    else:
        parameter_sets = generate_parameter_sets()
    
    print(f"Generated {len(parameter_sets)} parameter configurations")
    print(f"Testing with compressors: {compressors}")
    
    # Run evaluation
    results = evaluator.evaluate_parameters(parameter_sets, compressors)
    
    # Save and summarize results
    if results:
        evaluator.save_results(results)
        evaluator.generate_summary(results)
    else:
        print("No successful evaluations completed")

if __name__ == "__main__":
    main()
