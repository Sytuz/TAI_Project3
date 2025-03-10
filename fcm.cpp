#include "FCMModel.h"
#include <iostream>
#include <string>
#include <cstdlib>

using namespace std;

void printError() {
    cout << "Usage: ./fcm <input_file> -k <context_size> -a <alpha> -o <output_model> [--json]\n";
    cout << "Example: ./fcm sequences/sequence1.txt -k 3 -a 0.1 -o model --json\n";
    cout << "\nOptions:\n";
    cout << "  -k <order>     : Context size (default: 3)\n";
    cout << "  -a <alpha>     : Smoothing parameter (default: 0.1)\n";
    cout << "  -o <outfile>   : Output file for the model (without extension)\n";
    cout << "  --json         : Save in JSON format (default is binary)\n";
}

int main(int argc, char* argv[]) {
    if (argc < 8) {
        printError();
        return 1;
    }

    string inputFile = argv[1];
    int k = 3;  // default context size
    double alpha = 0.1;  // default alpha
    string outputFile = "model";
    bool binary = true; // default to binary format

    for (int i = 2; i < argc; i += 2) {
        string arg = argv[i];
        if (arg == "-k") {
            k = atoi(argv[i + 1]);
        } else if (arg == "-a") {
            alpha = atof(argv[i + 1]);
        } else if (arg == "-o") {
            outputFile = argv[i + 1];
        } else if (arg == "--json") {
            binary = false;
        }
    }

    try {
        FCMModel model(k, alpha);
        string text = readFile(inputFile);
        model.learn(text);
        
        // Lock the model before export
        model.lockModel();
        
        // Calculate and display information content
        double avgInfoCont = model.computeAverageInformationContent(text);
        cout << "\nAnalysis Results for " << inputFile << ":\n";
        cout << "Context Size (k): " << k << "\n";
        cout << "Alpha: " << alpha << "\n";
        cout << "Average Information Content: " << avgInfoCont << " bits per symbol\n";
        
        // Export the model with the specified format
        string exportedFilename = model.exportModel(outputFile, binary);
        cout << "\nModel successfully exported to: " << exportedFilename << "\n";

    } catch (const exception& e) {
        cerr << "Error: " << e.what() << endl;
        return 1;
    }

    return 0;
}