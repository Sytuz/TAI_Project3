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

#endif // UTILS_H