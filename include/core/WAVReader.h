#ifndef WAVREADER_H
#define WAVREADER_H

#include <string>
#include <vector>

using namespace std;

/**
 * @brief WAVReader reads WAV files (PCM, 8/16/24/32-bit) and provides samples.
 */
class WAVReader {
    public:
        WAVReader() = default;
        ~WAVReader() = default;

        /**
         * @brief Load WAV file from disk.
         * @param filename Path to WAV file.
         * @return true if loaded successfully.
         */
        bool load(const string& filename);

        /**
         * @brief Get the loaded samples as 32-bit integers.
         * @return Vector of samples.
         */
        const vector<int32_t>& samples() const { return data; }

        /**
         * @brief Check if the audio is stereo.
         * @return true if stereo, false if mono.
         */
        bool isStereo() const { return channels == 2; }

        /**
         * @brief Get the sample rate.
         * @return Sample rate in Hz.
         */
        int getSampleRate() const { return samplerate; }

        /**
         * @brief Get the bits per sample.
         * @return Bits per sample.
         */
        int getBitsPerSample() const { return bitsPerSample; }

        /**
         * @brief Get the number of channels.
         * @return Number of channels.
         */
        int getChannels() const { return channels; }

    private:
        int samplerate = 0;
        int channels = 0;
        int bitsPerSample = 0;
        vector<int32_t> data;  // Using 32-bit to store samples of any bit depth

        bool parseHeader(ifstream& in);
        void read8BitSamples(ifstream& in, int32_t dataSize);
        void read16BitSamples(ifstream& in, int32_t dataSize);
        void read24BitSamples(ifstream& in, int32_t dataSize);
        void read32BitSamples(ifstream& in, int32_t dataSize);
};

#endif // WAVREADER_H