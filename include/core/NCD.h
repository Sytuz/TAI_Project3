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
    /**
     * Compute the normalized compression distance between two files
     * @param file1 Path to the first file
     * @param file2 Path to the second file
     * @param compressor Name of the compressor to use (gzip, bzip2, etc.)
     * @return The NCD value between 0.0 and 1.0 (smaller means more similar)
     */
    double computeNCD(const string& file1, const string& file2, const string& compressor);
    
    /**
     * Compute the NCD matrix for a set of files
     * @param files Vector of file paths
     * @param compressor Name of the compressor to use
     * @return Matrix of NCD values between each pair of files
     */
    vector<vector<double>> computeMatrix(const vector<string>& files, const string& compressor);
};

#endif