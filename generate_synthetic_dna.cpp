#include <iostream>
#include <fstream>
#include <random>
#include <string>
#include <ctime>

// Function to generate a random DNA sequence
std::string generateDNASequence(int length, std::mt19937& rng) {
    std::string dna;
    dna.reserve(length);

    static const char nucleotides[] = "ACGT";
    std::uniform_int_distribution<int> dist(0, 3);

    for (int i = 0; i < length; ++i) {
        dna += nucleotides[dist(rng)];
    }

    return dna;
}

// Function to create a database file
void createDatabaseFile(const std::string& filename, int numSequences,
                        int minLength, int maxLength, std::mt19937& rng) {
    std::ofstream file(filename);

    if (!file.is_open()) {
        std::cerr << "Error: Could not open file " << filename << std::endl;
        return;
    }

    std::uniform_int_distribution<int> lengthDist(minLength, maxLength);

    for (int i = 0; i < numSequences; ++i) {
        int length = lengthDist(rng);
        std::string sequence = generateDNASequence(length, rng);

        file << "@Species_" << (i + 1) << "\n";

        // Write sequence with line wrapping (70 characters per line)
        for (size_t j = 0; j < sequence.length(); j += 70) {
            file << sequence.substr(j, 70) << "\n";
        }
    }

    file.close();
    std::cout << "Created database file " << filename << " with " << numSequences << " sequences\n";
}

// Function to create a metagenomic sample file
void createMetagenomicFile(const std::string& filename, int numSequences,
                            int minLength, int maxLength, std::mt19937& rng) {
    std::ofstream file(filename);

    if (!file.is_open()) {
        std::cerr << "Error: Could not open file " << filename << std::endl;
        return;
    }

    std::uniform_int_distribution<int> lengthDist(minLength, maxLength);

    // For metagenomic file, just concatenate sequences
    for (int i = 0; i < numSequences; ++i) {
        int length = lengthDist(rng);
        std::string sequence = generateDNASequence(length, rng);
        file << sequence;
    }

    file.close();
    std::cout << "Created metagenomic file " << filename << " with " << numSequences << " sequences\n";
}

int main(int argc, char* argv[]) {
    // Seed random number generator
    std::random_device rd;
    std::mt19937 rng(rd());

    // Default parameters
    int dbSequences = 10;
    int metaSequences = 5;
    int minLength = 1000;
    int maxLength = 5000;
    std::string dbFilename = "samples/synthetic/synthetic_db.txt";
    std::string metaFilename = "samples/synthetic/synthetic_meta.txt";

    // Generate files
    createDatabaseFile(dbFilename, dbSequences, minLength, maxLength, rng);
    createMetagenomicFile(metaFilename, metaSequences, minLength, maxLength, rng);

    std::cout << "Synthetic DNA data generation complete.\n";
    std::cout << "Try running: ./MetaClass -d " << dbFilename << " -s " << metaFilename << " -k 10 -a 0.1 -t 20\n";
    return 0;
}
