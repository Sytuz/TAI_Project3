#include "FCMModel.h"
#include <sstream>
#include <unordered_set>

using namespace std;

FCMModel::FCMModel(): k(3), alpha(0.1), alphabetSize(0) {}  // default
FCMModel::FCMModel(int k, double alpha): k(k), alpha(alpha), alphabetSize(0) {}

int getAlphabetSize(const string &text){
    unordered_set<char> uniqueChars(text.begin(), text.end());
    return uniqueChars.size();
}

void FCMModel::buildModel(const string &text){
    frequencyTable.clear();
    contextCount.clear();

    alphabetSize = getAlphabetSize(text);

    for(size_t i = k; i < text.length(); ++i){
        string context = text.substr(i-k, k);
        char symbol = text[i];

        frequencyTable[context][symbol]++;
        contextCount[context]++;
    }

    generateProbabilityTable();
}

double FCMModel::getProbability(const string &context, char symbol) const{
    auto contextI = frequencyTable.find(context);
    if(contextI == frequencyTable.end()){
        return 1.0/alphabetSize;  // if no context is found --> smoothed uniform probability
    }

    auto symbolI = contextI->second.find(symbol);
    int count = (symbolI != contextI->second.end() ? symbolI->second : 0);
    int totalCount = contextCount.at(context);

    return (count + alpha) / (totalCount + alpha * alphabetSize);  // smoothing
}

void FCMModel::generateProbabilityTable(){
    probabilityTable.clear();

    for(const auto &contextPair : frequencyTable){
        const string &context = contextPair.first;
        int totalCount = contextCount[context];

        for (const auto &symbolPair : contextPair.second){
            char symbol = symbolPair.first;
            int count = symbolPair.second;

            probabilityTable[context][symbol] = (count + alpha) / (totalCount + alpha * alphabetSize);
        }
    }
}

// uses Shannon entropy (average information content per symbol)
double FCMModel::computeAverageInformationContent(const string &text) const{
    if(text.length() <= static_cast<size_t>(k)){
        return 0.0;
    }

    double totalInformation = 0.0;
    int symbolCount = 0;

    for(size_t i = k; i < text.length(); i++){
        string context = text.substr(i-k, k);
        char symbol = text[i];

        double probability = getProbability(context, symbol);
        totalInformation += -log2(probability);  // information content of each symbol
        symbolCount++;
    }

    return totalInformation / symbolCount;
}

// char FCMModel::predict(const string &context) const{
//     // TODO
// }

// void FCMModel::exportModel(const string &filename){
//     // TODO
// }

// void FCMModel::importModel(const string &filename){
//     // TODO
// }

string readFile(const string &filename){
    ifstream file(filename);
    if(!file){
        throw runtime_error("The file " + filename + " could not be opened!");
    }

    stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}