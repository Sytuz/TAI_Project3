#include "../../include/core/SpectralExtractor.h"
#include <complex>
#include <cmath>
#include <sstream>
#include <algorithm>
#include <numeric>

using namespace std;

SpectralExtractor::SpectralExtractor(int bins) : numBins(bins) {
    if (numBins <= 0) numBins = 32;  // Default to 32 frequency bins
}

void SpectralExtractor::computeFFT(const vector<int16_t>& frame, vector<double>& magnitudes) {
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
    
    // Normalize by N
    for (auto& mag : magnitudes) {
        mag /= N;
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
    
    // Use logarithmic frequency bins (more resolution in lower frequencies)
    // Skip the DC component (i=0)
    
    double maxFreq = magnitudes.size();
    
    for (size_t i = 1; i < magnitudes.size(); i++) {
        // Calculate bin index using logarithmic scale
        // Map i (1...maxFreq) to bin (0...numBins-1)
        double logPos = log(i) / log(maxFreq);
        int binIdx = min(numBins - 1, static_cast<int>(logPos * numBins));
        
        // Add magnitude to bin (use max to prevent smaller values from being masked)
        binned[binIdx] = max(binned[binIdx], magnitudes[i]);
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
    for (size_t i = 0; i + frameSize <= monoSamples.size(); i += hopSize) {
        // Extract frame
        vector<int16_t> frame(monoSamples.begin() + i, monoSamples.begin() + i + frameSize);
        
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
            // Quantize and convert to integer to save space
            int binValue = static_cast<int>(bins[j] * 100);
            ss << binValue;
        }
        ss << endl;
    }
    
    return ss.str();
}