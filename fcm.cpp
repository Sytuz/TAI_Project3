#include "FCMModel.h"
#include <iostream>
#include <string>
#include <cstdlib>

using namespace std;

void printHelpMenu() {
    cout << "Usage: ./fcm <input_file> -k <context_size> -a <alpha> -o <output_model> [--json]\n";
    cout << "Example: ./fcm sequences/sequence1.txt -k 3 -a 0.1 -o model --json\n";
    cout << "\nOptions:\n";
    cout << "  -k <order>     : Context size (default: 3)\n";
    cout << "  -a <alpha>     : Smoothing parameter (default: 0.1)\n";
    cout << "  -o <outfile>   : Output file for the model (without extension)\n";
    cout << "  --json         : Save in JSON format (default is binary)\n";
    cout << "  -h             : Display this help menu\n";
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printHelpMenu();
        return 1;
    }
    
    // Check for help flag
    string firstArg = argv[1];
    if (firstArg == "-h") {
        printHelpMenu();
        return 0;
    }

    if (argc < 8) {
        printHelpMenu();
        return 1;
    }

    string inputFile = argv[1];
    int k = 3;  // default context size
    double alpha = 0.1;  // default alpha
    string outputFile = "model";
    bool binary = true; // default to binary format

    for (int i = 2; i < argc; i++) {
        string arg = argv[i];
        
        if (arg == "--json") {
            binary = false;
            continue;
        }
        
        // Check if there's a next argument for options that require values
        if (arg == "-k" || arg == "-a" || arg == "-o") {
            if (i + 1 >= argc) {
                cerr << "Error: Missing value for argument " << arg << endl;
                printHelpMenu();
                return 1;
            }
            
            // Parse arguments with values
            if (arg == "-k") {
                k = atoi(argv[++i]);
            } else if (arg == "-a") {
                alpha = atof(argv[++i]);
            } else if (arg == "-o") {
                outputFile = argv[++i];
            }
        } else if (arg != "--json") {
            cerr << "Error: Unknown option " << arg << endl;
            printHelpMenu();
            return 1;
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