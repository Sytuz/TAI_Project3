#ifndef IO_UTILS_H
#define IO_UTILS_H

#include <string>
#include <vector>
#include <map>

// Structure to hold reference sequence information
struct Reference
{
    std::string name;
    std::string sequence;
    double nrc = 0.0;
    double kld = 0.0;
    double compressionBits = 0.0;
};

// File reading functions
std::string readMetagenomicSample(const std::string &filename);
std::vector<Reference> readReferenceDatabase(const std::string &filename);

// Configuration parsing functions
bool parseConfigFile(const std::string &configFile, std::map<std::string, std::string> &configParams);
bool stringToBool(const std::string &value);

// Results saving functions
bool saveResultsToJson(const std::vector<std::pair<std::pair<int, double>, std::pair<std::vector<Reference>, double>>> &allResults,
                       const std::string &outputFile);
bool saveResultsToCsv(const std::vector<std::pair<std::pair<int, double>, std::pair<std::vector<Reference>, double>>> &allResults,
                      const std::string &outputFile);
bool saveAllResultsToJson(const std::vector<std::pair<std::pair<int, double>, std::pair<std::vector<Reference>, double>>> &allResults,
                          const std::string &outputFile);
bool saveAllResultsToCsv(const std::vector<std::pair<std::pair<int, double>, std::pair<std::vector<Reference>, double>>> &allResults,
                         const std::string &outputFile);

#endif // IO_UTILS_H