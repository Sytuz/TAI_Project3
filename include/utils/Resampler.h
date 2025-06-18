#ifndef RESAMPLER_H
#define RESAMPLER_H

#include <vector>
#include <cstdint>

using namespace std;

/**
 * @brief Simple audio resampler for converting sample rates
 */
class Resampler {
public:
    /**
     * @brief Resample audio data from one sample rate to another
     * @param inputSamples Input audio samples
     * @param inputSampleRate Original sample rate
     * @param outputSampleRate Target sample rate
     * @return Resampled audio samples
     */
    static vector<int32_t> resample(const vector<int32_t>& inputSamples, 
                                   int inputSampleRate, 
                                   int outputSampleRate);

private:
    /**
     * @brief Linear interpolation between two samples
     * @param sample1 First sample
     * @param sample2 Second sample  
     * @param fraction Interpolation fraction (0.0 to 1.0)
     * @return Interpolated sample
     */
    static int32_t interpolate(int32_t sample1, int32_t sample2, double fraction);
};

#endif // RESAMPLER_H
