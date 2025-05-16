#include "../../include/core/TreeBuilder.h"
#include <cstdlib>
#include <iostream>

using namespace std;

bool TreeBuilder::buildTree(const string& matrixFile, const string& outputNewick) {
    // Example: call an external script or tool (not implemented here)
    // For demonstration, simply echo a fake Newick tree.
    string cmd = "echo \"(A:0.1,(B:0.2,C:0.2):0.3);\" > " + outputNewick;
    int ret = system(cmd.c_str());
    if (ret != 0) {
        cerr << "Error building tree via external tool." << endl;
        return false;
    }
    cout << "Tree (Newick) written to " << outputNewick << endl;
    return true;
}

bool TreeBuilder::generateTreeImage(const string& newickFile, const string& outputImage) {
    // Call the Python script for visualization
    string cmd = "python3 visualization/plot_tree.py " + newickFile + " " + outputImage;
    int ret = system(cmd.c_str());
    if (ret != 0) {
        cerr << "Error generating tree image." << endl;
        return false;
    }
    cout << "Tree image saved to " << outputImage << endl;
    return true;
}
