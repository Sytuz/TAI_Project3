import sys
import matplotlib.pyplot as plt
from Bio import Phylo

def main():
    if len(sys.argv) < 3:
        print("Usage: plot_tree.py <tree.newick> <output.png>")
        return
    tree_file = sys.argv[1]
    out_img = sys.argv[2]
    tree = Phylo.read(tree_file, "newick")
    fig = plt.figure(figsize=(6,6))
    Phylo.draw(tree, do_show=False)
    plt.savefig(out_img)
    print("Saved tree image to", out_img)

if __name__ == "__main__":
    main()
