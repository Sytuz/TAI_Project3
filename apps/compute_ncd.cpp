#include "../include/core/NCD.h"
#include <iostream>
#include <filesystem>
#include <fstream>
#include <algorithm>
#include <vector>

using namespace std;

void printUsage() {
    cout << "Usage: compute_ncd [OPTIONS] <input_feat_folder> <output_matrix.csv>\n";
    cout << "Options:\n";
    cout << "  --compressor <comp>   Compressor to use (gzip, bzip2, lzma, zstd) [default: gzip]\n";
    cout << "  -h, --help            Show this help message\n";
    cout << endl;
}

/**
 * Compute NCD matrix and save to file
 */
bool computeNCDMatrix(const string& featFolder, const string& outputFile, const string& compressor) {
    // Gather feature files
    vector<string> files;
    vector<string> filenames; // Just for display
    try {
        for (auto& entry : filesystem::directory_iterator(featFolder)) {
            if (entry.is_regular_file()) {
                files.push_back(entry.path().string());
                filenames.push_back(entry.path().filename().string());
            }
        }
    } catch (const filesystem::filesystem_error& e) {
        cerr << "Error reading directory: " << e.what() << endl;
        return false;
    }

    if (files.empty()) {
        cerr << "Error: No files found in input directory: " << featFolder << endl;
        return false;
    }

    // Sort alphabetically for consistent results
    sort(files.begin(), files.end());
    sort(filenames.begin(), filenames.end());

    cout << "Found " << files.size() << " feature files" << endl;

    // Compute NCD matrix
    NCD ncd;
    auto matrix = ncd.computeMatrix(files, compressor);

    if (matrix.size() != files.size() || (matrix.size() > 0 && matrix[0].size() != files.size())) {
        cerr << "Error: Inconsistent matrix dimensions in NCD calculation" << endl;
        return false;
    }

    // Save matrix as CSV (rows and columns in same order)
    ofstream out(outputFile);
    if (!out) {
        cerr << "Error: Could not open output file for writing: " << outputFile << endl;
        return false;
    }

    // Write header with filenames
    out << "File";
    for (const auto& fname : filenames) {
        out << "," << fname;
    }
    out << "\n";

    // Write matrix data
    for (size_t i = 0; i < matrix.size(); ++i) {
        out << filenames[i];
        for (size_t j = 0; j < matrix[i].size(); ++j) {
            out << "," << matrix[i][j];
        }
        out << "\n";
    }
    out.close();

    cout << "NCD matrix saved to " << outputFile << endl;
    return true;
}

/**
 * @brief Compute pairwise NCD for feature files.
 * Usage: compute_ncd --compressor [gzip|bzip2|lzma|zstd] input_features_folder output_matrix.csv
 */
int main(int argc, char* argv[]) {
    // Default values
    string compressor = "gzip";
    string featFolder;
    string outputFile;
    
    // Parse command line arguments
    for (int i = 1; i < argc; i++) {
        string arg = argv[i];
        
        if (arg == "-h" || arg == "--help") {
            printUsage();
            return 0;
        } else if (arg == "--compressor" && i + 1 < argc) {
            compressor = argv[++i];
        } else if (featFolder.empty()) {
            featFolder = arg;
        } else if (outputFile.empty()) {
            outputFile = arg;
        }
    }
    
    // Validate required arguments
    if (featFolder.empty() || outputFile.empty()) {
        cerr << "Error: Missing required input folder or output file\n";
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

    // Check if input folder exists
    if (!filesystem::exists(featFolder) || !filesystem::is_directory(featFolder)) {
        cerr << "Error: Input features folder does not exist or is not a directory: " << featFolder << endl;
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

    cout << "Computing NCD using " << compressor << " compressor" << endl;
    cout << "Input features: " << featFolder << endl;
    cout << "Output matrix: " << outputFile << endl;

    if (!computeNCDMatrix(featFolder, outputFile, compressor)) {
        return 1;
    }
    
    return 0;
}