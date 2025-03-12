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
    
    int totalContextsProcessed = 0;
    
    // Make a separate pass for each context length from 1 to k
    // Start tracking overall learning time
    auto startLearningTime = std::chrono::high_resolution_clock::now();
    
    for (int contextLength = 1; contextLength <= getK(); ++contextLength) {
        int contextsProcessedThisPass = 0;
        int uniqueContextsThisPass = 0;
        std::map<std::string, bool> seenContexts;
        
        if (!clearLogs) {
            std::cout << "Processing contexts of length " << contextLength << "..." << std::endl;
        }
        
        auto startTimeThisPass = std::chrono::high_resolution_clock::now();
        
        // Process each valid position in the text for this context length
        for (std::size_t i = 0; i <= characters.size() - contextLength - 1; ++i) {
            // Build context of current length
            std::string context;
            for (std::size_t j = i; j < i + contextLength; ++j) {
                context += characters[j];
            }
            
            // Get the next symbol after this context
            std::string symbol = characters[i + contextLength];
            
            // Track if this is a new context
            if (seenContexts.find(context) == seenContexts.end()) {
                seenContexts[context] = true;
                uniqueContextsThisPass++;
            }
            
            // Add to alphabet
            alphabet.insert(symbol);
            
            // Update frequency tables for this context length
            rFrequencyTable[contextLength][context][symbol]++;
            rContextCount[contextLength][context]++;
            
            contextsProcessedThisPass++;
            
            // Print detailed progress every 100,000 contexts if logs are enabled
            if (!clearLogs && contextsProcessedThisPass % 100000 == 0) {
                std::cout << "    Processed " << contextsProcessedThisPass << " contexts..." << std::endl;
            }
        }
        
        auto endTimeThisPass = std::chrono::high_resolution_clock::now();
        auto durationThisPass = std::chrono::duration_cast<std::chrono::milliseconds>(endTimeThisPass - startTimeThisPass).count();
        
        totalContextsProcessed += contextsProcessedThisPass;
        
        if (!clearLogs) {
            std::cout << "  Processed " << contextsProcessedThisPass << " contexts of length " << contextLength << std::endl;
            std::cout << "  Found " << uniqueContextsThisPass << " unique contexts" << std::endl;
            std::cout << "  Time taken: " << durationThisPass << " ms" << std::endl;
            
            // Show stats about frequency distribution for this context length
            if (!rFrequencyTable[contextLength].empty()) {
                int maxEntries = 0;
                std::string busyContext;
                
                for (const auto &contextPair : rFrequencyTable[contextLength]) {
                    if (contextPair.second.size() > maxEntries) {
                        maxEntries = contextPair.second.size();
                        busyContext = contextPair.first;
                    }
                }
                
                std::cout << "  Most diverse context: '" << busyContext 
                          << "' with " << maxEntries << " different following symbols" << std::endl;
            }
        }
    }
    
    auto endLearningTime = std::chrono::high_resolution_clock::now();
    auto totalDuration = std::chrono::duration_cast<std::chrono::milliseconds>(endLearningTime - startLearningTime).count();
    
    if (!clearLogs) {
        std::cout << "Total learning time: " << totalDuration << " ms" << std::endl;
    }
    
    if (!clearLogs) {
        std::cout << "Learning complete. Processed " << totalContextsProcessed << " contexts total." << std::endl;
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

double RFCMModel::getProbability(const std::string &context, const std::string &symbol, int tableIndex) const {

    // If we're using locked probability tables
    if (isLocked()) {
        auto contextIt = rProbabilityTable.at(tableIndex).find(context);
        if (contextIt != rProbabilityTable.at(tableIndex).end()) {
            auto symbolIt = contextIt->second.find(symbol);
            if (symbolIt != contextIt->second.end()) {
                return symbolIt->second;
            }
        }
        return 1.0 / getAlphabetSize();  // Default fallback

    } else {
        // If the model is unlocked, apply Laplace smoothing on the frequency table
        auto contextIt = rFrequencyTable.at(tableIndex).find(context);
        if (contextIt == rFrequencyTable.at(tableIndex).end()) {
            return 1.0 / getAlphabetSize();  // Default fallback
        }
        
        auto symbolIt = contextIt->second.find(symbol);
        int count = (symbolIt != contextIt->second.end() ? symbolIt->second : 0);
        int totalCount = rContextCount.at(tableIndex).at(context);
        
        return (count + getAlpha()) / (totalCount + getAlpha() * getAlphabetSize());
    }
}

std::string RFCMModel::predict(const std::string &context) const {
    // For simplicity, let's use a weighted random selection based on getProbability
    
    // If the context is empty or too short, use a fallback
    if (context.empty()) {
        std::cout << "Warning: Context is empty, using fallback" << std::endl;
        // Fallback to random selection from alphabet
        if (!alphabet.empty()) {
            auto it = alphabet.begin();
            std::advance(it, rand() % alphabet.size());
            return *it;
        }
        return " ";  // Default fallback
    }

    // Find which context length to use (k or less)
    int contextLength = std::min(static_cast<int>(context.length()), getK());

    std::string reducedContext = getReducedContext(context, contextLength);

    // Find the frequency table that contains the context (the higher order the better)
    while (contextLength > 0) {
        std::cout << "Checking context - " << reducedContext << " - with length " << contextLength << std::endl;
        auto contextIt = rFrequencyTable.at(contextLength).find(reducedContext);
        if (contextIt != rFrequencyTable.at(contextLength).end()) {
            // Found the context, break out of the loop
            break;
        }
        contextLength--;
        reducedContext = getReducedContext(reducedContext, contextLength);
    }

    if (contextLength == 0) {
        std::cout << "Warning: No context found, using fallback" << std::endl;
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
          // Adjust table index based on context length
        double prob = getProbability(reducedContext, symbol, contextLength);
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
    // Split context into UTF-8 characters
    std::vector<std::string> characters = splitIntoUTF8Characters(context);
    
    // If the context has fewer characters than requested, return an empty string
    if (characters.size() < static_cast<std::size_t>(contextLength)) {
        return "";
    }
    
    // Extract the last 'contextLength' UTF-8 characters
    std::string reducedContext;
    for (std::size_t i = characters.size() - contextLength; i < characters.size(); ++i) {
        reducedContext += characters[i];
    }
    
    return reducedContext;
}

// First, add a new helper method to make UTF-8 strings display-friendly
std::string RFCMModel::makeDisplayFriendly(const std::string &str) const {
    // For UTF-8 strings, we should preserve valid characters
    // and only replace truly non-printable control characters
    std::string result;
    
    // Process the string character by character (UTF-8 aware)
    std::vector<std::string> chars = splitIntoUTF8Characters(str);
    for (const auto &c : chars) {
        // If it's a single byte character, check if it's a control character
        if (c.length() == 1) {
            unsigned char byte = static_cast<unsigned char>(c[0]);
            if (byte < 32 || byte == 127) { // Control characters
                result += "?";
            } else {
                result += c;
            }
        } else {
            // Multi-byte UTF-8 character - keep as is
            result += c;
        }
    }
    return result;
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
                
                // Make context display-friendly
                std::string displayContext = makeDisplayFriendly(contextPair.first);
                
                std::cout << "    Context '" << displayContext << "' -> ";
                int symbolCount = 0;
                
                for (const auto &symbolPair : contextPair.second) {
                    if (symbolCount >= 3) {
                        std::cout << "...";
                        break;
                    }
                    
                    // Make symbol display-friendly
                    std::string displaySymbol = makeDisplayFriendly(symbolPair.first);
                    
                    std::cout << "'" << displaySymbol << "'(" << symbolPair.second << ") ";
                    symbolCount++;
                }
                std::cout << std::endl;
                
                count++;
            }
        }
    }
    
    // Print full frequency tables for each context length
    std::cout << "\nFrequency Tables (full):" << std::endl;
    for (int contextLength = getK(); contextLength >= 1; --contextLength) {
        if (rFrequencyTable.find(contextLength) == rFrequencyTable.end()) {
            continue;  // Skip if no entries for this context length
        }
        
        std::cout << "  Order " << contextLength << " contexts:" << std::endl;
        
        for (const auto &contextPair : rFrequencyTable.at(contextLength)) {
            // Make context display-friendly
            std::string displayContext = makeDisplayFriendly(contextPair.first);
            
            std::cout << "    \"" << displayContext << "\" → {";
            
            // Print all symbol frequencies
            int symbolCount = 0;
            for (const auto &symbolPair : contextPair.second) {
                if (symbolCount > 0) std::cout << ", ";
                
                // Make symbol display-friendly
                std::string displaySymbol = makeDisplayFriendly(symbolPair.first);
                
                std::cout << "\"" << displaySymbol << "\": " << symbolPair.second;
                symbolCount++;
            }
            
            std::cout << "}" << std::endl;
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