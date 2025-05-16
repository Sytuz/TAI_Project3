#include "../../include/core/TreeBuilder.h"
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <algorithm>
#include <filesystem>

using namespace std;

// Helper class for UPGMA algorithm
class Node {
public:
    string name;
    double height = 0.0;
    Node* left = nullptr;
    Node* right = nullptr;

    Node(string n) : name(n) {}
    Node(Node* l, Node* r, double h) : left(l), right(r), height(h) {}

    string toNewick() {
        if (!left && !right) {
            return name;
        }

        string leftStr = left ? left->toNewick() : "";
        string rightStr = right ? right->toNewick() : "";

        return "(" + leftStr + ":" + to_string(height - left->height) + "," + rightStr + ":" + to_string(height - right->height) + ")";
    }
};

bool TreeBuilder::buildTree(const string& matrixFile, const string& outputNewick) {
    // Read distance matrix
    ifstream inFile(matrixFile);
    if (!inFile) {
        cerr << "Failed to open matrix file: " << matrixFile << endl;
        return false;
    }

    vector<vector<double>> distMatrix;
    string line;
    while (getline(inFile, line)) {
        stringstream ss(line);
        vector<double> row;
        double val;
        while (ss >> val) {
            row.push_back(val);
            // Skip comma if present
            if (ss.peek() == ',') ss.ignore();
        }
        distMatrix.push_back(row);
    }

    int n = distMatrix.size();
    if (n == 0) {
        cerr << "Empty distance matrix." << endl;
        return false;
    }

    // Initialize leaf nodes
    vector<Node*> nodes;
    for (int i = 0; i < n; i++) {
        nodes.push_back(new Node("Seq" + to_string(i)));
    }

    // UPGMA algorithm
    while (nodes.size() > 1) {
        // Find minimum distance pair
        int minI = 0, minJ = 1;
        double minDist = distMatrix[0][1];

        for (size_t i = 0; i < nodes.size(); i++) {
            for (size_t j = i + 1; j < nodes.size(); j++) {
                if (distMatrix[i][j] < minDist) {
                    minDist = distMatrix[i][j];
                    minI = i;
                    minJ = j;
                }
            }
        }

        // Create new node
        Node* newNode = new Node(nodes[minI], nodes[minJ], minDist / 2.0);

        // Calculate distances to the new merged node
        vector<double> newDists;
        for (size_t k = 0; k < nodes.size(); k++) {
            if (k != minI && k != minJ) {
                // UPGMA uses arithmetic mean of distances
                double newDist = (distMatrix[minI][k] + distMatrix[minJ][k]) / 2.0;
                newDists.push_back(newDist);
            }
        }

        // Update distance matrix - remove rows/cols for merged nodes
        vector<vector<double>> newMatrix;
        for (size_t i = 0; i < nodes.size(); i++) {
            if (i != minI && i != minJ) {
                vector<double> row;
                for (size_t j = 0; j < nodes.size(); j++) {
                    if (j != minI && j != minJ) {
                        row.push_back(distMatrix[i][j]);
                    }
                }
                newMatrix.push_back(row);
            }
        }

        // Add row/col for new node
        for (size_t i = 0; i < newDists.size(); i++) {
            newMatrix[i].push_back(newDists[i]);
        }

        vector<double> newRow = newDists;
        newRow.push_back(0.0);  // Distance to self = 0
        newMatrix.push_back(newRow);

        // Update nodes list
        vector<Node*> newNodes;
        for (size_t i = 0; i < nodes.size(); i++) {
            if (i != minI && i != minJ) {
                newNodes.push_back(nodes[i]);
            }
        }
        newNodes.push_back(newNode);

        // Update for next iteration
        nodes = newNodes;
        distMatrix = newMatrix;
    }

    // Write Newick format tree
    ofstream outFile(outputNewick);
    if (!outFile) {
        cerr << "Failed to open output file: " << outputNewick << endl;
        return false;
    }

    outFile << nodes[0]->toNewick() << ";" << endl;
    outFile.close();

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
