#include "../../include/core/SpectralExtractor.h"
#include <complex>
#include <cmath>
#include <sstream>
#include <algorithm>
#include <numeric>
#include <cstring>

using namespace std;

SpectralExtractor::SpectralExtractor(int bins) : numBins(bins) {
    if (numBins <= 0) numBins = 32;  // Default to 32 frequency bins
}

void SpectralExtractor::computeFFT(const vector<int16_t>& frame, vector<double>& magnitudes) {
    int N = frame.size();
    vector<complex<double>> fft(N);
    
    // Initialize FFT input
    for (int i = 0; i < N; i++) {
        fft[i] = complex<double>(frame[i] / 32768.0, 0); // Normalize to [-1, 1]
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
    // Use more frequency bins for better resolution
    int usefulBins = min(N/2, N/4 + numBins * 8); // Adaptive number of bins
    magnitudes.resize(usefulBins);
    for (int i = 0; i < usefulBins; i++) {
        magnitudes[i] = abs(fft[i]);
    }
    
    // Apply logarithmic scaling for better perceptual representation
    for (auto& mag : magnitudes) {
        mag = log1p(mag); // log(1 + x) for numerical stability
    }
}

void SpectralExtractor::applyWindow(vector<int16_t>& frame) {
    // Hann window function
    const int size = frame.size();
    for (int i = 0; i < size; i++) {
        double window = 0.5 * (1.0 - cos(2.0 * M_PI * i / (size - 1)));
        frame[i] = static_cast<int16_t>(frame[i] * window);
    }
}

vector<double> SpectralExtractor::getBinnedSpectrum(const vector<double>& magnitudes) {
    vector<double> binned(numBins, 0.0);
    
    // Use linear frequency bins for better distribution and faster computation
    // Skip the DC component (i=0) and use only meaningful frequency range
    int startBin = 1; // Skip DC component
    int endBin = min(static_cast<int>(magnitudes.size()), static_cast<int>(magnitudes.size() * 0.8)); // Use up to 80% of spectrum
    
    double binSize = static_cast<double>(endBin - startBin) / numBins;
    
    for (int bin = 0; bin < numBins; bin++) {
        int startIdx = startBin + static_cast<int>(bin * binSize);
        int endIdx = startBin + static_cast<int>((bin + 1) * binSize);
        endIdx = min(endIdx, endBin);
        
        // Use RMS (energy)
        double energy = 0.0;
        int count = 0;
        for (int i = startIdx; i < endIdx; i++) {
            energy += magnitudes[i] * magnitudes[i];
            count++;
        }
        
        if (count > 0) {
            binned[bin] = sqrt(energy / count); // RMS value
        }
    }
    
    // Normalize to prevent overflow and improve NCD performance
    double maxVal = *max_element(binned.begin(), binned.end());
    if (maxVal > 0) {
        for (auto& val : binned) {
            val /= maxVal;
        }
    }
    
    return binned;
}

string SpectralExtractor::extractFeatures(const vector<int16_t>& samples, int channels, 
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
    ss << "# SpectralExtractor features" << endl;
    ss << "# Channels: " << channels << endl;
    ss << "# Frame size: " << frameSize << endl;
    ss << "# Hop size: " << hopSize << endl;
    ss << "# Sample rate: " << sampleRate << endl;
    ss << "# Frequency bins: " << numBins << endl;
    
    // Process frames
    vector<int16_t> frame(frameSize); // Pre-allocate frame buffer
    for (size_t i = 0; i + frameSize <= monoSamples.size(); i += hopSize) {
        // Copy frame data (more efficient than creating new vector)
        memcpy(frame.data(), &monoSamples[i], frameSize * sizeof(int16_t));
        
        // Apply window function
        applyWindow(frame);
        
        // Compute FFT
        vector<double> magnitudes;
        computeFFT(frame, magnitudes);
        
        // Get binned spectrum
        vector<double> bins = getBinnedSpectrum(magnitudes);
        
        // Output binned spectrum
        for (size_t j = 0; j < bins.size(); j++) {
            if (j > 0) ss << " ";
            // Use fixed-point representation
            int binValue = static_cast<int>(bins[j] * 10000);
            ss << binValue;
        }
        ss << endl;
    }
    
    return ss.str();
}

std::vector<std::vector<float>> SpectralExtractor::extractFeaturesBinary(const std::vector<int16_t>& samples, int channels, int frameSize, int hopSize, int sampleRate) {
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
    std::vector<int16_t> frame(frameSize);
    for (size_t i = 0; i + frameSize <= monoSamples.size(); i += hopSize) {
        memcpy(frame.data(), &monoSamples[i], frameSize * sizeof(int16_t));
        applyWindow(frame);
        std::vector<double> magnitudes;
        computeFFT(frame, magnitudes);
        std::vector<double> bins = getBinnedSpectrum(magnitudes);
        std::vector<float> binsFloat(bins.begin(), bins.end());
        features.push_back(binsFloat);
    }
    return features;
}