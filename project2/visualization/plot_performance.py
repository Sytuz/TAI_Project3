import matplotlib.pyplot as plt
import os

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
