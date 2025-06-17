#include "../../include/core/MaxFreqImprovedExtractor.h"
#include <algorithm>
#include <numeric>
#include <sstream>
#include <iostream>
#include <cmath>

using namespace std;

MaxFreqImprovedExtractor::MaxFreqImprovedExtractor(int numFrequencies, int downSamplingFactor) 
    : numFreqs(numFrequencies), downSamplingFactor(downSamplingFactor), 
      fftPlan(nullptr), fftIn(nullptr), fftOut(nullptr), powerSpectrum(nullptr), currentFrameSize(0) {
    if (numFreqs <= 0) numFreqs = 4;  // Default to 4 frequencies per frame
    if (downSamplingFactor <= 0) downSamplingFactor = 4;  // Default like GetMaxFreqs.cpp
}

MaxFreqImprovedExtractor::~MaxFreqImprovedExtractor() {
    cleanupFFTW();
}

void MaxFreqImprovedExtractor::initializeFFTW(int frameSize) {
    if (currentFrameSize == frameSize && fftPlan != nullptr) {
        return;  // Already initialized for this frame size
    }
    
    cleanupFFTW();  // Clean up previous allocation
    
    currentFrameSize = frameSize;
    fftIn = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * frameSize);
    fftOut = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * frameSize);
    powerSpectrum = new double[frameSize / 2];
    
    fftPlan = fftw_plan_dft_1d(frameSize, fftIn, fftOut, FFTW_FORWARD, FFTW_ESTIMATE);
}

void MaxFreqImprovedExtractor::cleanupFFTW() {
    if (fftPlan) {
        fftw_destroy_plan(fftPlan);
        fftPlan = nullptr;
    }
    if (fftIn) {
        fftw_free(fftIn);
        fftIn = nullptr;
    }
    if (fftOut) {
        fftw_free(fftOut);
        fftOut = nullptr;
    }
    if (powerSpectrum) {
        delete[] powerSpectrum;
        powerSpectrum = nullptr;
    }
    currentFrameSize = 0;
}

vector<double> MaxFreqImprovedExtractor::extractAndDownsampleFrame(
    const vector<int16_t>& samples, int channels, size_t frameStart, int frameSize) {
    
    vector<double> frame(frameSize, 0.0);
    
    for (int k = 0; k < frameSize; ++k) {
        double sum = 0.0;
        
        // Convert to mono and down-sample like GetMaxFreqs.cpp
        for (int l = 0; l < downSamplingFactor; ++l) {
            size_t sampleIdx = frameStart + k * downSamplingFactor + l;
            
            if (channels == 2) {
                // Stereo: add left + right channels
                if (sampleIdx * 2 + 1 < samples.size()) {
                    sum += static_cast<double>(samples[sampleIdx * 2]) + 
                           static_cast<double>(samples[sampleIdx * 2 + 1]);
                }
            } else {
                // Mono: just use the sample
                if (sampleIdx < samples.size()) {
                    sum += static_cast<double>(samples[sampleIdx]);
                }
            }
        }
        
        frame[k] = sum;
    }
    
    return frame;
}

void MaxFreqImprovedExtractor::computeFFTW(const vector<double>& frame, vector<double>& powerOut) {
    // Fill FFTW input buffer
    for (size_t k = 0; k < frame.size(); ++k) {
        fftIn[k][0] = frame[k];  // Real part
        fftIn[k][1] = 0.0;       // Imaginary part
    }
    
    // Execute FFT
    fftw_execute(fftPlan);
    
    // Compute power spectrum (only up to Nyquist frequency)
    int halfSize = currentFrameSize / 2;
    powerOut.resize(halfSize);
    for (int k = 0; k < halfSize; ++k) {
        powerOut[k] = fftOut[k][0] * fftOut[k][0] + fftOut[k][1] * fftOut[k][1];
    }
}

vector<int> MaxFreqImprovedExtractor::getTopFreqIndices(const vector<double>& power) {
    int halfSize = power.size();
    
    // Create index array
    vector<int> indices(halfSize);
    iota(indices.begin(), indices.end(), 0);  // Fill with 0, 1, 2, ...
    
    // Partial sort to find top N frequencies (like GetMaxFreqs.cpp)
    partial_sort(indices.begin(), indices.begin() + min(numFreqs, halfSize), indices.end(),
                [&power](int i, int j) { return power[i] > power[j]; });
    
    // Return top indices, truncated to 255 like GetMaxFreqs.cpp
    vector<int> result;
    for (int i = 0; i < min(numFreqs, halfSize); i++) {
        // Truncate to max of 255 like GetMaxFreqs.cpp does
        int truncatedIdx = indices[i] > 255 ? 255 : indices[i];
        result.push_back(truncatedIdx);
    }
    
    return result;
}

string MaxFreqImprovedExtractor::extractFeatures(const vector<int16_t>& samples, int channels, 
                                                int frameSize, int hopSize, int sampleRate) {
    // Initialize FFTW for this frame size
    initializeFFTW(frameSize);
    
    // Prepare result
    stringstream ss;
    
    // Header information
    ss << "# MaxFreqImprovedExtractor features" << endl;
    ss << "# Channels: " << channels << endl;
    ss << "# Frame size: " << frameSize << endl;
    ss << "# Hop size: " << hopSize << endl;
    ss << "# Sample rate: " << sampleRate << endl;
    ss << "# Frequencies per frame: " << numFreqs << endl;
    ss << "# Down-sampling factor: " << downSamplingFactor << endl;
    
    // Calculate effective hop size considering down-sampling
    int effectiveHopSize = hopSize * downSamplingFactor;
    int effectiveFrameSize = frameSize * downSamplingFactor;
    
    // Process frames
    for (size_t i = 0; i + effectiveFrameSize <= samples.size(); i += effectiveHopSize) {
        // Extract and down-sample frame
        vector<double> frame = extractAndDownsampleFrame(samples, channels, i, frameSize);
        
        // Compute FFT
        vector<double> power;
        computeFFTW(frame, power);
        
        // Get top frequency indices
        vector<int> topIndices = getTopFreqIndices(power);
        
        // Output indices
        for (size_t j = 0; j < topIndices.size(); j++) {
            if (j > 0) ss << " ";
            ss << topIndices[j];
        }
        ss << endl;
    }
    
    return ss.str();
}

std::vector<std::vector<float>> MaxFreqImprovedExtractor::extractFeaturesBinary(
    const std::vector<int16_t>& samples, int channels, int frameSize, int hopSize, int sampleRate) {
    
    // Initialize FFTW for this frame size
    initializeFFTW(frameSize);
    
    std::vector<std::vector<float>> features;
    
    // Calculate effective hop size considering down-sampling
    int effectiveHopSize = hopSize * downSamplingFactor;
    int effectiveFrameSize = frameSize * downSamplingFactor;
    
    // Process frames
    for (size_t i = 0; i + effectiveFrameSize <= samples.size(); i += effectiveHopSize) {
        // Extract and down-sample frame
        vector<double> frame = extractAndDownsampleFrame(samples, channels, i, frameSize);
        
        // Compute FFT
        vector<double> power;
        computeFFTW(frame, power);
        
        // Get top frequency indices
        vector<int> topIndices = getTopFreqIndices(power);
        
        // Convert to float vector
        std::vector<float> indicesFloat(topIndices.begin(), topIndices.end());
        features.push_back(indicesFloat);
    }
    
    return features;
}
