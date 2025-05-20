#include "../include/utils/json.hpp" 
#include "../include/core/FeatureExtractor.h"

#include <iostream>
#include <filesystem>
#include <fstream>
#include <chrono>
#include <vector>
#include <algorithm>
#include <thread>
#include <mutex>
#include <atomic>

using namespace std;
using namespace FeatureExtractor;
using json = nlohmann::json;

void printUsage() {
    cout << "Usage: extract_features [OPTIONS] <input_path> <output_folder>\n";
    cout << "Options:\n";
    cout << "  --method <method>      Feature extraction method (spectral, maxfreq) [default: spectral]\n";
    cout << "  --frequencies <n>      Number of frequencies per frame (maxfreq) [default: 4]\n";
    cout << "  --bins <n>             Number of frequency bins (spectral) [default: 32]\n";
    cout << "  --frame-size <n>       Frame size in samples [default: 1024]\n";
    cout << "  --hop-size <n>         Hop size in samples [default: 512]\n";
    cout << "  --config <file>        Load parameters from JSON config file\n";
    cout << "  -h, --help             Show this help message\n";
    cout << "  -i, --input <path>     Input folder or WAV file\n";
    cout << "  -o, --output <folder>  Output folder for extracted features\n";
    cout << endl;
}

/**
 * Process all WAV files in a directory using multiple threads
 */
void processDirectory(
    const string& inFolder, 
    const string& outFolder, 
    const string& method, 
    int numFrequencies, 
    int numBins,
    int frameSize, 
    int hopSize
) {
    // Track metrics with atomic variables for thread safety
    atomic<int> filesProcessed(0);
    atomic<int> filesSkipped(0);
    auto startTime = chrono::high_resolution_clock::now();

    cout << "Starting feature extraction using method: " << method << endl;
    cout << "Frame size: " << frameSize << ", Hop size: " << hopSize << endl;
    
    if (method == "maxfreq") {
        cout << "Extracting " << numFrequencies << " peak frequencies per frame" << endl;
    } else {
        cout << "Using " << numBins << " frequency bins" << endl;
    }
    
    // Get list of WAV files
    vector<string> wavFiles;
    try {
        for (auto& entry : filesystem::directory_iterator(inFolder)) {
            if (entry.path().extension() == ".wav") {
                wavFiles.push_back(entry.path().string());
            }
        }
    } catch (const filesystem::filesystem_error& e) {
        cerr << "Error reading directory: " << e.what() << endl;
        throw;
    }
    
    cout << "Found " << wavFiles.size() << " WAV files to process" << endl;
    
    // Determine optimal number of threads
    unsigned int threadCount = thread::hardware_concurrency();
    if (threadCount == 0) threadCount = 2;  // Default if detection fails
    
    // Limit threads based on file count
    threadCount = min(threadCount, static_cast<unsigned int>(wavFiles.size()));
    cout << "Using " << threadCount << " threads to process " << wavFiles.size() << " files" << endl;
    
    // Thread-safe console output
    mutex coutMutex;
    
    // Define the file processing function for each thread
    auto processFileBatch = [&](size_t start, size_t end) {
        for (size_t i = start; i < end && i < wavFiles.size(); i++) {
            extractFeaturesFromFile(
                wavFiles[i], outFolder, method,
                numFrequencies, numBins, frameSize, hopSize,
                coutMutex, filesProcessed, filesSkipped
            );
        }
    };
    
    // Create and launch threads
    vector<thread> threads;
    size_t filesPerThread = (wavFiles.size() + threadCount - 1) / threadCount;
    
    for (unsigned int t = 0; t < threadCount; t++) {
        size_t start = t * filesPerThread;
        size_t end = min((t + 1) * filesPerThread, wavFiles.size());
        
        if (start >= wavFiles.size()) break;
        
        threads.emplace_back(processFileBatch, start, end);
    }
    
    // Wait for all threads to complete
    for (auto& t : threads) {
        if (t.joinable()) {
            t.join();
        }
    }
    
    auto endTime = chrono::high_resolution_clock::now();
    auto totalTime = chrono::duration_cast<chrono::seconds>(endTime - startTime).count();
    
    cout << "\nFeature extraction summary:" << endl;
    cout << "  Files processed: " << filesProcessed << endl;
    cout << "  Files skipped: " << filesSkipped << endl;
    cout << "  Total time: " << totalTime << " seconds" << endl;
    
    // Save configuration to output directory
    saveConfig(outFolder, method, numFrequencies, numBins, frameSize, hopSize, filesProcessed);
}

/**
 * Process a single WAV file
 */
void processFile(
    const string& wavFile, 
    const string& outFolder, 
    const string& method, 
    int numFrequencies, 
    int numBins,
    int frameSize, 
    int hopSize
) {
    cout << "Processing single WAV file: " << wavFile << endl;
    
    atomic<int> filesProcessed(0);
    atomic<int> filesSkipped(0);
    mutex coutMutex; // Not really needed for single file but keeps interface consistent

    auto startTime = chrono::high_resolution_clock::now();
    
    extractFeaturesFromFile(
        wavFile, outFolder, method,
        numFrequencies, numBins, frameSize, hopSize,
        coutMutex, filesProcessed, filesSkipped
    );
    
    auto endTime = chrono::high_resolution_clock::now();
    auto totalTime = chrono::duration_cast<chrono::seconds>(endTime - startTime).count();
    
    cout << "\nFeature extraction summary:" << endl;
    cout << "  Files processed: " << filesProcessed << endl;
    cout << "  Files skipped: " << filesSkipped << endl;
    cout << "  Total time: " << totalTime << " seconds" << endl;
    
    // Save configuration to output directory
    saveConfig(outFolder, method, numFrequencies, numBins, frameSize, hopSize, filesProcessed);
}

/**
 * @brief Extract features from WAV files.
 * Usage: extract_features --method [spectral|maxfreq] input_folder output_folder
 */
int main(int argc, char* argv[]) {
    // Default values
    string method = "spectral";
    int numFrequencies = 4;
    int numBins = 32;
    int frameSize = 1024;
    int hopSize = 512;
    string inputPath;
    string outFolder;
    string configFile;
    
    // Parse command line arguments
    for (int i = 1; i < argc; i++) {
        string arg = argv[i];
        
        if (arg == "-h" || arg == "--help") {
            printUsage();
            return 0;
        } else if (arg == "--method" && i + 1 < argc) {
            method = argv[++i];
        } else if (arg == "--frequencies" && i + 1 < argc) {
            numFrequencies = stoi(argv[++i]);
        } else if (arg == "--bins" && i + 1 < argc) {
            numBins = stoi(argv[++i]);
        } else if (arg == "--frame-size" && i + 1 < argc) {
            frameSize = stoi(argv[++i]);
        } else if (arg == "--hop-size" && i + 1 < argc) {
            hopSize = stoi(argv[++i]);
        } else if (arg == "--config" && i + 1 < argc) {
            configFile = argv[++i];
        } else if ((arg == "-i" || arg == "--input") && i + 1 < argc) {
            inputPath = argv[++i];
        } else if ((arg == "-o" || arg == "--output") && i + 1 < argc) {
            outFolder = argv[++i];
        } else if (inputPath.empty()) {
            inputPath = arg;
        } else if (outFolder.empty()) {
            outFolder = arg;
        }
    }
    
    // If config file provided, load and override defaults
    if (!configFile.empty()) {
        try {
            ifstream f(configFile);
            if (!f) {
                cerr << "Error: Could not open config file: " << configFile << endl;
                return 1;
            }
            
            json config = json::parse(f);
            
            // Override defaults with config values
            if (config.contains("method")) method = config["method"];
            if (config.contains("frequencies")) numFrequencies = config["frequencies"];
            if (config.contains("bins")) numBins = config["bins"];
            if (config.contains("frameSize")) frameSize = config["frameSize"];
            if (config.contains("hopSize")) hopSize = config["hopSize"];
            if (config.contains("input")) inputPath = config["input"];
            if (config.contains("output")) outFolder = config["output"];
            
            cout << "Loaded configuration from " << configFile << endl;
            
        } catch (const exception& e) {
            cerr << "Error parsing config file: " << e.what() << endl;
            return 1;
        }
    }
    
    // Validate required arguments - allow defaults from config file
    if (inputPath.empty()) {
        inputPath = "data/full_tracks";  // Default input path
        cout << "Using default input path: " << inputPath << endl;
    }
    
    if (outFolder.empty()) {
        outFolder = "data/features";  // Default output directory
        cout << "Using default output folder: " << outFolder << endl;
    }

    // Validate method
    vector<string> validMethods = {"spectral", "maxfreq"};
    if (find(validMethods.begin(), validMethods.end(), method) == validMethods.end()) {
        cerr << "Error: Invalid method: " << method << endl;
        cerr << "Valid options: spectral, maxfreq" << endl;
        return 1;
    }

    // Check if input exists
    if (!filesystem::exists(inputPath)) {
        cerr << "Error: Input path does not exist: " << inputPath << endl;
        return 1;
    }

    // Create output directory if it doesn't exist
    try {
        filesystem::create_directories(outFolder);
    } catch (const filesystem::filesystem_error& e) {
        cerr << "Error creating output directory: " << e.what() << endl;
        return 1;
    }

    try {
        // Check if input is a file or a directory
        if (filesystem::is_regular_file(inputPath)) {
            // Process a single file
            if (filesystem::path(inputPath).extension() != ".wav") {
                cerr << "Error: Input file is not a WAV file: " << inputPath << endl;
                return 1;
            }
            
            processFile(inputPath, outFolder, method,
                       numFrequencies, numBins, frameSize, hopSize);
        } 
        else if (filesystem::is_directory(inputPath)) {
            // Process a directory
            processDirectory(inputPath, outFolder, method,
                           numFrequencies, numBins, frameSize, hopSize);
        }
        else {
            cerr << "Error: Input path is neither a file nor a directory: " << inputPath << endl;
            return 1;
        }
        return 0;
    } 
    catch (const exception& e) {
        cerr << "Error during processing: " << e.what() << endl;
        return 1;
    }
}