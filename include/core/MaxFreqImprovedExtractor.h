#ifndef MAXFREQIMPROVEDEXTRACTOR_H
#define MAXFREQIMPROVEDEXTRACTOR_H

#include <vector>
#include <string>
#include <fftw3.h>

using namespace std;

/**
 * @brief Improved MaxFreq extractor based on GetMaxFreqs.cpp optimizations
 * 
 * Key improvements over MaxFreqExtractor:
 * - Uses FFTW3 library for optimized FFT computation
 * - Implements down-sampling for computational efficiency
 * - Better frequency index handling with truncation
 * - More efficient frame processing
 */
class MaxFreqImprovedExtractor {
public:
    MaxFreqImprovedExtractor(int numFrequencies = 4, int downSamplingFactor = 4);
    ~MaxFreqImprovedExtractor();
    
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
    int downSamplingFactor;  // Down-sampling factor (like DS in GetMaxFreqs.cpp)
    fftw_plan fftPlan;
    fftw_complex* fftIn;
    fftw_complex* fftOut;
    double* powerSpectrum;
    int currentFrameSize;
    
    /**
     * @brief Initialize FFTW3 plan and buffers
     * @param frameSize Size of FFT window
     */
    void initializeFFTW(int frameSize);
    
    /**
     * @brief Clean up FFTW3 resources
     */
    void cleanupFFTW();
    
    /**
     * @brief Convert to mono and apply down-sampling like GetMaxFreqs.cpp
     * @param samples Input stereo/mono samples
     * @param channels Number of input channels
     * @param frameStart Starting index in samples
     * @param frameSize Size of the frame to extract
     * @return Down-sampled mono frame
     */
    vector<double> extractAndDownsampleFrame(const vector<int16_t>& samples, int channels, 
                                            size_t frameStart, int frameSize);
    
    /**
     * @brief Compute FFT using FFTW3 and get power spectrum
     * @param frame Input audio frame (down-sampled)
     * @param powerOut Output power spectrum
     */
    void computeFFTW(const vector<double>& frame, vector<double>& powerOut);
    
    /**
     * @brief Get top N frequency indices from power spectrum
     * @param power Power spectrum
     * @return Vector of frequency indices (truncated to 255 like GetMaxFreqs.cpp)
     */
    vector<int> getTopFreqIndices(const vector<double>& power);
};

#endif
