#include "../../include/utils/Segmenter.h"

using namespace std;

vector<vector<int16_t>> Segmenter::segment(const vector<int16_t>& samples, int channels, int frameSize, int hopSize) {
    vector<vector<int16_t>> frames;
    // If stereo, just segment interleaved; downstream code averages channels if needed.
    for (size_t start = 0; start + frameSize*channels <= samples.size(); start += hopSize*channels) {
        vector<int16_t> frame(samples.begin() + start, samples.begin() + start + frameSize*channels);
        frames.push_back(frame);
    }
    return frames;
}
