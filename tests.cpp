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
#include "json.hpp"

using namespace std;
using namespace std::chrono;
using json = nlohmann::json;

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
    
    // Calculate NRC for each reference
    DNACompressor compressor(model);
    for (auto& ref : references) {
        ref.nrc = compressor.calculateNRC(ref.sequence);
        ref.kld = compressor.calculateKLD(ref.sequence);
    }
    
    // Sort by NRC (lowest to highest = most similar to least similar)
    sort(references.begin(), references.end(), 
         [](const Reference& a, const Reference& b) { return a.nrc < b.nrc; });
    
    // Keep only top N results
    if (topN < static_cast<int>(references.size())) {
        references.resize(topN);
    }
    
    auto endTime = high_resolution_clock::now();
    execTime = duration<double, milli>(endTime - startTime).count();
    
    return references;
}

// Save results to JSON file
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

// Save results to CSV file
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

// Main function with interactive menu
int main() {
    cout << "===============================================" << endl;
    cout << "   MetaClass NRC Parameter Testing Utility    " << endl;
    cout << "===============================================" << endl;
    
    // Default files
    string sampleFile = "samples/meta.txt";
    string dbFile = "samples/db.txt";
    
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
    int minK = getIntInput("Enter minimum context size (k): ", 1, 20);
    int maxK = getIntInput("Enter maximum context size (k): ", minK, 20);
    
    double minAlpha = getDoubleInput("Enter minimum alpha value: ", 0.0, 1.0);
    double maxAlpha = getDoubleInput("Enter maximum alpha value: ", minAlpha, 1.0);
    int alphaTicks = getIntInput("Enter number of alpha values to test (1-20): ", 1, 20);
    
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
    
    int topN = getIntInput("Enter number of top matches to save for each test: ", 1, 100);
    
    // Ask for output format and file
    bool useJson = askYesNo("\nSave results as JSON? (No for CSV)");
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
    
    cout << "Output will be saved to:" << endl;
    cout << "- " << timestampFilename << endl;
    cout << "- " << latestFilename << endl;
    
    // Run all tests and collect results
    vector<pair<pair<int, double>, pair<vector<Reference>, double>>> allResults;
    
    int testCounter = 0;
    
    for (int k : kValues) {
        for (double alpha : alphaValues) {
            testCounter++;
            cout << "\n[Test " << testCounter << "/" << totalTests << "] Running with k=" << k << ", alpha=" << alpha << endl;
            
            double execTime;
            vector<Reference> results = runTest(sampleFile, dbFile, k, alpha, topN, execTime);
            
            if (!results.empty()) {
                allResults.push_back({{k, alpha}, {results, execTime}});
                
                // Display brief results for this test
                cout << "Completed test for k=" << k << ", alpha=" << alpha << " in " << execTime << " ms" << endl;
                cout << "Top match: " << results[0].name << " (NRC: " << fixed << setprecision(6) << results[0].nrc << ")" << endl;
            }
            
            cout << "Progress: " << testCounter << "/" << totalTests << " tests completed (" 
                 << fixed << setprecision(1) << (100.0 * testCounter / totalTests) << "%)" << endl;
        }
    }
    
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
    
    // Save results to both directories
    bool saved = false;
    if (useJson) {
        saved = saveResultsToJson(allResults, timestampFilename) && 
                saveResultsToJson(allResults, latestFilename);
    } else {
        saved = saveResultsToCsv(allResults, timestampFilename) && 
                saveResultsToCsv(allResults, latestFilename);
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
            infoFile << "Output format: " << (useJson ? "JSON" : "CSV") << endl;
        }
        
        infoTimestamp.close();
        infoLatest.close();
    }

        
    // Ask if user wants to analyze symbol information for top organisms
    if (!allResults.empty() && askYesNo("\nWould you like to analyze symbol information for top organisms?")) {
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
        
        // Ask how many top organisms to analyze
        int numOrgs = std::min(getIntInput("How many top organisms to analyze? (1-5): ", 1, 5), 
                         static_cast<int>(allResults[bestTestIndex].second.first.size()));
        
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
    if (askYesNo("\nWould you like to test model saving and loading?")) {
        string modelOutfile = "test_model";
        bool useJsonModel = askYesNo("Use JSON format for model? (No for binary)");
        testModelSaveLoad(sampleFile, modelOutfile, useJsonModel);
    }
    
    cout << "\nTesting complete!" << endl;
    return 0;
}