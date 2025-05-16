#include "../include/utils/CLIParser.h"
#include <iostream>
#include <filesystem>
#include <chrono>
#include <algorithm>

using namespace std;

/**
 * @brief High-level pipeline: extract features, compute NCD, build tree.
 * Usage: music_id [--method maxfreq|fft] [--compressor gzip|...] [--add-noise SNR] input_wav_folder output_prefix
 * Produces: output_prefix_matrix.csv, output_prefix_tree.newick, and optional image.
 */
int main(int argc, char* argv[]) {
    CLIParser parser(argc, argv);
    string method = parser.getOption("--method", "fft");
    string compressor = parser.getOption("--compressor", "gzip");
    bool addNoise = parser.flagExists("--add-noise");
    string snr = parser.getOption("--add-noise", "60");
    auto args = parser.getArgs();

    // cout << "Parsed arguments: " << args.size() << " positional args" << endl;
    // for (size_t i = 0; i < args.size(); i++) {
    //     cout << "  Arg " << i << ": " << args[i] << endl;
    // }

    // Skip known option values in the positional arguments
    vector<string> filteredArgs;
    for (const auto& arg : args) {
        if (arg != method && arg != compressor &&
            (!addNoise || arg != snr)) {
            filteredArgs.push_back(arg);
        }
    }

    // // Debug the filtered arguments
    // cout << "After filtering, found " << filteredArgs.size() << " positional args" << endl;
    // for (size_t i = 0; i < filteredArgs.size(); i++) {
    //     cout << "  Filtered Arg " << i << ": " << filteredArgs[i] << endl;
    // }

    if (args.size() < 2) {
        cout << "Usage: music_id [--method maxfreq|fft] [--compressor gzip|bzip2|lzma|zstd] [--add-noise SNR] <input_wav_folder> <output_prefix>\n";
        return 1;
    }

    string wavFolder = filteredArgs[0];
    string prefix = filteredArgs[1];

    // Validate parameters
    vector<string> validMethods = {"fft", "maxfreq"};
    if (find(validMethods.begin(), validMethods.end(), method) == validMethods.end()) {
        cerr << "Error: Invalid method: " << method << endl;
        cerr << "Valid options: fft, maxfreq" << endl;
        return 1;
    }

    vector<string> validCompressors = {"gzip", "bzip2", "lzma", "zstd"};
    if (find(validCompressors.begin(), validCompressors.end(), compressor) == validCompressors.end()) {
        cerr << "Error: Invalid compressor: " << compressor << endl;
        cerr << "Valid options: gzip, bzip2, lzma, zstd" << endl;
        return 1;
    }

    // Check if input folder exists
    cout << "Checking input folder: " << wavFolder << endl;
    if (!filesystem::exists(wavFolder) || !filesystem::is_directory(wavFolder)) {
        cerr << "Error: Input WAV folder does not exist or is not a directory: " << wavFolder << endl;
        return 1;
    }

    // Ensure output directory exists
    filesystem::path outPath(prefix);
    try {
        if (!outPath.empty() && outPath.has_parent_path()) {
            filesystem::create_directories(outPath.parent_path());
        }
    } catch (const filesystem::filesystem_error& e) {
        cerr << "Error creating output directory: " << e.what() << endl;
    }

    auto startTime = chrono::high_resolution_clock::now();

    cout << "=== Music Identification Pipeline ===" << endl;
    cout << "Method: " << method << endl;
    cout << "Compressor: " << compressor << endl;
    if (addNoise) {
        cout << "Noise SNR: " << snr << " dB" << endl;
    }
    cout << "Input: " << wavFolder << endl;
    cout << "Output prefix: " << prefix << endl;
    cout << "=================================" << endl;

    // Step 1: extract features
    string featFolder = prefix + "_features";
    cout << "\n==> STEP 1: Feature Extraction" << endl;
    string cmd1 = "./extract_features --method " + method;
    if (addNoise) cmd1 += " --add-noise " + snr;
    cmd1 += " \"" + wavFolder + "\" \"" + featFolder + "\"";
    cout << "Executing: " << cmd1 << endl;
    int ret1 = system(cmd1.c_str());
    if (ret1 != 0) {
        cerr << "Error during feature extraction (step 1)" << endl;
        return 1;
    }

    // Step 2: compute NCD
    string matrixFile = prefix + "_matrix.csv";
    cout << "\n==> STEP 2: Computing NCD Matrix" << endl;
    string cmd2 = "./compute_ncd --compressor " + compressor + " \"" + featFolder + "\" \"" + matrixFile + "\"";
    cout << "Executing: " << cmd2 << endl;
    int ret2 = system(cmd2.c_str());
    if (ret2 != 0) {
        cerr << "Error during NCD computation (step 2)" << endl;
        return 1;
    }

    // Step 3: build tree
    string newickFile = prefix + "_tree.newick";
    string treeImage = prefix + "_tree.png";
    cout << "\n==> STEP 3: Building Similarity Tree" << endl;
    string cmd3 = "./build_tree \"" + matrixFile + "\" \"" + newickFile + "\" --output-image \"" + treeImage + "\"";
    cout << "Executing: " << cmd3 << endl;
    int ret3 = system(cmd3.c_str());
    if (ret3 != 0) {
        cerr << "Error during tree building (step 3)" << endl;
        return 1;
    }

    auto endTime = chrono::high_resolution_clock::now();
    auto totalTime = chrono::duration_cast<chrono::seconds>(endTime - startTime).count();

    cout << "\n=== Music ID Pipeline Completed ===" << endl;
    cout << "Execution time: " << totalTime << " seconds" << endl;
    cout << "Results:" << endl;
    cout << "  - Feature files: " << featFolder << endl;
    cout << "  - NCD Matrix: " << matrixFile << endl;
    cout << "  - Tree (Newick): " << newickFile << endl;
    cout << "  - Tree (Image): " << treeImage << endl;
    cout << "=================================" << endl;

    return 0;
}