#include "../include/utils/CLIParser.h"
#include "../include/core/NCD.h"
#include <iostream>
#include <filesystem>
#include <fstream>
#include <algorithm>

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

    // Gather feature files
    vector<string> files;
    for (auto& entry : filesystem::directory_iterator(featFolder)) {
        if (entry.is_regular_file()) {
            files.push_back(entry.path().string());
        }
    }
    sort(files.begin(), files.end());

    NCD ncd;
    auto matrix = ncd.computeMatrix(files, compressor);

    // Save matrix as CSV (rows and columns in same order)
    ofstream out(outputFile);
    for (size_t i = 0; i < matrix.size(); ++i) {
        for (size_t j = 0; j < matrix.size(); ++j) {
            out << matrix[i][j] << (j+1 < matrix.size() ? "," : "");
        }
        out << "\n";
    }
    out.close();
    cout << "NCD matrix saved to " << outputFile << endl;
    return 0;
}
