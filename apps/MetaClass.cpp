#include "FCMModel.h"
#include "utils.h"
#include <iostream>
#include <fstream>
#include <cmath>
#include <algorithm>
#include <getopt.h>
#include <vector>
#include <string>
#include <iomanip>
#include <thread>
#include <mutex>
#include <chrono>

using namespace std;
using namespace std::chrono;

// Print help menu
void printHelp(const char* programName) {
    cout << "MetaClass: A tool for DNA sequence similarity using Normalized Relative Compression" << endl;
    cout << "Usage: " << programName << " [options]" << endl;
    cout << "\nOptions:" << endl;
    cout << "  -h, --help                 Show this help message" << endl;
    cout << "  -d, --database <file>      Reference database file" << endl;
    cout << "  -s, --sample <file>        Metagenomic sample file" << endl;
    cout << "  -k, --context <size>       Context size (default: 10)" << endl;
    cout << "  -a, --alpha <value>        Smoothing parameter (default: 0.1)" << endl;
    cout << "  -t, --top <count>          Number of top results to display (default: 20)" << endl;
    cout << "  -m, --save-model <file>    Save the trained model to a file (without extension)" << endl;
    cout << "  -l, --load-model <file>    Load a model from file instead of training" << endl;
    cout << "  --json                     Use JSON format for model saving/loading (default is binary)" << endl;
    cout << "  -p, --threads <count>      Number of parallel threads to use (default: hardware cores)" << endl;
    cout << "\nExamples:" << endl;
    cout << "  " << programName << " -d ../data/samples/db.txt -s ../data/samples/meta.txt" << endl;
    cout << "  " << programName << " -d ../data/samples/db.txt -s ../data/samples/meta.txt -k 8 -a 0.05 -t 10" << endl;
    cout << "  " << programName << " -d ../data/samples/db.txt -s ../data/samples/meta.txt -m ../data/models/model" << endl;
    cout << "  " << programName << " -d ../data/samples/db.txt -l ../data/models/model.bson" << endl;
    cout << "  " << programName << " -d ../data/samples/db.txt -s ../data/samples/meta.txt -m ../data/models/model --json" << endl;
}

// Worker function for parallel processing
void calculateMetricsBatch(vector <Reference>& reference, size_t start, size_t end, DNACompressor& compressor, mutex& mtx){
    for (size_t i = start; i < end && i < reference.size(); ++i){
        double nrc = compressor.calculateNRC(reference[i].sequence);
        double kld = compressor.calculateKLD(reference[i].sequence);

        // Thread-safe update of the references
        lock_guard<mutex> lock(mtx);
        reference[i].nrc = nrc;
        reference[i].kld = kld;
    }
}

// Main function
int main(int argc, char* argv[]) {
    // Default parameter values
    string dbFile, sampleFile, saveModelFile, loadModelFile;
    int k = 10;
    double alpha = 0.1;
    int topN = 20;
    bool showHelp = false;
    bool useJson = false;
    int numThreads = thread::hardware_concurrency(); // Default to hardware cores

    // Define long options
    static struct option long_options[] = {
        {"help",       no_argument,       0, 'h'},
        {"database",   required_argument, 0, 'd'},
        {"sample",     required_argument, 0, 's'},
        {"context",    required_argument, 0, 'k'},
        {"alpha",      required_argument, 0, 'a'},
        {"top",        required_argument, 0, 't'},
        {"save-model", required_argument, 0, 'm'},
        {"load-model", required_argument, 0, 'l'},
        {"json",       no_argument,       0, 'j'},
        {"threads",    required_argument, 0, 'p'},
        {0, 0, 0, 0}
    };

    // Parse command line options
    int option_index = 0;
    int opt;
    while ((opt = getopt_long(argc, argv, "hd:s:k:a:t:m:l:jp:", long_options, &option_index)) != -1) {
        switch (opt) {
            case 'h':
                showHelp = true;
                break;
            case 'd':
                dbFile = optarg;
                break;
            case 's':
                sampleFile = optarg;
                break;
            case 'k':
                k = stoi(optarg);
                break;
            case 'a':
                alpha = stod(optarg);
                break;
            case 't':
                topN = stoi(optarg);
                break;
            case 'm':
                saveModelFile = optarg;
                break;
            case 'l':
                loadModelFile = optarg;
                break;
            case 'j':
                useJson = true;
                break;
            case 'p':
                numThreads = stoi(optarg);
                break;
            default:
                cerr << "Unknown option. Use --help for usage information." << endl;
                return 1;
        }
    }

    // Show help if requested or if required arguments are missing
    if (showHelp || (dbFile.empty() && !showHelp)) {
        printHelp(argv[0]);
        return showHelp ? 0 : 1;
    }

    // Verify database file is provided
    if (dbFile.empty()) {
        cerr << "Error: Database file is required." << endl;
        return 1;
    }

    // Create FCMModel
    FCMModel model(k, alpha);

    try {
        // Load model if specified
        if (!loadModelFile.empty()) {
            cout << "Loading model from file: " << loadModelFile << endl;
            
            // Determine if the file is JSON or binary based on extension
            bool isBinaryFile = loadModelFile.find(".json") == string::npos;
            
            // Override with command line option
            if (useJson) {
                isBinaryFile = false;
            }
            
            model.importModel(loadModelFile, isBinaryFile);
            k = model.getK();
            alpha = model.getAlpha();
            cout << "Model loaded successfully (k=" << k << ", alpha=" << alpha << ")" << endl;
        }
        // Otherwise, train on the sample
        else if (!sampleFile.empty()) {
            cout << "Reading metagenomic sample from: " << sampleFile << endl;
            string sample = readMetagenomicSample(sampleFile);

            if (sample.empty()) {
                cerr << "Error: Empty metagenomic sample" << endl;
                return 1;
            }

            cout << "Metagenomic sample length: " << sample.length() << " nucleotides" << endl;
            cout << "Training FCM model with k=" << k << ", alpha=" << alpha << endl;
            
            model.learn(sample);
            model.lockModel(); // Lock the model after training
            
            // Save model if requested
            if (!saveModelFile.empty()) {
                cout << "Saving model to file: " << saveModelFile << endl;
                string exportedFile = model.exportModel(saveModelFile, !useJson);
                cout << "Model saved to: " << exportedFile << endl;
            }
        } else {
            cerr << "Error: Either a sample file (-s) or a model file (-l) must be provided." << endl;
            return 1;
        }

        // Read the reference database
        cout << "Reading reference database from: " << dbFile << endl;
        vector<Reference> references = readReferenceDatabase(dbFile);

        if (references.empty()) {
            cerr << "Error: No references found in database" << endl;
            return 1;
        }

        cout << "Found " << references.size() << " reference sequences" << endl;

        // Create DNA compressor using the FCMModel
        DNACompressor compressor(model);

        auto startTime = high_resolution_clock::now();

        // Multi-threaded calculation of NRC and KLD metrics
        cout << "Calculating metrics using " << numThreads << " threads..." << endl;
        vector<thread> threads;
        mutex mtx;  // Mutex for thread synchronization

        size_t batchSize = (references.size() + numThreads - 1) / numThreads;

        for (int t = 0; t < numThreads; ++t) {
            size_t start = t * batchSize;
            size_t end = min((t + 1) * batchSize, references.size());

            if (start >= references.size()) break;

            threads.push_back(thread(calculateMetricsBatch,
                                    ref(references),
                                    start,
                                    end,
                                    ref(compressor),
                                    ref(mtx)
            ));
        }

        // Wait for all threads to complete
        for (auto& t : threads){
            t.join();
        }

        auto endTime = high_resolution_clock::now();
        auto duration = duration_cast<milliseconds>(endTime - startTime).count();

        cout << "Metrics calculation completed in " << duration << " ms" << endl;

        // Sort references by NRC (lowest to highest = most to least similar)
        sort(references.begin(), references.end(), 
            [](const Reference& a, const Reference& b) { return a.nrc < b.nrc; });

        // Print the top N results
        cout << "\nTop " << topN << " most similar sequences:" << endl;
        cout << "-----------------------------------------------------------------------" << endl;
        cout << setw(4) << "Rank" << " | " << setw(10) << "NRC" << " | " 
             << setw(10) << "KL-Div" << " | " << "Reference Name" << endl;
        cout << "-----------------------------------------------------------------------" << endl;
        
        for (int i = 0; i < min(topN, static_cast<int>(references.size())); ++i) {
            cout << setw(4) << i+1 << " | " 
                 << setw(10) << fixed << setprecision(6) << references[i].nrc << " | "
                 << setw(10) << fixed << setprecision(6) << references[i].kld << " | "
                 << references[i].name << endl;
        }

    } catch (const exception& e) {
        cerr << "Error: " << e.what() << endl;
        return 1;
    }

    return 0;
}