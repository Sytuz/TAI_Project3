#ifndef TREEBUILDER_H
#define TREEBUILDER_H

#include <string>

using namespace std;

/**
 * @brief TreeBuilder invokes an external quartet-based method to build a tree.
 */
class TreeBuilder {
public:
    TreeBuilder() = default;
    ~TreeBuilder() = default;

    /**
     * @brief Build a tree (Newick format) from a distance matrix file.
     * @param matrixFile Input distance matrix (e.g., CSV or whitespace separated).
     * @param outputNewick Path to output Newick file.
     * @return true on success.
     */
    bool buildTree(const string& matrixFile, const string& outputNewick);

    /**
     * @brief Optionally, generate an image (PNG) of the tree using a Python script.
     * @param newickFile Newick file path.
     * @param outputImage PNG output path.
     * @return true if image generated.
     */
    bool generateTreeImage(const string& newickFile, const string& outputImage);
};

#endif // TREEBUILDER_H
