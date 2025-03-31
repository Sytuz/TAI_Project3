#include "FCMModel.h"
#include <iostream>
#include <fstream>
#include <cmath>
#include <algorithm>
#include <getopt.h>
#include <vector>
#include <string>
#include <iomanip>

using namespace std;

// Structure to hold reference sequence information
struct Reference {
    string name;
    string sequence;
    double nrc;
};

// DNA-specific NRC calculator for FCMModel
class DNACompressor {
private:
    FCMModel& model;

public:
    DNACompressor(FCMModel& fcm) : model(fcm) {}

    // Calculate bits needed to encode a DNA sequence using the FCMModel
    double calculateBits(const string& sequence) const {
        double totalBits = 0.0;
        int k = model.getK();

        if (sequence.length() <= static_cast<size_t>(k)) {
            return 2.0 * sequence.length(); // Log2(4) = 2 bits per symbol for DNA
        }

        // Process each k+1 length substring
        for (size_t i = 0; i <= sequence.length() - k - 1; ++i) {
            string context = sequence.substr(i, k);
            string nextSymbol = sequence.substr(i + k, 1);
            
            double probability = model.getProbability(context, nextSymbol);
            totalBits += -log2(probability);
        }

        return totalBits;
    }

    // Calculate Normalized Relative Compression for DNA
    double calculateNRC(const string& sequence) const {
        if (sequence.length() <= static_cast<size_t>(model.getK())) {
            return 1.0; // Handle short sequences
        }

        double bits = calculateBits(sequence);
        // For DNA: |x|*log2(A) = |x|*2 since alphabet size is 4
        return bits / (2.0 * sequence.length());
    }
};

// Utility Functions

// Read a metagenomic sample from file, filtering out non-DNA characters
string readMetagenomicSample(const string& filename) {
    ifstream file(filename);
    string sample;
    string line;

    if (!file.is_open()) {
        cerr << "Error opening file: " << filename << endl;
        exit(1);
    }

    while (getline(file, line)) {
        // Process the line to keep only DNA nucleotides
        for (char c : line) {
            // Only keep A, C, G, T (case insensitive)
            if (c == 'A' || c == 'a' || c == 'C' || c == 'c' || 
                c == 'G' || c == 'g' || c == 'T' || c == 't') {
                sample += toupper(c);  // Convert to uppercase
            }
        }
    }

    file.close();
    return sample;
}

// Read reference database from file, filtering out non-DNA characters
vector<Reference> readReferenceDatabase(const string& filename) {
    ifstream file(filename);
    vector<Reference> references;
    Reference reference;
    string line;

    if (!file.is_open()) {
        cerr << "Error opening file: " << filename << endl;
        exit(1);
    }

    while (getline(file, line)) {
        // Skip empty lines
        if (line.empty()) continue;

        // Lines starting with '@' indicate a new sequence name
        if (line[0] == '@') {
            // Save previous reference if exists
            if (!reference.name.empty() && !reference.sequence.empty()) {
                references.push_back(reference);
            }

            // Start a new reference
            reference.name = line.substr(1);    // Remove '@' character
            reference.sequence = "";
        }
        else {
            // Process the line to keep only DNA nucleotides
            for (char c : line) {
                // Only keep A, C, G, T (case insensitive)
                if (c == 'A' || c == 'a' || c == 'C' || c == 'c' || 
                    c == 'G' || c == 'g' || c == 'T' || c == 't') {
                    reference.sequence += toupper(c);  // Convert to uppercase
                }
            }
        }
    }

    // Add the last reference
    if (!reference.name.empty() && !reference.sequence.empty()) {
        references.push_back(reference);
    }

    file.close();
    return references;
}

// Print help menu
void printHelp(const char* programName) {
    cout << "MetaClass: A tool for DNA sequence similarity using Normalized Relative Compression" << endl;
    cout << "Usage: " << programName << " [options]" << endl;
    cout << "\nOptions:" << endl;
    cout << "  -h, --help                 Show this help message" << endl;
    cout << "  -d, --database <file>      Reference database file" << endl;
    cout << "  -s, --sample <file>        Metagenomic sample file" << endl;
    cout << "  -k, --context <size>       Context size (default: 10)" << endl;
    cout << "  -a, --alpha <value>        Smoothing parameter (default: 0.1)" << endl;
    cout << "  -t, --top <count>          Number of top results to display (default: 20)" << endl;
    cout << "  -m, --save-model <file>    Save the trained model to a file (without extension)" << endl;
    cout << "  -l, --load-model <file>    Load a model from file instead of training" << endl;
    cout << "  --json                     Use JSON format for model saving/loading (default is binary)" << endl;
    cout << "\nExamples:" << endl;
    cout << "  " << programName << " -d samples/db.txt -s samples/meta.txt" << endl;
    cout << "  " << programName << " -d samples/db.txt -s samples/meta.txt -k 8 -a 0.05 -t 10" << endl;
    cout << "  " << programName << " -d samples/db.txt -s samples/meta.txt -m model" << endl;
    cout << "  " << programName << " -d samples/db.txt -l model.bson" << endl;
    cout << "  " << programName << " -d samples/db.txt -s samples/meta.txt -m model --json" << endl;
}

// Main function
int main(int argc, char* argv[]) {
    // Default parameter values
    string dbFile, sampleFile, saveModelFile, loadModelFile;
    int k = 10;
    double alpha = 0.1;
    int topN = 20;
    bool showHelp = false;
    bool useJson = false;

    // Define long options
    static struct option long_options[] = {
        {"help",       no_argument,       0, 'h'},
        {"database",   required_argument, 0, 'd'},
        {"sample",     required_argument, 0, 's'},
        {"context",    required_argument, 0, 'k'},
        {"alpha",      required_argument, 0, 'a'},
        {"top",        required_argument, 0, 't'},
        {"save-model", required_argument, 0, 'm'},
        {"load-model", required_argument, 0, 'l'},
        {"json",       no_argument,       0, 'j'},
        {0, 0, 0, 0}
    };

    // Parse command line options
    int option_index = 0;
    int opt;
    while ((opt = getopt_long(argc, argv, "hd:s:k:a:t:m:l:j", long_options, &option_index)) != -1) {
        switch (opt) {
            case 'h':
                showHelp = true;
                break;
            case 'd':
                dbFile = optarg;
                break;
            case 's':
                sampleFile = optarg;
                break;
            case 'k':
                k = stoi(optarg);
                break;
            case 'a':
                alpha = stod(optarg);
                break;
            case 't':
                topN = stoi(optarg);
                break;
            case 'm':
                saveModelFile = optarg;
                break;
            case 'l':
                loadModelFile = optarg;
                break;
            case 'j':
                useJson = true;
                break;
            default:
                cerr << "Unknown option. Use --help for usage information." << endl;
                return 1;
        }
    }

    // Show help if requested or if required arguments are missing
    if (showHelp || (dbFile.empty() && !showHelp)) {
        printHelp(argv[0]);
        return showHelp ? 0 : 1;
    }

    // Verify database file is provided
    if (dbFile.empty()) {
        cerr << "Error: Database file is required." << endl;
        return 1;
    }

    // Create FCMModel
    FCMModel model(k, alpha);

    try {
        // Load model if specified
        if (!loadModelFile.empty()) {
            cout << "Loading model from file: " << loadModelFile << endl;
            
            // Determine if the file is JSON or binary based on extension
            bool isBinaryFile = loadModelFile.find(".json") == string::npos;
            
            // Override with command line option
            if (useJson) {
                isBinaryFile = false;
            }
            
            model.importModel(loadModelFile, isBinaryFile);
            k = model.getK();
            alpha = model.getAlpha();
            cout << "Model loaded successfully (k=" << k << ", alpha=" << alpha << ")" << endl;
        }
        // Otherwise, train on the sample
        else if (!sampleFile.empty()) {
            cout << "Reading metagenomic sample from: " << sampleFile << endl;
            string sample = readMetagenomicSample(sampleFile);

            if (sample.empty()) {
                cerr << "Error: Empty metagenomic sample" << endl;
                return 1;
            }

            cout << "Metagenomic sample length: " << sample.length() << " nucleotides" << endl;
            cout << "Training FCM model with k=" << k << ", alpha=" << alpha << endl;
            
            model.learn(sample);
            model.lockModel(); // Lock the model after training
            
            // Save model if requested
            if (!saveModelFile.empty()) {
                cout << "Saving model to file: " << saveModelFile << endl;
                string exportedFile = model.exportModel(saveModelFile, !useJson);
                cout << "Model saved to: " << exportedFile << endl;
            }
        } else {
            cerr << "Error: Either a sample file (-s) or a model file (-l) must be provided." << endl;
            return 1;
        }

        // Read the reference database
        cout << "Reading reference database from: " << dbFile << endl;
        vector<Reference> references = readReferenceDatabase(dbFile);

        if (references.empty()) {
            cerr << "Error: No references found in database" << endl;
            return 1;
        }

        cout << "Found " << references.size() << " reference sequences" << endl;

        // Create DNA compressor using the FCMModel
        DNACompressor compressor(model);

        // Calculate NRC for each reference sequence
        cout << "Calculating NRC for each reference..." << endl;
        for (auto& ref : references) {
            ref.nrc = compressor.calculateNRC(ref.sequence);
        }

        // Sort references by NRC (lowest to highest = most to least similar)
        sort(references.begin(), references.end(), 
            [](const Reference& a, const Reference& b) { return a.nrc < b.nrc; });

        // Print the top N results
        cout << "\nTop " << topN << " most similar sequences:" << endl;
        cout << "----------------------------------------" << endl;
        cout << setw(4) << "Rank" << " | " << setw(10) << "NRC" << " | " << "Reference Name" << endl;
        cout << "----------------------------------------" << endl;
        
        for (int i = 0; i < min(topN, static_cast<int>(references.size())); ++i) {
            cout << setw(4) << i+1 << " | " << setw(10) << fixed << setprecision(6) 
                << references[i].nrc << " | " << references[i].name << endl;
        }

    } catch (const exception& e) {
        cerr << "Error: " << e.what() << endl;
        return 1;
    }

    return 0;
}