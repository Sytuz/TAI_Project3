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

/* string readFile(const std::string &filename){
    ifstream file(filename);
    if(!file){
        throw runtime_error("The file " + filename + " could not be opened!");
    }

    stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
} */

/* string readFile(const std::string &filename){
    ifstream file(filename);
    if(!file){
        throw runtime_error("The file " + filename + " could not be opened!");
    }

    stringstream buffer;
    buffer << file.rdbuf();

    cout << "Read " << buffer.str().length() << " characters from " << filename << endl;

    return buffer.str();
} */

std::string readFile(const std::string &filename) {
    std::ifstream file(filename, std::ios::binary); // Open in binary mode
    if (!file) {
        throw std::runtime_error("The file " + filename + " could not be opened!");
    }

    std::ostringstream buffer;
    buffer << file.rdbuf();
    
    std::string content = buffer.str();
    std::cout << "Read " << content.length() << " characters from " << filename << std::endl;

    return content;
}

// Class methods
void FCMModel::learn(const std::string &text){
    if (locked) return;

    // We need to ensure we're working with complete UTF-8 characters
    std::vector<std::string> characters;
    
    // First, split the text into proper UTF-8 characters
    for (size_t i = 0; i < text.length(); /* incremented in the loop */) {
        int charLen = 1;
        
        // Determine UTF-8 character length based on first byte
        unsigned char firstByte = static_cast<unsigned char>(text[i]);
        if ((firstByte & 0x80) == 0) {
            charLen = 1;  // ASCII character
        } else if ((firstByte & 0xE0) == 0xC0) {
            charLen = 2;  // 2-byte UTF-8
        } else if ((firstByte & 0xF0) == 0xE0) {
            charLen = 3;  // 3-byte UTF-8
        } else if ((firstByte & 0xF8) == 0xF0) {
            charLen = 4;  // 4-byte UTF-8
        }
        
        // Check if we have a complete character
        if (i + charLen <= text.length()) {
            characters.push_back(text.substr(i, charLen));
        }
        
        i += charLen;
    }
    
    // Now process the characters
    for (size_t i = k; i < characters.size(); ++i) {
        std::string context;
        for (size_t j = i - k; j < i; ++j) {
            context += characters[j];
        }
        
        std::string symbol = characters[i];
        
        // Add new characters to the alphabet dynamically
        // Note: we're now storing full UTF-8 characters in the alphabet
        alphabet.insert(symbol);  // This might need adjustment based on your alphabet definition
        
        frequencyTable[context][symbol]++;  // This might need adjustment based on how you store frequencies
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

double FCMModel::getProbability(const std::string &context, const std::string &symbol) const {
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

    return (count + alpha) / (totalCount + alpha * getAlphabetSize());
}

void FCMModel::generateProbabilityTable(){
    probabilityTable.clear();

    for(const auto &contextPair : frequencyTable){
        const std::string &context = contextPair.first;
        int totalCount = contextCount[context];

        for (const auto &symbolPair : contextPair.second){
            const std::string &symbol = symbolPair.first;
            int count = symbolPair.second;

            probabilityTable[context][symbol] = (count + alpha) / (totalCount + alpha * getAlphabetSize());
        }
    }
}

// uses Shannon entropy (average information content per symbol)
double FCMModel::computeAverageInformationContent(const std::string &text) const{
    // First split the text into UTF-8 characters
    std::vector<std::string> characters = splitIntoUTF8Characters(text);
    
    if(characters.size() <= static_cast<std::size_t>(k)){
        return 0.0;
    }

    double totalInformation = 0.0;
    int symbolCount = 0;

    for(std::size_t i = k; i < characters.size(); i++){
        std::string context;
        for(std::size_t j = i - k; j < i; j++){
            context += characters[j];
        }
        
        std::string symbol = characters[i];

        double probability = getProbability(context, symbol);
        totalInformation += -std::log2(static_cast<float>(probability));  // information content of each symbol
        symbolCount++;
    }

    return totalInformation / symbolCount;
}

std::string FCMModel::predict(const std::string &context) const {
    // First check if the context exists in the probability table
    auto contextIt = probabilityTable.find(context);
    if (contextIt == probabilityTable.end()) {
        // If context doesn't exist, return random character from alphabet
        if (!alphabet.empty()) {
            auto it = alphabet.begin();
            std::advance(it, rand() % alphabet.size());
            return *it;
        }
        return " ";  // Default fallback
    }

    // Calculate cumulative probabilities for weighted random selection
    std::vector<std::pair<std::string, double>> cumulativeProbabilities;
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
    return !alphabet.empty() ? *alphabet.begin() : " ";
}

std::string FCMModel::predict(const std::string &initialContext, int n) const{
    // First split the initial context into UTF-8 characters
    std::vector<std::string> contextChars = splitIntoUTF8Characters(initialContext);
    
    // Ensure we have enough characters for the context
    if (contextChars.size() < static_cast<size_t>(k)) {
        // Handle error or pad with spaces
        while (contextChars.size() < static_cast<size_t>(k)) {
            contextChars.insert(contextChars.begin(), " ");
        }
    }
    
    // Take the last k characters as our starting context
    std::string rollingContext;
    for (size_t i = contextChars.size() - k; i < contextChars.size(); i++) {
        rollingContext += contextChars[i];
    }
    
    std::string prediction = rollingContext;
    std::string result = prediction;

    for(int i = 0; i < n; i++){
        std::string nextSymbol = predict(rollingContext);
        result += nextSymbol;
        
        // Update the rolling context by removing the first character and adding the new one
        std::vector<std::string> contextChars = splitIntoUTF8Characters(rollingContext);
        rollingContext = "";
        
        // Skip the first character and add all others plus the new symbol
        for (size_t j = 1; j < contextChars.size(); j++) {
            rollingContext += contextChars[j];
        }
        rollingContext += nextSymbol;
    }

    return result;
}

void FCMModel::exportModel(const std::string &filename) {
    // Open file in binary mode to ensure exact byte writing.
    std::ofstream file(filename, std::ios::binary);
    if(!file){
        throw std::runtime_error("The file " + filename + " could not be opened!");
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

    // Dump JSON with indent of 4 spaces and write to file
    file << modelJson.dump(4, ' ', true) << std::endl;
    // No need to explicitly call close(); destructor will flush and close.
}

void FCMModel::importModel(const std::string &filename){    
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

// Helper method to split a string into UTF-8 characters
std::vector<std::string> FCMModel::splitIntoUTF8Characters(const std::string &text) const {
    std::vector<std::string> characters;
    
    for (size_t i = 0; i < text.length(); /* incremented in the loop */) {
        int charLen = 1;
        
        // Determine UTF-8 character length based on first byte
        unsigned char firstByte = static_cast<unsigned char>(text[i]);
        if ((firstByte & 0x80) == 0) {
            charLen = 1;  // ASCII character
        } else if ((firstByte & 0xE0) == 0xC0) {
            charLen = 2;  // 2-byte UTF-8
        } else if ((firstByte & 0xF0) == 0xE0) {
            charLen = 3;  // 3-byte UTF-8
        } else if ((firstByte & 0xF8) == 0xF0) {
            charLen = 4;  // 4-byte UTF-8
        }
        
        // Check if we have a complete character
        if (i + charLen <= text.length()) {
            characters.push_back(text.substr(i, charLen));
        }
        
        i += charLen;
    }
    
    return characters;
}