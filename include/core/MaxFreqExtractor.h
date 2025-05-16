#ifndef MAXFREQEXTRACTOR_H
#define MAXFREQEXTRACTOR_H

#include <vector>
#include <cstdint>
#include <string>

using namespace std;

/**
 * @brief MaxFreqExtractor finds the frequency with maximum magnitude in each frame.
 * This mimics a provided method GetMaxFreqs.
 */
class MaxFreqExtractor {
    public:
        MaxFreqExtractor() = default;
        ~MaxFreqExtractor() = default;

        /**
         * @brief For a given frame of samples, compute the index of max-frequency component.
         * Uses naive DFT for demonstration.
         */
        int getMaxFreqIndex(const vector<int16_t>& frame);

        /**
         * @brief Extract features as max-frequency bin indices for each frame.
         */
        string extractFeatures(const vector<int16_t>& samples, int channels,
                                    int frameSize, int hopSize);
};

#endif // MAXFREQEXTRACTOR_H
