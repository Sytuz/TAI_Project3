#include <iostream>
#include <fstream>
#include <unordered_map>
#include <cmath>
#include <vector>

using namespace std;

// hash table (unordered_map) to store occurrences
unordered_map<string, unordered_map<char, int>> frequencyTable;
unordered_map<string, int> contextCount;

// read the text file
string readFile(const string &filename){
    ifstream inputFile(filename);
    if (!inputFile.is_open()) {
        cerr << "Error opening file: " << filename << endl;
        exit(1);
    }

    string text((istreambuf_iterator<char>(inputFile)), istreambuf_iterator<char>());

    inputFile.close();
    return text;
}

// build frequency model
void buildFrequencyModel(const string &text, int k){
    for(size_t i = k; i < text.length(); i++){
        string context = text.substr(i-k, k);
        char symbol = text[i];

        frequencyTable[context][symbol]++;
        contextCount[context]++;
    }
}

// estimate probability
double getProbability(const string &context, char symbol, double alpha, int alphaSize){
    int count = frequencyTable[context][symbol];
    int total = contextCount[context];

    return (count + alpha) / (total + alpha * alphaSize);
}

// compute the average information content
double computeAverageInformationContent(const string &text, int k, double alpha){
    double H = 0.0;
    int alphabetSize = 256;  // ASCII characters
    int n = text.length() - k;

    for (size_t i = k; i < text.length(); i++){
        string context = text.substr(i-k, k);
        char symbol = text[i];

        double p = getProbability(context, symbol, alpha, alphabetSize);
        H += log2(p);
    }

    return -H / n;
}

int main(int argc, char *argv[]){
    if (argc != 6) {
        cerr << "Usage: ./fcm input.txt -k <order> -a <alpha>" << endl;
        return 1;
    }

    string filename = argv[1];
    int k = stoi(argv[3]);
    double alpha = stod(argv[5]);

    string text = readFile(filename);
    buildFrequencyModel(text, k);

    double entropy = computeAverageInformationContent(text, k, alpha);
    cout << "Average Information Content: " << entropy << " bps" << endl;

    return 0;
}