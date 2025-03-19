#include "MetaClass.h"
#include <iostream>
#include <fstream>
#include <cmath>
#include <algorithm>
#include <getopt.h>

// Markov Model Class

// Used to index into the count vectors for each context - Maps DNA nucleotides to indices (A=0, C=1, G=2, T=3)
int MarkovModel::symbolToIndex(char symbol) {
    switch (symbol) {
        case 'A':
            return 0;
        case 'C':
            return 1;
        case 'G':
            return 2;
        case 'T':
            return 3;
        default:
            return -1;  // Handle invalid symbols
    }
}

// Used to calculate the total observations for a particular context (sums all the counts in a vector)
int MarkovModel::getTotalCount(const vector<int>& counts) {
    int total = 0;
    for (int count : counts) {
        total += count;
    }
    return total;
}

// Initializes the Markov model with 2 parameters: contextSize (k- length); smoothinParam (alpha value for Laplace smoothing)
MarkovModel::MarkovModel(size_t contextSize, double smoothingParam) : k(contextSize), alpha(smoothingParam) {}

void MarkovModel::train(const string& sequence) {
    // Verifies if the sequence is long enough to extract at least one k- context
    if (sequence.length() <= k) {
        cerr << "Warning: Sequence is too short for the given context size k=" << k << "." << endl;
        return;
    }

    // Iterates through the sequence with a sliding window approach
    // For each symbol extracts the k- context (k nucleotides starting at position i), identifies the next symbol after the context, converts the next symbol to its numeric index, and skips invalid symbols (non-ACGT characters)
    for (size_t i = 0; i <= sequence.length() - k - 1; ++i) {
        string context = sequence.substr(i, k);
        char nextSymbol = sequence[i + k];
        int symbolIndex = symbolToIndex(nextSymbol);

        if (symbolIndex == -1) {
            continue;   // Skip invalid symbols
        }

        // Frequency Counting (this builds a statistical model of how often each nucleotide follows each k- context):

        // Initialize counts for new context
        if (model.find(context) == model.end()) {
            model[context] = vector<int>(4, 0);
        }

        // Increment count for the observed symbol
        model[context][symbolIndex]++;
    }
}

double MarkovModel::calculateBits(const string& sequence) {
    double totalBits = 0.0;

    if (sequence.length() <= k) {   // Handles edge cases for sequences shorter than k+1
        // For short sequences, return a default value - Uses 2 bits per symbol as default/maximum (log2(4)=2 for uniformly distributed symbols)
        return 2.0 * sequence.length();
    }

    // (similar to the training loop): iterates through the sequence and extracts each context and the symbol that follows it, skipping invalid symbols
    for (size_t i = 0; i <= sequence.length() - k - 1; ++i) {
        string context = sequence.substr(i, k);
        char nextSymbol = sequence[i + k];
        int symbolIndex = symbolToIndex(nextSymbol);

        if (symbolIndex == -1) {
            continue;   // Skip invalid symbols
        }

        double probability;

        // For each symbol after a context:
        // - if the context exists in the model, calculates the smoothed probability:
        //          - (observed count + alpha)/(total count + 4*alpha)
        //          - the 4*alpha term adds smoothing for all 4 possible nucleotides
        // - if the context is not found, use uniform probability (0.25)
        // - Calculate the information content as -log2(probability) and add to total

        if (model.find(context) != model.end()) {
            // Context found in the model
            const vector<int>& counts = model[context];
            int totalCount = getTotalCount(counts) + 4 * alpha;
            probability = (counts[symbolIndex] + alpha) / totalCount;
        }
        else {
            // Context not found in the model, use uniform probability
            probability = 0.25; // 1/4 for DNA alphabet
        }

        totalBits += -log2(probability);
    }

    // The total bits represent how efficiently the model can encode the sequence (lower bits means better compression (more predictable sequence))
    return totalBits;
}

// Calculates how efficiently the sequence is compressed relative to maximum compression
double MarkovModel::calculateNRC(const string& sequence) {
    if (sequence.length() <= k) {
        return 1.0; // Handle short sequences
    }

    double bits = calculateBits(sequence);

    // Divides by 2.0*sequence.length() (the maximum possible bits)
    // Returns a value between 0 and 1 (1 = no compression (completely different from training data)) (values closer to 0 == better compression (more similar to training daat))
    return bits / (2.0 * sequence.length());
}

// Utility Functions:

// Sample File Reading
string readMetagenomicSample(const string& filename) {
    ifstream file(filename);
    string sample;
    string line;

    if (!file.is_open()) {
        cerr << "Error opening file: " << filename << endl;
        exit(1);
    }

    while (getline(file, line)) {
        // Skip empty lines
        if (!line.empty()) {
            sample += line;
        }
    }

    file.close();
    return sample;
}

// Reference Database Reading
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
        if (line.empty()) {
            continue;
        }

        // Lines starting with '@' indicate a new sequence name (other lines contain sequence data)
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
            // Append to the current sequence
            reference.sequence += line;
        }
    }

    // Add the last reference
    if (!reference.name.empty() &&!reference.sequence.empty()) {
        references.push_back(reference);
    }

    file.close();
    return references;
}

void parseCommandLineArguments(int argc, char* argv[], std::string& dbFile, std::string& sampleFile, int& k, double& alpha, int& topN) {
    int option;

    while ((option = getopt(argc, argv, "d:s:k:a:t:"))!= -1) {
        switch (option) {
            case 'd':   // Database file path
                dbFile = optarg;
                break;
            case 's':   // Sample file path
                sampleFile = optarg;
                break;
            case 'k':   // Context size
                k = stoi(optarg);
                break;
            case 'a':   // Alpha smoothing parameter
                alpha = stod(optarg);
                break;
            case 't':   // Number of top results to display
                topN = stoi(optarg);
                break;
            default:
                std::cerr << "Usage: " << argv[0] << " -d <database_file> -s <sample_file> -k <context_size> -a <alpha> -t <top_n>" << std::endl;
                exit(1);
        }
    }

    // Check if the required arguments are provided
    if (dbFile.empty() || sampleFile.empty()) {
        std::cerr << "Database file and sample file are required." << std::endl;
        std::cerr << "Usage: " << argv[0] << " -d <database_file> -s <sample_file> -k <context_size> -a <alpha> -t <top_n>" << std::endl;
        exit(1);
    }
}