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

bool saveFeaturesBinary(const string& outFile, const vector<float>& featData) {
    ofstream out(outFile + ".featbin", ios::binary);
    if (!out) {
        cerr << "  Error: Could not open output file: " << outFile << ".featbin" << endl;
        return false;
    }
    out.write(reinterpret_cast<const char*>(featData.data()), featData.size() * sizeof(float));
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
    atomic<int>& filesSkipped,
    bool useBinary
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
    std::vector<std::vector<float>> featDataBin; // changed from vector<float>
    auto extractStart = chrono::high_resolution_clock::now();
    
    if (method == "spectral") {
        if (useBinary) {
            featDataBin = specExt.extractFeaturesBinary(samples16bit, channels, frameSize, hopSize, sampleRate);
        } else {
            featData = specExt.extractFeatures(samples16bit, channels, frameSize, hopSize, sampleRate);
        }
    } else if (method == "maxfreq") {
        if (useBinary) {
            featDataBin = mfExt.extractFeaturesBinary(samples16bit, channels, frameSize, hopSize, sampleRate);
        } else {
            featData = mfExt.extractFeatures(samples16bit, channels, frameSize, hopSize, sampleRate);
        }
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
    
    bool saveSuccess = false;
    if (useBinary) {
        // Flatten 2D feature vector to 1D for binary saving
        std::vector<float> flatFeatDataBin;
        for (const auto& frame : featDataBin) {
            flatFeatDataBin.insert(flatFeatDataBin.end(), frame.begin(), frame.end());
        }
        saveSuccess = saveFeaturesBinary(outFile, flatFeatDataBin);
    } else {
        saveSuccess = saveFeaturesText(outFile, featData);
    }
    if (!saveSuccess) {
        filesSkipped++;
        return false;
    }
    
    {
        lock_guard<mutex> lock(coutMutex);
        if (useBinary) {
            cout << "  Extracted features to " << outFile << ".featbin" << endl;
        } else {
            cout << "  Extracted features to " << outFile << ".feat" << endl;
        }
    }
    
    filesProcessed++;
    return true;
}

}