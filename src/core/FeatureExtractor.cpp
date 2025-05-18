#include "../../include/core/FeatureExtractor.h"
#include "../../include/core/SpectralExtractor.h"
#include "../../include/core/MaxFreqExtractor.h"
#include "../../include/core/WAVReader.h"

#include <iostream>
#include <fstream>
#include <filesystem>
#include <chrono>
#include <iomanip>

namespace FeatureExtractor {

void saveConfig(
    const string& outFolder, 
    const string& method,
    int numFrequencies, 
    int numBins,
    int frameSize, 
    int hopSize, 
    int filesProcessed
) {
    // Get current time
    auto now = chrono::system_clock::to_time_t(chrono::system_clock::now());
    
    // Create config summary in text format for easy reading
    ofstream txtConfig(outFolder + "/extraction_config.txt");
    if (txtConfig) {
        txtConfig << "Feature Extraction Configuration" << endl;
        txtConfig << "===============================" << endl;
        txtConfig << "Date: " << put_time(localtime(&now), "%Y-%m-%d %H:%M:%S") << endl;
        txtConfig << "Method: " << method << endl;
        txtConfig << "Format: text" << endl;
        txtConfig << "Frame size: " << frameSize << " samples" << endl;
        txtConfig << "Hop size: " << hopSize << " samples" << endl;
        
        if (method == "maxfreq") {
            txtConfig << "Frequencies per frame: " << numFrequencies << endl;
        } else {
            txtConfig << "Frequency bins: " << numBins << endl;
        }
        
        txtConfig << "Files processed: " << filesProcessed << endl;
        txtConfig.close();
        
        cout << "Configuration saved to " << outFolder << "/extraction_config.txt" << endl;
    }
}

bool saveFeaturesText(const string& outFile, const string& featData) {
    ofstream out(outFile + ".feat");
    if (!out) {
        cerr << "  Error: Could not open output file: " << outFile << ".feat" << endl;
        return false;
    }
    
    out << featData;
    out.close();
    return true;
}

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
) {
    WAVReader reader;
    SpectralExtractor specExt(numBins);
    MaxFreqExtractor mfExt(numFrequencies);
    
    {
        lock_guard<mutex> lock(coutMutex);
        cout << "Processing: " << wavFile << endl;
    }
    
    if (!reader.load(wavFile)) {
        lock_guard<mutex> lock(coutMutex);
        cout << "  Skipping due to load error" << endl;
        filesSkipped++;
        return false;
    }
    
    auto samples = reader.samples();
    int channels = reader.getChannels();
    int sampleRate = reader.getSampleRate();
    
    // Convert to int16_t for feature extraction
    vector<int16_t> samples16bit(samples.size());
    for (size_t j = 0; j < samples.size(); j++) {
        samples16bit[j] = static_cast<int16_t>(samples[j]);
    }
    
    // Extract features
    string featData;
    auto extractStart = chrono::high_resolution_clock::now();
    
    if (method == "spectral") {
        featData = specExt.extractFeatures(samples16bit, channels, frameSize, hopSize, sampleRate);
    } else if (method == "maxfreq") {
        featData = mfExt.extractFeatures(samples16bit, channels, frameSize, hopSize, sampleRate);
    }
    
    auto extractEnd = chrono::high_resolution_clock::now();
    auto extractTime = chrono::duration_cast<chrono::milliseconds>(extractEnd - extractStart).count();
    
    {
        lock_guard<mutex> lock(coutMutex);
        cout << "  Feature extraction took " << extractTime << " ms" << endl;
    }
    
    // Save to output
    string base = filesystem::path(wavFile).stem().string();
    string outFile = outFolder + "/" + base + "_" + method;
    
    bool saveSuccess = saveFeaturesText(outFile, featData);
    if (!saveSuccess) {
        filesSkipped++;
        return false;
    }
    
    {
        lock_guard<mutex> lock(coutMutex);
        cout << "  Extracted features to " << outFile << ".feat" << endl;
    }
    
    filesProcessed++;
    return true;
}

}