#include "../../include/core/NCD.h"
#include "../../include/utils/CompressorWrapper.h"
#include <filesystem>
#include <iostream>
#include <fstream>

using namespace std;

double NCD::computeNCD(const string& file1, const string& file2, const string& compressor) {
    CompressorWrapper cw;
    // Compute compressed sizes
    long Cx = cw.compressAndGetSize(compressor, file1);
    long Cy = cw.compressAndGetSize(compressor, file2);
    // Create concatenated file
    string catFile = "tmp_cat";
    {
        ofstream out(catFile, ios::binary);
        ifstream in1(file1, ios::binary), in2(file2, ios::binary);
        out << in1.rdbuf() << in2.rdbuf();
    }
    long Cxy = cw.compressAndGetSize(compressor, catFile);
    filesystem::remove(catFile);
    // Formula NCD = (C(xy)-min(Cx,Cy)) / max(Cx, Cy)
    long Cmin = min(Cx, Cy);
    long Cmax = max(Cx, Cy);
    if (Cmax == 0) return 0.0;
    return double(Cxy - Cmin) / double(Cmax);
}

vector<vector<double>> NCD::computeMatrix(const vector<string>& files, const string& compressor) {
    int n = files.size();
    vector<vector<double>> mat(n, vector<double>(n, 0.0));
    for (int i = 0; i < n; ++i) {
        mat[i][i] = 0.0;
        for (int j = i+1; j < n; ++j) {
            double d = computeNCD(files[i], files[j], compressor);
            mat[i][j] = mat[j][i] = d;
            cout << "NCD("<<i<<","<<j<<") = " << d << endl;
        }
    }
    return mat;
}
