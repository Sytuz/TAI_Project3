#include "../../include/core/NCD.h"
#include "../../include/utils/CompressorWrapper.h"
#include <filesystem>
#include <iostream>
#include <fstream>
#include <cmath>

using namespace std;

double NCD::computeNCD(const string& file1, const string& file2, const string& compressor) {
    CompressorWrapper cw;
    
    // Compute compressed sizes with error checking
    long Cx = cw.compressAndGetSize(compressor, file1);
    if (Cx <= 0) {
        cerr << "Error: Failed to compress file1: " << file1 << endl;
        return 1.0; // Maximum distance on error
    }
    
    long Cy = cw.compressAndGetSize(compressor, file2);
    if (Cy <= 0) {
        cerr << "Error: Failed to compress file2: " << file2 << endl;
        return 1.0;
    }
    
    // Create a unique temporary filename
    string catFile = filesystem::temp_directory_path().string() + "/tmp_cat_" + 
                    to_string(chrono::system_clock::now().time_since_epoch().count());
    
    // Improved concatenation with error checking
    bool concatenationSuccess = false;
    {
        ofstream out(catFile, ios::binary);
        if (!out) {
            cerr << "Error: Could not create temporary file " << catFile << endl;
            return 1.0;
        }
        
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
        
        // Read and write complete files
        out << in1.rdbuf();
        out << in2.rdbuf();
        
        // Make sure everything was written
        concatenationSuccess = out.good() && !out.fail();
        
        // Explicitly close to ensure data is flushed
        out.close();
    }
    
    if (!concatenationSuccess) {
        cerr << "Error: File concatenation failed" << endl;
        filesystem::remove(catFile);
        return 1.0;
    }
    
    // Compress concatenated file
    long Cxy = cw.compressAndGetSize(compressor, catFile);
    
    // Clean up temporary file
    bool removed = false;
    try {
        removed = filesystem::remove(catFile);
    } catch (const filesystem::filesystem_error& e) {
        cerr << "Warning: Could not remove temp file: " << e.what() << endl;
    }
    
    if (!removed) {
        cerr << "Warning: Failed to remove temporary file " << catFile << endl;
    }
    
    if (Cxy <= 0) {
        cerr << "Error: Failed to compress concatenated file." << endl;
        return 1.0;
    }
    
    // Compute NCD
    long Cmin = min(Cx, Cy);
    long Cmax = max(Cx, Cy);
    
    // Ensure valid values and handle edge cases
    if (Cmax == 0) {
        cerr << "Error: Maximum compressed size is zero." << endl;
        return 1.0;
    }
    
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
