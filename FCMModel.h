#ifndef FCMMODEL_H
#define FCMMODEL_H

#include <iostream>
#include <fstream>
#include <unordered_map>
#include <cmath>
#include <vector>
#include <set>

/**
 * @class FCMModel
 * @brief A class to represent a Finite Context Model (FCM) for text analysis.
 */
class FCMModel {
public:
    /* --- Constructors --- */

    /**
     * @brief Default constructor for FCMModel.
     */
    FCMModel();

    /**
     * @brief Constructs an FCMModel object with the given parameters.
     * @param k The order of the model (length of the context).
     * @param alpha The smoothing parameter.
     * @param verbose Whether to print verbose output during learning.
     */
    FCMModel(int k, double alpha, bool verbose = true)
        : k(k), alpha(alpha), verbose(verbose) {}


    /* --- Model Management --- */

    /**
     * @brief Locks the model to prevent further learning.
     */
    virtual void lockModel();
    
    /**
     * @brief Unlocks the model to allow further learning.
     */
    virtual void unlockModel();

    /**
     * @brief Clears the model by resetting all the tables.
     */
    virtual void clearModel();

    /* --- Accessors --- */

    /**
     * @brief Gets the order of the model (context size).
     * @return The context size k.
     */
    int getK() const { return k; }

    /**
     * @brief Sets the order of the model (context size).
     * @param k The new context size.
     */
    void setK(int k) { this->k = k; }

    /**
     * @brief Gets the smoothing parameter of the model.
     * @return The smoothing parameter alpha.
     */
    double getAlpha() const { return alpha; }

    /**
     * @brief Sets the smoothing parameter of the model.
     * @param alpha The new smoothing parameter.
     */
    void setAlpha(double alpha) { this->alpha = alpha; }

    /**
     * @brief Gets the alphabet of the model.
     * @return The alphabet.
     */
    int getAlphabetSize() const;

    /**
     * @brief Computes the probability of a symbol given a context.
     * @param context The context string.
     * @param symbol The symbol to compute the probability for.
     * @return The probability of the symbol given the context.
     */
    virtual double getProbability(const std::string &context, const std::string &symbol) const;

    /**
     * @brief Gets the alphabet of the model.
     * @return The alphabet.
     */
    const std::set<std::string>& getAlphabet() const { return alphabet; }

    /**
     * @brief Checks if the model is locked.
     * @return True if the model is locked, false otherwise.
     */
    bool isLocked() const { return locked; }


    /* --- Core Methods --- */

    /**
     * @brief Learns from the given text by updating the model.
     * @param text The input text to learn from.
     */
    virtual void learn(const std::string &text);

    /**
     * @brief Computes the average information content of the given text based on the model.
     * @param text The input text to compute the information content for.
     * @return The average information content of the text.
     */
    double computeAverageInformationContent(const std::string &text) const;

    /**
     * @brief Computes the information content for each symbol in the given text.
     * @param text The input text to compute the information content for.
     * @return A vector containing the information content for each symbol.
     */
    std::vector<double> computeSymbolInformation(const std::string &text) const;

    /**
     * @brief Exports the information content for each symbol to a file.
     * @param text The input text to compute the information content for.
     * @param filename The name of the file to export the information to.
     * @return The filename of the exported information.
     */
    std::string exportSymbolInformation(const std::string &text, const std::string &filename) const;

    /**
     * @brief Predicts the next n symbols in the given context based on the model.
     * @param context The context string to predict the next symbols for.
     * @param n The number of symbols to predict.
     * @return The predicted symbols.
     */
    virtual std::string predict(const std::string &context, int n) const;
    
    /**
     * @brief Imports a model from the given file.
     * @param filename The name of the file to import the model from.
     * @param binary Whether to import a file in binary format (true) or JSON format (false).
     */
    virtual void importModel(const std::string &filename, bool binary = true);

    /**
     * @brief Exports the model to the given file.
     * @param filename The name of the file to export the model to.
     * @param binary Whether to export in binary format (true) or JSON format (false).
     * @return The filename of the exported model.
     */
    virtual std::string exportModel(const std::string &filename, bool binary = true);

    /*
    The exported file should contain the following fields:
     - k: The order of the model (length of the context).
     - alpha: The smoothing parameter.
     - alphabet: The alphabet.
     - frequencyTable: The frequency table.
     - probabilityTable: The probability table.
     - contextCount: The context count.
     - locked: Whether the model is locked or not.
    */

    /*--- Diagnostic methods ---*/

    /**
     * @brief Checks if the model is empty.
     * @return True if the model is empty, false otherwise.
     */
    virtual bool isModelEmpty() const;

    /**
     * @brief Gets the number of unique contexts in the model.
     * @return The number of unique contexts.
     */
    std::size_t getContextCount() const;

    /**
     * @brief Prints a summary of the model properties.
     */
    virtual void printModelSummary() const;

    /**
     * @brief Gets the total number of transitions in the model.
     * @return The total number of transitions.
     */
    std::size_t getTotalTransitionCount() const;

protected:
    std::set<std::string> alphabet;

private:
    int k;
    double alpha;
    bool locked = false;
    bool verbose;

    std::unordered_map<std::string, std::unordered_map<std::string, int>> frequencyTable;
    std::unordered_map<std::string, std::unordered_map<std::string, float>> probabilityTable;
    std::unordered_map<std::string, int> contextCount;
    

    /**
     * @brief Predicts the next symbol in the given context based on the model.
     * @param context The context string to predict the next symbol for.
     * @return The predicted symbol.
     */
    virtual std::string predict(const std::string &context) const;
    
    /**
     * @brief Generates the probability table based on the frequency table.
     */
    virtual void generateProbabilityTable();

    /**
     * @brief Splits the given text into UTF-8 characters.
     * @param text The input text to split.
     * @return A vector of UTF-8 characters.
     */
    std::vector<std::string> splitIntoUTF8Characters(const std::string &text) const;
};

std::string readFile(const std::string &filename);

#endif