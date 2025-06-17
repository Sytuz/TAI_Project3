#ifndef PROFMAXFREQEXTRACTOR_H
#define PROFMAXFREQEXTRACTOR_H

#include <vector>
#include <string>

using namespace std;

/**
 * @brief Professor's original maximum frequency extractor implementation
 * This class implements the exact algorithm from the original GetMaxFreqs.cpp
 * for compatibility and comparison purposes.
 */
class ProfMaxFreqExtractor {
public:
    ProfMaxFreqExtractor(int numFrequencies = 4, int windowSize = 1024, int shift = 256, int downSampling = 4);
    ~ProfMaxFreqExtractor() = default;
    
    /**
     * @brief Extract features from audio samples using professor's algorithm
     * @param samples Audio samples (int16_t format)
     * @param channels Number of channels (must be 2 for stereo)
     * @param frameSize Size of each analysis frame (not used, uses internal windowSize)
     * @param hopSize Number of samples to advance between frames (not used, uses internal shift)
     * @param sampleRate Audio sample rate in Hz (must be 44100)
     * @return Feature string representation (empty for profmaxfreq - uses binary output)
     */
    string extractFeatures(const vector<int16_t>& samples, int channels, 
                           int frameSize, int hopSize, int sampleRate = 44100);
    
    /**
     * @brief Extract features as binary vectors using professor's algorithm
     * @param samples Audio samples (int16_t format)
     * @param channels Number of channels (must be 2 for stereo)
     * @param frameSize Size of each analysis frame (not used, uses internal windowSize)
     * @param hopSize Number of samples to advance between frames (not used, uses internal shift)
     * @param sampleRate Audio sample rate in Hz (must be 44100)
     * @return Vector of feature vectors (each vector contains numFreqs bytes)
     */
    std::vector<std::vector<float>> extractFeaturesBinary(const std::vector<int16_t>& samples, int channels, int frameSize, int hopSize, int sampleRate = 44100);

    // Getters for configuration
    int getWindowSize() const { return ws; }
    int getShift() const { return sh; }
    int getDownSampling() const { return ds; }
    int getNumFreqs() const { return nf; }

private:
    int ws;  // Window size (default 1024)
    int sh;  // Shift/hop size (default 256)
    int ds;  // Down-sampling factor (default 4)
    int nf;  // Number of significant frequencies (default 4)
    
    /**
     * @brief Process audio samples and extract frequency features
     * This implements the core algorithm from the original GetMaxFreqs.cpp
     * @param samples Audio samples array
     * @param numFrames Total number of frames in audio
     * @return Vector of frequency indices for each analysis window
     */
    std::vector<std::vector<unsigned char>> processAudioFrames(const short* samples, long numFrames);
};

#endif
