#include "MetaClass.h"
#include <iostream>
#include <algorithm>

int main(int argc, char* argv[]) {
    // Default parameter values
    std::string dbFile, sampleFile;
    int k = 10;
    double alpha = 0.1;
    int topN = 20;

    // Parse the command line arguments
    parseCommandLineArguments(argc, argv, dbFile, sampleFile, k, alpha, topN);

    std::cout << "Reading metagenomic sample from: " << sampleFile << std::endl;
    std::cout << "Reading reference database from: " << dbFile << std::endl;

    // Read the metagenomic sample
    std::string sample = readMetagenomicSample(sampleFile);

    if (sample.empty()) {
        std::cerr << "Error: Empty metagenomic sample" << std::endl;
        return 1;
    }

    std::cout << "Metagenomic sample length: " << sample.length() << " nucleotides" << std::endl;

    // Train the Markov model on the sample
    std::cout << "Training Markov model with k=" << k << ", alpha=" << alpha << std::endl;
    MarkovModel model(k, alpha);
    model.train(sample);

    // Read the referenced database
    std::vector<Reference> references = readReferenceDatabase(dbFile);

    if (references.empty()) {
        std::cerr << "Error: No references found in database" << std::endl;
        return 1;
    }

    std::cout << "Found " << references.size() << " reference sequences" << std::endl;

    // Calculate NRC for each reference sequence
    std::cout << "Calculating NRC for each reference..." << std::endl;
    for (auto& ref : references) {
        ref.nrc = model.calculateNRC(ref.sequence);
    }

    // Sort references by NRC
    std::sort(references.begin(), references.end(), [](const Reference& a, const Reference& b) { return a.nrc < b.nrc; });

    // Print the top N results
    std::cout << "\nTop " << topN << " most similar sequences:" << std::endl;
    std::cout << "----------------------------------------" << std::endl;
    for (int i = 0; i < std::min(topN, static_cast<int>(references.size())); ++i) {
        std::cout << i+1 << ". " << references[i].name << " (NRC: " << references[i].nrc << ")" << std::endl;
    }

    return 0;
}
