#include "../include/utils/CLIParser.h"
#include "../include/core/NCD.h"
#include <iostream>
#include <filesystem>
#include <fstream>
#include <algorithm>
#include <vector>

using namespace std;

/**
 * @brief Compute pairwise NCD for feature files.
 * Usage: compute_ncd --compressor [gzip|bzip2|lzma|zstd] input_features_folder output_matrix.csv
 */
int main(int argc, char* argv[]) {
    CLIParser parser(argc, argv);
    string compressor = parser.getOption("--compressor", "gzip");
    auto args = parser.getArgs();

    if (args.size() < 2) {
        cout << "Usage: compute_ncd --compressor [gzip|bzip2|lzma|zstd] <input_feat_folder> <output_matrix.csv>\n";
        return 1;
    }

    string featFolder = args[0];
    string outputFile = args[1];

    // Check if input folder exists
    if (!filesystem::exists(featFolder) || !filesystem::is_directory(featFolder)) {
        cerr << "Error: Input features folder does not exist or is not a directory: " << featFolder << endl;
        return 1;
    }

    // Ensure output directory exists
    filesystem::path outPath(outputFile);
    try {
        filesystem::create_directories(outPath.parent_path());
    } catch (const filesystem::filesystem_error& e) {
        cerr << "Error creating output directory: " << e.what() << endl;
    }

    // Validate compressor
    vector<string> validCompressors = {"gzip", "bzip2", "lzma", "zstd"};
    if (find(validCompressors.begin(), validCompressors.end(), compressor) == validCompressors.end()) {
        cerr << "Error: Invalid compressor: " << compressor << endl;
        cerr << "Valid options: gzip, bzip2, lzma, zstd" << endl;
        return 1;
    }

    cout << "Computing NCD using " << compressor << " compressor" << endl;
    cout << "Input features: " << featFolder << endl;
    cout << "Output matrix: " << outputFile << endl;

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
        return 1;
    }

    if (files.empty()) {
        cerr << "Error: No files found in input directory: " << featFolder << endl;
        return 1;
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
        return 1;
    }

    // Save matrix as CSV (rows and columns in same order)
    ofstream out(outputFile);
    if (!out) {
        cerr << "Error: Could not open output file for writing: " << outputFile << endl;
        return 1;
    }

    // Write header with filenames (optional)
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
    return 0;
}