#ifndef EXTRACTION_UTILS_H
#define EXTRACTION_UTILS_H

#include <string>
#include <atomic>
#include <mutex>
#include <vector>

using namespace std;

namespace FeatureExtractor {
    /**
     * Save extraction configuration to a text file in the output directory
     */
    void saveConfig(
        const string& outFolder, 
        const string& method,
        int numFrequencies, 
        int numBins,
        int frameSize, 
        int hopSize, 
        int filesProcessed
    );
    
    /**
     * Save features in text format
     */
    bool saveFeaturesText(const string& outFile, const string& featData);
    
    /**
     * Extract features from a single WAV file
     */
    bool extractFeaturesFromFile(
        const string& wavFile, 
        const string& outFolder, 
        const string& method, 
        int numFrequencies, 
        int numBins,
        int frameSize, 
        int hopSize,
        mutex& coutMutex,
        atomic<int>& filesProcessed,
        atomic<int>& filesSkipped
    );
}

#endif // EXTRACTION_UTILS_H