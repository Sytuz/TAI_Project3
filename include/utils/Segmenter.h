#ifndef SEGMENTER_H
#define SEGMENTER_H

#include <vector>
#include <cstdint>

using namespace std;

/**
 * @brief Segmenter splits audio samples into overlapping frames.
 */
class Segmenter {
public:
    Segmenter() = default;
    ~Segmenter() = default;

    /**
     * @brief Split samples into frames (mono or interleaved stereo).
     * @param samples Input samples.
     * @param channels Number of channels (1 or 2).
     * @param frameSize Samples per frame.
     * @param hopSize Step between frames.
     * @return Vector of frames (each frame is vector<int16_t> of size frameSize*channels).
     */
    vector<vector<int16_t>> segment(const vector<int16_t>& samples, int channels, int frameSize, int hopSize);
};

#endif // SEGMENTER_H
