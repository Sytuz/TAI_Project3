#include "../../include/utils/CompressorWrapper.h"
#include <cstdlib>
#include <filesystem>
#include <iostream>

using namespace std;

long CompressorWrapper::compressAndGetSize(const string& compressor, const string& inputFile) {
    string tempOut = "tmp_compressed";
    string cmd;
    if (compressor == "gzip") {
        cmd = "gzip -c \"" + inputFile + "\" > " + tempOut;
    } else if (compressor == "bzip2") {
        cmd = "bzip2 -z -c \"" + inputFile + "\" > " + tempOut;
    } else if (compressor == "lzma") {
        cmd = "lzma -c \"" + inputFile + "\" > " + tempOut;
    } else if (compressor == "zstd") {
        cmd = "zstd -q -c \"" + inputFile + "\" > " + tempOut;
    } else {
        cerr << "Unknown compressor: " << compressor << endl;
        return 0;
    }
    int ret = system(cmd.c_str());
    if (ret != 0) {
        cerr << "Compression failed with command: " << cmd << endl;
        return 0;
    }
    long size = filesystem::file_size(tempOut);
    filesystem::remove(tempOut);
    return size;
}
