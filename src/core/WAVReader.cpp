#include "../../include/core/WAVReader.h"
#include <fstream>
#include <iostream>

using namespace std;

bool WAVReader::load(const string& filename) {
    ifstream in(filename, ios::binary);
    if (!in) {
        cerr << "Failed to open WAV file: " << filename << endl;
        return false;
    }
    // Parse WAV header (simple PCM 16-bit little-endian, stereo or mono)
    char riff[4], wave[4], fmt[4], dataId[4];
    in.read(riff, 4);
    in.read(wave, 4);
    if (string(riff,4) != "RIFF" || string(wave,4) != "WAVE") {
        cerr << "Not a valid WAV file: " << filename << endl;
        return false;
    }
    // Read chunks
    int32_t chunkSize;
    in.read(reinterpret_cast<char*>(&chunkSize), 4);
    // Read fmt chunk
    in.read(fmt, 4);
    int32_t subChunk1Size = 0;
    in.read(reinterpret_cast<char*>(&subChunk1Size), 4);
    int16_t audioFormat = 0;
    in.read(reinterpret_cast<char*>(&audioFormat), 2);
    in.read(reinterpret_cast<char*>(&channels), 2);
    in.read(reinterpret_cast<char*>(&samplerate), 4);
    int32_t byteRate;
    in.read(reinterpret_cast<char*>(&byteRate), 4);
    int16_t blockAlign;
    in.read(reinterpret_cast<char*>(&blockAlign), 2);
    in.read(reinterpret_cast<char*>(&bitsPerSample), 2);

    // Skip possible extra params if subChunk1Size > 16
    if (subChunk1Size > 16) {
        in.seekg(subChunk1Size - 16, ios::cur);
    }

    // Find data chunk
    while (true) {
        if (!in.read(dataId, 4)) return false;
        int32_t dataSize = 0;
        in.read(reinterpret_cast<char*>(&dataSize), 4);
        if (string(dataId,4) == "data") {
            data.resize(dataSize / (bitsPerSample/8));
            in.read(reinterpret_cast<char*>(data.data()), dataSize);
            break;
        }
        // skip unknown chunk
        in.seekg(dataSize, ios::cur);
    }
    return true;
}
