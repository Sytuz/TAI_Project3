#ifndef COMPRESSORWRAPPER_H
#define COMPRESSORWRAPPER_H

#include <string>

using namespace std;

/**
 * @brief CompressorWrapper calls external compressors to compress files and returns compressed size.
 */
class CompressorWrapper {
public:
    CompressorWrapper() = default;
    ~CompressorWrapper() = default;

    /**
     * @brief Compress file with specified compressor and return the size of compressed output.
     * Compressors supported: gzip, bzip2, lzma, zstd.
     * Temporary files are created and cleaned up.
     */
    long compressAndGetSize(const string& compressor, const string& inputFile);
};

#endif // COMPRESSORWRAPPER_H
