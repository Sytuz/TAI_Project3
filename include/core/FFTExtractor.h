#ifndef FFTEXTRACTOR_H
#define FFTEXTRACTOR_H

#include <vector>
#include <cstdint>
#include <string>

using namespace std;

/**
 * @brief FFTExtractor computes a simple FFT-based feature vector from audio samples.
 * This uses a naive DFT for simplicity (not efficient for large frames).
 */
class FFTExtractor {
    public:
        FFTExtractor() = default;
        ~FFTExtractor() = default;

        /**
         * @brief Compute magnitude spectrum of a frame (length N, power of two).
         * @param frame Input time-domain samples.
         * @return Magnitudes of frequency bins (size N/2).
         */
        vector<double> computeSpectrum(const vector<int16_t>& frame);

        /**
         * @brief Extract features for entire sample buffer: splits into frames and collects top frequencies.
         * @param samples Audio samples (mono or interleaved stereo).
         * @param frameSize Number of samples per frame.
         * @param hopSize   Step between successive frames.
         * @return Feature string (e.g., space-separated top freq bins for each frame).
         */
        string extractFeatures(const vector<int16_t>& samples, int channels,
                                    int frameSize, int hopSize);
};

#endif // FFTEXTRACTOR_H
