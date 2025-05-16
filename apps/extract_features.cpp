#include "../include/utils/CLIParser.h"
#include "../include/utils/Segmenter.h"
#include "../include/utils/NoiseInjector.h"
#include "../include/core/FFTExtractor.h"
#include "../include/core/MaxFreqExtractor.h"
#include "../include/core/WAVReader.h"

#include <iostream>
#include <filesystem>
#include <fstream>

using namespace std;

/**
 * @brief Extract features from WAV files.
 * Usage: extract_features --method [maxfreq|fft] [--add-noise SNRdb] input_folder output_folder
 */
int main(int argc, char* argv[]) {
    CLIParser parser(argc, argv);
    string method = parser.getOption("--method", "fft");
    bool addNoise = parser.flagExists("--add-noise");
    double snr = 60.0;
    if (addNoise) {
        snr = stod(parser.getOption("--add-noise", "60"));
    }
    auto args = parser.getArgs();
    if (args.size() < 2) {
        cout << "Usage: extract_features --method [maxfreq|fft] [--add-noise SNR] <input_wav_folder> <output_features_folder>\n";
        return 1;
    }
    string inFolder = args[0];
    string outFolder = args[1];
    filesystem::create_directories(outFolder);

    WAVReader reader;
    Segmenter seg;
    NoiseInjector injector(snr);
    FFTExtractor fftExt;
    MaxFreqExtractor mfExt;

    int frameSize = 1024;
    int hopSize = 512;

    for (auto& entry : filesystem::directory_iterator(inFolder)) {
        if (entry.path().extension() == ".wav") {
            string wavFile = entry.path().string();
            reader.load(wavFile);
            auto samples = reader.samples();
            int channels = reader.isStereo() ? 2 : 1;

            // Add noise if requested
            if (addNoise) {
                injector.addNoise(samples);
            }

            // Extract features
            string featData;
            if (method == "fft") {
                featData = fftExt.extractFeatures(samples, channels, frameSize, hopSize);
            } else {
                featData = mfExt.extractFeatures(samples, channels, frameSize, hopSize);
            }

            // Save to output
            string base = entry.path().stem().string();
            string outFile = outFolder + "/" + base + "_" + method + ".feat";
            ofstream out(outFile);
            out << featData;
            out.close();
            cout << "Extracted features to " << outFile << endl;
        }
    }
    return 0;
}
