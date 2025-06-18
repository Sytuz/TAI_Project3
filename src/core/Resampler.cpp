#include "../../include/core/Resampler.h"
#include <iostream>
#include <cmath>

using namespace std;

vector<int32_t> Resampler::resample(const vector<int32_t>& inputSamples, 
                                   int inputSampleRate, 
                                   int outputSampleRate) {
    if (inputSampleRate == outputSampleRate) {
        // No resampling needed
        return inputSamples;
    }

    if (inputSamples.empty()) {
        cerr << "Warning: Empty input samples for resampling" << endl;
        return {};
    }

    // Calculate resampling ratio
    double ratio = static_cast<double>(inputSampleRate) / outputSampleRate;
    
    // Calculate output size
    size_t outputSize = static_cast<size_t>(inputSamples.size() / ratio);
    
    vector<int32_t> outputSamples;
    outputSamples.reserve(outputSize);
    
    cout << "Resampling from " << inputSampleRate << " Hz to " << outputSampleRate 
         << " Hz (ratio: " << ratio << ")" << endl;
    cout << "Input samples: " << inputSamples.size() << ", Output samples: " << outputSize << endl;
    
    // Linear interpolation resampling
    for (size_t i = 0; i < outputSize; i++) {
        double sourceIndex = i * ratio;
        size_t lowerIndex = static_cast<size_t>(floor(sourceIndex));
        size_t upperIndex = lowerIndex + 1;
        
        if (upperIndex >= inputSamples.size()) {
            upperIndex = inputSamples.size() - 1;
            lowerIndex = upperIndex;
        }
        
        if (lowerIndex == upperIndex) {
            // No interpolation needed
            outputSamples.push_back(inputSamples[lowerIndex]);
        } else {
            // Linear interpolation
            double fraction = sourceIndex - lowerIndex;
            int32_t interpolatedSample = interpolate(inputSamples[lowerIndex], 
                                                   inputSamples[upperIndex], 
                                                   fraction);
            outputSamples.push_back(interpolatedSample);
        }
    }
    
    return outputSamples;
}

int32_t Resampler::interpolate(int32_t sample1, int32_t sample2, double fraction) {
    // Linear interpolation: result = sample1 + fraction * (sample2 - sample1)
    double result = sample1 + fraction * (sample2 - sample1);
    
    // Clamp to int32_t range to prevent overflow
    if (result > INT32_MAX) return INT32_MAX;
    if (result < INT32_MIN) return INT32_MIN;
    
    return static_cast<int32_t>(result);
}