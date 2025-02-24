#include "FCMModel.h"
#include <sstream>
#include "json.hpp"
#include <unordered_set>

using namespace std;
using json = nlohmann::json;

// JSON for serialization
FCMModel::FCMModel(): k(3), alpha(0.1), alphabetSize(0) {}  // default
FCMModel::FCMModel(int k, double alpha): k(k), alpha(alpha), alphabetSize(0) {}

// Helper functions
int getAlphabetSize(const std::string &text){
    unordered_set<char> uniqueChars(text.begin(), text.end());
    return uniqueChars.size();
}

string readFile(const std::string &filename){
    ifstream file(filename);
    if(!file){
        throw runtime_error("The file " + filename + " could not be opened!");
    }

    stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

// Class methods
void FCMModel::learn(const std::string &text){

    if(locked){
        return;
    }

    alphabetSize = getAlphabetSize(text);

    for(std::size_t i = k; i < text.length(); ++i){
        std::string context = text.substr(i-k, k);
        char symbol = text[i];

        frequencyTable[context][symbol]++;
        contextCount[context]++;
    }

    generateProbabilityTable();
}

void FCMModel::clearModel(){
    if (locked) {
        return;
    }

    frequencyTable.clear();
    probabilityTable.clear();
    contextCount.clear();
}

void FCMModel::lockModel(){
    generateProbabilityTable();
    locked = 1;
}

void FCMModel::unlockModel(){
    locked = 0;
}

double FCMModel::getProbability(const std::string &context, char symbol) const {
    if (locked) {
        auto contextI = probabilityTable.find(context);
        if (contextI != probabilityTable.end()) {
            auto symbolI = contextI->second.find(symbol);
            if (symbolI != contextI->second.end()) {
                return symbolI->second;
            }
        }
        return 1.0 / alphabetSize;
    }

    auto contextI = frequencyTable.find(context);
    if (contextI == frequencyTable.end()) {
        return 1.0 / alphabetSize;
    }

    int count = contextI->second.at(symbol);
    int totalCount = contextCount.at(context);

    return (count + alpha) / (totalCount + alpha * alphabetSize);
}

void FCMModel::generateProbabilityTable(){
    probabilityTable.clear();

    for(const auto &contextPair : frequencyTable){
        const std::string &context = contextPair.first;
        int totalCount = contextCount[context];

        for (const auto &symbolPair : contextPair.second){
            char symbol = symbolPair.first;
            int count = symbolPair.second;

            probabilityTable[context][symbol] = (count + alpha) / (totalCount + alpha * alphabetSize);
        }
    }
}

// uses Shannon entropy (average information content per symbol)
double FCMModel::computeAverageInformationContent(const std::string &text) const{
    if(text.length() <= static_cast<std::size_t>(k)){
        return 0.0;
    }

    double totalInformation = 0.0;
    int symbolCount = 0;

    for(std::size_t i = k; i < text.length(); i++){
        std::string context = text.substr(i-k, k);
        char symbol = text[i];

        double probability = getProbability(context, symbol);
        totalInformation += -std::log2(static_cast<float>(probability));  // information content of each symbol
        symbolCount++;
    }

    return totalInformation / symbolCount;
}

char FCMModel::predict(const std::string &context) const{
    // Use the probability table to predict the next symbol
    double maxProbability = 0.0;
    char predictedSymbol = '\0';

    for(const auto &symbolPair : probabilityTable.at(context)){
        char symbol = symbolPair.first;
        double probability = symbolPair.second;

        if(probability > maxProbability){
            maxProbability = probability;
            predictedSymbol = symbol;
        }
    }

    return predictedSymbol;
}

string FCMModel::predict(const std::string &context, int n) const{
    std::string prediction = context;

    for(int i = 0; i < n; i++){
        char symbol = predict(prediction);
        prediction += symbol;
    }

    return prediction;
}

void FCMModel::exportModel(const std::string &filename){
    // Export the model to a JSON file
    // The JSON file should contain the following fields:
    // - k: The order of the model (length of the context).
    // - alpha: The smoothing parameter.
    // - alphabetSize: The size of the alphabet.
    // - frequencyTable: The frequency table.
    // - probabilityTable: The probability table.
    // - contextCount: The context count.
    // - locked: Whether the model is locked or not.

    // Create the file if it does not exist
    ofstream file(filename);
    if(!file){
        throw runtime_error("The file " + filename + " could not be opened!");
    }

    json modelJson;
    modelJson["k"] = k;
    modelJson["alpha"] = alpha;
    modelJson["alphabetSize"] = alphabetSize;
    modelJson["locked"] = locked;

    for(const auto &contextPair : frequencyTable){
        const std::string &context = contextPair.first;
        modelJson["frequencyTable"][context] = contextPair.second;
    }

    for(const auto &contextPair : probabilityTable){
        const std::string &context = contextPair.first;
        modelJson["probabilityTable"][context] = contextPair.second;
    }

    modelJson["contextCount"] = contextCount;

    file << modelJson.dump(4);
    file.close();
}

void FCMModel::importModel(const std::string &filename){
    // Import the model from a JSON file
    // The JSON file should contain the following fields:
    // - k: The order of the model (length of the context).
    // - alpha: The smoothing parameter.
    // - alphabetSize: The size of the alphabet.
    // - frequencyTable: The frequency table.
    // - probabilityTable: The probability table.
    // - contextCount: The context count.
    // - locked: Whether the model is locked or not.

    std::string modelJsonString = readFile(filename);
    json modelJson = json::parse(modelJsonString);

    k = modelJson["k"];
    alpha = modelJson["alpha"];
    alphabetSize = modelJson["alphabetSize"];
    locked = modelJson["locked"];

    frequencyTable.clear();
    probabilityTable.clear();
    contextCount.clear();

    for(const auto &contextPair : modelJson["frequencyTable"].items()){
        const std::string &context = contextPair.key();
        frequencyTable[context] = contextPair.value();
    }

    for(const auto &contextPair : modelJson["probabilityTable"].items()){
        const std::string &context = contextPair.key();
        probabilityTable[context] = contextPair.value();
    }

    contextCount = modelJson["contextCount"];
 }