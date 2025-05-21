#include "../include/core/NCD.h"
#include <iostream>
#include <filesystem>
#include <fstream>
#include <algorithm>
#include <vector>
#include <iomanip>

using namespace std;

void printUsage() {
    cout << "Usage: music_id [OPTIONS] <query_feature_file> <database_dir> <output_file>\n";
    cout << "Options:\n";
    cout << "  --compressor <comp>   Compressor to use (gzip, bzip2, lzma, zstd) [default: gzip]\n";
    cout << "  --top <n>             Show only top N matches [default: 10]\n";
    cout << "  -h, --help            Show this help message\n";
    cout << endl;
}

/**
 * Identify music by comparing query against database using NCD
 */
bool identifyMusic(const string& queryFile, const string& dbDir, 
                 const string& outputFile, const string& compressor, int topN) {
    // Ensure query file exists
    if (!filesystem::exists(queryFile)) {
        cerr << "Error: Query file does not exist: " << queryFile << endl;
        return false;
    }
    
    // Get the query filename for display
    string queryFilename = filesystem::path(queryFile).filename().string();
    
    // Gather database feature files
    vector<string> dbFiles;
    vector<string> dbFilenames; // For display
    
    try {
        for (auto& entry : filesystem::directory_iterator(dbDir)) {
            // Check if it's a regular file with .feat extension
            if (entry.is_regular_file()) {
                string filename = entry.path().filename().string();
                string extension = entry.path().extension().string();
                
                if (extension == ".feat") {
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
        double ncdValue = ncd.computeNCD(queryFile, dbFiles[i], compressor);
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
    return true;
}

int main(int argc, char* argv[]) {
    // Default values
    string compressor = "gzip";
    string queryFile;
    string dbDir;
    string outputFile;
    int topN = 10;
    
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

    if (!identifyMusic(queryFile, dbDir, outputFile, compressor, topN)) {
        return 1;
    }
    
    return 0;
}