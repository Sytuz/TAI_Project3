#include "../../include/core/WAVReader.h"
#include <fstream>
#include <iostream>
#include <string.h>
#include <cstdint>

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

    // Parse WAV header (PCM 16-bit or 24-bit, little-endian, stereo or mono)
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
    char riff[4];
    if (!in.read(riff, 4)) return false;
    if (strncmp(riff, "RIFF", 4) != 0) {
        cerr << "Not a RIFF file" << endl;
        return false;
    }

    // File size (minus 8 bytes)
    int32_t chunkSize;
    if (!in.read(reinterpret_cast<char*>(&chunkSize), 4)) return false;

    // Check for WAVE format
    char wave[4];
    if (!in.read(wave, 4)) return false;
    if (strncmp(wave, "WAVE", 4) != 0) {
        cerr << "Not a WAVE file" << endl;
        return false;
    }

    // Search for fmt and data chunks - they can be in any order
    bool foundFmt = false;
    bool foundData = false;
    
    while (in && (!foundFmt || !foundData)) {
        char chunkId[4];
        int32_t chunkSize = 0;

        // Read chunk ID
        if (!in.read(chunkId, 4)) {
            if (in.eof() && foundFmt) break;  // End of file but we have fmt info
            cerr << "Error reading chunk ID" << endl;
            return false;
        }

        // Read chunk size
        if (!in.read(reinterpret_cast<char*>(&chunkSize), 4)) {
            cerr << "Error reading chunk size" << endl;
            return false;
        }

        // Process chunk based on ID
        if (strncmp(chunkId, "fmt ", 4) == 0) {
            foundFmt = true;
            
            // Audio format (1 = PCM, 3 = IEEE float, etc.)
            int16_t audioFormat = 0;
            if (!in.read(reinterpret_cast<char*>(&audioFormat), 2)) return false;
            
            // For now, we support PCM (1) and some compressed formats
            if (audioFormat != 1) {
                // Try to continue anyway - some files report wrong format
                cout << "Note: Non-PCM format detected (" << audioFormat << "), attempting to read anyway" << endl;
            }

            // Number of channels
            if (!in.read(reinterpret_cast<char*>(&channels), 2)) return false;
            if (channels != 1 && channels != 2) {
                cerr << "Only mono or stereo WAV files are supported (found " << channels << " channels)" << endl;
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
            
            // We support more bit depths now
            if (bitsPerSample != 8 && bitsPerSample != 16 && bitsPerSample != 24 && bitsPerSample != 32) {
                cerr << "Unsupported bits per sample: " << bitsPerSample << endl;
                return false;
            }

            // Skip any extra params in fmt chunk
            if (chunkSize > 16) {
                in.seekg(chunkSize - 16, ios::cur);
            }
        }
        else if (strncmp(chunkId, "data", 4) == 0) {
            foundData = true;
            
            if (!foundFmt) {
                cerr << "Data chunk found before format chunk" << endl;
                return false;
            }
            
            // Read samples based on bits per sample
            if (bitsPerSample == 8) {
                read8BitSamples(in, chunkSize);
            } else if (bitsPerSample == 16) {
                read16BitSamples(in, chunkSize);
            } else if (bitsPerSample == 24) {
                read24BitSamples(in, chunkSize);
            } else if (bitsPerSample == 32) {
                read32BitSamples(in, chunkSize);
            }
        }
        else {
            // Skip unknown chunk
            in.seekg(chunkSize, ios::cur);
        }
    }

    return foundFmt && (foundData || data.size() > 0);
}

void WAVReader::read8BitSamples(ifstream& in, int32_t dataSize) {
    size_t numSamples = dataSize;  // 1 byte per sample
    data.resize(numSamples);

    // Read 8-bit samples (unsigned)
    vector<uint8_t> tempData(numSamples);
    if (!in.read(reinterpret_cast<char*>(tempData.data()), dataSize)) {
        cerr << "Error reading 8-bit WAV data" << endl;
        data.clear();
        return;
    }

    // Convert from uint8_t to int32_t (scale from 0-255 to -32768 to 32767)
    for (size_t i = 0; i < numSamples; i++) {
        // Convert unsigned 8-bit (0-255) to signed 16-bit (-32768 to 32767)
        int32_t sample = static_cast<int32_t>(tempData[i]) - 128;
        data[i] = sample * 256;  // Scale to 16-bit range
    }
}

void WAVReader::read16BitSamples(ifstream& in, int32_t dataSize) {
    size_t numSamples = dataSize / 2;  // 2 bytes per sample
    data.resize(numSamples);

    // Read 16-bit samples
    vector<int16_t> tempData(numSamples);
    if (!in.read(reinterpret_cast<char*>(tempData.data()), dataSize)) {
        cerr << "Error reading 16-bit WAV data" << endl;
        data.clear();
        return;
    }

    // Convert from int16_t to int32_t
    for (size_t i = 0; i < numSamples; i++) {
        data[i] = static_cast<int32_t>(tempData[i]);
    }
}

void WAVReader::read24BitSamples(ifstream& in, int32_t dataSize) {
    size_t numSamples = dataSize / 3;  // 3 bytes per sample
    data.resize(numSamples);

    // Read 24-bit samples (3 bytes per sample)
    vector<unsigned char> rawBytes(dataSize);
    if (!in.read(reinterpret_cast<char*>(rawBytes.data()), dataSize)) {
        cerr << "Error reading 24-bit WAV data" << endl;
        data.clear();
        return;
    }

    // Convert 24-bit (3 bytes) to 32-bit signed integers
    for (size_t i = 0; i < numSamples; i++) {
        // Read 3 bytes and convert to 32-bit signed integer (little-endian)
        int32_t sample = 0;
        size_t bytePos = i * 3;
        
        // Read 3 bytes (little endian)
        sample = rawBytes[bytePos] | 
                (rawBytes[bytePos + 1] << 8) | 
                (rawBytes[bytePos + 2] << 16);
                
        // Sign extend if the highest bit is set
        if (sample & 0x800000) {
            sample |= 0xFF000000;  // Set high byte for negative values
        }
        
        data[i] = sample;
    }
}

void WAVReader::read32BitSamples(ifstream& in, int32_t dataSize) {
    size_t numSamples = dataSize / 4;  // 4 bytes per sample
    data.resize(numSamples);

    // For 32-bit samples, read directly into our int32_t array
    if (!in.read(reinterpret_cast<char*>(data.data()), dataSize)) {
        cerr << "Error reading 32-bit WAV data" << endl;
        data.clear();
        return;
    }
}