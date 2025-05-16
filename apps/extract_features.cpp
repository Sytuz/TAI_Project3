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
#include <thread>
#include <mutex>
#include <atomic>

using namespace std;

void printUsage() {
    cout << "Usage: extract_features [OPTIONS] <input_folder> <output_folder>\n";
    cout << "Options:\n";
    cout << "  --method <method>    Feature extraction method (fft, maxfreq) [default: fft]\n";
    cout << "  --add-noise <SNR>    Add noise with specified SNR in dB [default: no noise]\n";
    cout << "  -h, --help           Show this help message\n";
    cout << "  -i, --input          Input folder containing WAV files\n";
    cout << "  -o, --output         Output folder for extracted features\n";
    cout << endl;
}

/**
 * Process all audio files in the input directory and extract features
 */
void processFiles(const string& inFolder, const string& outFolder, 
                 const string& method, bool addNoise, double snr) {
    // Track metrics with atomic variables for thread safety
    atomic<int> filesProcessed(0);
    atomic<int> filesSkipped(0);
    auto startTime = chrono::high_resolution_clock::now();

    cout << "Starting feature extraction using method: " << method << endl;
    if (addNoise) {
        cout << "Adding noise with SNR = " << snr << " dB" << endl;
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
    
    // Limit threads based on file count (no point in having more threads than files)
    threadCount = min(threadCount, static_cast<unsigned int>(wavFiles.size()));
    cout << "Using " << threadCount << " threads to process " << wavFiles.size() << " files" << endl;
    
    // Thread-safe console output
    mutex coutMutex;
    
    // Define the file processing function for each thread
    auto processFileBatch = [&](size_t start, size_t end) {
        // Each thread gets its own instances of these objects
        WAVReader reader;
        NoiseInjector injector(snr);
        FFTExtractor fftExt;
        MaxFreqExtractor mfExt;
        
        int frameSize = 1024;
        int hopSize = 512;
        
        for (size_t i = start; i < end && i < wavFiles.size(); i++) {
            string wavFile = wavFiles[i];
            
            {
                lock_guard<mutex> lock(coutMutex);
                cout << "Processing: " << wavFile << endl;
            }
            
            if (!reader.load(wavFile)) {
                lock_guard<mutex> lock(coutMutex);
                cout << "  Skipping due to load error" << endl;
                filesSkipped++;
                continue;
            }
            
            auto samples = reader.samples();
            int channels = reader.isStereo() ? 2 : 1;
            
            // Add noise if requested
            if (addNoise) {
                // Need to convert from int32_t to int16_t for noise injection
                vector<int16_t> samples16bit(samples.size());
                for (size_t j = 0; j < samples.size(); j++) {
                    samples16bit[j] = static_cast<int16_t>(samples[j]);
                }
                
                injector.addNoise(samples16bit);
                
                // Convert back to int32_t
                for (size_t j = 0; j < samples.size(); j++) {
                    samples[j] = static_cast<int32_t>(samples16bit[j]);
                }
            }
            
            // Extract features
            string featData;
            auto extractStart = chrono::high_resolution_clock::now();
            
            if (method == "fft") {
                // Convert to int16_t for feature extraction 
                vector<int16_t> samples16bit(samples.size());
                for (size_t j = 0; j < samples.size(); j++) {
                    samples16bit[j] = static_cast<int16_t>(samples[j]);
                }
                featData = fftExt.extractFeatures(samples16bit, channels, frameSize, hopSize);
            } else if (method == "maxfreq") {
                // Convert to int16_t for feature extraction
                vector<int16_t> samples16bit(samples.size());
                for (size_t j = 0; j < samples.size(); j++) {
                    samples16bit[j] = static_cast<int16_t>(samples[j]);
                }
                featData = mfExt.extractFeatures(samples16bit, channels, frameSize, hopSize);
            }
            
            auto extractEnd = chrono::high_resolution_clock::now();
            auto extractTime = chrono::duration_cast<chrono::milliseconds>(extractEnd - extractStart).count();
            
            {
                lock_guard<mutex> lock(coutMutex);
                cout << "  Feature extraction took " << extractTime << " ms" << endl;
            }
            
            // Save to output
            string base = filesystem::path(wavFile).stem().string();
            string outFile = outFolder + "/" + base + "_" + method + ".feat";
            
            ofstream out(outFile);
            if (!out) {
                lock_guard<mutex> lock(coutMutex);
                cerr << "  Error: Could not open output file: " << outFile << endl;
                filesSkipped++;
                continue;
            }
            
            out << featData;
            out.close();
            
            {
                lock_guard<mutex> lock(coutMutex);
                cout << "  Extracted features to " << outFile << endl;
            }
            
            filesProcessed++;
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
}

/**
 * @brief Extract features from WAV files.
 * Usage: extract_features --method [maxfreq|fft] [--add-noise SNRdb] input_folder output_folder
 */
int main(int argc, char* argv[]) {
    // Default values
    string method = "fft";
    bool addNoise = false;
    double snr = 60.0;
    string inFolder;
    string outFolder;
    
    // Parse command line arguments
    for (int i = 1; i < argc; i++) {
        string arg = argv[i];
        
        if (arg == "-h" || arg == "--help") {
            printUsage();
            return 0;
        } else if (arg == "--method" && i + 1 < argc) {
            method = argv[++i];
        } else if (arg == "--add-noise" && i + 1 < argc) {
            addNoise = true;
            snr = stod(argv[++i]);
        } else if ((arg == "-i" || arg == "--input") && i + 1 < argc) {
            inFolder = argv[++i];
        } else if ((arg == "-o" || arg == "--output") && i + 1 < argc) {
            outFolder = argv[++i];
        } else if (inFolder.empty()) {
            inFolder = arg;
        } else if (outFolder.empty()) {
            outFolder = arg;
        }
    }
    
    // Validate required arguments
    if (inFolder.empty() || outFolder.empty()) {
        cerr << "Error: Missing required input or output folder\n";
        printUsage();
        return 1;
    }

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

    try {
        processFiles(inFolder, outFolder, method, addNoise, snr);
        return 0;
    } catch (const exception& e) {
        cerr << "Error during processing: " << e.what() << endl;
        return 1;
    }
}