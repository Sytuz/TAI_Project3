#ifndef NOISEINJECTOR_H
#define NOISEINJECTOR_H

#include <vector>
#include <random>

using namespace std;

/**
 * @brief NoiseInjector adds Gaussian noise to audio samples.
 */
class NoiseInjector {
public:
    NoiseInjector(double snrDb = 60.0);
    ~NoiseInjector() = default;

    /**
     * @brief Add noise to samples in-place.
     * @param samples Audio samples (int16 PCM).
     */
    void addNoise(vector<int16_t>& samples);

    /**
     * @brief Set desired signal-to-noise ratio (dB).
     */
    void setSNR(double snrDb);

private:
    double snrDb;
    mt19937 rng;
    normal_distribution<double> dist;
};

#endif // NOISEINJECTOR_H
