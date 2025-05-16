#include "../../include/utils/NoiseInjector.h"
#include <cmath>
#include <algorithm>

using namespace std;

NoiseInjector::NoiseInjector(double snr) : snrDb(snr), rng(random_device{}()), dist(0.0, 1.0) {}

void NoiseInjector::setSNR(double snr) {
    snrDb = snr;
}

void NoiseInjector::addNoise(vector<int16_t>& samples) {
    // Compute signal power
    double power = 0.0;
    for (auto s : samples) {
        power += s*s;
    }
    power /= samples.size();
    double signalPower = power;
    double noisePower = signalPower / pow(10.0, snrDb/10.0);
    double noiseStd = sqrt(noisePower);

    for (size_t i = 0; i < samples.size(); ++i) {
        double noise = dist(rng) * noiseStd;
        int sample = samples[i] + static_cast<int>(noise);
        sample = clamp(sample, -32768, 32767);
        samples[i] = static_cast<int16_t>(sample);
    }
}
