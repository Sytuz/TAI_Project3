#include "FCMModel.h"
#include <iostream>
#include <string>
#include <cstdlib>
#include <stdexcept>

using namespace std;

void printError() {
    cout << "Usage: \n";
    cout << "  Generate text from an existing model:\n";
    cout << "    ./generator -m <model_file> -p <prior> -s <size>\n";
    cout << "  Generate model from text and then generate text:\n";
    cout << "    ./generator -f <text_file> -k <order> -a <alpha> [-o <output_model>] [--json] -p <prior> -s <size>\n";
    cout << "\nExamples:\n";
    cout << "  ./generator -m model.json -p \"the\" -s 100\n";
    cout << "  ./generator -f sequences/sequence1.txt -k 3 -a 0.1 -p \"the\" -s 100\n";
    cout << "  ./generator -f sequences/sequence1.txt -k 3 -a 0.1 -o new_model --json -p \"the\" -s 100\n";
    cout << "\nOptions:\n";
    cout << "  -m <model_file>  : Path to an existing model file (.json or .bson format)\n";
    cout << "  -f <text_file>   : Path to a text file to learn from\n";
    cout << "  -k <order>       : Context size (default: 3)\n";
    cout << "  -a <alpha>       : Smoothing parameter (default: 0.1)\n";
    cout << "  -o <output_model>: Optional name to save the trained model\n";
    cout << "  --json           : Save model in JSON format (default is binary)\n";
    cout << "  -p <prior>       : Prior context to start text generation\n";
    cout << "  -s <size>        : Number of symbols to generate (default: 100)\n";
    cout << "\nNote: The format for existing models is detected automatically from the extension.\n";
}

void validatePrior(const string& prior, int k) {
    if (static_cast<int>(prior.length()) != k) { // Static cast to avoid a warning of unsigned int comparison
        throw runtime_error("Prior length must match the model's context size k");
    }
}

int main(int argc, char* argv[]) {
    if (argc < 6) {
        printError();
        return 1;
    }

    // Variables for model loading mode
    string modelFile;
    bool loadModel = false;
    
    // Variables for training mode
    string textFile;
    bool trainModel = false;
    int k = 3;
    double alpha = 0.1;
    string outputModel;
    bool saveModel = false;
    bool jsonFormat = false;
    
    // Common variables
    string prior;
    int size = 100;

    // Parse command line arguments
    for (int i = 1; i < argc; i++) {
        string arg = argv[i];
        
        if (arg == "--json") {
            jsonFormat = true;
            continue;
        }
        
        // Check if there's a next argument
        if (i + 1 >= argc) {
            cerr << "Error: Missing value for argument " << arg << endl;
            printError();
            return 1;
        }
        
        // Parse arguments
        if (arg == "-m") {
            modelFile = argv[++i];
            loadModel = true;
        } else if (arg == "-f") {
            textFile = argv[++i];
            trainModel = true;
        } else if (arg == "-k") {
            k = atoi(argv[++i]);
        } else if (arg == "-a") {
            alpha = atof(argv[++i]);
        } else if (arg == "-o") {
            outputModel = argv[++i];
            saveModel = true;
        } else if (arg == "-p") {
            prior = argv[++i];
        } else if (arg == "-s") {
            size = atoi(argv[++i]);
        }
    }

    // Validate modes - we need exactly one source (either load or train)
    if ((loadModel && trainModel) || (!loadModel && !trainModel)) {
        cerr << "Error: Must specify exactly one of -m or -f" << endl;
        printError();
        return 1;
    }

    // Check for required parameters for each mode
    if (trainModel && (prior.empty() || size <= 0)) {
        cerr << "Error: Training mode requires both -p and -s parameters" << endl;
        printError();
        return 1;
    }

    if (loadModel && (modelFile.empty() || prior.empty() || size <= 0)) {
        cerr << "Error: Model loading mode requires -m, -p, and -s parameters" << endl;
        printError();
        return 1;
    }

    // Run the generator
    try {
        FCMModel model;
        
        if (loadModel) {
            // Load an existing model
            bool binary = false;
            size_t dotPosition = modelFile.find_last_of('.');
            
            if (dotPosition != string::npos) {
                string fileExtension = modelFile.substr(dotPosition);
                if (fileExtension == ".bson") {
                    binary = true;
                } else if (fileExtension != ".json") {
                    cout << "Warning: Unrecognized file extension. Assuming JSON format." << endl;
                }
            } else {
                cout << "Warning: No file extension found. Assuming JSON format." << endl;
            }
            
            model.importModel(modelFile, binary);
            cout << "Model successfully loaded from: " << modelFile << endl;
            
        } else if (trainModel) {
            // Train a new model from text
            model = FCMModel(k, alpha);
            
            // Read the text file
            string text = readFile(textFile);
            cout << "Learning from text file: " << textFile << endl;
            
            // Train the model
            model.learn(text);
            cout << "Model successfully trained with k=" << k << ", alpha=" << alpha << endl;
            
            // Save model if requested
            if (saveModel) {
                string exportedFile = model.exportModel(outputModel, !jsonFormat);
                cout << "Model saved to: " << exportedFile << endl;
            }
        }
        
        // Lock the model before generating
        if (!model.isLocked()) {
            model.lockModel();
        }
        
        // Validate prior length against model's k
        validatePrior(prior, model.getK());
        
        // Generate text
        string generatedText = model.predict(prior, size);
        
        // Output results
        cout << "\nText Generation Results:\n";
        cout << "Source: " << (loadModel ? modelFile : textFile) << "\n";
        cout << "Model Order (k): " << model.getK() << "\n";
        cout << "Smoothing (alpha): " << model.getAlpha() << "\n";
        cout << "Prior Context: " << prior << "\n";
        cout << "Generated Size: " << size << "\n";
        cout << "\nGenerated Text:\n" << generatedText << "\n";

    } catch (const exception& e) {
        cerr << "Error: " << e.what() << endl;
        return 1;
    }

    return 0;
}