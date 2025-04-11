#include "FCMModel.h"
#include "utils.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <chrono>
#include <cmath>
#include <string>
#include <algorithm>
#include <filesystem>
#include <iomanip>
#include <thread>
#include <mutex>
#include "json.hpp"

using namespace std;
using namespace std::chrono;
using json = nlohmann::json;


// Function to process reference batches in parallel
void processReferenceBatch(vector<Reference>& references, size_t start, size_t end, DNACompressor& compressor, mutex& mtx) {
    for (size_t i = start; i < end && i < references.size(); ++i) {
        double nrc = compressor.calculateNRC(references[i].sequence);
        double kld = compressor.calculateKLD(references[i].sequence);
        
        // Calculate compression size in bits (non-normalized bits required for compression)
        double compressionBits = nrc * references[i].sequence.length();

        // Thread-safe update of references
        lock_guard<mutex> lock(mtx);
        references[i].nrc = nrc;
        references[i].kld = kld;
        references[i].compressionBits = compressionBits; // Store the new metric
    }
}

// Utility function to read DNA sequences
string readDNASequence(const string& filename) {
    ifstream file(filename);
    string sequence;
    string line;

    if (!file.is_open()) {
        cerr << "Error opening file: " << filename << endl;
        return "";
    }

    while (getline(file, line)) {
        for (char c : line) {
            if (c == 'A' || c == 'a' || c == 'C' || c == 'c' || 
                c == 'G' || c == 'g' || c == 'T' || c == 't') {
                sequence += toupper(c);
            }
        }
    }

    file.close();
    return sequence;
}

// Run test with specific parameters and return results
vector<Reference> runTest(const string& sampleFile, const string& dbFile, int k, double alpha, int topN, double& execTime) {
    auto startTime = high_resolution_clock::now();
    
    string sample = readDNASequence(sampleFile);
    vector<Reference> references = readReferenceDatabase(dbFile);
    
    if (sample.empty() || references.empty()) {
        cerr << "Error: Empty sample or database" << endl;
        return {};
    }
    
    cout << "Running test with k=" << k << ", alpha=" << alpha << endl;
    cout << "Sample length: " << sample.length() << " nucleotides" << endl;
    cout << "Number of references: " << references.size() << endl;
    
    // Train model on sample
    FCMModel model(k, alpha);
    model.learn(sample);
    model.lockModel();

    // Calculate NRC in parallel for each reference
    DNACompressor compressor(model);

    // Get system thread with a minimum of 2
    unsigned int threadCount = max(2u, thread::hardware_concurrency());
    cout << "Using " << threadCount << " threads for parallel processing" << endl;

    vector<thread> threads;
    mutex mtx;

    size_t batchSize = (references.size() + threadCount - 1) / threadCount;

    for (unsigned int t = 0; t < threadCount; ++t) {
        size_t start = t * batchSize;
        size_t end = min((t + 1) * batchSize, references.size());

        if (start >= references.size()) break;

        threads.push_back(thread(processReferenceBatch,
                                ref(references),
                                start,
                                end,
                                ref(compressor),
                                ref(mtx)
        ));
    }

    // Wait for all threads to complete
    for (auto& t: threads) {
        t.join();
    }

    // Sort by NRC (lowest to highest = most similar to least similar)
    sort(references.begin(), references.end(), 
         [](const Reference& a, const Reference& b) { return a.nrc < b.nrc; });
    
    auto endTime = high_resolution_clock::now();
    execTime = duration<double, milli>(endTime - startTime).count();
    
    return references;
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

// Test model saving and loading
void testModelSaveLoad(const string& sampleFile, const string& outFile, bool useJson = false) {
    cout << "\nTesting model saving and loading:" << endl;
    cout << "--------------------------------" << endl;
    
    // Train model
    string sample = readDNASequence(sampleFile);
    if (sample.empty()) {
        cerr << "Error: Empty sample" << endl;
        return;
    }
    
    FCMModel model(10, 0.1);
    model.learn(sample);
    
    // Save model
    cout << "Saving model to: " << outFile << endl;
    string savedFile = model.exportModel(outFile, !useJson);
    cout << "Model saved as: " << savedFile << endl;
    
    // Load model
    FCMModel loadedModel;
    cout << "Loading model from: " << savedFile << endl;
    loadedModel.importModel(savedFile, !useJson);
    
    cout << "Original model parameters: k=" << model.getK() << ", alpha=" << model.getAlpha() << endl;
    cout << "Loaded model parameters: k=" << loadedModel.getK() << ", alpha=" << loadedModel.getAlpha() << endl;
    
    // Verify by comparing a few probabilities
    string testContext = "ACGT";
    string testSymbol = "A";
    
    if (sample.length() >= 5) {
        testContext = sample.substr(0, 4);
        testSymbol = string(1, sample[4]);
    }
    
    double origProb = model.getProbability(testContext, testSymbol);
    double loadedProb = loadedModel.getProbability(testContext, testSymbol);
    
    cout << "Test probability for '" << testContext << "' → '" << testSymbol << "':" << endl;
    cout << "  Original model: " << origProb << endl;
    cout << "  Loaded model: " << loadedProb << endl;
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

// Generate a vector of evenly spaced alpha values
vector<double> generateAlphaValues(double minAlpha, double maxAlpha, int numTicks) {
    vector<double> alphas;
    
    if (numTicks <= 1) {
        alphas.push_back(minAlpha);
        return alphas;
    }
    
    double step = (maxAlpha - minAlpha) / (numTicks - 1);
    for (int i = 0; i < numTicks; i++) {
        alphas.push_back(minAlpha + i * step);
    }
    
    return alphas;
}

// New function to analyze and export symbol information for reference sequences
bool analyzeSymbolInformation(const string& sampleFile, const vector<Reference>& topRefs, int k, double alpha, 
                            const string& timestampSymbolDir, const string& latestSymbolDir) {
    cout << "\n=============================================" << endl;
    cout << "Analyzing symbol information for top matches" << endl;
    cout << "=============================================" << endl;
    
    string sample = readDNASequence(sampleFile);
    if (sample.empty()) {
        cerr << "Error: Empty sample" << endl;
        return false;
    }
    
    // Create FCM model from sample
    FCMModel model(k, alpha);
    model.learn(sample);
    model.lockModel();
    
    // Generate timestamp for unique filenames
    auto now = system_clock::now();
    time_t now_time = system_clock::to_time_t(now);
    tm* now_tm = localtime(&now_time);
    
    char timeStr[20];
    strftime(timeStr, sizeof(timeStr), "%Y%m%d_%H%M%S", now_tm);
    
    // Process each top reference
    vector<string> exportedFiles;
    for (size_t i = 0; i < topRefs.size(); i++) {
        cout << "Processing " << topRefs[i].name << " (rank " << i+1 << ")..." << endl;
        
        // Create base filename with organism name and rank
        string safeName = topRefs[i].name;
        // Replace problematic characters in filename
        for (char& c : safeName) {
            if (c == ' ' || c == '/' || c == '\\' || c == ':' || c == '*' || c == '?' || c == '"' || c == '<' || c == '>' || c == '|') {
                c = '_';
            }
        }
        
        string baseTimestampFile = timestampSymbolDir + "/" + "rank" + to_string(i+1) + "_" + safeName;
        string baseLatestFile = latestSymbolDir + "/" + "rank" + to_string(i+1) + "_" + safeName;
        
        try {
            // Export symbol information to both directories
            string exportedTimestampFile = model.exportSymbolInformation(topRefs[i].sequence, baseTimestampFile);
            string exportedLatestFile = model.exportSymbolInformation(topRefs[i].sequence, baseLatestFile);
            
            exportedFiles.push_back(exportedTimestampFile);
            
            // Calculate average information content for comparison
            double avgInfo = model.computeAverageInformationContent(topRefs[i].sequence);
            cout << "  Average information content: " << fixed << setprecision(6) << avgInfo << " bits/symbol" << endl;
            cout << "  Symbol information exported to: " << endl;
            cout << "    - " << exportedTimestampFile << endl;
            cout << "    - " << exportedLatestFile << endl;
        }
        catch (const exception& e) {
            cerr << "  Error processing reference: " << e.what() << endl;
        }
    }
    
    if (!exportedFiles.empty()) {
        cout << "\nExported symbol information for " << exportedFiles.size() << " references." << endl;
        cout << "Files saved to directories: " << endl;
        cout << "  - " << filesystem::absolute(timestampSymbolDir).string() << endl;
        cout << "  - " << filesystem::absolute(latestSymbolDir).string() << endl;
        return true;
    }
    
    return false;
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

// Convert string to boolean
bool stringToBool(const string& value) {
    string lowerValue = value;
    transform(lowerValue.begin(), lowerValue.end(), lowerValue.begin(), ::tolower);
    return (lowerValue == "true" || lowerValue == "yes" || lowerValue == "y" || lowerValue == "1");
}

// Function to print usage instructions
void printUsage(const string& programName) {
    cout << "Usage: " << programName << " [--config <config_file_path>]" << endl;
    cout << "If --config is provided, the program will use parameters from the specified JSON file." << endl;
    cout << "Otherwise, it will run in interactive mode." << endl;
    cout << "\nExample JSON configuration file format:" << endl;
    cout << "{\n";
    cout << "  \"input\": {\n";
    cout << "    \"sample_file\": \"samples/meta.txt\",\n";
    cout << "    \"db_file\": \"samples/db.txt\"\n";
    cout << "  },\n";
    cout << "  \"parameters\": {\n";
    cout << "    \"context_size\": {\n";
    cout << "      \"min\": 3,\n";
    cout << "      \"max\": 6\n";
    cout << "    },\n";
    cout << "    \"alpha\": {\n";
    cout << "      \"min\": 0.001,\n";
    cout << "      \"max\": 0.5,\n";
    cout << "      \"ticks\": 5\n";
    cout << "    }\n";
    cout << "  },\n";
    cout << "  \"output\": {\n";
    cout << "    \"top_n\": 10,\n";
    cout << "    \"use_json\": true\n";
    cout << "  },\n";
    cout << "  \"analysis\": {\n";
    cout << "    \"analyze_symbol_info\": true,\n";
    cout << "    \"num_orgs_to_analyze\": 3,\n";
    cout << "    \"analyze_chunks\": true,\n";
    cout << "    \"chunk_size\": 5000,\n";
    cout << "    \"chunk_overlap\": 1000\n";
    cout << "  }\n";
    cout << "}" << endl;
}

// Function to analyze sample chunks and determine best matching organisms
bool analyzeChunks(const string& sampleFile, const vector<Reference>& references, int k, double alpha,
                 int chunkSize, int overlap, const string& timestampDir, const string& latestDir) {
    // Read the sample DNA sequence
    string sample = readDNASequence(sampleFile);
    if (sample.empty()) {
        cerr << "Error: Empty sample" << endl;
        return false;
    }
    
    // Create chunks with overlap
    vector<pair<int, string>> chunks;  // position, sequence
    for (int i = 0; i <= static_cast<int>(sample.length()) - chunkSize; i += (chunkSize - overlap)) {
        chunks.push_back({i, sample.substr(i, chunkSize)});
    }
    
    cout << "\nDivided sample into " << chunks.size() << " chunks (size: " << chunkSize 
         << ", overlap: " << overlap << " nucleotides)" << endl;
    
    // Compute NRC for each chunk against each reference
    json resultsJson;
    resultsJson["k"] = k;
    resultsJson["alpha"] = alpha;
    resultsJson["chunk_size"] = chunkSize;
    resultsJson["overlap"] = overlap;
    resultsJson["sample_length"] = sample.length();
    resultsJson["chunk_count"] = chunks.size();
    
    json chunksJson = json::array();
    
    for (size_t i = 0; i < chunks.size(); ++i) {
        const auto& chunk = chunks[i];
        
        // Train model on this chunk
        FCMModel chunkModel(k, alpha);
        chunkModel.learn(chunk.second);
        chunkModel.lockModel();
        DNACompressor compressor(chunkModel);
        
        json chunkJson;
        chunkJson["position"] = chunk.first;
        chunkJson["length"] = chunk.second.length();
        
        // Find best matching reference for this chunk
        vector<pair<string, double>> chunkResults;
        double bestNrc = numeric_limits<double>::max();
        string bestReference = "";
        
        for (const auto& ref : references) {
            double nrc = compressor.calculateNRC(ref.sequence);
            chunkResults.push_back({ref.name, nrc});
            
            if (nrc < bestNrc) {
                bestNrc = nrc;
                bestReference = ref.name;
            }
        }
        
        chunkJson["best_match"] = bestReference;
        chunkJson["best_nrc"] = bestNrc;
        
        // Add top 3 matches
        json matchesJson = json::array();
        sort(chunkResults.begin(), chunkResults.end(), 
             [](const auto& a, const auto& b) { return a.second < b.second; });
        
        for (size_t j = 0; j < min(size_t(3), chunkResults.size()); ++j) {
            json matchJson;
            matchJson["name"] = chunkResults[j].first;
            matchJson["nrc"] = chunkResults[j].second;
            matchesJson.push_back(matchJson);
        }
        
        chunkJson["top_matches"] = matchesJson;
        chunksJson.push_back(chunkJson);
        
        // Progress report
        if (i % 10 == 0 || i == chunks.size() - 1) {
            cout << "\rProcessed " << i + 1 << " of " << chunks.size() << " chunks (" 
                 << fixed << setprecision(1) << (100.0 * (i + 1) / chunks.size()) << "%)" << flush;
        }
    }
    cout << endl;
    
    resultsJson["chunks"] = chunksJson;
    
    // Save to both timestamp and latest directories
    string timestampFile = timestampDir + "/chunk_analysis.json";
    string latestFile = latestDir + "/chunk_analysis.json";
    
    ofstream fileTimestamp(timestampFile);
    if (!fileTimestamp) {
        cerr << "Error: Could not open output file: " << timestampFile << endl;
        return false;
    }
    
    ofstream fileLatest(latestFile);
    if (!fileLatest) {
        cerr << "Error: Could not open output file: " << latestFile << endl;
        return false;
    }
    
    fileTimestamp << setw(4) << resultsJson << endl;
    fileLatest << setw(4) << resultsJson << endl;
    
    cout << "Chunk analysis saved to: " << endl;
    cout << "- " << timestampFile << endl;
    cout << "- " << latestFile << endl;
    
    return true;
}

// Main function with support for config file
int main(int argc, char** argv) {
    cout << "===============================================" << endl;
    cout << "   MetaClass NRC Parameter Testing Utility    " << endl;
    cout << "===============================================" << endl;
    
    // Check for command-line arguments
    bool useConfigFile = false;
    string configFilePath;
    
    for (int i = 1; i < argc; i++) {
        string arg = argv[i];
        if (arg == "--config" && i + 1 < argc) {
            useConfigFile = true;
            configFilePath = argv[i + 1];
            i++; // Skip the next argument as we've already processed it
        } else if (arg == "--help" || arg == "-h") {
            printUsage(argv[0]);
            return 0;
        }
    }
    
    // Default values
    string sampleFile = "samples/meta.txt";
    string dbFile = "samples/db.txt";
    int minK = 3;
    int maxK = 6;
    double minAlpha = 0.001;
    double maxAlpha = 0.5;
    int alphaTicks = 5;
    int topN = 10;
    bool useJson = true;
    bool analyzeSymbolInfo = false;
    int numOrgsToAnalyze = 3;
    bool runModelSaveLoadTest = false;  // Renamed from testModelSaveLoad to avoid name collision with function
    bool useJsonModel = true;
    bool shouldAnalyzeChunks = false;  // Renamed from analyzeChunks to avoid conflict with function
    int chunkSize = 5000;
    int chunkOverlap = 1000;
    
    if (useConfigFile) {
        // Parse config file
        map<string, string> configParams;
        if (!parseConfigFile(configFilePath, configParams)) {
            cerr << "Failed to parse configuration file. Exiting." << endl;
            return 1;
        }
        
        // Extract configuration parameters
        if (configParams.count("sample_file")) sampleFile = configParams["sample_file"];
        if (configParams.count("db_file")) dbFile = configParams["db_file"];
        if (configParams.count("min_k")) minK = stoi(configParams["min_k"]);
        if (configParams.count("max_k")) maxK = stoi(configParams["max_k"]);
        if (configParams.count("min_alpha")) minAlpha = stod(configParams["min_alpha"]);
        if (configParams.count("max_alpha")) maxAlpha = stod(configParams["max_alpha"]);
        if (configParams.count("alpha_ticks")) alphaTicks = stoi(configParams["alpha_ticks"]);
        if (configParams.count("top_n")) topN = stoi(configParams["top_n"]);
        if (configParams.count("use_json")) useJson = stringToBool(configParams["use_json"]);
        if (configParams.count("analyze_symbol_info")) analyzeSymbolInfo = stringToBool(configParams["analyze_symbol_info"]);
        if (configParams.count("num_orgs_to_analyze")) numOrgsToAnalyze = stoi(configParams["num_orgs_to_analyze"]);
        if (configParams.count("test_model_save_load")) runModelSaveLoadTest = stringToBool(configParams["test_model_save_load"]);
        if (configParams.count("use_json_model")) useJsonModel = stringToBool(configParams["use_json_model"]);
        if (configParams.count("analyze_chunks")) shouldAnalyzeChunks = stringToBool(configParams["analyze_chunks"]);
        if (configParams.count("chunk_size")) chunkSize = stoi(configParams["chunk_size"]);
        if (configParams.count("chunk_overlap")) chunkOverlap = stoi(configParams["chunk_overlap"]);
        
        // Validate paths
        if (!filesystem::exists(sampleFile)) {
            cerr << "Error: Sample file does not exist: " << sampleFile << endl;
            return 1;
        }
        
        if (!filesystem::exists(dbFile)) {
            cerr << "Error: Database file does not exist: " << dbFile << endl;
            return 1;
        }
        
        cout << "\nRunning with parameters from configuration file." << endl;
    } else {
        // Interactive mode - use existing code flow
        // Check if default files exist
        if (!filesystem::exists(sampleFile) || !filesystem::exists(dbFile)) {
            cout << "Default test files not found." << endl;
            sampleFile = getStringInput("Enter metagenomic sample file path: ");
            dbFile = getStringInput("Enter reference database file path: ");
        } else {
            cout << "Default files found:" << endl;
            cout << "- Sample: " << sampleFile << endl;
            cout << "- Database: " << dbFile << endl;
            
            if (!askYesNo("Use default files?")) {
                sampleFile = getStringInput("Enter metagenomic sample file path: ");
                dbFile = getStringInput("Enter reference database file path: ");
            }
        }
        
        // Get parameter ranges
        cout << "\nParameter Range Setup:" << endl;
        cout << "----------------------" << endl;
        minK = getIntInput("Enter minimum context size (k): ", 1, 20);
        maxK = getIntInput("Enter maximum context size (k): ", minK, 20);
        
        minAlpha = getDoubleInput("Enter minimum alpha value: ", 0.0, 1.0);
        maxAlpha = getDoubleInput("Enter maximum alpha value: ", minAlpha, 1.0);
        alphaTicks = getIntInput("Enter number of alpha values to test (1-20): ", 1, 20);
        
        topN = getIntInput("Enter number of top matches to save for each test: ", 1, 100);
        
        // Ask for output format
        useJson = askYesNo("\nSave results as JSON? (No for CSV)");
    }

    // Generate all parameter combinations
    vector<int> kValues;
    for (int k = minK; k <= maxK; k++) {
        kValues.push_back(k);
    }
    
    vector<double> alphaValues = generateAlphaValues(minAlpha, maxAlpha, alphaTicks);
    
    // Calculate total number of tests
    int totalTests = kValues.size() * alphaValues.size();
    cout << "\nWill perform " << totalTests << " tests (" 
         << kValues.size() << " k-values × " << alphaValues.size() << " alpha-values)" << endl;
    
    // Setup result directories
    string baseOutputDir = "results";
    
    // Create results directory if it doesn't exist
    if (!filesystem::exists(baseOutputDir)) {
        filesystem::create_directory(baseOutputDir);
    }
    
    // Generate timestamp for directory name
    auto now = system_clock::now();
    time_t now_time = system_clock::to_time_t(now);
    tm* now_tm = localtime(&now_time);
    
    char timeStr[20];
    strftime(timeStr, sizeof(timeStr), "%Y%m%d_%H%M%S", now_tm);
    string timestamp = string(timeStr);
    
    // Create timestamped directory and latest directory
    string timestampDir = baseOutputDir + "/" + timestamp;
    string latestDir = baseOutputDir + "/latest";
    
    // Create directories
    if (filesystem::exists(latestDir)) {
        filesystem::remove_all(latestDir);
    }
    filesystem::create_directory(timestampDir);
    filesystem::create_directory(latestDir);
    
    // Create symbol_info directories
    filesystem::create_directory(timestampDir + "/symbol_info");
    filesystem::create_directory(latestDir + "/symbol_info");
    
    // Define filenames
    string extension = useJson ? ".json" : ".csv";
    string timestampFilename = timestampDir + "/test_results" + extension;
    string latestFilename = latestDir + "/test_results" + extension;
    
    // Add new filenames for top organisms results
    string topOrgTimestampFilename = timestampDir + "/top_organisms_results" + extension;
    string topOrgLatestFilename = latestDir + "/top_organisms_results" + extension;
    
    cout << "Output will be saved to:" << endl;
    cout << "- " << timestampFilename << " (all organisms)" << endl;
    cout << "- " << topOrgTimestampFilename << " (top matches only)" << endl;
    
    // Track total test time
    auto totalTestStartTime = high_resolution_clock::now();
    
    // Run all tests and collect results
    vector<pair<pair<int, double>, pair<vector<Reference>, double>>> allResults;
    vector<pair<pair<int, double>, pair<vector<Reference>, double>>> topResults;
    
    int testCounter = 0;
    
    for (int k : kValues) {
        for (double alpha : alphaValues) {
            testCounter++;
            cout << "\n[Test " << testCounter << "/" << totalTests << "] Running with k=" << k << ", alpha=" << alpha << endl;
            
            double execTime;
            vector<Reference> results = runTest(sampleFile, dbFile, k, alpha, topN, execTime);
            
            if (!results.empty()) {
                // Store all results (all references)
                allResults.push_back({{k, alpha}, {results, execTime}});
                
                // Create a copy of only the top N results for the top organisms file
                vector<Reference> topMatchesOnly;
                for (size_t i = 0; i < min(static_cast<size_t>(topN), results.size()); i++) {
                    topMatchesOnly.push_back(results[i]);
                }
                topResults.push_back({{k, alpha}, {topMatchesOnly, execTime}});
                
                // Display brief results for this test
                cout << "Completed test for k=" << k << ", alpha=" << alpha << " in " << execTime << " ms" << endl;
                cout << "Top match: " << results[0].name << " (NRC: " << fixed << setprecision(6) << results[0].nrc << ")" << endl;
            }
            
            cout << "Progress: " << testCounter << "/" << totalTests << " tests completed (" 
                 << fixed << setprecision(1) << (100.0 * testCounter / totalTests) << "%)" << endl;
        }
    }
    
    // Calculate total test time
    auto totalTestEndTime = high_resolution_clock::now();
    double totalTestTime = duration<double, milli>(totalTestEndTime - totalTestStartTime).count();
    
    // Display detailed results for all tests
    cout << "\n\n===============================================" << endl;
    cout << "                  Results                     " << endl;
    cout << "===============================================" << endl;
    
    // Group results by k value for clearer presentation
    map<int, vector<pair<double, pair<vector<Reference>, double>>>> resultsByK;
    for (const auto& result : allResults) {
        int k = result.first.first;
        double alpha = result.first.second;
        resultsByK[k].push_back({alpha, result.second});
    }
    
    for (const auto& kGroup : resultsByK) {
        cout << "\nResults for context size k=" << kGroup.first << ":" << endl;
        cout << "----------------------------------------" << endl;
        
        for (const auto& alphaResult : kGroup.second) {
            double alpha = alphaResult.first;
            const auto& references = alphaResult.second.first;
            double execTime = alphaResult.second.second;
            
            cout << "Alpha=" << fixed << setprecision(4) << alpha << " (exec time: " << execTime << " ms):" << endl;
            cout << "  Top 3 matches:" << endl;
            
            // Show top 3 or fewer if not available
            for (size_t i = 0; i < min(size_t(3), references.size()); i++) {
                cout << "    " << i+1 << ". " << references[i].name 
                     << " (NRC: " << fixed << setprecision(6) << references[i].nrc 
                     << ", KLD: " << references[i].kld << ")" << endl;
            }
        }
    }
    
    // Save results to both directories - now save both full and top results
    bool saved = false;
    if (useJson) {
        // Save top organisms results (previous behavior)
        saved = saveResultsToJson(topResults, topOrgTimestampFilename) && 
                saveResultsToJson(topResults, topOrgLatestFilename);
                
        // Save all organisms results with compression bits
        saved = saved && 
                saveAllResultsToJson(allResults, timestampFilename) && 
                saveAllResultsToJson(allResults, latestFilename);
    } else {
        // Save top organisms results (previous behavior)
        saved = saveResultsToCsv(topResults, topOrgTimestampFilename) && 
                saveResultsToCsv(topResults, topOrgLatestFilename);
                
        // Save all organisms results with compression bits
        saved = saved && 
                saveAllResultsToCsv(allResults, timestampFilename) && 
                saveAllResultsToCsv(allResults, latestFilename);
    }
    
    // Create info.txt with test parameters in both directories
    ofstream infoTimestamp(timestampDir + "/info.txt");
    ofstream infoLatest(latestDir + "/info.txt");
    
    if (infoTimestamp && infoLatest) {
        // Write test parameters to both files
        for (ofstream& infoFile : {ref(infoTimestamp), ref(infoLatest)}) {
            infoFile << "Test Parameters" << endl;
            infoFile << "===============" << endl;
            infoFile << "Date and Time: " << timestamp << endl;
            infoFile << "Sample file: " << sampleFile << endl;
            infoFile << "Database file: " << dbFile << endl;
            infoFile << "Context sizes (k): " << minK << " to " << maxK << endl;
            infoFile << "Alpha values: " << minAlpha << " to " << maxAlpha << " (" << alphaTicks << " ticks)" << endl;
            infoFile << "Top matches saved: " << topN << endl;
            infoFile << "Total tests: " << totalTests << endl;
            infoFile << "Total test time: " << fixed << setprecision(2) << (totalTestTime / 1000.0) << " seconds" << endl;
            infoFile << "Output format: " << (useJson ? "JSON" : "CSV") << endl;
        }
        
        infoTimestamp.close();
        infoLatest.close();
    }

        
    // Ask if user wants to analyze symbol information for top organisms
    if (!allResults.empty() && (analyzeSymbolInfo || (!useConfigFile && askYesNo("\nWould you like to analyze symbol information for top organisms?")))) {
        // Get best k and alpha values (simplistic approach - use the first test's values)
        int bestK = allResults[0].first.first;
        double bestAlpha = allResults[0].first.second;
        
        // Find best test result based on top match's NRC
        double bestNrc = std::numeric_limits<double>::max();
        size_t bestTestIndex = 0;
        
        for (size_t i = 0; i < allResults.size(); i++) {
            if (!allResults[i].second.first.empty() && allResults[i].second.first[0].nrc < bestNrc) {
                bestNrc = allResults[i].second.first[0].nrc;
                bestTestIndex = i;
                bestK = allResults[i].first.first;
                bestAlpha = allResults[i].first.second;
            }
        }
        
        cout << "\nUsing best performing parameters: k=" << bestK << ", alpha=" << bestAlpha << endl;
        
        // Determine how many top organisms to analyze
        int numOrgs;
        if (useConfigFile) {
            numOrgs = std::min(numOrgsToAnalyze, static_cast<int>(allResults[bestTestIndex].second.first.size()));
        } else {
            numOrgs = std::min(getIntInput("How many top organisms to analyze? (1-5): ", 1, 5), 
                          static_cast<int>(allResults[bestTestIndex].second.first.size()));
        }
        
        vector<Reference> topRefs(allResults[bestTestIndex].second.first.begin(),
                                 allResults[bestTestIndex].second.first.begin() + numOrgs);
        
        // Pass both symbol_info directories to the function
        analyzeSymbolInformation(sampleFile, topRefs, bestK, bestAlpha, 
                               timestampDir + "/symbol_info", latestDir + "/symbol_info");
    }   
    
    if (saved) {
        cout << "\nResults successfully saved to both directories" << endl;
    } else {
        cerr << "\nFailed to save results" << endl;
    }
    
    // Ask if user wants to test model saving/loading
    if (runModelSaveLoadTest || (!useConfigFile && askYesNo("\nWould you like to test model saving and loading?"))) {
        string modelOutfile = "test_model";
        bool jsonModel = useConfigFile ? useJsonModel : askYesNo("Use JSON format for model? (No for binary)");
        testModelSaveLoad(sampleFile, modelOutfile, jsonModel);
    }
    
    // Ask if user wants to analyze chunks
    if (shouldAnalyzeChunks || (!useConfigFile && askYesNo("\nWould you like to analyze sample chunks?"))) {
        // Get best k and alpha values (simplistic approach - use the first test's values)
        int bestK = allResults[0].first.first;
        double bestAlpha = allResults[0].first.second;
        
        // Find best test result based on top match's NRC
        double bestNrc = std::numeric_limits<double>::max();
        size_t bestTestIndex = 0;
        
        for (size_t i = 0; i < allResults.size(); i++) {
            if (!allResults[i].second.first.empty() && allResults[i].second.first[0].nrc < bestNrc) {
                bestNrc = allResults[i].second.first[0].nrc;
                bestTestIndex = i;
                bestK = allResults[i].first.first;
                bestAlpha = allResults[i].first.second;
            }
        }
        
        cout << "\nUsing best performing parameters: k=" << bestK << ", alpha=" << bestAlpha << endl;
        
        // Determine chunk size and overlap
        if (!useConfigFile) {
            chunkSize = getIntInput("Enter chunk size: ", 100, 100000);
            chunkOverlap = getIntInput("Enter chunk overlap: ", 0, chunkSize - 1);
        }
        
        // Perform chunk analysis
        analyzeChunks(sampleFile, allResults[bestTestIndex].second.first, bestK, bestAlpha, 
                      chunkSize, chunkOverlap, timestampDir, latestDir);
    }
    
    cout << "\nTesting complete!" << endl;
    return 0;
}