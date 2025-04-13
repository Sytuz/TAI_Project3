#include "io_utils.h"
#include <iostream>
#include <fstream>
#include <algorithm>
#include <iomanip>
#include "json.hpp"

using namespace std;
using json = nlohmann::json;

string readMetagenomicSample(const string &filename)
{
    ifstream file(filename);
    string sample;
    string line;

    if (!file.is_open())
    {
        cerr << "Error opening file: " << filename << endl;
        exit(1);
    }

    while (getline(file, line))
    {
        // Process the line to keep only DNA nucleotides
        for (char c : line)
        {
            // Only keep A, C, G, T (case insensitive)
            if (c == 'A' || c == 'a' || c == 'C' || c == 'c' ||
                c == 'G' || c == 'g' || c == 'T' || c == 't')
            {
                sample += toupper(c); // Convert to uppercase
            }
        }
    }

    file.close();
    return sample;
}

vector<Reference> readReferenceDatabase(const string &filename)
{
    ifstream file(filename);
    vector<Reference> references;
    Reference reference;
    string line;

    if (!file.is_open())
    {
        cerr << "Error opening file: " << filename << endl;
        exit(1);
    }

    while (getline(file, line))
    {
        // Skip empty lines
        if (line.empty())
            continue;

        // Lines starting with '@' indicate a new sequence name
        if (line[0] == '@')
        {
            // Save previous reference if exists
            if (!reference.name.empty() && !reference.sequence.empty())
            {
                references.push_back(reference);
            }

            // Start a new reference
            reference.name = line.substr(1); // Remove '@' character
            reference.sequence = "";
        }
        else
        {
            // Process the line to keep only DNA nucleotides
            for (char c : line)
            {
                // Only keep A, C, G, T (case insensitive)
                if (c == 'A' || c == 'a' || c == 'C' || c == 'c' ||
                    c == 'G' || c == 'g' || c == 'T' || c == 't')
                {
                    reference.sequence += toupper(c); // Convert to uppercase
                }
            }
        }
    }

    // Add the last reference
    if (!reference.name.empty() && !reference.sequence.empty())
    {
        references.push_back(reference);
    }

    file.close();
    return references;
}

// JSON parsing for configuration
bool parseConfigFile(const string &configFile, map<string, string> &configParams)
{
    cout << "Reading JSON configuration from: " << configFile << endl;

    try
    {
        // Read the JSON file
        ifstream file(configFile);
        if (!file)
        {
            cerr << "Error: Could not open JSON configuration file: " << configFile << endl;
            return false;
        }

        json config;
        file >> config;

        // Extract parameters from JSON
        if (config.contains("input"))
        {
            if (config["input"].contains("sample_file"))
                configParams["sample_file"] = config["input"]["sample_file"];
            if (config["input"].contains("db_file"))
                configParams["db_file"] = config["input"]["db_file"];
        }

        if (config.contains("parameters"))
        {
            if (config["parameters"].contains("context_size"))
            {
                if (config["parameters"]["context_size"].contains("min"))
                    configParams["min_k"] = to_string(config["parameters"]["context_size"]["min"].get<int>());
                if (config["parameters"]["context_size"].contains("max"))
                    configParams["max_k"] = to_string(config["parameters"]["context_size"]["max"].get<int>());
            }

            if (config["parameters"].contains("alpha"))
            {
                if (config["parameters"]["alpha"].contains("min"))
                    configParams["min_alpha"] = to_string(config["parameters"]["alpha"]["min"].get<double>());
                if (config["parameters"]["alpha"].contains("max"))
                    configParams["max_alpha"] = to_string(config["parameters"]["alpha"]["max"].get<double>());
                if (config["parameters"]["alpha"].contains("ticks"))
                    configParams["alpha_ticks"] = to_string(config["parameters"]["alpha"]["ticks"].get<int>());
            }
        }

        if (config.contains("output"))
        {
            if (config["output"].contains("top_n"))
                configParams["top_n"] = to_string(config["output"]["top_n"].get<int>());
            if (config["output"].contains("use_json"))
                configParams["use_json"] = config["output"]["use_json"].get<bool>() ? "true" : "false";
        }

        if (config.contains("analysis"))
        {
            if (config["analysis"].contains("analyze_symbol_info"))
                configParams["analyze_symbol_info"] = config["analysis"]["analyze_symbol_info"].get<bool>() ? "true" : "false";
            if (config["analysis"].contains("num_orgs_to_analyze"))
                configParams["num_orgs_to_analyze"] = to_string(config["analysis"]["num_orgs_to_analyze"].get<int>());
            if (config["analysis"].contains("analyze_chunks"))
                configParams["analyze_chunks"] = config["analysis"]["analyze_chunks"].get<bool>() ? "true" : "false";
            if (config["analysis"].contains("chunk_size"))
                configParams["chunk_size"] = to_string(config["analysis"]["chunk_size"].get<int>());
            if (config["analysis"].contains("chunk_overlap"))
                configParams["chunk_overlap"] = to_string(config["analysis"]["chunk_overlap"].get<int>());
        }

        if (config.contains("model"))
        {
            if (config["model"].contains("test_save_load"))
                configParams["test_model_save_load"] = config["model"]["test_save_load"].get<bool>() ? "true" : "false";
            if (config["model"].contains("use_json"))
                configParams["use_json_model"] = config["model"]["use_json"].get<bool>() ? "true" : "false";
        }

        // Print all loaded configuration parameters
        cout << "Loaded configuration parameters:" << endl;
        for (const auto &param : configParams)
        {
            cout << "  " << param.first << " = " << param.second << endl;
        }

        return true;
    }
    catch (const json::exception &e)
    {
        cerr << "Error parsing JSON configuration file: " << e.what() << endl;
        return false;
    }
}

// Convert string to boolean
bool stringToBool(const string &value)
{
    string lowerValue = value;
    transform(lowerValue.begin(), lowerValue.end(), lowerValue.begin(), ::tolower);
    return (lowerValue == "true" || lowerValue == "yes" || lowerValue == "y" || lowerValue == "1");
}

// Save results to CSV file with only top matches
bool saveResultsToCsv(const vector<pair<pair<int, double>, pair<vector<Reference>, double>>> &allResults,
                      const string &outputFile)
{
    ofstream file(outputFile);
    if (!file)
    {
        cerr << "Error: Could not open output file: " << outputFile << endl;
        return false;
    }

    // Write header
    file << "test_id,k,alpha,rank,reference_name,nrc,kld,exec_time_ms" << endl;

    for (size_t testIdx = 0; testIdx < allResults.size(); testIdx++)
    {
        const auto &test = allResults[testIdx];
        int k = test.first.first;
        double alpha = test.first.second;
        const auto &references = test.second.first;
        double execTime = test.second.second;

        for (size_t i = 0; i < references.size(); i++)
        {
            file << testIdx + 1 << ","
                 << k << ","
                 << alpha << ","
                 << i + 1 << ","
                 << "\"" << references[i].name << "\"" << ","
                 << references[i].nrc << ","
                 << references[i].kld << ","
                 << execTime << endl;
        }
    }

    return true;
}

// Save results to JSON file with ALL organisms
bool saveAllResultsToJson(const vector<pair<pair<int, double>, pair<vector<Reference>, double>>> &allResults,
                          const string &outputFile)
{
    json resultsJson;

    for (size_t testIdx = 0; testIdx < allResults.size(); testIdx++)
    {
        const auto &test = allResults[testIdx];
        int k = test.first.first;
        double alpha = test.first.second;
        const auto &references = test.second.first;
        double execTime = test.second.second;

        json testJson;
        testJson["k"] = k;
        testJson["alpha"] = alpha;
        testJson["execTime_ms"] = execTime;

        json refsJson = json::array();
        for (size_t i = 0; i < references.size(); i++)
        {
            json refJson;
            refJson["rank"] = i + 1;
            refJson["name"] = references[i].name;
            refJson["nrc"] = references[i].nrc;
            refJson["kld"] = references[i].kld;
            refJson["compressionBits"] = references[i].compressionBits;
            refsJson.push_back(refJson);
        }

        testJson["references"] = refsJson;
        resultsJson.push_back(testJson);
    }

    ofstream file(outputFile);
    if (!file)
    {
        cerr << "Error: Could not open output file: " << outputFile << endl;
        return false;
    }

    file << setw(4) << resultsJson << endl;
    return true;
}

// Save results to CSV file with ALL organisms
bool saveAllResultsToCsv(const vector<pair<pair<int, double>, pair<vector<Reference>, double>>> &allResults,
                         const string &outputFile)
{
    ofstream file(outputFile);
    if (!file)
    {
        cerr << "Error: Could not open output file: " << outputFile << endl;
        return false;
    }

    // Write header with compression bits
    file << "test_id,k,alpha,rank,reference_name,nrc,kld,compression_bits,exec_time_ms" << endl;

    for (size_t testIdx = 0; testIdx < allResults.size(); testIdx++)
    {
        const auto &test = allResults[testIdx];
        int k = test.first.first;
        double alpha = test.first.second;
        const auto &references = test.second.first;
        double execTime = test.second.second;

        for (size_t i = 0; i < references.size(); i++)
        {
            file << testIdx + 1 << ","
                 << k << ","
                 << alpha << ","
                 << i + 1 << ","
                 << "\"" << references[i].name << "\"" << ","
                 << references[i].nrc << ","
                 << references[i].kld << ","
                 << references[i].compressionBits << ","
                 << execTime << endl;
        }
    }

    return true;
}