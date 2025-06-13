#ifndef SpectralExtractor_H
#define SpectralExtractor_H

#include <vector>
#include <string>

using namespace std;

/**
 * @brief Extracts frequency features using FFT and multiple spectral bins
 */
class SpectralExtractor {
public:
    SpectralExtractor(int numBins = 32);
    ~SpectralExtractor() = default;
    
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
    int numBins;  // Number of frequency bins to use
    
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
    
    /**
     * @brief Convert full FFT spectrum to reduced bins
     * @param magnitudes Full magnitude spectrum
     * @return Reduced spectrum with 'numBins' bins
     */
    vector<double> getBinnedSpectrum(const vector<double>& magnitudes);
};

#endif