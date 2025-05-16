#ifndef NCD_H
#define NCD_H

#include <string>
#include <vector>

using namespace std;

/**
 * @brief NCD computes the normalized compression distance between files.
 * Uses an external compressor selected by name (gzip, bzip2, etc.).
 */
class NCD {
public:
    NCD() = default;
    ~NCD() = default;

    /**
     * @brief Compute pairwise NCD matrix for given feature files.
     * @param files List of feature filenames.
     * @param compressor Name of compressor ("gzip", "bzip2", "lzma", "zstd").
     * @return Matrix NxN of NCD values.
     */
    vector<vector<double>> computeMatrix(const vector<string>& files, const string& compressor);

private:
    /**
     * @brief Compute NCD distance between two files.
     */
    double computeNCD(const string& file1, const string& file2, const string& compressor);

    /**
     * @brief Helper: get compressed size of file with compressor.
     */
    long compressedSize(const string& filename, const string& compressor);
};

#endif // NCD_H
