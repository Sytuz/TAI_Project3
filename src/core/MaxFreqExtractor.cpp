#include "../../include/core/MaxFreqExtractor.h"
#include <complex>
#include <cmath>
#include <sstream>
#include <algorithm>
#include <numeric>

using namespace std;

MaxFreqExtractor::MaxFreqExtractor(int numFrequencies) : numFreqs(numFrequencies) {
    if (numFreqs <= 0) numFreqs = 4;  // Default to 4 frequencies per frame
}

void MaxFreqExtractor::computeFFT(const vector<int16_t>& frame, vector<double>& magnitudes) {
    int N = frame.size();
    vector<complex<double>> fft(N);
    
    // Initialize FFT input
    for (int i = 0; i < N; i++) {
        fft[i] = complex<double>(frame[i], 0);
    }
    
    // Cooley-Tukey FFT implementation (bit-reversal + butterfly)
    // Bit reversal
    int j = 0;
    for (int i = 0; i < N-1; i++) {
        if (i < j) {
            swap(fft[i], fft[j]);
        }
        int k = N >> 1;
        while (k <= j) {
            j -= k;
            k >>= 1;
        }
        j += k;
    }
    
    // Butterfly computation
    for (int step = 1; step < N; step <<= 1) {
        double theta = -M_PI / step;  // Base angle
        complex<double> w_m(cos(theta), sin(theta));  // Principal root of unity
        
        for (int m = 0; m < step; m++) {
            complex<double> w(1, 0);  // Start with unity
            
            for (int i = m; i < N; i += 2*step) {
                complex<double> t = w * fft[i + step];
                complex<double> u = fft[i];
                fft[i] = u + t;
                fft[i + step] = u - t;
                w *= w_m;  // Rotate by the principal root
            }
        }
    }
    
    // Calculate magnitudes (only up to Nyquist frequency)
    magnitudes.resize(N/2);
    for (int i = 0; i < N/2; i++) {
        magnitudes[i] = abs(fft[i]);
    }
    
    // Normalize by N to adjust for window size
    for (auto& mag : magnitudes) {
        mag /= N;
    }
}

void MaxFreqExtractor::applyWindow(vector<int16_t>& frame) {
    // Hann window function
    const int size = frame.size();
    for (int i = 0; i < size; i++) {
        double window = 0.5 * (1.0 - cos(2.0 * M_PI * i / (size - 1)));
        frame[i] = static_cast<int16_t>(frame[i] * window);
    }
}

vector<int> MaxFreqExtractor::getTopFreqIndices(const vector<double>& magnitudes) {
    // Create index array
    vector<int> indices(magnitudes.size());
    iota(indices.begin(), indices.end(), 0);  // Fill with 0, 1, 2, ...
    
    // Skip DC component (0 Hz)
    indices[0] = -1;
    
    // Partial sort to find top N frequencies (much faster than full sort)
    partial_sort(indices.begin(), indices.begin() + numFreqs, indices.end(),
                [&magnitudes](int a, int b) {
                    if (a == -1) return false;
                    if (b == -1) return true;
                    return magnitudes[a] > magnitudes[b];
                });
    
    // Return top indices
    vector<int> result;
    for (int i = 0; i < numFreqs && i < static_cast<int>(magnitudes.size()); i++) {
        if (indices[i] > 0) {  // Skip any marked indices (-1)
            result.push_back(indices[i]);
        }
    }
    
    return result;
}

string MaxFreqExtractor::extractFeatures(const vector<int16_t>& samples, int channels, 
                                       int frameSize, int hopSize, int sampleRate) {
    // Convert to mono if needed
    vector<int16_t> monoSamples;
    if (channels == 2) {
        monoSamples.resize(samples.size() / 2);
        for (size_t i = 0, j = 0; i < samples.size(); i += 2, j++) {
            monoSamples[j] = (samples[i] + samples[i+1]) / 2;
        }
    } else {
        monoSamples = samples;
    }
    
    // Prepare result
    stringstream ss;
    
    // Header information
    ss << "# MaxFreqExtractor features" << endl;
    ss << "# Channels: " << channels << endl;
    ss << "# Frame size: " << frameSize << endl;
    ss << "# Hop size: " << hopSize << endl;
    ss << "# Sample rate: " << sampleRate << endl;
    ss << "# Frequencies per frame: " << numFreqs << endl;
    
    // Process frames
    for (size_t i = 0; i + frameSize <= monoSamples.size(); i += hopSize) {
        // Extract frame
        vector<int16_t> frame(monoSamples.begin() + i, monoSamples.begin() + i + frameSize);
        
        // Apply window function
        applyWindow(frame);
        
        // Compute FFT
        vector<double> magnitudes;
        computeFFT(frame, magnitudes);
        
        // Get top frequency indices
        vector<int> topIndices = getTopFreqIndices(magnitudes);
        
        // Convert to actual frequencies and output
        for (size_t j = 0; j < topIndices.size(); j++) {
            double freq = topIndices[j] * sampleRate / frameSize;
            if (j > 0) ss << " ";
            ss << topIndices[j];
        }
        ss << endl;
    }
    
    return ss.str();
}

std::vector<std::vector<float>> MaxFreqExtractor::extractFeaturesBinary(const std::vector<int16_t>& samples, int channels, int frameSize, int hopSize, int sampleRate) {
    // Convert to mono if needed
    std::vector<int16_t> monoSamples;
    if (channels == 2) {
        monoSamples.resize(samples.size() / 2);
        for (size_t i = 0, j = 0; i < samples.size(); i += 2, j++) {
            monoSamples[j] = (samples[i] + samples[i+1]) / 2;
        }
    } else {
        monoSamples = samples;
    }
    std::vector<std::vector<float>> features;
    for (size_t i = 0; i + frameSize <= monoSamples.size(); i += hopSize) {
        std::vector<int16_t> frame(monoSamples.begin() + i, monoSamples.begin() + i + frameSize);
        applyWindow(frame);
        std::vector<double> magnitudes;
        computeFFT(frame, magnitudes);
        std::vector<int> topIndices = getTopFreqIndices(magnitudes);
        std::vector<float> indicesFloat(topIndices.begin(), topIndices.end());
        features.push_back(indicesFloat);
    }
    return features;
}