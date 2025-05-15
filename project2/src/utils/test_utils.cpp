#include "utils.h"
#include "test_utils.h"
#include "FCMModel.h"

#include <iostream>
#include <fstream>
#include <iomanip>
#include <chrono>
#include <filesystem>
#include <algorithm>
#include <limits>
#include "json.hpp"

using namespace std;
using namespace std::chrono;
using json = nlohmann::json;

// Generate a vector of evenly spaced alpha values
vector<double> generateAlphaValues(double minAlpha, double maxAlpha, int numTicks)
{
    vector<double> alphas;

    if (numTicks <= 1)
    {
        alphas.push_back(minAlpha);
        return alphas;
    }

    double step = (maxAlpha - minAlpha) / (numTicks - 1);
    for (int i = 0; i < numTicks; i++)
    {
        alphas.push_back(minAlpha + i * step);
    }

    return alphas;
}

// New function to analyze and export symbol information for reference sequences
bool analyzeSymbolInformation(const string &sampleFile, const vector<Reference> &topRefs, int k, double alpha,
                              const string &timestampSymbolDir, const string &latestSymbolDir)
{
    cout << "\n=============================================" << endl;
    cout << "Analyzing symbol information for top matches" << endl;
    cout << "=============================================" << endl;

    string sample = readMetagenomicSample(sampleFile);
    if (sample.empty())
    {
        cerr << "Error: Empty sample" << endl;
        return false;
    }

    // Create FCM model from sample
    FCMModel model(k, alpha);
    model.learn(sample);
    model.lockModel();

    // Generate timestamp for unique filenames
    auto now = system_clock::now();
    time_t now_time = system_clock::to_time_t(now);
    tm *now_tm = localtime(&now_time);

    char timeStr[20];
    strftime(timeStr, sizeof(timeStr), "%Y%m%d_%H%M%S", now_tm);

    // Process each top reference
    vector<string> exportedFiles;
    for (size_t i = 0; i < topRefs.size(); i++)
    {
        cout << "Processing " << topRefs[i].name << " (rank " << i + 1 << ")..." << endl;

        // Create base filename with organism name and rank
        string safeName = topRefs[i].name;
        // Replace problematic characters in filename
        for (char &c : safeName)
        {
            if (c == ' ' || c == '/' || c == '\\' || c == ':' || c == '*' || c == '?' || c == '"' || c == '<' || c == '>' || c == '|')
            {
                c = '_';
            }
        }

        string baseTimestampFile = timestampSymbolDir + "/" + "rank" + to_string(i + 1) + "_" + safeName;
        string baseLatestFile = latestSymbolDir + "/" + "rank" + to_string(i + 1) + "_" + safeName;

        try
        {
            // Export symbol information to both directories
            string exportedTimestampFile = model.exportSymbolInformation(topRefs[i].sequence, baseTimestampFile);
            string exportedLatestFile = model.exportSymbolInformation(topRefs[i].sequence, baseLatestFile);

            exportedFiles.push_back(exportedTimestampFile);

            // Calculate average information content for comparison
            double avgInfo = model.computeAverageInformationContent(topRefs[i].sequence);
            cout << "  Average information content: " << fixed << setprecision(6) << avgInfo << " bits/symbol" << endl;
            cout << "  Symbol information exported to: " << endl;
            cout << "    - " << exportedTimestampFile << endl;
            cout << "    - " << exportedLatestFile << endl;
        }
        catch (const exception &e)
        {
            cerr << "  Error processing reference: " << e.what() << endl;
        }
    }

    if (!exportedFiles.empty())
    {
        cout << "\nExported symbol information for " << exportedFiles.size() << " references." << endl;
        cout << "Files saved to directories: " << endl;
        cout << "  - " << filesystem::absolute(timestampSymbolDir).string() << endl;
        cout << "  - " << filesystem::absolute(latestSymbolDir).string() << endl;
        return true;
    }

    return false;
}

vector<pair<int, string>> createChunks(const string &sequence, int &chunkSize, int &overlap)
{
    if (chunkSize > 50000)
    {
        cout << "Warning: Chunk size too large. Limiting to 50000.\n";
        chunkSize = 50000;
    }
    if (overlap >= chunkSize)
    {
        cout << "Warning: Overlap >= chunk size. Adjusting overlap.\n";
        overlap = chunkSize / 2;
    }

    vector<pair<int, string>> chunks;
    for (int i = 0; i <= static_cast<int>(sequence.length()) - chunkSize; i += (chunkSize - overlap))
    {
        chunks.emplace_back(i, sequence.substr(i, chunkSize));
        if (chunks.size() >= 1000)
            break;
    }
    return chunks;
}

json analyzeChunk(const string &chunk, int position, int k, double alpha, const vector<Reference> &refs)
{
    string sanitized = chunk;
    for (char &c : sanitized)
        if (c != 'A' && c != 'C' && c != 'G' && c != 'T')
            c = 'A';

    json chunkResult = {
        {"position", position}, {"length", sanitized.length()}, {"best_match", "unknown"}, {"best_nrc", numeric_limits<double>::max()}, {"top_matches", json::array()}};

    try
    {
        FCMModel model(k, alpha);
        model.learn(sanitized);
        model.lockModel();
        DNACompressor compressor(model);

        vector<pair<string, double>> scores;
        for (const auto &ref : refs)
        {
            try
            {
                double nrc = compressor.calculateNRC(ref.sequence);
                scores.emplace_back(ref.name, nrc);
                if (nrc < chunkResult["best_nrc"])
                {
                    chunkResult["best_nrc"] = nrc;
                    chunkResult["best_match"] = ref.name;
                }
            }
            catch (...)
            {
                cerr << "Error calculating NRC for " << ref.name << endl;
            }
        }

        sort(scores.begin(), scores.end(), [](const auto &a, const auto &b)
             { return a.second < b.second; });
        for (size_t i = 0; i < min(size_t(3), scores.size()); ++i)
            chunkResult["top_matches"].push_back({{"name", scores[i].first}, {"nrc", scores[i].second}});
    }
    catch (const exception &e)
    {
        chunkResult["error"] = string("Processing error: ") + e.what();
    }
    catch (...)
    {
        chunkResult["error"] = "Unknown processing error";
    }

    return chunkResult;
}

bool analyzeChunks(const string &sampleFile, const vector<Reference> &references, int k, double alpha,
                   int chunkSize, int overlap, const string &timestampDir, const string &latestDir)
{
    cout << "\nStarting chunk analysis..." << endl;

    string sample = readMetagenomicSample(sampleFile);
    if (sample.empty())
    {
        cerr << "Error: Sample is empty.\n";
        return false;
    }

    auto chunks = createChunks(sample, chunkSize, overlap);
    cout << "Created " << chunks.size() << " chunks.\n";

    json results = {
        {"k", k}, {"alpha", alpha}, {"chunk_size", chunkSize}, {"overlap", overlap}, {"sample_length", sample.length()}, {"chunk_count", chunks.size()}, {"chunks", json::array()}, {"completed", true}};

    for (size_t i = 0; i < chunks.size(); ++i)
    {
        if (i % 10 == 0 || i == chunks.size() - 1)
            cout << "\rProcessing chunk " << (i + 1) << "/" << chunks.size() << flush;

        auto &[pos, seq] = chunks[i];
        results["chunks"].push_back(analyzeChunk(seq, pos, k, alpha, references));
    }

    cout << "\nAnalysis complete.\n";
    results["processed_chunks"] = results["chunks"].size();

    return saveResults(results, timestampDir + "/chunk_analysis.json", latestDir + "/chunk_analysis.json");
}