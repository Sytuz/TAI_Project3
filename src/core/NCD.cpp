#include "../../include/core/NCD.h"
#include "../../include/utils/CompressorWrapper.h"
#include <filesystem>
#include <iostream>
#include <fstream>
#include <cmath>

using namespace std;

double NCD::computeNCD(const string& file1, const string& file2, const string& compressor) {
    CompressorWrapper cw;
    // Compute compressed sizes
    long Cx = cw.compressAndGetSize(compressor, file1);
    long Cy = cw.compressAndGetSize(compressor, file2);

    if (Cx <= 0 || Cy <= 0) {
        cerr << "Error: Failed to compress individual files." << endl;
        return 1.0; // Maximum distance on error
    }

    // Create concatenated file
    string catFile = filesystem::temp_directory_path().string() + "/tmp_cat_" + to_string(hash<string>{}(file1 + file2)); // Unique temp filename
    {
        ofstream out(catFile, ios::binary);
        ifstream in1(file1, ios::binary);
        if (!in1) {
            cerr << "Error: Could not open file " << file1 << endl;
            return 1.0;
        }
        ifstream in2(file2, ios::binary);
        if (!in2) {
            cerr << "Error: Could not open file " << file2 << endl;
            return 1.0;
        }
        out << in1.rdbuf() << in2.rdbuf();
    }

    long Cxy = cw.compressAndGetSize(compressor, catFile);
    filesystem::remove(catFile);

    if (Cxy <= 0) {
        cerr << "Error: Failed to compress concatenated file." << endl;
        return 1.0;
    }

    // Formula NCD = (C(xy)-min(Cx,Cy)) / max(Cx, Cy)
    long Cmin = min(Cx, Cy);
    long Cmax = max(Cx, Cy);

    // Ensure valid values
    double ncd = double(Cxy - Cmin) / double(Cmax);

    // Clamp to valid range [0,1]
    return max(0.0, min(1.0, ncd));
}

vector<vector<double>> NCD::computeMatrix(const vector<string>& files, const string& compressor) {
    int n = files.size();
    vector<vector<double>> mat(n, vector<double>(n, 0.0));

    // Extract base filenames for display
    vector<string> baseNames;
    for (const auto& file : files) {
        baseNames.push_back(filesystem::path(file).filename().string());
    }

    cout << "Computing NCD matrix for " << n << " files using " << compressor << " compressor..." << endl;

    // Compute upper triangular matrix (diagonal is 0)
    for (int i = 0; i < n; ++i) {
        mat[i][i] = 0.0;
        for (int j = i+1; j < n; ++j) {
            double d = computeNCD(files[i], files[j], compressor);
            mat[i][j] = mat[j][i] = d;
            cout << "NCD(" << baseNames[i] << "," << baseNames[j] << ") = " << d << endl;
        }
        // Show progress
        cout << "Completed " << i+1 << "/" << n << " files" << endl;
    }
    return mat;
}
