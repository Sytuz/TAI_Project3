#include "RFCMModel.h"
#include <iostream>
#include <cmath>
#include <random>
#include "json.hpp"

using json = nlohmann::json;

RFCMModel::RFCMModel() : FCMModel() {}

RFCMModel::RFCMModel(int k, double alpha, double lambda) : FCMModel(k, alpha) {}

void RFCMModel::learn(const std::string &text, bool clearLogs) {
    if (isLocked()) {
        std::cout << "Cannot learn: model is locked" << std::endl;
        return;
    }
    
    if (text.empty()) {
        std::cout << "Cannot learn: input text is empty" << std::endl;
        return;
    }
    
    if (!clearLogs) {
        std::cout << "Learning from text of length: " << text.length() << " characters..." << std::endl;
    }
    
    // Split text into UTF-8 characters
    std::vector<std::string> characters = splitIntoUTF8Characters(text);
    if (!clearLogs) {
        std::cout << "Split into " << characters.size() << " UTF-8 characters" << std::endl;
    }
    
    // We need at least k+1 characters to create a context and a following symbol
    if (characters.size() <= static_cast<std::size_t>(getK())) {
        std::cout << "Text too short for model order k=" << getK() << std::endl;
        return;
    }
    
    int contextProcessed = 0;
    
    // Process each sequence of characters for all context lengths from k down to 1
    for (std::size_t i = 0; i <= characters.size() - getK() - 1; ++i) {
        // For each context length from k down to 1
        for (int contextLength = getK(); contextLength >= 1; --contextLength) {
            // Build context from contextLength consecutive characters
            std::string context;
            for (std::size_t j = i; j < i + contextLength; ++j) {
                context += characters[j];
            }
            
            // The next character after the context
            std::string symbol = characters[i + contextLength];
            
            // Add to alphabet (use the parent class's alphabet)
            alphabet.insert(symbol);
            
            // Update frequency tables for this context length
            rFrequencyTable[contextLength][context][symbol]++;
            rContextCount[contextLength][context]++;
        }
        
        contextProcessed++;
        
        // Optional: add progress indicator for large texts
        if (contextProcessed % 10000 == 0 && !clearLogs) {
            std::cout << "Processed " << contextProcessed << " contexts..." << std::endl;
        }
    }
    
    if (!clearLogs) {
        std::cout << "Learning complete. Processed " << contextProcessed << " contexts." << std::endl;
        std::cout << "Model now contains contexts of lengths 1 to " << getK() << "." << std::endl;
        std::cout << "Alphabet size: " << alphabet.size() << " unique symbols." << std::endl;
    }
}

void RFCMModel::clearModel() {
    if (isLocked()) {
        return;
    }
    
    rFrequencyTable.clear();
    rProbabilityTable.clear();
    rContextCount.clear();
    
    // Also clear the parent class's tables
    FCMModel::clearModel();
}

void RFCMModel::generateProbabilityTables() {
    std::cout << "Generating probability tables for all context lengths..." << std::endl;
    
    // Clear the current probability tables
    rProbabilityTable.clear();
    
    // If the model is empty, there's nothing to do
    if (rFrequencyTable.empty()) {
        std::cout << "Frequency tables are empty, no probabilities to calculate." << std::endl;
        return;
    }
    
    // For each context length from k down to 1
    for (int contextLength = getK(); contextLength >= 1; --contextLength) {
        if (rFrequencyTable.find(contextLength) == rFrequencyTable.end()) {
            continue;  // Skip if there are no entries for this context length
        }
        
        int contextCount = 0;
        
        // Iterate over each context in the frequency table for this context length
        for (const auto &contextPair : rFrequencyTable[contextLength]) {
            const std::string &context = contextPair.first;
            int totalCount = rContextCount[contextLength][context];
            
            if (totalCount <= 0) {
                std::cout << "Warning: Context '" << context << "' at length " << contextLength 
                          << " has zero or negative count." << std::endl;
                continue;
            }
            
            // Calculate probability for each symbol in this context
            for (const auto &symbolPair : contextPair.second) {
                const std::string &symbol = symbolPair.first;
                int count = symbolPair.second;
                
                // Apply Laplace smoothing
                double probability = (count + getAlpha()) / (totalCount + getAlpha() * getAlphabetSize());
                rProbabilityTable[contextLength][context][symbol] = probability;
            }
            
            contextCount++;
        }
        
        std::cout << "Probability table generated for " << contextCount << " contexts at length " 
                  << contextLength << "." << std::endl;
    }
}

double RFCMModel::getProbability(const std::string &context, const std::string &symbol) const {
    // If context is shorter than k, use the frequency table that fits the context
    int contextLength = std::min(static_cast<int>(context.length()), getK());
    
    // Start with the highest order model (largest context length)
    for (int currentLength = contextLength; currentLength >= 1; --currentLength) {
        std::string reducedContext = getReducedContext(context, currentLength);
        
        // Skip invalid contexts
        if (reducedContext.empty()) {
            continue;
        }
        
        // Check if this context exists in our frequency tables
        if (rFrequencyTable.find(currentLength) != rFrequencyTable.end() &&
            rFrequencyTable.at(currentLength).find(reducedContext) != rFrequencyTable.at(currentLength).end()) {
            
            // If we're using locked probability tables
            if (isLocked()) {
                if (rProbabilityTable.find(currentLength) != rProbabilityTable.end() &&
                    rProbabilityTable.at(currentLength).find(reducedContext) != rProbabilityTable.at(currentLength).end() &&
                    rProbabilityTable.at(currentLength).at(reducedContext).find(symbol) != rProbabilityTable.at(currentLength).at(reducedContext).end()) {
                    
                    // Found the symbol in this context, return its probability
                    return rProbabilityTable.at(currentLength).at(reducedContext).at(symbol);
                }
            } else {
                // Calculate probability on-the-fly with smoothing
                int count = 0;
                if (rFrequencyTable.at(currentLength).at(reducedContext).find(symbol) != 
                    rFrequencyTable.at(currentLength).at(reducedContext).end()) {
                    count = rFrequencyTable.at(currentLength).at(reducedContext).at(symbol);
                }
                
                int totalCount = rContextCount.at(currentLength).at(reducedContext);
                return (count + getAlpha()) / (totalCount + getAlpha() * getAlphabetSize());
            }
        }
        
        // If we reach here, this context level didn't have the symbol or context
        // We'll continue to the next lower order model
    }
    
    // If we've exhausted all context lengths without finding a match,
    // fall back to a uniform distribution
    return 1.0 / getAlphabetSize();
}

std::string RFCMModel::predict(const std::string &context) const {
    // For simplicity, let's use a weighted random selection based on getProbability
    
    // If the context is empty or too short, use a fallback
    if (context.empty() || context.length() < static_cast<std::size_t>(getK())) {
        std::cout << "Warning: Context is too short for prediction, using fallback" << std::endl;
        // Fallback to random selection from alphabet
        if (!alphabet.empty()) {
            auto it = alphabet.begin();
            std::advance(it, rand() % alphabet.size());
            return *it;
        }
        return " ";  // Default fallback
    }
    
    // Calculate the probability distribution for all possible next symbols
    std::vector<std::pair<std::string, double>> probabilities;
    double totalProbability = 0.0;
    
    for (const std::string &symbol : alphabet) {
        double prob = getProbability(context, symbol);
        probabilities.push_back({symbol, prob});
        totalProbability += prob;
    }
    
    // Select a symbol randomly based on probabilities
    double randomValue = static_cast<double>(rand()) / RAND_MAX * totalProbability;
    double cumulativeProbability = 0.0;
    
    for (const auto &pair : probabilities) {
        cumulativeProbability += pair.second;
        if (randomValue <= cumulativeProbability) {
            return pair.first;
        }
    }
    
    // Fallback (shouldn't reach here with proper smoothing)
    return !alphabet.empty() ? *alphabet.begin() : " ";
}

void RFCMModel::lockModel() {
    generateProbabilityTables();
    FCMModel::lockModel();
}

void RFCMModel::unlockModel() {
    FCMModel::unlockModel();
}

std::string RFCMModel::exportModel(const std::string &filename, bool binary) {
    lockModel();  // Lock the model before exporting
    
    json modelJson;
    modelJson["k"] = getK();
    modelJson["alpha"] = getAlpha();
    modelJson["alphabet"] = alphabet;
    modelJson["locked"] = isLocked();
    
    // Export recursive frequency tables
    for (const auto &lengthPair : rFrequencyTable) {
        int contextLength = lengthPair.first;
        for (const auto &contextPair : lengthPair.second) {
            const std::string &context = contextPair.first;
            modelJson["rFrequencyTable"][std::to_string(contextLength)][context] = contextPair.second;
        }
    }
    
    // Export recursive probability tables
    for (const auto &lengthPair : rProbabilityTable) {
        int contextLength = lengthPair.first;
        for (const auto &contextPair : lengthPair.second) {
            const std::string &context = contextPair.first;
            modelJson["rProbabilityTable"][std::to_string(contextLength)][context] = contextPair.second;
        }
    }
    
    // Export recursive context counts
    for (const auto &lengthPair : rContextCount) {
        int contextLength = lengthPair.first;
        modelJson["rContextCount"][std::to_string(contextLength)] = lengthPair.second;
    }
    
    // Update file extension based on format
    std::string fullFilename = filename + (binary ? ".bson" : ".json");
    
    // Open file in binary mode to ensure exact byte writing
    std::ofstream file(fullFilename, std::ios::binary);
    if (!file) {
        throw std::runtime_error("The file " + fullFilename + " could not be opened!");
    }
    
    if (binary) {
        // Use BSON format for binary export
        std::vector<uint8_t> bson = json::to_bson(modelJson);
        file.write(reinterpret_cast<const char*>(bson.data()), bson.size());
    } else {
        // Dump JSON with indent of 4 spaces and write to file
        file << modelJson.dump(4, ' ', true) << std::endl;
    }
    
    return fullFilename;
}

void RFCMModel::importModel(const std::string &filename, bool binary) {
    json modelJson;
    
    if (binary) {
        // Read binary file into vector
        std::ifstream file(filename, std::ios::binary);
        if (!file) {
            throw std::runtime_error("The file " + filename + " could not be opened!");
        }
        
        // Get file size
        file.seekg(0, std::ios::end);
        size_t fileSize = file.tellg();
        file.seekg(0, std::ios::beg);
        
        // Read the file content
        std::vector<uint8_t> data(fileSize);
        file.read(reinterpret_cast<char*>(data.data()), fileSize);
        
        // Parse BSON data
        modelJson = json::from_bson(data);
    } else {
        // Read and parse JSON file
        std::ifstream file(filename);
        if (!file) {
            throw std::runtime_error("The file " + filename + " could not be opened!");
        }
        
        file >> modelJson;
    }
    
    // Process the parsed data from file
    setK(modelJson["k"]);
    setAlpha(modelJson["alpha"]);
    alphabet = modelJson["alphabet"];
    bool locked = modelJson["locked"];
    
    // Clear existing tables
    rFrequencyTable.clear();
    rProbabilityTable.clear();
    rContextCount.clear();
    
    // Import recursive frequency tables
    if (modelJson.contains("rFrequencyTable")) {
        for (const auto &lengthPair : modelJson["rFrequencyTable"].items()) {
            int contextLength = std::stoi(lengthPair.key());
            for (const auto &contextPair : lengthPair.value().items()) {
                const std::string &context = contextPair.key();
                rFrequencyTable[contextLength][context] = contextPair.value();
            }
        }
    }
    
    // Import recursive probability tables
    if (modelJson.contains("rProbabilityTable")) {
        for (const auto &lengthPair : modelJson["rProbabilityTable"].items()) {
            int contextLength = std::stoi(lengthPair.key());
            for (const auto &contextPair : lengthPair.value().items()) {
                const std::string &context = contextPair.key();
                rProbabilityTable[contextLength][context] = contextPair.value();
            }
        }
    }
    
    // Import recursive context counts
    if (modelJson.contains("rContextCount")) {
        for (const auto &lengthPair : modelJson["rContextCount"].items()) {
            int contextLength = std::stoi(lengthPair.key());
            rContextCount[contextLength] = lengthPair.value();
        }
    }
    
    // Set the parent class's properties
    // Note: This assumes FCMModel has methods to set these properties
    // You might need to implement them or modify this approach
    // For now, we'll just call the parent class's importModel with a dummy file
    
    // Set the lock state
    if (locked) {
        lockModel();
    } else {
        unlockModel();
    }
}

std::string RFCMModel::getReducedContext(const std::string &context, int contextLength) const {
    // If the context is shorter than the requested length, return an empty string
    if (context.length() < static_cast<std::size_t>(contextLength)) {
        return "";
    }
    
    // Extract the last 'contextLength' characters from the context
    return context.substr(context.length() - contextLength);
}

void RFCMModel::printModelSummary() const {
    std::cout << "============= RFCM MODEL SUMMARY =============" << std::endl;
    std::cout << "Maximum Order (k): " << getK() << std::endl;
    std::cout << "Smoothing (alpha): " << getAlpha() << std::endl;
    std::cout << "Model is " << (isLocked() ? "locked" : "unlocked") << std::endl;
    std::cout << "Alphabet size: " << alphabet.size() << " unique symbols" << std::endl;
    
    // Print statistics for each context length
    for (int contextLength = getK(); contextLength >= 1; --contextLength) {
        if (rFrequencyTable.find(contextLength) == rFrequencyTable.end()) {
            continue;  // Skip if there are no entries for this context length
        }
        
        std::cout << "\nContext Length " << contextLength << ":" << std::endl;
        std::cout << "  Unique contexts: " << rFrequencyTable.at(contextLength).size() << std::endl;
        
        // Count total transitions for this context length
        std::size_t totalTransitions = 0;
        for (const auto &contextPair : rContextCount.at(contextLength)) {
            totalTransitions += contextPair.second;
        }
        std::cout << "  Total transitions: " << totalTransitions << std::endl;
        
        // Print example contexts for this length (up to 2)
        if (!rFrequencyTable.at(contextLength).empty()) {
            std::cout << "  Example contexts:" << std::endl;
            int count = 0;
            
            for (const auto &contextPair : rFrequencyTable.at(contextLength)) {
                if (count >= 2) break;
                
                std::string context = contextPair.first;
                
                // Replace non-printable characters with '?'
                for (char &c : context) {
                    if (!std::isprint(c)) {
                        c = '?';
                    }
                }
                
                std::cout << "    Context '" << context << "' -> ";
                int symbolCount = 0;
                
                for (const auto &symbolPair : contextPair.second) {
                    if (symbolCount >= 3) {
                        std::cout << "...";
                        break;
                    }
                    
                    std::cout << "'" << symbolPair.first << "'(" << symbolPair.second << ") ";
                    symbolCount++;
                }
                std::cout << std::endl;
                
                count++;
            }
        }
    }
    
    std::cout << "===============================================" << std::endl;
}

std::vector<std::string> RFCMModel::splitIntoUTF8Characters(const std::string &text) const {
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