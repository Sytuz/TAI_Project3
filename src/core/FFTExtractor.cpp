#include "../../include/core/FFTExtractor.h"
#include <complex>
#include <sstream>
#include <cmath>

using namespace std;

vector<double> FFTExtractor::computeSpectrum(const vector<int16_t>& frame) {
    int N = frame.size();
    vector<double> mag(N/2);
    for (int k = 0; k < N/2; ++k) {
        complex<double> sum(0,0);
        for (int n = 0; n < N; ++n) {
            double angle = -2 * M_PI * k * n / N;
            sum += polar((double)frame[n], angle);
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
            mono[i] = (samples[2*i] / 2 + samples[2*i+1] / 2);
        }
    } else {
        mono = samples;
    }
    // Frame processing
    for (size_t start = 0; start + frameSize <= mono.size(); start += hopSize) {
        vector<int16_t> frame(mono.begin() + start, mono.begin() + start + frameSize);
        auto mag = computeSpectrum(frame);
        // Example feature: take the index of top 3 frequencies
        for (int k = 0; k < 3 && k < (int)mag.size(); ++k) {
            int maxIdx = 0;
            for (int i = 1; i < (int)mag.size(); ++i) {
                if (mag[i] > mag[maxIdx]) maxIdx = i;
            }
            ss << maxIdx << (k < 2 ? " " : ""); // space-separated indices
            mag[maxIdx] = 0; // remove for next max
        }
        ss << "\n";
    }
    return ss.str();
}
