#include "../include/core/TreeBuilder.h"
#include <iostream>
#include <filesystem>

using namespace std;

void printUsage() {
    cout << "Usage: build_tree <input_matrix.csv> <output_tree.newick> [OPTIONS]\n";
    cout << "Options:\n";
    cout << "  --output-image <file>  Generate tree visualization as PNG\n";
    cout << "  -h, --help             Show this help message\n";
    cout << endl;
}

/**
 * @brief Build similarity tree from NCD matrix.
 * Usage: build_tree <input_matrix.csv> <output_tree.newick> [--output-image tree.png]
 */
int main(int argc, char* argv[]) {
    // Default values
    string matrixFile;
    string newickFile;
    bool doImage = false;
    string imageFile;
    
    // Parse command line arguments
    for (int i = 1; i < argc; i++) {
        string arg = argv[i];
        
        if (arg == "-h" || arg == "--help") {
            printUsage();
            return 0;
        } else if (arg == "--output-image" && i + 1 < argc) {
            doImage = true;
            imageFile = argv[++i];
        } else if (matrixFile.empty()) {
            matrixFile = arg;
        } else if (newickFile.empty()) {
            newickFile = arg;
        }
    }
    
    // Validate required arguments
    if (matrixFile.empty() || newickFile.empty()) {
        cerr << "Error: Missing required matrix file or output tree file\n";
        printUsage();
        return 1;
    }

    // Check if input file exists
    if (!filesystem::exists(matrixFile)) {
        cerr << "Error: Input matrix file does not exist: " << matrixFile << endl;
        return 1;
    }

    // Ensure output directory exists
    filesystem::path outPath(newickFile);
    try {
        if (!outPath.empty() && outPath.has_parent_path()) {
            filesystem::create_directories(outPath.parent_path());
        }
    } catch (const filesystem::filesystem_error& e) {
        cerr << "Error creating output directory: " << e.what() << endl;
    }

    cout << "Building tree from matrix: " << matrixFile << endl;
    cout << "Output Newick format: " << newickFile << endl;
    if (doImage) {
        cout << "Output visualization: " << imageFile << endl;
    }

    TreeBuilder builder;
    if (!builder.buildTree(matrixFile, newickFile)) {
        cerr << "Error building tree from matrix" << endl;
        return 1;
    }

    if (doImage && !imageFile.empty()) {
        if (!builder.generateTreeImage(newickFile, imageFile)) {
            cerr << "Warning: Failed to generate tree visualization" << endl;
        }
    }

    cout << "Tree building completed successfully" << endl;
    return 0;
}