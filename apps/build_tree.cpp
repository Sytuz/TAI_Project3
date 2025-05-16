#include "../include/utils/CLIParser.h"
#include "../include/core/TreeBuilder.h"
#include <iostream>

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
    bool doImage = parser.flagExists("--output-image");
    string imageFile = parser.getOption("--output-image", "");

    TreeBuilder builder;
    if (!builder.buildTree(matrixFile, newickFile)) {
        return 1;
    }
    if (doImage && !imageFile.empty()) {
        builder.generateTreeImage(newickFile, imageFile);
    }
    return 0;
}
