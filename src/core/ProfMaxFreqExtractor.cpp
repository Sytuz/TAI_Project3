#include "../../include/core/ProfMaxFreqExtractor.h"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <cstring>
#include <fftw3.h>

using namespace std;

ProfMaxFreqExtractor::ProfMaxFreqExtractor(int numFrequencies, int windowSize, int shift, int downSampling) 
    : ws(windowSize), sh(shift), ds(downSampling), nf(numFrequencies) {
    if (nf <= 0) nf = 4;
    if (ws <= 0) ws = 1024;
    if (sh <= 0) sh = 256;
    if (ds <= 0) ds = 4;
}

string ProfMaxFreqExtractor::extractFeatures(const vector<int16_t>& samples, int channels, 
                                           int frameSize, int hopSize, int sampleRate) {
    // The professor's algorithm outputs binary data, so we don't provide text features
    // This method returns an empty string to indicate binary-only output
    return "";
}

std::vector<std::vector<float>> ProfMaxFreqExtractor::extractFeaturesBinary(const std::vector<int16_t>& samples, int channels, int frameSize, int hopSize, int sampleRate) {
    // Validate input parameters (professor's algorithm requirements)
    if (channels != 2) {
        cerr << "Error: profmaxfreq currently supports only 2 channels (stereo)" << endl;
        return {};
    }
    
    if (sampleRate != 44100) {
        cerr << "Error: profmaxfreq currently supports only 44100 Hz sample rate" << endl;
        return {};
    }
    
    // Convert to short array format expected by the original algorithm
    vector<short> shortSamples(samples.begin(), samples.end());
    long numFrames = shortSamples.size() / 2; // Stereo samples
    
    // Process using the professor's algorithm
    auto frequencyIndices = processAudioFrames(shortSamples.data(), numFrames);
    
    // Convert to float format expected by the system
    std::vector<std::vector<float>> features;
    for (const auto& frameIndices : frequencyIndices) {
        std::vector<float> frameFeatures;
        for (unsigned char idx : frameIndices) {
            frameFeatures.push_back(static_cast<float>(idx));
        }
        features.push_back(frameFeatures);
    }
    
    return features;
}

std::vector<std::vector<unsigned char>> ProfMaxFreqExtractor::processAudioFrames(const short* samples, long numFrames) {
    std::vector<std::vector<unsigned char>> allFeatures;
    
    // Validate input
    if (numFrames <= 0 || samples == nullptr) {
        cerr << "Error: Invalid input data for profmaxfreq" << endl;
        return allFeatures;
    }
    
    // FFTW setup - use dynamic allocation instead of VLA
    fftw_complex* in = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * ws);
    fftw_complex* out = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * ws);
    if (!in || !out) {
        cerr << "Error: Failed to allocate FFTW memory" << endl;
        if (in) fftw_free(in);
        if (out) fftw_free(out);
        return allFeatures;
    }
    
    fftw_plan plan = fftw_plan_dft_1d(ws, in, out, FFTW_FORWARD, FFTW_ESTIMATE);
    if (!plan) {
        cerr << "Error: Failed to create FFTW plan" << endl;
        fftw_free(in);
        fftw_free(out);
        return allFeatures;
    }
    
    vector<double> power(ws/2);
    vector<unsigned> maxPowerIdx(ws/2);
    
    // Calculate maximum number of frames we can process safely
    long maxFrames = (numFrames - ws * ds) / (sh * ds);
    if (maxFrames < 0) {
        cerr << "Error: Not enough samples for profmaxfreq processing" << endl;
        fftw_destroy_plan(plan);
        fftw_free(in);
        fftw_free(out);
        return allFeatures;
    }
    
    // Process each analysis window
    for(int n = 0 ; n <= maxFrames ; ++n) {
        // Clear input buffer
        memset(in, 0, sizeof(fftw_complex) * ws);
        
        // Prepare input data (convert to mono and down-sample)
        for(int k = 0 ; k < ws ; ++k) {
            long baseIdx = (n * (sh * ds) + k * ds);
            
            // Check bounds before accessing samples
            if (baseIdx >= 0 && (baseIdx + ds - 1) < numFrames) {
                in[k][0] = (int)samples[baseIdx << 1] + samples[(baseIdx << 1) + 1];
                
                for(int l = 1 ; l < ds ; ++l) {
                    long sampleIdx = baseIdx + l;
                    if (sampleIdx < numFrames) {
                        in[k][0] += (int)samples[sampleIdx << 1] + samples[(sampleIdx << 1) + 1];
                    }
                }
            }
            in[k][1] = 0; // Imaginary part is zero
        }
        
        // Execute FFT
        fftw_execute(plan);
        
        // Calculate power spectrum
        for(int k = 0 ; k < ws/2 ; ++k) {
            power[k] = out[k][0] * out[k][0] + out[k][1] * out[k][1];
        }
        
        // Initialize indices
        for(int k = 0 ; k < ws/2 ; ++k) {
            maxPowerIdx[k] = k;
        }
        
        // Partial sort to get top nf frequencies
        partial_sort(maxPowerIdx.begin(), maxPowerIdx.begin() + nf, maxPowerIdx.begin() + ws/2,
                    [&power](int i, int j) { return power[i] > power[j]; });
        
        // Store the top frequency indices for this frame
        std::vector<unsigned char> frameFeatures;
        for(int i = 0 ; i < nf ; ++i) {
            // Truncate to max of 255 (to fit in byte)
            frameFeatures.push_back(maxPowerIdx[i] > 255 ? 255 : static_cast<unsigned char>(maxPowerIdx[i]));
        }
        allFeatures.push_back(frameFeatures);
    }
    
    // Clean up FFTW
    fftw_destroy_plan(plan);
    fftw_free(in);
    fftw_free(out);
    
    return allFeatures;
}
