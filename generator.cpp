#include "FCMModel.h"
#include <iostream>
#include <string>
#include <cstdlib>
#include <stdexcept>

using namespace std;

void printError() {
    cout << "Usage: ./generator <model_file.json> -p <prior> -s <size>\n";
    cout << "Example: ./generator learned_model.json -p \"the\" -s 100\n";
    cout << "\nNote: The model file should be created first using the fcm program.\n";
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
        // Load the pre-trained model
        FCMModel model;
        model.importModel(modelFile);
        
        // The model should already be locked from fcm export but could be locked here as well
        // model.lockModel();
        
        // Validate prior length against model's k
        validatePrior(prior, model.getK());
        
        // Generate text
        string generatedText = model.predict(prior, size);
        
        // Output results
        cout << "\nText Generation Results:\n";
        cout << "Model File: " << modelFile << "\n";
        cout << "Prior Context: " << prior << "\n";
        cout << "Generated Size: " << size << "\n";
        cout << "\nGenerated Text:\n" << generatedText << "\n";

    } catch (const exception& e) {
        cerr << "Error: " << e.what() << endl;
        return 1;
    }

    return 0;
}