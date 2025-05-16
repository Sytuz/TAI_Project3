#include "../include/utils/CLIParser.h"
#include "../include/core/TreeBuilder.h"
#include <iostream>
#include <filesystem>

using namespace std;

/**
 * @brief Build similarity tree from NCD matrix.
 * Usage: build_tree <input_matrix.csv> <output_tree.newick> [--output-image tree.png]
 */
int main(int argc, char* argv[]) {
    CLIParser parser(argc, argv);
    auto args = parser.getArgs();
    if (args.size() < 2) {
        cout << "Usage: build_tree <input_matrix.csv> <output_tree.newick> [--output-image tree.png]\n";
        return 1;
    }

    string matrixFile = args[0];
    string newickFile = args[1];

    // Check if input file exists
    if (!filesystem::exists(matrixFile)) {
        cerr << "Error: Input matrix file does not exist: " << matrixFile << endl;
        return 1;
    }

    // Ensure output directory exists
    filesystem::path outPath(newickFile);
    try {
        filesystem::create_directories(outPath.parent_path());
    } catch (const filesystem::filesystem_error& e) {
        cerr << "Error creating output directory: " << e.what() << endl;
    }

    bool doImage = parser.flagExists("--output-image");
    string imageFile = parser.getOption("--output-image", "");

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