#ifndef IO_UTILS_H
#define IO_UTILS_H

#include <string>
#include <vector>
#include <map>
#include "json.hpp"

using namespace std;
using json = nlohmann::json;

// Structure to hold reference sequence information
struct Reference
{
    string name;
    string sequence;
    double nrc = 0.0;
    double kld = 0.0;
    double compressionBits = 0.0;
};

// File reading functions
string readMetagenomicSample(const string &filename);
vector<Reference> readReferenceDatabase(const string &filename);

// Configuration parsing functions
bool parseConfigFile(const string &configFile, map<string, string> &configParams);
bool stringToBool(const string &value);

// Results saving functions
bool saveResults(const json& results, const string& timestampFile, const string& latestFile);
bool saveResultsToJson(const vector<pair<pair<int, double>, pair<vector<Reference>, double>>> &allResults,
                       const string &outputFile);
bool saveResultsToCsv(const vector<pair<pair<int, double>, pair<vector<Reference>, double>>> &allResults,
                      const string &outputFile);
bool saveAllResultsToJson(const vector<pair<pair<int, double>, pair<vector<Reference>, double>>> &allResults,
                          const string &outputFile);
bool saveAllResultsToCsv(const vector<pair<pair<int, double>, pair<vector<Reference>, double>>> &allResults,
                         const string &outputFile);

#endif // IO_UTILS_H