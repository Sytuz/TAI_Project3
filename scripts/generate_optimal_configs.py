"""
Optimal Configuration Generator
This script analyzes parameter evaluation results and generates optimal configuration files.
"""

import json
import argparse
from pathlib import Path

def generate_optimal_configs(results_file, output_dir):
    """Generate optimal configuration files based on evaluation results."""
    
    # Load results
    with open(results_file, 'r') as f:
        data = json.load(f)
    results = data['results']
    
    if not results:
        print("No results found in file")
        return
    
    output_dir = Path(output_dir)
    output_dir.mkdir(exist_ok=True)
    
    # Find best overall configuration
    best_overall = max(results, key=lambda x: x['top1_accuracy'])
    
    # Find best configuration for each method
    methods = set(r['method'] for r in results)
    best_by_method = {}
    
    for method in methods:
        method_results = [r for r in results if r['method'] == method]
        if method_results:
            best_by_method[method] = max(method_results, key=lambda x: x['top1_accuracy'])
    
    # Find best configuration for each compressor
    compressors = set(r['compressor'] for r in results)
    best_by_compressor = {}
    
    for compressor in compressors:
        comp_results = [r for r in results if r['compressor'] == compressor]
        if comp_results:
            best_by_compressor[compressor] = max(comp_results, key=lambda x: x['top1_accuracy'])
    
    # Generate configurations
    configs_generated = []
    
    # Best overall configuration
    config_name = "optimal_overall"
    config = {
        "method": best_overall['method'],
        "numFrequencies": best_overall['numFrequencies'],
        "numBins": best_overall['numBins'],
        "frameSize": best_overall['frameSize'],
        "hopSize": best_overall['hopSize']
    }
    
    config_file = output_dir / f"{config_name}.json"
    with open(config_file, 'w') as f:
        json.dump(config, f, indent=2)
    
    configs_generated.append({
        'name': config_name,
        'file': config_file,
        'accuracy': best_overall['top1_accuracy'],
        'description': f"Best overall configuration ({best_overall['top1_accuracy']:.1f}% accuracy)"
    })
    
    # Best configuration for each method
    for method, best_config in best_by_method.items():
        config_name = f"optimal_{method}"
        config = {
            "method": best_config['method'],
            "numFrequencies": best_config['numFrequencies'],
            "numBins": best_config['numBins'],
            "frameSize": best_config['frameSize'],
            "hopSize": best_config['hopSize']
        }
        
        config_file = output_dir / f"{config_name}.json"
        with open(config_file, 'w') as f:
            json.dump(config, f, indent=2)
        
        configs_generated.append({
            'name': config_name,
            'file': config_file,
            'accuracy': best_config['top1_accuracy'],
            'description': f"Best {method} configuration ({best_config['top1_accuracy']:.1f}% accuracy)"
        })
    
    # Generate fast vs accurate configurations
    # Fast configuration (smaller frame sizes, fewer features)
    fast_configs = [r for r in results if r['frameSize'] <= 1024 and 
                   ((r['method'] == 'maxfreq' and r['numFrequencies'] <= 4) or
                    (r['method'] == 'spectral' and r['numBins'] <= 32))]
    
    if fast_configs:
        best_fast = max(fast_configs, key=lambda x: x['top1_accuracy'])
        config_name = "optimal_fast"
        config = {
            "method": best_fast['method'],
            "numFrequencies": best_fast['numFrequencies'],
            "numBins": best_fast['numBins'],
            "frameSize": best_fast['frameSize'],
            "hopSize": best_fast['hopSize']
        }
        
        config_file = output_dir / f"{config_name}.json"
        with open(config_file, 'w') as f:
            json.dump(config, f, indent=2)
        
        configs_generated.append({
            'name': config_name,
            'file': config_file,
            'accuracy': best_fast['top1_accuracy'],
            'description': f"Fast configuration optimized for speed ({best_fast['top1_accuracy']:.1f}% accuracy)"
        })
    
    # Accurate configuration (larger frame sizes, more features)
    accurate_configs = [r for r in results if r['frameSize'] >= 1024 and 
                       ((r['method'] == 'maxfreq' and r['numFrequencies'] >= 6) or
                        (r['method'] == 'spectral' and r['numBins'] >= 64))]
    
    if accurate_configs:
        best_accurate = max(accurate_configs, key=lambda x: x['top1_accuracy'])
        config_name = "optimal_accurate"
        config = {
            "method": best_accurate['method'],
            "numFrequencies": best_accurate['numFrequencies'],
            "numBins": best_accurate['numBins'],
            "frameSize": best_accurate['frameSize'],
            "hopSize": best_accurate['hopSize']
        }
        
        config_file = output_dir / f"{config_name}.json"
        with open(config_file, 'w') as f:
            json.dump(config, f, indent=2)
        
        configs_generated.append({
            'name': config_name,
            'file': config_file,
            'accuracy': best_accurate['top1_accuracy'],
            'description': f"Accurate configuration optimized for precision ({best_accurate['top1_accuracy']:.1f}% accuracy)"
        })
    
    # Generate summary report
    summary_file = output_dir / "optimization_summary.txt"
    with open(summary_file, 'w') as f:
        f.write("OPTIMAL CONFIGURATION SUMMARY\n")
        f.write("="*40 + "\n\n")
        
        f.write("Generated Configurations:\n")
        f.write("-"*25 + "\n")
        
        for config_info in configs_generated:
            f.write(f"• {config_info['name']}.json\n")
            f.write(f"  {config_info['description']}\n")
            f.write(f"  File: {config_info['file']}\n\n")
        
        f.write("Detailed Analysis:\n")
        f.write("-"*18 + "\n")
        
        # Best overall
        f.write("Best Overall Configuration:\n")
        f.write(f"  Method: {best_overall['method']}\n")
        f.write(f"  Frequencies: {best_overall['numFrequencies']}\n")
        f.write(f"  Bins: {best_overall['numBins']}\n")
        f.write(f"  Frame Size: {best_overall['frameSize']}\n")
        f.write(f"  Hop Size: {best_overall['hopSize']}\n")
        f.write(f"  Compressor: {best_overall['compressor']}\n")
        f.write(f"  Top-1 Accuracy: {best_overall['top1_accuracy']:.1f}%\n")
        f.write(f"  Top-5 Accuracy: {best_overall['top5_accuracy']:.1f}%\n")
        f.write(f"  Top-10 Accuracy: {best_overall['top10_accuracy']:.1f}%\n\n")
        
        # Method comparison
        f.write("Method Comparison:\n")
        for method, config in best_by_method.items():
            f.write(f"  {method.upper()}: {config['top1_accuracy']:.1f}% accuracy\n")
        f.write("\n")
        
        # Compressor recommendation
        f.write("Compressor Recommendations:\n")
        sorted_compressors = sorted(best_by_compressor.items(), 
                                  key=lambda x: x[1]['top1_accuracy'], reverse=True)
        for i, (comp, config) in enumerate(sorted_compressors, 1):
            f.write(f"  {i}. {comp}: {config['top1_accuracy']:.1f}% accuracy\n")
        f.write("\n")
        
        # Usage recommendations
        f.write("Usage Recommendations:\n")
        f.write("-"*22 + "\n")
        f.write("• For best accuracy: Use optimal_overall.json\n")
        f.write("• For fast processing: Use optimal_fast.json\n")
        f.write("• For high precision: Use optimal_accurate.json\n")
        
        best_method = max(best_by_method.items(), key=lambda x: x[1]['top1_accuracy'])
        f.write(f"• Method preference: {best_method[0]} performs best\n")
        
        best_compressor = max(best_by_compressor.items(), key=lambda x: x[1]['top1_accuracy'])
        f.write(f"• Compressor preference: {best_compressor[0]} recommended\n")
    
    print(f"Generated {len(configs_generated)} optimal configurations:")
    for config_info in configs_generated:
        print(f"  - {config_info['name']}.json ({config_info['accuracy']:.1f}% accuracy)")
    
    print(f"\nConfiguration files saved to: {output_dir}")
    print(f"Summary report: {summary_file}")
    
    return configs_generated

def main():
    parser = argparse.ArgumentParser(description='Generate optimal configuration files from evaluation results')
    parser.add_argument('results_file', help='JSON file with parameter evaluation results')
    parser.add_argument('-o', '--output-dir', default='optimal_configs',
                       help='Output directory for configuration files')
    
    args = parser.parse_args()
    
    if not Path(args.results_file).exists():
        print(f"Error: Results file not found: {args.results_file}")
        return
    
    generate_optimal_configs(args.results_file, args.output_dir)

if __name__ == "__main__":
    main()
