#include "../../include/utils/NoiseInjector.h"
#include <cmath>
#include <algorithm>
#include <iostream>

using namespace std;

NoiseInjector::NoiseInjector(double snr) : snrDb(snr), rng(random_device{}()), dist(0.0, 1.0) {
    cout << "Noise injector initialized with SNR = " << snrDb << " dB" << endl;
}

void NoiseInjector::setSNR(double snr) {
    snrDb = snr;
    cout << "SNR set to " << snrDb << " dB" << endl;
}

void NoiseInjector::addNoise(vector<int16_t>& samples) {
    if (samples.empty()) {
        cerr << "Warning: Empty sample buffer for noise injection" << endl;
        return;
    }

    // Compute signal power
    double signalPower = 0.0;
    for (auto s : samples) {
        signalPower += s * s;
    }
    signalPower /= samples.size();

    // Avoid division by zero
    if (signalPower < 1.0) {
        cerr << "Warning: Very low signal power detected, using minimal power" << endl;
        signalPower = 1.0;
    }

    // Calculate noise power from SNR
    double noisePower = signalPower / pow(10.0, snrDb/10.0);
    double noiseStd = sqrt(noisePower);

    cout << "Adding noise: signal power = " << signalPower
            << ", noise power = " << noisePower
            << " (SNR = " << snrDb << " dB)" << endl;

    // Create normal distribution with proper standard deviation
    normal_distribution<double> noiseDistribution(0.0, noiseStd);

    // Add noise to each sample
    for (size_t i = 0; i < samples.size(); ++i) {
        double noise = noiseDistribution(rng);
        int sample = samples[i] + static_cast<int>(noise);
        // Clamp to valid 16-bit range
        sample = clamp(sample, -32768, 32767);
        samples[i] = static_cast<int16_t>(sample);
    }

    cout << "Noise added to " << samples.size() << " samples" << endl;
}
