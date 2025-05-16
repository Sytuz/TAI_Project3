import sys
import numpy as np
import matplotlib.pyplot as plt

if len(sys.argv) < 4:
    print("Usage: plot_heatmap.py <matrix.csv> <labels.txt> <output.png>")
    sys.exit(1)

matrix = np.loadtxt(sys.argv[1], delimiter=',')
labels = [line.strip() for line in open(sys.argv[2])]
plt.imshow(matrix, interpolation='nearest', cmap='viridis')
plt.colorbar()
plt.xticks(range(len(labels)), labels, rotation=90)
plt.yticks(range(len(labels)), labels)
plt.tight_layout()
plt.savefig(sys.argv[3])
