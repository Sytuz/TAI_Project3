#ifndef RFCMMODEL_H
#define RFCMMODEL_H

#include "FCMModel.h"
#include <vector>
#include <unordered_map>
#include <string>

/**
 * @class RFCMModel
 * @brief A recursive Finite Context Model that uses multiple context lengths.
 * 
 * This class extends FCMModel to implement a recursive approach, storing frequency
 * tables for contexts of length k down to 1, and using a probabilistic balance
 * between these different context lengths for prediction.
 */
class RFCMModel : public FCMModel {
public:
    /**
     * @brief Default constructor.
     */
    RFCMModel();
    
    /**
     * @brief Constructs an RFCMModel with the given parameters.
     * @param k The maximum order of the model (length of the context).
     * @param alpha The smoothing parameter.
     * @param lambda The interpolation weight factor for balancing different context lengths.
     */
    RFCMModel(int k, double alpha, double lambda = 0.5);
    
    /**
     * @brief Learns from the given text by updating the recursive model.
     * @param text The input text to learn from.
     * @param clearLogs Whether to suppress log messages.
     */
    void learn(const std::string &text, bool clearLogs = false) override;
    
    /**
     * @brief Computes the probability of a symbol given a context using the recursive model.
     * @param context The context string.
     * @param symbol The symbol to compute the probability for.
     * @return The probability of the symbol given the context.
     */
    double getProbability(const std::string &context, const std::string &symbol) const override;
    
    /**
     * @brief Clears the recursive model by resetting all tables.
     */
    void clearModel() override;
    
    /**
     * @brief Generates the probability tables for all context lengths.
     */
    void generateProbabilityTables();
    
    /**
     * @brief Locks the model to prevent further learning.
     */
    void lockModel() override;
    
    /**
     * @brief Unlocks the model to allow further learning.
     */
    void unlockModel() override;
    
    /**
     * @brief Exports the model to the given file.
     * @param filename The name of the file to export the model to.
     * @param binary Whether to export in binary format (true) or JSON format (false).
     * @return The filename of the exported model.
     */
    std::string exportModel(const std::string &filename, bool binary = true) override;
    
    /**
     * @brief Imports a model from the given file.
     * @param filename The name of the file to import the model from.
     * @param binary Whether to import a file in binary format (true) or JSON format (false).
     */
    void importModel(const std::string &filename, bool binary = true) override;
    
    /**
     * @brief Prints a summary of the recursive model properties.
     */
    void printModelSummary() const override;
    
private:
    // Recursive frequency tables for each context length (1 to k)
    std::unordered_map<int, std::unordered_map<std::string, std::unordered_map<std::string, int>>> rFrequencyTable;
    
    // Recursive probability tables for each context length (1 to k)
    std::unordered_map<int, std::unordered_map<std::string, std::unordered_map<std::string, float>>> rProbabilityTable;
    
    // Count of symbols following each context at each context length
    std::unordered_map<int, std::unordered_map<std::string, int>> rContextCount;
    
    /**
     * @brief Predicts the next symbol using the recursive model.
     * @param context The context string to predict from.
     * @return The predicted symbol.
     */
    std::string predict(const std::string &context) const override;
    
    /**
     * @brief Gets the reduced context by removing the first character.
     * @param context The original context.
     * @param contextLength The desired reduced context length.
     * @return The reduced context.
     */
    std::string getReducedContext(const std::string &context, int contextLength) const;
    
    /**
     * @brief Splits the given text into UTF-8 characters.
     * @param text The input text to split.
     * @return A vector of UTF-8 characters.
     */
    std::vector<std::string> splitIntoUTF8Characters(const std::string &text) const;
};

#endif // RFCMMODEL_H