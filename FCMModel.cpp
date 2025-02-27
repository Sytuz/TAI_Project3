#include "FCMModel.h"
#include <sstream>
#include "json.hpp"
#include <unordered_set>
#include <regex>

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

bool FCMModel::isModelEmpty() const {
    return frequencyTable.empty() && probabilityTable.empty();
}

std::size_t FCMModel::getContextCount() const {
    return frequencyTable.size();
}

std::size_t FCMModel::getTotalTransitionCount() const {
    std::size_t total = 0;
    for (const auto &contextPair : contextCount) {
        total += contextPair.second;
    }
    return total;
}

// Class methods
void FCMModel::learn(const std::string &text) {
    if (locked) {
        std::cout << "Cannot learn: model is locked" << std::endl;
        return;  // Early return if locked - consider throwing an exception instead
    }
    
    if (text.empty()) {
        std::cout << "Cannot learn: input text is empty" << std::endl;
        return;  // Early return if text is empty
    }
    
    std::cout << "Learning from text of length: " << text.length() << " characters..." << std::endl;
    
    // Split text into UTF-8 characters
    std::vector<std::string> characters = splitIntoUTF8Characters(text);
    std::cout << "Split into " << characters.size() << " UTF-8 characters" << std::endl;
    
    // We need at least k+1 characters to create a context and a following symbol
    if (characters.size() <= static_cast<std::size_t>(k)) {
        std::cout << "Text too short for model order k=" << k << std::endl;
        return;
    }
    
    int contextProcessed = 0;
    
    // Process each sequence of k characters to predict the next one
    for (std::size_t i = 0; i <= characters.size() - k - 1; ++i) {
        // Build context from k consecutive characters
        std::string context;
        for (std::size_t j = i; j < i + k; ++j) {
            context += characters[j];
        }
        
        // The next character after the context
        std::string symbol = characters[i + k];
        
        // Add to alphabet
        alphabet.insert(symbol);
        
        // Update frequency tables
        frequencyTable[context][symbol]++;
        contextCount[context]++;
        
        contextProcessed++;
        
        // Optional: add progress indicator for large texts
        if (contextProcessed % 10000 == 0) {
            std::cout << "Processed " << contextProcessed << " contexts..." << std::endl;
        }
    }
    
    std::cout << "Learning complete. Processed " << contextProcessed << " contexts." << std::endl;
    std::cout << "Model now contains " << frequencyTable.size() << " unique contexts." << std::endl;
    std::cout << "Alphabet size: " << alphabet.size() << " unique symbols." << std::endl;
    
    // Generate probability table after learning
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

    // If the model is locked, use the probability table
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

    // If the model is unlocked, apply Laplace smoothing on the frequency table
    auto contextI = frequencyTable.find(context);
    if (contextI == frequencyTable.end()) {
        return 1.0 / getAlphabetSize();
    }

    auto symbolI = contextI->second.find(symbol);
    int count = (symbolI != contextI->second.end() ? symbolI->second : 0);
    int totalCount = contextCount.at(context);

    return (count + alpha) / (totalCount + alpha * getAlphabetSize());
}

void FCMModel::generateProbabilityTable() {
    std::cout << "Generating probability table..." << std::endl;
    
    // Clear the current probability table
    probabilityTable.clear();
    
    // If the model is empty, there's nothing to do
    if (frequencyTable.empty()) {
        std::cout << "Frequency table is empty, no probabilities to calculate." << std::endl;
        return;
    }
    
    int contextCount = 0;
    
    // Iterate over each context in the frequency table
    for (const auto &contextPair : frequencyTable) {
        const std::string &context = contextPair.first;
        int totalCount = this->contextCount[context];
        
        if (totalCount <= 0) {
            std::cout << "Warning: Context '" << context << "' has zero or negative count." << std::endl;
            continue;
        }
        
        // Calculate probability for each symbol in this context
        for (const auto &symbolPair : contextPair.second) {
            const std::string &symbol = symbolPair.first;
            int count = symbolPair.second;
            
            // Apply Laplace smoothing
            double probability = (count + alpha) / (totalCount + alpha * getAlphabetSize());
            probabilityTable[context][symbol] = probability;
        }
        
        contextCount++;
    }
    
    std::cout << "Probability table generated for " << contextCount << " contexts." << std::endl;
}

// uses Shannon entropy (average information content per symbol)
double FCMModel::computeAverageInformationContent(const std::string &text) const{

    // First split the text into UTF-8 characters
    std::vector<std::string> characters = splitIntoUTF8Characters(text);
    
    // If the model is empty or the text is too short, return 0.0
    if(frequencyTable.empty() || characters.size() <= static_cast<std::size_t>(k)){
        return 0.0;
    } 

    double totalInformation = 0.0;
    int symbolCount = 0;

    // Iterate over each symbol in the text
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

std::string FCMModel::predict(const std::string &initialContext, int n) const {
    std::cout << "Starting prediction with context: '" << initialContext << "'" << std::endl;
    
    // First split the initial context into UTF-8 characters
    std::vector<std::string> contextChars = splitIntoUTF8Characters(initialContext);
    std::cout << "Context has " << contextChars.size() << " characters, model k=" << k << std::endl;
    
    // Ensure we have exactly k characters for the context
    if (contextChars.size() > static_cast<std::size_t>(k)) {
        // If we have more than k, take only the last k
        std::vector<std::string> tempChars;
        for (std::size_t i = contextChars.size() - k; i < contextChars.size(); i++) {
            tempChars.push_back(contextChars[i]);
        }
        contextChars = tempChars;
        std::cout << "Trimmed context to last " << k << " characters" << std::endl;
    } else if (contextChars.size() < static_cast<std::size_t>(k)) {
        // If we have fewer than k, pad with spaces
        while (contextChars.size() < static_cast<std::size_t>(k)) {
            contextChars.insert(contextChars.begin(), " ");
        }
        std::cout << "Padded context to " << k << " characters" << std::endl;
    }
    
    // Rebuild context string from the adjusted character array
    std::string rollingContext;
    for (const auto& c : contextChars) {
        rollingContext += c;
    }
    
    std::cout << "Using rolling context: '" << rollingContext << "'" << std::endl;
    
    // Initialize result as empty - we'll only return the predicted characters, not the context
    std::string result = "";
    
    // Generate n new characters
    for (int i = 0; i < n; i++) {
        // Predict next symbol based on current context
        std::string nextSymbol = predict(rollingContext);
        result += nextSymbol;
        
        // Update rolling context by removing first character and adding the new one
        std::vector<std::string> updatedChars = splitIntoUTF8Characters(rollingContext);
        rollingContext.clear();
        
        // Skip the first UTF-8 character and add all others
        for (std::size_t j = 1; j < updatedChars.size(); j++) {
            rollingContext += updatedChars[j];
        }
        // Add the new symbol
        rollingContext += nextSymbol;
    }
    
    std::cout << "Predicted " << n << " characters: '" << result << "'" << std::endl;
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
    
    for (std::size_t i = 0; i < text.length(); /* incremented in the loop */) {
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

void FCMModel::printModelSummary() const {
    std::cout << "============= FCM MODEL SUMMARY =============" << std::endl;
    std::cout << "Order (k): " << k << std::endl;
    std::cout << "Smoothing (alpha): " << alpha << std::endl;
    std::cout << "Model is " << (locked ? "locked" : "unlocked") << std::endl;
    std::cout << "Alphabet size: " << alphabet.size() << " unique symbols" << std::endl;
    std::cout << "Contexts: " << frequencyTable.size() << " unique contexts" << std::endl;
    std::cout << "Total transitions: " << getTotalTransitionCount() << std::endl;

    if (!frequencyTable.empty()) {
        std::cout << "\nExample contexts:" << std::endl;
        int count = 0;

        for (const auto &contextPair : frequencyTable) {
            if (count >= 5) break;

            std::string context = contextPair.first;

            // Replace non-printable characters with '?'
            for (char &c : context) {
                if (!std::isprint(c)) {
                    c = '?';
                }
            }

            std::cout << "Context '" << context << "' -> ";
            for (const auto &symbolPair : contextPair.second) {
                std::cout << "'" << symbolPair.first << "'(" << symbolPair.second << ") ";
            }
            std::cout << std::endl;

            count++;
        }
    }
    std::cout << "=============================================" << std::endl;
}