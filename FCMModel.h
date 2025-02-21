#ifndef FCMMODEL_H
#define FCMMODEL_H

#include <iostream>
#include <fstream>
#include <unordered_map>
#include <cmath>
#include <vector>

/**
 * @class FCMModel
 * @brief A class to represent a Finite Context Model (FCM) for text analysis.
 */
class FCMModel {
public:
    /**
     * @brief Default constructor for FCMModel.
     */
    FCMModel();

    /**
     * @brief Constructs an FCMModel object with the given parameters.
     * @param k The order of the model (length of the context).
     * @param alpha The smoothing parameter.
     */
    FCMModel(int k, double alpha);

    /**
     * @brief Builds the model by analyzing the given text.
     * @param text The input text to build the model from.
     */
    void buildModel(const std::string &text);

    /**
     * @brief Computes the average information content of the given text based on the model.
     * @param text The input text to compute the information content for.
     * @return The average information content of the text.
     */
    double computeAverageInformationContent(const std::string &text) const;

    /**
     * @brief Predicts the next symbol in the given context based on the model.
     * @param context The context string to predict the next symbol for.
     * @return The predicted symbol.
     */
    char predict(const std::string &context) const;
    
    /**
     * @brief Imports a model from the given file.
     * @param filename The name of the file to import the model from.
     */
    void importModel(const std::string &filename);

    /**
     * @brief Exports the model to the given file.
     * @param filename The name of the file to export the model to.
     */
    void exportModel(const std::string &filename);

private:
    int k;
    double alpha;
    int alphabetSize;
    std::unordered_map<std::string, std::unordered_map<char, int>> frequencyTable;
    std::unordered_map<std::string, std::unordered_map<char, float>> probabilityTable;
    std::unordered_map<std::string, int> contextCount;

    /**
     * @brief Computes the probability of a symbol given a context.
     * @param context The context string.
     * @param symbol The symbol to compute the probability for.
     * @return The probability of the symbol given the context.
     */
    double getProbability(const std::string &context, char symbol) const;
    
    /**
     * @brief Generates the probability table based on the frequency table.
     */
    void generateProbabilityTable();
};

std::string readFile(const std::string &filename);

#endif