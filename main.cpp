#include "FCMModel.h"
// #include <iostream>
// #include <string>
// #include <cstdlib>

using namespace std;

void printError() {
    cout << "Enter: ./fcm <input_file> -k <context_size> -a <alpha>\n";
    cout << "Example: ./fcm sequences/sequence1.txt -k 3 -a 0.01\n";
}

int main(int argc, char* argv[]) {
    if (argc != 6) {
        printError();
        return 1;
    }

    string inputFile = argv[1];
    int k = 3;  // default context size
    double alpha = 0.1;  // default alpha

    for (int i = 2; i < argc; i += 2) {
        string arg = argv[i];
        if (arg == "-k") {
            k = atoi(argv[i + 1]);
        } else if (arg == "-a") {
            alpha = atof(argv[i + 1]);
        }
    }

    try {
        FCMModel model(k, alpha);
        string text = readFile(inputFile);
        model.buildModel(text);

        double avgInfoCont = model.computeAverageInformationContent(text);
        
        cout << "\nAnalysis Results for " << inputFile << ":\n";
        cout << "Context Size (k): " << k << "\n";
        cout << "Alpha: " << alpha << "\n";
        cout << "Average Information Content: " << avgInfoCont << " bits per symbol\n";

    } catch (const exception& e) {
        cerr << "Error: " << e.what() << endl;
        return 1;
    }

    return 0;
}