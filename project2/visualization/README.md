# NRC Results Visualization

This tool visualizes Network Relevance Complexity (NRC) results and generates various plots to help analyze the data.

## Requirements

Install the required Python packages:

```bash
pip install -r requirements.txt
```

## Usage

The visualization script can be run from the command line:

```bash
python visualize_results.py [options]
```

### Command Line Options

- `--input`, `-i`: Path to the test results folder (default: ../results/latest)
- `--output`, `-o`: Base directory for visualization results (default: visualization_results)

### Examples

Run with default settings:
```bash
python visualize_results.py
```

Specify custom input and output directories:
```bash
python visualize_results.py --input ../results/my_experiment --output my_visualizations
```

## Expected Outputs

The script will generate visualization results in two directories:
1. A timestamped directory (based on the timestamp from the input data)
2. A "latest" directory (always overwritten with the most recent results)

### Generated Visualizations

The following plots will be generated if the corresponding data is available:

1. **Organism Information Profile**: Displays information about the top-ranked organisms
2. **Top Organisms NRC**: Shows the organisms with the highest NRC scores
3. **Execution Time Analysis**: Plots the execution time for different parameter configurations
4. **Rank Stability**: Visualizes how stable organism rankings are across parameters
5. **Parameter Influence**: Shows the effect of parameters (k and alpha) on NRC values
6. **NRC Boxplot by k**: Displays distribution of NRC values for different k values
7. **Cross Comparison**: Compares results across different configurations
8. **Chunk Analysis**: Visualizes chunk-based analysis results (if available)

### Summary Statistics

The script also provides summary statistics including:
- Most frequent top-ranking organism
- Best NRC value
- Best parameter combination (k and alpha values)

## Troubleshooting

If the input file isn't found at the specified location, the script will try to locate it in common relative paths. If the file still can't be found, you'll receive an error message asking you to provide a valid file path.