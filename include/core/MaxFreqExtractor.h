#ifndef MAXFREQEXTRACTOR_H
#define MAXFREQEXTRACTOR_H

#include <vector>
#include <string>

using namespace std;

/**
 * @brief Extracts multiple dominant frequencies from audio frames using FFT
 */
class MaxFreqExtractor {
public:
    MaxFreqExtractor(int numFrequencies = 4);
    ~MaxFreqExtractor() = default;
    
    /**
     * @brief Extract features from audio samples
     * @param samples Audio samples
     * @param channels Number of channels (1=mono, 2=stereo)
     * @param frameSize Size of each analysis frame
     * @param hopSize Number of samples to advance between frames
     * @param sampleRate Audio sample rate in Hz
     * @return Feature string representation
     */
    string extractFeatures(const vector<int16_t>& samples, int channels, 
                           int frameSize, int hopSize, int sampleRate = 44100);
    
    /**
     * @brief Extract features as binary vectors (one vector per frame)
     * @param samples Audio samples
     * @param channels Number of channels (1=mono, 2=stereo)
     * @param frameSize Size of each analysis frame
     * @param hopSize Number of samples to advance between frames
     * @param sampleRate Audio sample rate in Hz
     * @return Vector of feature vectors (float)
     */
    std::vector<std::vector<float>> extractFeaturesBinary(const std::vector<int16_t>& samples, int channels, int frameSize, int hopSize, int sampleRate = 44100);

private:
    int numFreqs;  // Number of frequencies to extract per frame

    /**
     * @brief Get top N frequency indices from magnitudes
     * @param magnitudes FFT magnitude spectrum
     * @return Vector of frequency indices
     */
    vector<int> getTopFreqIndices(const vector<double>& magnitudes);
    
    /**
     * @brief Compute FFT magnitude spectrum of frame
     * @param frame Input audio frame
     * @param magnitudes Output magnitude spectrum
     */
    void computeFFT(const vector<int16_t>& frame, vector<double>& magnitudes);
    
    /**
     * @brief Apply window function to frame before FFT
     * @param frame Audio frame data
     */
    void applyWindow(vector<int16_t>& frame);
};

#endif