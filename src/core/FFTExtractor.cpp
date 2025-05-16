#include "../../include/core/FFTExtractor.h"
#include <complex>
#include <sstream>
#include <cmath>
#include <algorithm>

using namespace std;

vector<double> FFTExtractor::computeSpectrum(const vector<int16_t>& frame) {
    int N = frame.size();
    vector<double> mag(N/2);
    for (int k = 0; k < N/2; ++k) {
        complex<double> sum(0,0);
        for (int n = 0; n < N; ++n) {
            double angle = -2 * M_PI * k * n / N;
            sum += complex<double>((double)frame[n] * cos(angle), (double)frame[n] * sin(angle));
        }
        mag[k] = abs(sum);
    }
    return mag;
}

string FFTExtractor::extractFeatures(const vector<int16_t>& samples, int channels,
                                        int frameSize, int hopSize) {
    stringstream ss;
    // Convert interleaved stereo to mono by averaging if needed
    vector<int16_t> mono;
    if (channels == 2) {
        mono.resize(samples.size()/2);
        for (size_t i = 0; i < mono.size(); ++i) {
            mono[i] = static_cast<int16_t>((static_cast<int>(samples[2*i]) + static_cast<int>(samples[2*i+1])) / 2);
        }
    } else {
        mono = samples;
    }
    // Frame processing
    for (size_t start = 0; start + frameSize <= mono.size(); start += hopSize) {
        vector<int16_t> frame(mono.begin() + start, mono.begin() + start + frameSize);
        auto mag = computeSpectrum(frame);
        // Example feature: take the index of top 3 frequencies
        vector<size_t> indices(mag.size());
        for (size_t i = 0; i < indices.size(); ++i) {
            indices[i] = i;
        }
        // Sort indices by magnitude
        partial_sort(indices.begin(), indices.begin() + 3, indices.end(),
            [&mag](size_t i1, size_t i2) { return mag[i1] > mag[i2]; });

        // Output top 3 indices
        for (int k = 0; k < 3 && k < (int)mag.size(); ++k) {
            ss << indices[k] << (k < 2 ? " " : "");
        }
        ss << "\n";
    }
    return ss.str();
}
