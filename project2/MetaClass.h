#ifndef METACLASS_H
#define METACLASS_H

#include <string>
#include <vector>
#include <unordered_map>

using namespace std;

// Structure to store the sequence data
struct Reference {
    string name;
    string sequence;
    double nrc; // Normalized Relative Compression (measure of how well the trained model compresses the sequence - lower values indicate greater similarity)
};

// Class for the Markov model
class MarkovModel {
    private:
        size_t k;  // Context size
        double alpha;   // Smoothing parameter
        unordered_map<string, vector<int>> model;   // Maps context to counts (maps each k-length context to a vector of counts for each possible next symbol)

        // Helper function to convert the nucleotide to index (A=0, C=1, G=2, T=3)
        int symbolToIndex(char symbol);

        // Helper function to get the total counts for a context (used in probability calculations)
        int getTotalCount(const vector<int>& counts);

    public:
        MarkovModel(size_t contextSize, double smoothingParam);

        // Train the model on a sequence
        void train(const string& sequence);

        // Calculate the bits (information content) needed to encode a sequence using the trained model
        double calculateBits(const string& sequence);

        // Calculate NRC for a sequence (normalizes the bit count to allow fair comparison between sequences of different lengths)
        double calculateNRC(const string& sequence);
};

// Function to read the metagenomic sample file
string readMetagenomicSample(const string& filename);

// Function to read the reference database
vector<Reference> readReferenceDatabase(const string& filename);

// Function to parse the command line arguments
void parseCommandLineArguments(int argc, char* argv[], string& dbFile, string& sampleFile, int& k, double& alpha, int& topN);

#endif // METACLASS_H
