#include "../../include/core/WAVReader.h"
#include <fstream>
#include <iostream>
#include <string.h>

using namespace std;

bool WAVReader::load(const string& filename) {
    ifstream in(filename, ios::binary);
    if (!in) {
        cerr << "Failed to open WAV file: " << filename << endl;
        return false;
    }

    // Reset state
    samplerate = 0;
    channels = 0;
    bitsPerSample = 0;
    data.clear();

    // Parse WAV header (PCM 16-bit little-endian, stereo or mono)
    if (!parseHeader(in)) {
        cerr << "Failed to parse WAV header: " << filename << endl;
        return false;
    }

    cout << "Loaded WAV file: " << filename << endl;
    cout << "  Sample rate: " << samplerate << " Hz" << endl;
    cout << "  Channels: " << channels << endl;
    cout << "  Bits per sample: " << bitsPerSample << endl;
    cout << "  Total samples: " << data.size() << endl;

    return true;
}

bool WAVReader::parseHeader(ifstream& in) {
    // RIFF header chunk
    char riff[4], wave[4], fmt[4], dataId[4];

    if (!in.read(riff, 4)) return false;
    if (strncmp(riff, "RIFF", 4) != 0) {
        cerr << "Not a RIFF file" << endl;
        return false;
    }

    // File size (minus 8 bytes)
    int32_t chunkSize;
    if (!in.read(reinterpret_cast<char*>(&chunkSize), 4)) return false;

    if (!in.read(wave, 4)) return false;
    if (strncmp(wave, "WAVE", 4) != 0) {
        cerr << "Not a WAVE file" << endl;
        return false;
    }

    // Format sub-chunk
    if (!in.read(fmt, 4)) return false;
    if (strncmp(fmt, "fmt ", 4) != 0) {
        cerr << "Format chunk not found" << endl;
        return false;
    }

    // Sub-chunk size
    int32_t subChunk1Size = 0;
    if (!in.read(reinterpret_cast<char*>(&subChunk1Size), 4)) return false;

    // Audio format (1 = PCM)
    int16_t audioFormat = 0;
    if (!in.read(reinterpret_cast<char*>(&audioFormat), 2)) return false;
    if (audioFormat != 1) {
        cerr << "Not a PCM WAV file (compression not supported)" << endl;
        return false;
    }

    // Number of channels
    if (!in.read(reinterpret_cast<char*>(&channels), 2)) return false;
    if (channels != 1 && channels != 2) {
        cerr << "Only mono or stereo WAV files are supported" << endl;
        return false;
    }

    // Sample rate
    if (!in.read(reinterpret_cast<char*>(&samplerate), 4)) return false;

    // Byte rate (sampleRate * channels * bitsPerSample/8)
    int32_t byteRate;
    if (!in.read(reinterpret_cast<char*>(&byteRate), 4)) return false;

    // Block align (channels * bitsPerSample/8)
    int16_t blockAlign;
    if (!in.read(reinterpret_cast<char*>(&blockAlign), 2)) return false;

    // Bits per sample
    if (!in.read(reinterpret_cast<char*>(&bitsPerSample), 2)) return false;
    if (bitsPerSample != 16) {
        cerr << "Only 16-bit WAV files are supported" << endl;
        return false;
    }

    // Skip possible extra params if subChunk1Size > 16
    if (subChunk1Size > 16) {
        in.seekg(subChunk1Size - 16, ios::cur);
    }

    // Find data chunk (may have other chunks before data)
    bool foundData = false;
    while (!foundData && in) {
        if (!in.read(dataId, 4)) return false;

        int32_t dataSize = 0;
        if (!in.read(reinterpret_cast<char*>(&dataSize), 4)) return false;

        if (strncmp(dataId, "data", 4) == 0) {
            // Data chunk found
            foundData = true;

            // Calculate number of samples and read them
            size_t numSamples = dataSize / (bitsPerSample/8);
            data.resize(numSamples);

            if (!in.read(reinterpret_cast<char*>(data.data()), dataSize)) {
                cerr << "Error reading WAV data" << endl;
                return false;
            }
        } else {
            // Skip unknown chunk
            in.seekg(dataSize, ios::cur);
        }
    }

    return foundData;
}
