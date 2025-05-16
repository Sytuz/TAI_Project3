#include "../include/utils/CLIParser.h"
#include <iostream>

using namespace std;

/**
 * @brief High-level pipeline: extract features, compute NCD, build tree.
 * Usage: music_id [--method maxfreq|fft] [--compressor gzip|...] [--add-noise SNR] input_wav_folder output_prefix
 * Produces: output_prefix_matrix.csv, output_prefix_tree.newick, and optional image.
 */
int main(int argc, char* argv[]) {
    CLIParser parser(argc, argv);
    // Forward everything to individual apps (this is just a convenience wrapper)
    string method = parser.getOption("--method", "fft");
    string compressor = parser.getOption("--compressor", "gzip");
    bool addNoise = parser.flagExists("--add-noise");
    string snr = parser.getOption("--add-noise", "60");
    auto args = parser.getArgs();
    if (args.size() < 2) {
        cout << "Usage: music_id [--method maxfreq|fft] [--compressor gzip|...] [--add-noise SNR] <input_wav_folder> <output_prefix>\n";
        return 1;
    }
    string wavFolder = args[0];
    string prefix = args[1];

    // Step 1: extract features
    string featFolder = prefix + "_features";
    string cmd1 = "apps/extract_features --method " + method;
    if (addNoise) cmd1 += " --add-noise " + snr;
    cmd1 += " " + wavFolder + " " + featFolder;
    system(cmd1.c_str());

    // Step 2: compute NCD
    string matrixFile = prefix + "_matrix.csv";
    string cmd2 = "apps/compute_ncd --compressor " + compressor + " " + featFolder + " " + matrixFile;
    system(cmd2.c_str());

    // Step 3: build tree
    string newickFile = prefix + "_tree.newick";
    string cmd3 = "apps/build_tree " + matrixFile + " " + newickFile + " --output-image " + prefix + "_tree.png";
    system(cmd3.c_str());

    cout << "Music ID pipeline completed. Tree at " << newickFile << endl;
    return 0;
}
