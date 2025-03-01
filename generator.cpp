#include "FCMModel.h"
#include <iostream>
#include <string>
#include <cstdlib>
#include <stdexcept>

using namespace std;

void printError() {
    cout << "Usage: ./generator <model_file> -p <prior> -s <size>\n";
    cout << "Example: ./generator model.json -p \"the\" -s 100\n";
    cout << "         ./generator model.bson -p \"the\" -s 100\n";
    cout << "\nOptions:\n";
    cout << "  <model_file>  : Path to the model file (.json or .bson format)\n";
    cout << "  -p <prior>    : Prior context to start text generation\n";
    cout << "  -s <size>     : Number of symbols to generate (default: 100)\n";
    cout << "\nNote: The format (JSON or binary) is detected automatically from the file extension.\n";
    cout << "      Models should be created first using the fcm program.\n";
}

void validatePrior(const string& prior, int k) {
    if (static_cast<int>(prior.length()) != k) { // Static cast to avoid a warning of unsigned int comparison
        throw runtime_error("Prior length must match the model's context size k");
    }
}

int main(int argc, char* argv[]) {
    if (argc != 6) {
        printError();
        return 1;
    }

    string modelFile = argv[1];
    string prior;
    int size = 100;  // default size

    // Parse command line arguments
    for (int i = 2; i < argc; i += 2) {
        string arg = argv[i];
        if (arg == "-p") {
            prior = argv[i + 1];
        } else if (arg == "-s") {
            size = atoi(argv[i + 1]);
        }
    }

    try {
        // Determine if the model file is binary based on extension
        bool binary = false;
        string fileExtension;
        size_t dotPosition = modelFile.find_last_of('.');
        
        if (dotPosition != string::npos) {
            fileExtension = modelFile.substr(dotPosition);
            if (fileExtension == ".bson") {
                binary = true;
            } else if (fileExtension != ".json") {
                // Warning for unexpected extension
                cout << "Warning: Unrecognized file extension '" << fileExtension 
                     << "'. Assuming JSON format." << endl;
            }
        } else {
            // No extension provided
            cout << "Warning: No file extension found. Assuming JSON format." << endl;
        }

        // Load the pre-trained model
        FCMModel model;
        model.importModel(modelFile, binary);
        
        // The model should already be locked from fcm export but could be locked here as well
        // model.lockModel();
        
        // Validate prior length against model's k
        validatePrior(prior, model.getK());
        
        // Generate text
        string generatedText = model.predict(prior, size);
        
        // Output results
        cout << "\nText Generation Results:\n";
        cout << "Model File: " << modelFile << "\n";
        cout << "Format: " << (binary ? "Binary (BSON)" : "JSON") << "\n";
        cout << "Prior Context: " << prior << "\n";
        cout << "Generated Size: " << size << "\n";
        cout << "\nGenerated Text:\n" << generatedText << "\n";

    } catch (const exception& e) {
        cerr << "Error: " << e.what() << endl;
        return 1;
    }

    return 0;
}