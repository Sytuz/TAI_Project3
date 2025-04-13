#include "dna_compressor.h"
#include <cmath>
#include <unordered_map>

using namespace std;

DNACompressor::DNACompressor(FCMModel &fcm) : model(fcm) {}

double DNACompressor::calculateBits(const string &sequence) const
{
    double totalBits = 0.0;
    int k = model.getK();

    if (sequence.length() <= static_cast<size_t>(k))
    {
        return 2.0 * sequence.length(); // Log2(4) = 2 bits per symbol for DNA
    }

    // Process each k+1 length substring
    for (size_t i = 0; i <= sequence.length() - static_cast<size_t>(k) - 1; ++i)
    {
        string context = sequence.substr(i, k);
        string nextSymbol = sequence.substr(i + k, 1);

        double probability = model.getProbability(context, nextSymbol);
        totalBits += -log2(probability);
    }

    return totalBits;
}

double DNACompressor::calculateNRC(const string &sequence) const
{
    if (sequence.length() <= static_cast<size_t>(model.getK()))
    {
        return 1.0; // Handle short sequences
    }

    double bits = calculateBits(sequence);
    // For DNA: |x|*log2(A) = |x|*2 since alphabet size is 4
    return bits / (2.0 * sequence.length());
}

double DNACompressor::calculateKLD(const string &sequence) const
{
    int k = model.getK();

    if (sequence.length() <= static_cast<size_t>(k))
    {
        return 0.0; // Not enough data to calculate KLD
    }

    // Count k+1-grams in the sequence to compute empirical distribution
    unordered_map<string, unordered_map<char, int>> empiricalCounts;
    unordered_map<string, int> contextTotals;

    // Process the sequence to collect k+1-gram statistics
    for (size_t i = 0; i <= sequence.length() - static_cast<size_t>(k) - 1; ++i)
    {
        string context = sequence.substr(i, k);
        char nextSymbol = sequence[i + k];

        empiricalCounts[context][nextSymbol]++;
        contextTotals[context]++;
    }

    // Calculate KLD: D_KL(P||Q) = sum_x P(x) * log(P(x)/Q(x))
    double kld = 0.0;
    for (const auto &contextPair : empiricalCounts)
    {
        const string &context = contextPair.first;
        const auto &symbolCounts = contextPair.second;
        int total = contextTotals[context];

        for (const auto &symbolPair : symbolCounts)
        {
            char symbol = symbolPair.first;
            int count = symbolPair.second;

            // Calculate empirical probability P(x)
            double empiricalProb = static_cast<double>(count) / total;

            // Get model probability Q(x)
            string symbolStr(1, symbol);
            double modelProb = model.getProbability(context, symbolStr);

            // Accumulate KLD
            kld += empiricalProb * log2(empiricalProb / modelProb);
        }
    }

    return kld;
}