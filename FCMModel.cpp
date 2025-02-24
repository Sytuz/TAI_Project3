#include "FCMModel.h"
#include <sstream>
#include "json.hpp"
#include <unordered_set>

using namespace std;
using json = nlohmann::json;

// JSON for serialization
FCMModel::FCMModel(): k(3), alpha(0.1) {}  // default
FCMModel::FCMModel(int k, double alpha): k(k), alpha(alpha) {}

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
    if (locked) return;

    for (std::size_t i = k; i < text.length(); ++i) {
        std::string context = text.substr(i - k, k);
        char symbol = text[i];

        // Add new characters to the alphabet dynamically
        alphabet.insert(symbol);

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
    locked = true;
}

void FCMModel::unlockModel(){
    locked = false;
}

int FCMModel::getAlphabetSize() const {
    return alphabet.size();
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
        return 1.0 / getAlphabetSize();
    }

    auto contextI = frequencyTable.find(context);
    if (contextI == frequencyTable.end()) {
        return 1.0 / getAlphabetSize();
    }

    auto symbolI = contextI->second.find(symbol);
    int count = (symbolI != contextI->second.end() ? symbolI->second : 0);
    int totalCount = contextCount.at(context);
    //int count = contextI->second.at(symbol);
    //int totalCount = contextCount.at(context);

    return (count + alpha) / (totalCount + alpha * getAlphabetSize());
}

void FCMModel::generateProbabilityTable(){
    probabilityTable.clear();

    for(const auto &contextPair : frequencyTable){
        const std::string &context = contextPair.first;
        int totalCount = contextCount[context];

        for (const auto &symbolPair : contextPair.second){
            char symbol = symbolPair.first;
            int count = symbolPair.second;

            probabilityTable[context][symbol] = (count + alpha) / (totalCount + alpha * getAlphabetSize());
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
        //std::cout << "Probability: " << probability << std::endl;
        totalInformation += -std::log2(static_cast<float>(probability));  // information content of each symbol
        symbolCount++;
    }

    return totalInformation / symbolCount;
}

//? Old code - generates the most probable symbol only (so the same one every time)
// char FCMModel::predict(const std::string &context) const {
//     // First check if the context exists in the probability table
//     auto contextIt = probabilityTable.find(context);
//     if (contextIt == probabilityTable.end()) {
//         // If context doesn't exist, return most common character or a default
//         if (!alphabet.empty()) {
//             return *alphabet.begin();  // Return first character in alphabet as fallback
//         }
//         return ' ';  // Default fallback
//     }

//     double maxProbability = 0.0;
//     char predictedSymbol = '\0';

//     for (const auto &symbolPair : contextIt->second) {
//         char symbol = symbolPair.first;
//         double probability = symbolPair.second;

//         if (probability > maxProbability) {
//             maxProbability = probability;
//             predictedSymbol = symbol;
//         }
//     }

//     // If we couldn't find a prediction (shouldn't happen with proper smoothing)
//     if (predictedSymbol == '\0' && !alphabet.empty()) {
//         return *alphabet.begin();
//     }

//     return predictedSymbol;
// }

char FCMModel::predict(const std::string &context) const {
    // First check if the context exists in the probability table
    auto contextIt = probabilityTable.find(context);
    if (contextIt == probabilityTable.end()) {
        // If context doesn't exist, return random character from alphabet
        if (!alphabet.empty()) {
            auto it = alphabet.begin();
            std::advance(it, rand() % alphabet.size());
            return *it;
        }
        return ' ';  // Default fallback
    }

    // Calculate cumulative probabilities for weighted random selection
    std::vector<std::pair<char, double>> cumulativeProbabilities;
    double sum = 0.0;

    for (const auto &symbolPair : contextIt->second) {
        sum += symbolPair.second;
        cumulativeProbabilities.push_back({symbolPair.first, sum});
    }

    // Generate random number between 0 and total probability
    double random = static_cast<double>(rand()) / RAND_MAX * sum;

    // Find the corresponding symbol using binary search
    for (const auto &pair : cumulativeProbabilities) {
        if (random <= pair.second) {
            return pair.first;
        }
    }

    // Fallback (shouldn't reach here with proper smoothing)
    return !alphabet.empty() ? *alphabet.begin() : ' ';
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
    // - alphabet: The alphabet.
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
    modelJson["alphabet"] = alphabet;
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
    // - alphabet: The alphabet.
    // - frequencyTable: The frequency table.
    // - probabilityTable: The probability table.
    // - contextCount: The context count.
    // - locked: Whether the model is locked or not.

    std::string modelJsonString = readFile(filename);
    json modelJson = json::parse(modelJsonString);

    k = modelJson["k"];
    alpha = modelJson["alpha"];
    alphabet = modelJson["alphabet"];
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