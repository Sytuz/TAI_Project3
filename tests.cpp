#include "FCMModel.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <chrono>
#include <cmath>
#include <string>
#include <algorithm>
#include <filesystem>

using namespace std;
using namespace std::chrono;

// Structure to hold reference sequence information (same as in MetaClass.cpp)
struct Reference {
    string name;
    string sequence;
    double nrc;
};

// DNA-specific NRC calculator
class DNACompressor {
private:
    FCMModel& model;

public:
    DNACompressor(FCMModel& fcm) : model(fcm) {}

    double calculateBits(const string& sequence) const {
        double totalBits = 0.0;
        int k = model.getK();

        if (sequence.length() <= static_cast<size_t>(k)) {
            return 2.0 * sequence.length();
        }

        for (size_t i = 0; i <= sequence.length() - static_cast<size_t>(k) - 1; ++i) {
            string context = sequence.substr(i, k);
            string nextSymbol = sequence.substr(i + k, 1);
            
            double probability = model.getProbability(context, nextSymbol);
            totalBits += -log2(probability);
        }

        return totalBits;
    }

    double calculateNRC(const string& sequence) const {
        if (sequence.length() <= static_cast<size_t>(model.getK())) {
            return 1.0;
        }

        double bits = calculateBits(sequence);
        return bits / (2.0 * sequence.length());
    }
};

// Utility function to read DNA sequences
string readDNASequence(const string& filename) {
    ifstream file(filename);
    string sequence;
    string line;

    if (!file.is_open()) {
        cerr << "Error opening file: " << filename << endl;
        return "";
    }

    while (getline(file, line)) {
        for (char c : line) {
            if (c == 'A' || c == 'a' || c == 'C' || c == 'c' || 
                c == 'G' || c == 'g' || c == 'T' || c == 't') {
                sequence += toupper(c);
            }
        }
    }

    file.close();
    return sequence;
}

// Test function to measure NRC between two DNA sequences
void testNRC(const string& sampleFile, const string& referenceFile, int k = 10, double alpha = 0.1) {
    auto startTime = high_resolution_clock::now();
    
    string sample = readDNASequence(sampleFile);
    string reference = readDNASequence(referenceFile);
    
    if (sample.empty() || reference.empty()) {
        cerr << "Error: Empty sequence(s)" << endl;
        return;
    }
    
    cout << "Sample length: " << sample.length() << " nucleotides" << endl;
    cout << "Reference length: " << reference.length() << " nucleotides" << endl;
    
    // Train model on sample
    FCMModel model(k, alpha);
    model.learn(sample);
    model.lockModel();
    
    // Calculate NRC
    DNACompressor compressor(model);
    double nrc = compressor.calculateNRC(reference);
    
    auto endTime = high_resolution_clock::now();
    double execTime = duration<double, milli>(endTime - startTime).count();
    
    cout << "NRC(reference||sample): " << nrc << endl;
    cout << "Execution time: " << execTime << " ms" << endl;
}

// Main function with simple test cases
int main(int argc, char* argv[]) {
    cout << "MetaClass NRC Tests" << endl;
    cout << "===================" << endl;
    
    if (argc >= 3) {
        // Use command line arguments if provided
        string sampleFile = argv[1];
        string referenceFile = argv[2];
        int k = (argc > 3) ? stoi(argv[3]) : 10;
        double alpha = (argc > 4) ? stod(argv[4]) : 0.1;
        
        cout << "Testing NRC with:" << endl;
        cout << "- Sample: " << sampleFile << endl;
        cout << "- Reference: " << referenceFile << endl;
        cout << "- k: " << k << endl;
        cout << "- alpha: " << alpha << endl << endl;
        
        testNRC(sampleFile, referenceFile, k, alpha);
    }
    else {
        cout << "Usage: " << argv[0] << " <sample_file> <reference_file> [k=10] [alpha=0.1]" << endl;
        cout << "Running default tests..." << endl << endl;
        
        // Check if sample files exist
        if (filesystem::exists("samples/meta.txt") && filesystem::exists("samples/db.txt")) {
            testNRC("samples/meta.txt", "samples/db.txt");
        } else {
            cout << "Default sample files not found. Please provide file paths as arguments." << endl;
        }
    }
    
    return 0;
}