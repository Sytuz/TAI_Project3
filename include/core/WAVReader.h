#ifndef WAVREADER_H
#define WAVREADER_H

#include <string>
#include <vector>

using namespace std;

/**
 * @brief WAVReader reads WAV files (PCM, 16-bit) and provides samples.
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
         * @brief Get audio samples.
         * Returns interleaved samples (if stereo) or mono samples.
         */
        const vector<int16_t>& samples() const { return data; }

        /**
         * @brief Get sample rate of WAV file.
         */
        int sampleRate() const { return samplerate; }

        /**
         * @brief Check if file is stereo.
         */
        bool isStereo() const { return channels == 2; }

    private:
        int samplerate = 0;
        int channels = 0;
        int bitsPerSample = 0;
        vector<int16_t> data;

        bool parseHeader(ifstream& in);
};

#endif // WAVREADER_H
