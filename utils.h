#ifndef UTILS_H
#define UTILS_H

#include "FCMModel.h"
#include <string>
#include <vector>
#include <map>

// Structure to hold reference sequence information
struct Reference {
    std::string name;
    std::string sequence;
    double nrc = 0.0;
    double kld = 0.0;  // Added Kullback-Leibler divergence
    double compressionBits = 0.0;  // Add this new field
};

// DNA-specific calculator for FCMModel metrics
class DNACompressor {
private:
    FCMModel& model;

public:
    DNACompressor(FCMModel& fcm);

    // Calculate bits needed to encode a DNA sequence using the FCMModel
    double calculateBits(const std::string& sequence) const;

    // Calculate Normalized Relative Compression for DNA
    double calculateNRC(const std::string& sequence) const;
    
    // Calculate Kullback-Leibler Divergence between model and sequence distribution
    double calculateKLD(const std::string& sequence) const;
};

// Utility Functions
std::string readMetagenomicSample(const std::string& filename);
std::vector<Reference> readReferenceDatabase(const std::string& filename);

// Configuration parsing functions
bool parseConfigFile(const std::string& configFile, std::map<std::string, std::string>& configParams);
bool stringToBool(const std::string& value);

// Results saving functions
bool saveResultsToJson(const std::vector<std::pair<std::pair<int, double>, std::pair<std::vector<Reference>, double>>>& allResults, 
                       const std::string& outputFile);
bool saveAllResultsToJson(const std::vector<std::pair<std::pair<int, double>, std::pair<std::vector<Reference>, double>>>& allResults,
                          const std::string& outputFile);
bool saveResultsToCsv(const std::vector<std::pair<std::pair<int, double>, std::pair<std::vector<Reference>, double>>>& allResults,
                        const std::string& outputFile);
bool saveAllResultsToCsv(const std::vector<std::pair<std::pair<int, double>, std::pair<std::vector<Reference>, double>>>& allResults,
                          const std::string& outputFile);

// User interface functions
int getIntInput(const std::string& prompt, int minValue, int maxValue);
double getDoubleInput(const std::string& prompt, double minValue, double maxValue);
std::string getStringInput(const std::string& prompt);
bool askYesNo(const std::string& prompt);

#endif // UTILS_H