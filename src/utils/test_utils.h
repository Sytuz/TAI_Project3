#ifndef TEST_UTILS_H
#define TEST_UTILS_H

#include <vector>
#include <string>
#include <utility>
#include "json.hpp"
#include "io_utils.h" // Include io_utils.h to get the Reference struct

using namespace std;
using json = nlohmann::json;

//Generates a vector of alpha values evenly distributed between min and max.
vector<double> generateAlphaValues(double minAlpha, double maxAlpha, int numTicks);

// Analyzes symbol information for top references and saves results.
bool analyzeSymbolInformation(const string& sampleFile,
                             const vector<Reference>& topRefs,
                             int k, 
                             double alpha,
                             const string& timestampSymbolDir,
                             const string& latestSymbolDir);

// Creates chunks from a sequence with specified size and overlap.
vector<pair<int, string>> createChunks(const string& sequence,
                                     int& chunkSize, 
                                     int& overlap);

// Analyzes a single chunk of DNA.
json analyzeChunk(const string& chunk,
                          int position, 
                          int k, 
                          double alpha, 
                          const vector<Reference>& refs);

// Analyzes chunks of a sample file against references.
bool analyzeChunks(const string& sampleFile,
                  const vector<Reference>& references,
                  int k, 
                  double alpha,
                  int chunkSize, 
                  int overlap, 
                  const string& timestampDir,
                  const string& latestDir);

// Evaluates synthetic data with known ground truth.
void evaluateSyntheticData(const string& sampleFile,
                          const string& dbFile,
                          const string& groundTruthFile,
                          int k, 
                          double alpha, 
                          double threshold);

// Performs cross-comparison between top organisms.
void performCrossComparison(const vector<Reference>& topReferences,
                          int k, 
                          double alpha,
                          const string& timestampDir,
                          const string& latestDir);

#endif // TEST_UTILS_H