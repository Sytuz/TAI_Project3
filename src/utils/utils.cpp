#include "utils.h"
#include <iostream>
#include <fstream>
#include <cmath>
#include <unordered_map>
#include <algorithm>
#include "json.hpp"

using namespace std;
using json = nlohmann::json;

DNACompressor::DNACompressor(FCMModel& fcm) : model(fcm) {}

double DNACompressor::calculateBits(const string& sequence) const {
    double totalBits = 0.0;
    int k = model.getK();

    if (sequence.length() <= static_cast<size_t>(k)) {
        return 2.0 * sequence.length(); // Log2(4) = 2 bits per symbol for DNA
    }

    // Process each k+1 length substring
    for (size_t i = 0; i <= sequence.length() - static_cast<size_t>(k) - 1; ++i) {
        string context = sequence.substr(i, k);
        string nextSymbol = sequence.substr(i + k, 1);
        
        double probability = model.getProbability(context, nextSymbol);
        totalBits += -log2(probability);
    }

    return totalBits;
}

double DNACompressor::calculateNRC(const string& sequence) const {
    if (sequence.length() <= static_cast<size_t>(model.getK())) {
        return 1.0; // Handle short sequences
    }

    double bits = calculateBits(sequence);
    // For DNA: |x|*log2(A) = |x|*2 since alphabet size is 4
    return bits / (2.0 * sequence.length());
}

double DNACompressor::calculateKLD(const string& sequence) const {
    int k = model.getK();
    
    if (sequence.length() <= static_cast<size_t>(k)) {
        return 0.0; // Not enough data to calculate KLD
    }
    
    // Count k+1-grams in the sequence to compute empirical distribution
    unordered_map<string, unordered_map<char, int>> empiricalCounts;
    unordered_map<string, int> contextTotals;
    
    // Process the sequence to collect k+1-gram statistics
    for (size_t i = 0; i <= sequence.length() - static_cast<size_t>(k) - 1; ++i) {
        string context = sequence.substr(i, k);
        char nextSymbol = sequence[i + k];
        
        empiricalCounts[context][nextSymbol]++;
        contextTotals[context]++;
    }
    
    // Calculate KLD: D_KL(P||Q) = sum_x P(x) * log(P(x)/Q(x))
    double kld = 0.0;
    for (const auto& contextPair : empiricalCounts) {
        const string& context = contextPair.first;
        const auto& symbolCounts = contextPair.second;
        int total = contextTotals[context];
        
        for (const auto& symbolPair : symbolCounts) {
            char symbol = symbolPair.first;
            int count = symbolPair.second;
            
            // Calculate empirical probability P(x)
            double empiricalProb = static_cast<double>(count) / total;
            
            // Get model probability Q(x)
            string symbolStr(1, symbol);
            double modelProb = model.getProbability(context, symbolStr);
            
            // Accumulate KLD
            kld += empiricalProb * log2(empiricalProb / modelProb);
        }
    }
    
    return kld;
}

string readMetagenomicSample(const string& filename) {
    ifstream file(filename);
    string sample;
    string line;

    if (!file.is_open()) {
        cerr << "Error opening file: " << filename << endl;
        exit(1);
    }

    while (getline(file, line)) {
        // Process the line to keep only DNA nucleotides
        for (char c : line) {
            // Only keep A, C, G, T (case insensitive)
            if (c == 'A' || c == 'a' || c == 'C' || c == 'c' || 
                c == 'G' || c == 'g' || c == 'T' || c == 't') {
                sample += toupper(c);  // Convert to uppercase
            }
        }
    }

    file.close();
    return sample;
}

vector<Reference> readReferenceDatabase(const string& filename) {
    ifstream file(filename);
    vector<Reference> references;
    Reference reference;
    string line;

    if (!file.is_open()) {
        cerr << "Error opening file: " << filename << endl;
        exit(1);
    }

    while (getline(file, line)) {
        // Skip empty lines
        if (line.empty()) continue;

        // Lines starting with '@' indicate a new sequence name
        if (line[0] == '@') {
            // Save previous reference if exists
            if (!reference.name.empty() && !reference.sequence.empty()) {
                references.push_back(reference);
            }

            // Start a new reference
            reference.name = line.substr(1);    // Remove '@' character
            reference.sequence = "";
        }
        else {
            // Process the line to keep only DNA nucleotides
            for (char c : line) {
                // Only keep A, C, G, T (case insensitive)
                if (c == 'A' || c == 'a' || c == 'C' || c == 'c' || 
                    c == 'G' || c == 'g' || c == 'T' || c == 't') {
                    reference.sequence += toupper(c);  // Convert to uppercase
                }
            }
        }
    }

    // Add the last reference
    if (!reference.name.empty() && !reference.sequence.empty()) {
        references.push_back(reference);
    }

    file.close();
    return references;
}

// Convert string to boolean
bool stringToBool(const string& value) {
    string lowerValue = value;
    transform(lowerValue.begin(), lowerValue.end(), lowerValue.begin(), ::tolower);
    return (lowerValue == "true" || lowerValue == "yes" || lowerValue == "y" || lowerValue == "1");
}

// JSON parsing for configuration
bool parseConfigFile(const string& configFile, map<string, string>& configParams) {
    cout << "Reading JSON configuration from: " << configFile << endl;
    
    try {
        // Read the JSON file
        ifstream file(configFile);
        if (!file) {
            cerr << "Error: Could not open JSON configuration file: " << configFile << endl;
            return false;
        }
        
        json config;
        file >> config;
        
        // Extract parameters from JSON
        if (config.contains("input")) {
            if (config["input"].contains("sample_file"))
                configParams["sample_file"] = config["input"]["sample_file"];
            if (config["input"].contains("db_file"))
                configParams["db_file"] = config["input"]["db_file"];
        }
        
        if (config.contains("parameters")) {
            if (config["parameters"].contains("context_size")) {
                if (config["parameters"]["context_size"].contains("min"))
                    configParams["min_k"] = to_string(config["parameters"]["context_size"]["min"].get<int>());
                if (config["parameters"]["context_size"].contains("max"))
                    configParams["max_k"] = to_string(config["parameters"]["context_size"]["max"].get<int>());
            }
            
            if (config["parameters"].contains("alpha")) {
                if (config["parameters"]["alpha"].contains("min"))
                    configParams["min_alpha"] = to_string(config["parameters"]["alpha"]["min"].get<double>());
                if (config["parameters"]["alpha"].contains("max"))
                    configParams["max_alpha"] = to_string(config["parameters"]["alpha"]["max"].get<double>());
                if (config["parameters"]["alpha"].contains("ticks"))
                    configParams["alpha_ticks"] = to_string(config["parameters"]["alpha"]["ticks"].get<int>());
            }
        }
        
        if (config.contains("output")) {
            if (config["output"].contains("top_n"))
                configParams["top_n"] = to_string(config["output"]["top_n"].get<int>());
            if (config["output"].contains("use_json"))
                configParams["use_json"] = config["output"]["use_json"].get<bool>() ? "true" : "false";
        }
        
        if (config.contains("analysis")) {
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
        
        if (config.contains("model")) {
            if (config["model"].contains("test_save_load"))
                configParams["test_model_save_load"] = config["model"]["test_save_load"].get<bool>() ? "true" : "false";
            if (config["model"].contains("use_json"))
                configParams["use_json_model"] = config["model"]["use_json"].get<bool>() ? "true" : "false";
        }
        
        // Print all loaded configuration parameters
        cout << "Loaded configuration parameters:" << endl;
        for (const auto& param : configParams) {
            cout << "  " << param.first << " = " << param.second << endl;
        }
        
        return true;
    }
    catch (const json::exception& e) {
        cerr << "Error parsing JSON configuration file: " << e.what() << endl;
        return false;
    }
}

// Save results to JSON file with only top matches
bool saveResultsToJson(const vector<pair<pair<int, double>, pair<vector<Reference>, double>>>& allResults, 
                      const string& outputFile) {
    json resultsJson;
    
    for (size_t testIdx = 0; testIdx < allResults.size(); testIdx++) {
        const auto& test = allResults[testIdx];
        int k = test.first.first;
        double alpha = test.first.second;
        const auto& references = test.second.first;
        double execTime = test.second.second;
        
        json testJson;
        testJson["k"] = k;
        testJson["alpha"] = alpha;
        testJson["execTime_ms"] = execTime;
        
        json refsJson = json::array();
        for (size_t i = 0; i < references.size(); i++) {
            json refJson;
            refJson["rank"] = i + 1;
            refJson["name"] = references[i].name;
            refJson["nrc"] = references[i].nrc;
            refJson["kld"] = references[i].kld;
            refsJson.push_back(refJson);
        }
        
        testJson["references"] = refsJson;
        resultsJson.push_back(testJson);
    }
    
    ofstream file(outputFile);
    if (!file) {
        cerr << "Error: Could not open output file: " << outputFile << endl;
        return false;
    }
    
    file << setw(4) << resultsJson << endl;
    return true;
}

// Save results to CSV file with only top matches
bool saveResultsToCsv(const vector<pair<pair<int, double>, pair<vector<Reference>, double>>>& allResults, 
                     const string& outputFile) {
    ofstream file(outputFile);
    if (!file) {
        cerr << "Error: Could not open output file: " << outputFile << endl;
        return false;
    }
    
    // Write header
    file << "test_id,k,alpha,rank,reference_name,nrc,kld,exec_time_ms" << endl;
    
    for (size_t testIdx = 0; testIdx < allResults.size(); testIdx++) {
        const auto& test = allResults[testIdx];
        int k = test.first.first;
        double alpha = test.first.second;
        const auto& references = test.second.first;
        double execTime = test.second.second;
        
        for (size_t i = 0; i < references.size(); i++) {
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
bool saveAllResultsToJson(const vector<pair<pair<int, double>, pair<vector<Reference>, double>>>& allResults, 
                      const string& outputFile) {
    json resultsJson;
    
    for (size_t testIdx = 0; testIdx < allResults.size(); testIdx++) {
        const auto& test = allResults[testIdx];
        int k = test.first.first;
        double alpha = test.first.second;
        const auto& references = test.second.first;
        double execTime = test.second.second;
        
        json testJson;
        testJson["k"] = k;
        testJson["alpha"] = alpha;
        testJson["execTime_ms"] = execTime;
        
        json refsJson = json::array();
        for (size_t i = 0; i < references.size(); i++) {
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
    if (!file) {
        cerr << "Error: Could not open output file: " << outputFile << endl;
        return false;
    }
    
    file << setw(4) << resultsJson << endl;
    return true;
}

// Save results to CSV file with ALL organisms
bool saveAllResultsToCsv(const vector<pair<pair<int, double>, pair<vector<Reference>, double>>>& allResults, 
                     const string& outputFile) {
    ofstream file(outputFile);
    if (!file) {
        cerr << "Error: Could not open output file: " << outputFile << endl;
        return false;
    }
    
    // Write header with compression bits
    file << "test_id,k,alpha,rank,reference_name,nrc,kld,compression_bits,exec_time_ms" << endl;
    
    for (size_t testIdx = 0; testIdx < allResults.size(); testIdx++) {
        const auto& test = allResults[testIdx];
        int k = test.first.first;
        double alpha = test.first.second;
        const auto& references = test.second.first;
        double execTime = test.second.second;
        
        for (size_t i = 0; i < references.size(); i++) {
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

// Function to get valid integer input
int getIntInput(const string& prompt, int minValue = 1, int maxValue = 100) {
    int value;
    while (true) {
        cout << prompt;
        if (cin >> value && value >= minValue && value <= maxValue) {
            break;
        }
        cout << "Please enter a valid integer between " << minValue << " and " << maxValue << endl;
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
    }
    return value;
}

// Function to get valid double input
double getDoubleInput(const string& prompt, double minValue = 0.0, double maxValue = 1.0) {
    double value;
    while (true) {
        cout << prompt;
        if (cin >> value && value >= minValue && value <= maxValue) {
            break;
        }
        cout << "Please enter a valid number between " << minValue << " and " << maxValue << endl;
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
    }
    return value;
}

// Function to get string input
string getStringInput(const string& prompt) {
    string value;
    cout << prompt;
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
    getline(cin, value);
    return value;
}

// Function to ask yes/no question with yes as default
bool askYesNo(const string& prompt) {
    char response;
    while (true) {
        cout << prompt << " (Y/n): ";
        string line;
        getline(cin, line);
        
        // If user just pressed enter, default to yes
        if (line.empty()) {
            return true;
        }
        
        response = tolower(line[0]);
        if (response == 'y' || response == 'n') {
            break;
        }
        cout << "Please enter 'y' or 'n'" << endl;
    }
    return (response == 'y');
}