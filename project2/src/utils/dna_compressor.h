#ifndef DNA_COMPRESSOR_H
#define DNA_COMPRESSOR_H

#include "FCMModel.h"
#include <string>

// DNA-specific calculator for FCMModel metrics
class DNACompressor
{
private:
    FCMModel &model;

public:
    DNACompressor(FCMModel &fcm);

    // Calculate bits needed to encode a DNA sequence using the FCMModel
    double calculateBits(const std::string &sequence) const;

    // Calculate Normalized Relative Compression for DNA
    double calculateNRC(const std::string &sequence) const;

    // Calculate Kullback-Leibler Divergence between model and sequence distribution
    double calculateKLD(const std::string &sequence) const;
};

#endif // DNA_COMPRESSOR_H