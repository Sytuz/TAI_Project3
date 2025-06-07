#include "../include/core/NCD.h"
#include "../include/core/FeatureExtractor.h"
#include "../include/utils/json.hpp"
#include <iostream>
#include <filesystem>
#include <fstream>
#include <algorithm>
#include <vector>
#include <iomanip>
#include <mutex>
#include <atomic>
#include <unistd.h>  // for getpid()

using namespace std;
using json = nlohmann::json;

void printUsage() {
    cout << "Usage: music_id [OPTIONS] <query_file> <database_dir> <output_file>\n";
    cout << "Query file can be either:\n";
    cout << "  - A feature file (.feat extension) - for direct comparison\n";
    cout << "  - A WAV file (.wav extension) - will extract features automatically\n";
    cout << "\nOptions:\n";
    cout << "  --compressor <comp>   Compressor to use (gzip, bzip2, lzma, zstd) [default: gzip]\n";
    cout << "  --top <n>             Show only top N matches [default: 10]\n";
    cout << "  --config <file>       Config file for feature extraction (when using WAV) [default: config/feature_extraction_spectral_default.json]\n";
    cout << "  --binary              Use binary feature files (.featbin) instead of text (.feat)\n";
    cout << "  -h, --help            Show this help message\n";
    cout << endl;
}

/**
 * Load feature extraction configuration from JSON file
 */
bool loadConfig(const string& configFile, string& method, int& numFrequencies, 
                int& numBins, int& frameSize, int& hopSize) {
    ifstream file(configFile);
    if (!file.is_open()) {
        cerr << "Error: Could not open config file: " << configFile << endl;
        return false;
    }
    
    try {
        json config;
        file >> config;
        
        method = config.value("method", "spectral");
        numFrequencies = config.value("numFrequencies", 4);
        numBins = config.value("numBins", 32);
        frameSize = config.value("frameSize", 1024);
        hopSize = config.value("hopSize", 512);
        
        return true;
    } catch (const exception& e) {
        cerr << "Error parsing config file: " << e.what() << endl;
        return false;
    }
}

/**
 * Extract features from a WAV file to a temporary feature file
 */
string extractFeaturesFromWAV(const string& wavFile, const string& configFile, bool useBinary = false) {
    // Load configuration
    string method;
    int numFrequencies, numBins, frameSize, hopSize;
    
    if (!loadConfig(configFile, method, numFrequencies, numBins, frameSize, hopSize)) {
        return "";
    }
    
    cout << "Extracting features from WAV file using method: " << method << endl;
    cout << "Frame size: " << frameSize << ", Hop size: " << hopSize << endl;
    
    if (method == "maxfreq") {
        cout << "Extracting " << numFrequencies << " peak frequencies per frame" << endl;
    } else {
        cout << "Using " << numBins << " frequency bins" << endl;
    }
    
    // Create temporary directory for feature extraction
    string tempDir = "/tmp/music_id_" + to_string(getpid());
    try {
        filesystem::create_directories(tempDir);
    } catch (const filesystem::filesystem_error& e) {
        cerr << "Error creating temporary directory: " << e.what() << endl;
        return "";
    }
    
    // Extract features
    mutex coutMutex;
    atomic<int> filesProcessed(0);
    atomic<int> filesSkipped(0);
    
    bool success = false;
    if (useBinary) {
        success = FeatureExtractor::extractFeaturesFromFile(
            wavFile, tempDir, method, numFrequencies, numBins, 
            frameSize, hopSize, coutMutex, filesProcessed, filesSkipped, true
        );
    } else {
        success = FeatureExtractor::extractFeaturesFromFile(
            wavFile, tempDir, method, numFrequencies, numBins, 
            frameSize, hopSize, coutMutex, filesProcessed, filesSkipped, false
        );
    }
    
    if (!success) {
        cerr << "Error: Failed to extract features from WAV file" << endl;
        // Clean up
        try {
            filesystem::remove_all(tempDir);
        } catch (...) {}
        return "";
    }
    
    // Find the generated feature file
    string featFile;
    try {
        for (auto& entry : filesystem::directory_iterator(tempDir)) {
            if ((useBinary && entry.path().extension() == ".featbin") || (!useBinary && entry.path().extension() == ".feat")) {
                featFile = entry.path().string();
                break;
            }
        }
    } catch (const filesystem::filesystem_error& e) {
        cerr << "Error finding generated feature file: " << e.what() << endl;
        return "";
    }
    
    if (featFile.empty()) {
        cerr << "Error: No feature file was generated" << endl;
        // Clean up
        try {
            filesystem::remove_all(tempDir);
        } catch (...) {}
        return "";
    }
    
    cout << "Features extracted successfully" << endl;
    return featFile;
}

/**
 * Clean up temporary files
 */
void cleanupTempFiles(const string& featFile) {
    if (!featFile.empty() && featFile.find("/tmp/music_id_") != string::npos) {
        try {
            string tempDir = filesystem::path(featFile).parent_path().string();
            filesystem::remove_all(tempDir);
        } catch (const filesystem::filesystem_error& e) {
            cerr << "Warning: Could not clean up temporary files: " << e.what() << endl;
        }
    }
}

/**
 * Identify music by comparing query against database using NCD
 */
bool identifyMusic(const string& queryFile, const string& dbDir, 
                 const string& outputFile, const string& compressor, int topN,
                 const string& configFile, bool useBinary = false) {
    // Ensure query file exists
    if (!filesystem::exists(queryFile)) {
        cerr << "Error: Query file does not exist: " << queryFile << endl;
        return false;
    }
    
    // Determine if query is WAV or feature file
    string actualQueryFile = queryFile;
    string tempFeatFile = "";
    bool isWavFile = false;
    
    string extension = filesystem::path(queryFile).extension().string();
    transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
    
    if (extension == ".wav") {
        isWavFile = true;
        cout << "Detected WAV file input - extracting features first..." << endl;
        tempFeatFile = extractFeaturesFromWAV(queryFile, configFile, useBinary);
        if (tempFeatFile.empty()) {
            return false;
        }
        actualQueryFile = tempFeatFile;
    } else if (extension == ".feat") {
        cout << "Detected feature file input - proceeding with direct comparison..." << endl;
    } else {
        cerr << "Error: Query file must be either .wav or .feat format" << endl;
        return false;
    }
    
    // Get the query filename for display
    string queryFilename = filesystem::path(queryFile).filename().string();
    
    // Gather database feature files
    vector<string> dbFiles;
    vector<string> dbFilenames; // For display
    try {
        for (auto& entry : filesystem::directory_iterator(dbDir)) {
            if (entry.is_regular_file()) {
                string filename = entry.path().filename().string();
                string extension = entry.path().extension().string();
                if ((useBinary && extension == ".featbin") || (!useBinary && extension == ".feat")) {
                    dbFiles.push_back(entry.path().string());
                    dbFilenames.push_back(filename);
                }
            }
        }
    } catch (const filesystem::filesystem_error& e) {
        cerr << "Error reading database directory: " << e.what() << endl;
        return false;
    }

    if (dbFiles.empty()) {
        cerr << "Error: No files found in database directory: " << dbDir << endl;
        return false;
    }

    cout << "Comparing query against " << dbFiles.size() << " database entries" << endl;

    // Compute NCD between query and each database entry
    NCD ncd;
    vector<pair<string, double>> results;
    
    for (size_t i = 0; i < dbFiles.size(); ++i) {
        double ncdValue = ncd.computeNCD(actualQueryFile, dbFiles[i], compressor);
        results.push_back({dbFilenames[i], ncdValue});
        
        // Show progress for large databases
        if (dbFiles.size() > 20 && i % 10 == 0) {
            cout << "Processed " << i << "/" << dbFiles.size() << " entries\r" << flush;
        }
    }
    
    if (dbFiles.size() > 20) {
        cout << "Processed " << dbFiles.size() << "/" << dbFiles.size() << " entries" << endl;
    }

    // Sort results by NCD (lowest first = best match)
    sort(results.begin(), results.end(), 
        [](const auto& a, const auto& b) { return a.second < b.second; });
    
    // Limit to top N if requested
    if (topN > 0 && topN < static_cast<int>(results.size())) {
        results.resize(topN);
    }

    // Save results to output file
    ofstream out(outputFile);
    if (!out) {
        cerr << "Error: Could not open output file for writing: " << outputFile << endl;
        return false;
    }

    // Write header
    out << "Query: " << queryFilename << "\n";
    out << "Compressor: " << compressor << "\n\n";
    out << "Rank,File,NCD\n";

    // Write sorted results
    for (size_t i = 0; i < results.size(); ++i) {
        out << (i+1) << "," << results[i].first << "," << fixed << setprecision(6) << results[i].second << "\n";
    }
    out.close();

    // Display top results on console
    cout << "\nTop matches for query '" << queryFilename << "':\n" << endl;
    
    const int rankWidth = 5;
    const int ncdWidth = 10;
    const int terminalWidth = 80; // Standard terminal width
    const int filenameWidth = terminalWidth - rankWidth - ncdWidth - 2; // -2 for spacing
    
    cout << setw(rankWidth) << "Rank" << setw(filenameWidth) << "File" << setw(ncdWidth) << "NCD" << endl;
    cout << string(terminalWidth - 2, '-') << endl;
    
    int displayCount = min(5, static_cast<int>(results.size()));
    for (int i = 0; i < displayCount; ++i) {
        // Truncate filename if necessary
        string displayName = results[i].first;
        if (displayName.length() > filenameWidth - 3) {
            displayName = displayName.substr(0, filenameWidth - 3) + "...";
        }
        
        cout << setw(rankWidth) << (i+1) 
             << setw(filenameWidth) << displayName
             << setw(ncdWidth) << fixed << setprecision(6) << results[i].second << endl;
    }

    cout << "\nFull results saved to " << outputFile << endl;
    
    // Clean up temporary files if WAV file was used
    if (isWavFile) {
        cleanupTempFiles(tempFeatFile);
    }
    
    return true;
}

int main(int argc, char* argv[]) {
    // Default values
    string compressor = "gzip";
    string queryFile;
    string dbDir;
    string outputFile;
    string configFile = "config/feature_extraction_spectral_default.json";
    int topN = 10;
    bool useBinary = false;
    
    // Parse command line arguments
    for (int i = 1; i < argc; i++) {
        string arg = argv[i];
        
        if (arg == "-h" || arg == "--help") {
            printUsage();
            return 0;
        } else if (arg == "--compressor" && i + 1 < argc) {
            compressor = argv[++i];
        } else if (arg == "--top" && i + 1 < argc) {
            topN = stoi(argv[++i]);
        } else if (arg == "--config" && i + 1 < argc) {
            configFile = argv[++i];
        } else if (arg == "--binary") {
            useBinary = true;
        } else if (queryFile.empty()) {
            queryFile = arg;
        } else if (dbDir.empty()) {
            dbDir = arg;
        } else if (outputFile.empty()) {
            outputFile = arg;
        }
    }
    
    // Validate required arguments
    if (queryFile.empty() || dbDir.empty() || outputFile.empty()) {
        cerr << "Error: Missing required arguments\n";
        printUsage();
        return 1;
    }

    // Validate compressor
    vector<string> validCompressors = {"gzip", "bzip2", "lzma", "zstd"};
    if (find(validCompressors.begin(), validCompressors.end(), compressor) == validCompressors.end()) {
        cerr << "Error: Invalid compressor: " << compressor << endl;
        cerr << "Valid options: gzip, bzip2, lzma, zstd" << endl;
        return 1;
    }

    // Check if database directory exists
    if (!filesystem::exists(dbDir) || !filesystem::is_directory(dbDir)) {
        cerr << "Error: Database directory does not exist: " << dbDir << endl;
        return 1;
    }

    // Ensure output directory exists
    filesystem::path outPath(outputFile);
    try {
        if (!outPath.empty() && outPath.has_parent_path()) {
            filesystem::create_directories(outPath.parent_path());
        }
    } catch (const filesystem::filesystem_error& e) {
        cerr << "Error creating output directory: " << e.what() << endl;
    }

    cout << "Music identification using " << compressor << " compressor" << endl;
    cout << "Query: " << queryFile << endl;
    cout << "Database: " << dbDir << endl;
    cout << "Output file: " << outputFile << endl;
    
    // Check if config file exists (only needed for WAV files)
    string extension = filesystem::path(queryFile).extension().string();
    transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
    if (extension == ".wav" && !filesystem::exists(configFile)) {
        cerr << "Error: Config file does not exist: " << configFile << endl;
        return 1;
    }

    if (!identifyMusic(queryFile, dbDir, outputFile, compressor, topN, configFile, useBinary)) {
        return 1;
    }
    
    return 0;
}