#include "../../include/core/MaxFreqExtractor.h"
#include <complex>
#include <cmath>
#include <sstream>

using namespace std;

int MaxFreqExtractor::getMaxFreqIndex(const vector<int16_t>& frame) {
    int N = frame.size();
    int maxIdx = 0;
    double maxMag = 0;
    // Naive DFT
    for (int k = 0; k < N/2; ++k) {
        complex<double> sum(0,0);
        for (int n = 0; n < N; ++n) {
            double angle = -2 * M_PI * k * n / N;
            sum += complex<double>((double)frame[n] * cos(angle), (double)frame[n] * sin(angle));
        }
        double mag = abs(sum);
        if (mag > maxMag) {
            maxMag = mag;
            maxIdx = k;
        }
    }
    return maxIdx;
}

string MaxFreqExtractor::extractFeatures(const vector<int16_t>& samples, int channels, int frameSize, int hopSize) {
    stringstream ss;
    // Convert to mono if needed
    vector<int16_t> mono;
    if (channels == 2) {
        mono.resize(samples.size()/2);
        for (size_t i = 0; i < mono.size(); ++i)
        mono[i] = static_cast<int16_t>((static_cast<int>(samples[2*i]) + static_cast<int>(samples[2*i+1])) / 2);
    } else {
        mono = samples;
    }
    for (size_t start = 0; start + frameSize <= mono.size(); start += hopSize) {
        vector<int16_t> frame(mono.begin() + start, mono.begin() + start + frameSize);
        int maxIdx = getMaxFreqIndex(frame);
        ss << maxIdx << "\n";
    }
    return ss.str();
}
