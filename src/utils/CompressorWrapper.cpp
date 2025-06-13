#include "../../include/utils/CompressorWrapper.h"
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <random>
#include <sstream>

using namespace std;

long CompressorWrapper::compressAndGetSize(const string& compressor, const string& inputFile) {
    // Generate unique temporary output filename
    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<> distrib(10000, 99999);
    string tempOut = filesystem::temp_directory_path().string() + "/tmp_" + to_string(distrib(gen)) + "_compressed";

    string cmd;
    if (compressor == "gzip") {
        cmd = "gzip -c -9 \"" + inputFile + "\" > \"" + tempOut + "\"";
    } else if (compressor == "bzip2") {
        cmd = "bzip2 -z -9 -c \"" + inputFile + "\" > \"" + tempOut + "\"";
    } else if (compressor == "lzma") {
        cmd = "lzma -9 -c \"" + inputFile + "\" > \"" + tempOut + "\"";
    } else if (compressor == "zstd") {
        cmd = "zstd -19 -q -c \"" + inputFile + "\" > \"" + tempOut + "\"";
    } else {
        cerr << "Unknown compressor: " << compressor << endl;
        return 0;
    }

    int ret = system(cmd.c_str());
    long size = 0;

    if (ret != 0) {
        cerr << "Compression failed with command: " << cmd << endl;
    } else {
        // Get file size
        try {
            size = filesystem::file_size(tempOut);
        } catch (const filesystem::filesystem_error& e) {
            cerr << "Error getting compressed file size: " << e.what() << endl;
            size = 0;
        }
    }

    // Clean up temp file
    try {
        if (filesystem::exists(tempOut)) {
            filesystem::remove(tempOut);
        }
    } catch (const filesystem::filesystem_error& e) {
        cerr << "Error removing temporary file: " << e.what() << endl;
    }

    return size;
}