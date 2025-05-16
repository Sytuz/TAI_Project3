#include "../include/utils/CLIParser.h"
#include "../include/utils/Segmenter.h"
#include "../include/utils/NoiseInjector.h"
#include "../include/core/FFTExtractor.h"
#include "../include/core/MaxFreqExtractor.h"
#include "../include/core/WAVReader.h"

#include <iostream>
#include <filesystem>
#include <fstream>
#include <chrono>
#include <vector>
#include <algorithm>

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

    // Validate method
    vector<string> validMethods = {"fft", "maxfreq"};
    if (find(validMethods.begin(), validMethods.end(), method) == validMethods.end()) {
        cerr << "Error: Invalid method: " << method << endl;
        cerr << "Valid options: fft, maxfreq" << endl;
        return 1;
    }

    // Check if input folder exists
    if (!filesystem::exists(inFolder) || !filesystem::is_directory(inFolder)) {
        cerr << "Error: Input folder does not exist or is not a directory: " << inFolder << endl;
        return 1;
    }

    // Create output directory if it doesn't exist
    try {
        filesystem::create_directories(outFolder);
    } catch (const filesystem::filesystem_error& e) {
        cerr << "Error creating output directory: " << e.what() << endl;
        return 1;
    }

    WAVReader reader;
    Segmenter seg;
    NoiseInjector injector(snr);
    FFTExtractor fftExt;
    MaxFreqExtractor mfExt;

    int frameSize = 1024;
    int hopSize = 512;

    // Track metrics
    int filesProcessed = 0;
    int filesSkipped = 0;
    auto startTime = chrono::high_resolution_clock::now();

    cout << "Starting feature extraction using method: " << method << endl;
    if (addNoise) {
        cout << "Adding noise with SNR = " << snr << " dB" << endl;
    }

    try {
        for (auto& entry : filesystem::directory_iterator(inFolder)) {
            if (entry.path().extension() == ".wav") {
                string wavFile = entry.path().string();
                cout << "Processing: " << wavFile << endl;

                if (!reader.load(wavFile)) {
                    cout << "  Skipping due to load error" << endl;
                    filesSkipped++;
                    continue;
                }

                auto samples = reader.samples();
                int channels = reader.isStereo() ? 2 : 1;

                // Add noise if requested
                if (addNoise) {
                    injector.addNoise(samples);
                }

                // Extract features
                string featData;
                auto extractStart = chrono::high_resolution_clock::now();
                if (method == "fft") {
                    featData = fftExt.extractFeatures(samples, channels, frameSize, hopSize);
                } else if (method == "maxfreq") {
                    featData = mfExt.extractFeatures(samples, channels, frameSize, hopSize);
                }
                auto extractEnd = chrono::high_resolution_clock::now();
                auto extractTime = chrono::duration_cast<chrono::milliseconds>(extractEnd - extractStart).count();

                cout << "  Feature extraction took " << extractTime << " ms" << endl;

                // Save to output
                string base = entry.path().stem().string();
                string outFile = outFolder + "/" + base + "_" + method + ".feat";
                ofstream out(outFile);
                if (!out) {
                    cerr << "  Error: Could not open output file: " << outFile << endl;
                    filesSkipped++;
                    continue;
                }
                out << featData;
                out.close();

                cout << "  Extracted features to " << outFile << endl;
                filesProcessed++;
            }
        }
    } catch (const filesystem::filesystem_error& e) {
        cerr << "Error reading directory: " << e.what() << endl;
        return 1;
    }

    auto endTime = chrono::high_resolution_clock::now();
    auto totalTime = chrono::duration_cast<chrono::seconds>(endTime - startTime).count();

    cout << "\nFeature extraction summary:" << endl;
    cout << "  Files processed: " << filesProcessed << endl;
    cout << "  Files skipped: " << filesSkipped << endl;
    cout << "  Total time: " << totalTime << " seconds" << endl;

    return 0;
}