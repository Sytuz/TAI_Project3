#include <iostream>
#include <filesystem>
#include <chrono>
#include <algorithm>
#include <vector>

using namespace std;

void printUsage() {
    cout << "Usage: music_id [OPTIONS] <input_wav_folder> <output_prefix>\n";
    cout << "Options:\n";
    cout << "  --method <method>      Feature extraction method (fft, maxfreq) [default: fft]\n";
    cout << "  --compressor <comp>    Compressor to use (gzip, bzip2, lzma, zstd) [default: gzip]\n";
    cout << "  --add-noise <SNR>      Add noise with specified SNR in dB [default: no noise]\n";
    cout << "  -h, --help             Show this help message\n";
    cout << endl;
}

/**
 * Run feature extraction step
 */
bool runFeatureExtraction(const string& wavFolder, const string& featFolder, 
                          const string& method, bool addNoise, const string& snr) {
    string cmd = "./apps/extract_features --method " + method;
    if (addNoise) cmd += " --add-noise " + snr;
    cmd += " \"" + wavFolder + "\" \"" + featFolder + "\"";
    
    cout << "Executing: " << cmd << endl;
    int ret = system(cmd.c_str());
    
    return ret == 0;
}

/**
 * Run NCD computation step
 */
bool runNCDComputation(const string& featFolder, const string& matrixFile, const string& compressor) {
    string cmd = "./apps/compute_ncd --compressor " + compressor + " \"" + featFolder + "\" \"" + matrixFile + "\"";
    
    cout << "Executing: " << cmd << endl;
    int ret = system(cmd.c_str());
    
    return ret == 0;
}

/**
 * Run tree building step
 */
bool runTreeBuilding(const string& matrixFile, const string& newickFile, const string& treeImage) {
    string cmd = "./apps/build_tree \"" + matrixFile + "\" \"" + newickFile + "\" --output-image \"" + treeImage + "\"";
    
    cout << "Executing: " << cmd << endl;
    int ret = system(cmd.c_str());
    
    return ret == 0;
}

/**
 * @brief High-level pipeline: extract features, compute NCD, build tree.
 * Usage: music_id [--method maxfreq|fft] [--compressor gzip|...] [--add-noise SNR] input_wav_folder output_prefix
 * Produces: output_prefix_matrix.csv, output_prefix_tree.newick, and image.
 */
int main(int argc, char* argv[]) {
    // Default values
    string method = "fft";
    string compressor = "gzip";
    bool addNoise = false;
    string snr = "60";
    string wavFolder;
    string prefix;
    
    // Parse command line arguments
    for (int i = 1; i < argc; i++) {
        string arg = argv[i];
        
        if (arg == "-h" || arg == "--help") {
            printUsage();
            return 0;
        } else if (arg == "--method" && i + 1 < argc) {
            method = argv[++i];
        } else if (arg == "--compressor" && i + 1 < argc) {
            compressor = argv[++i];
        } else if (arg == "--add-noise" && i + 1 < argc) {
            addNoise = true;
            snr = argv[++i];
        } else if (wavFolder.empty()) {
            wavFolder = arg;
        } else if (prefix.empty()) {
            prefix = arg;
        }
    }
    
    // Validate required arguments
    if (wavFolder.empty() || prefix.empty()) {
        cerr << "Error: Missing required input folder or output prefix\n";
        printUsage();
        return 1;
    }

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
    if (!runFeatureExtraction(wavFolder, featFolder, method, addNoise, snr)) {
        cerr << "Error during feature extraction (step 1)" << endl;
        return 1;
    }

    // Step 2: compute NCD
    string matrixFile = prefix + "_matrix.csv";
    cout << "\n==> STEP 2: Computing NCD Matrix" << endl;
    if (!runNCDComputation(featFolder, matrixFile, compressor)) {
        cerr << "Error during NCD computation (step 2)" << endl;
        return 1;
    }

    // Step 3: build tree
    string newickFile = prefix + "_tree.newick";
    string treeImage = prefix + "_tree.png";
    cout << "\n==> STEP 3: Building Similarity Tree" << endl;
    if (!runTreeBuilding(matrixFile, newickFile, treeImage)) {
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