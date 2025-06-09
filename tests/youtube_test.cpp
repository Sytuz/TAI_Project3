#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <filesystem>
#include <sstream>
#include <map>
#include <iomanip>
#include <algorithm>
#include <chrono>
#include <numeric>
#include <random>

using namespace std;

// Global output stream for both console and file
class DualOutput {
public:
    ofstream file_stream;
    
    DualOutput(const string& filename) {
        file_stream.open(filename);
    }
    
    ~DualOutput() {
        if (file_stream.is_open()) {
            file_stream.close();
        }
    }
    
    template<typename T>
    DualOutput& operator<<(const T& value) {
        cout << value;
        if (file_stream.is_open()) {
            file_stream << value;
        }
        return *this;
    }
    
    DualOutput& operator<<(ostream& (*manip)(ostream&)) {
        cout << manip;
        if (file_stream.is_open()) {
            file_stream << manip;
        }
        return *this;
    }
};

DualOutput* dual_out = nullptr;

// Simple NCD calculation using system calls
double calculateNCD(const string& file1, const string& file2, const string& compressor = "lzma") {
    // Read files
    ifstream f1(file1);
    ifstream f2(file2);
    
    string content1, content2, line;
    while (getline(f1, line)) {
        content1 += line + "\n";
    }
    while (getline(f2, line)) {
        content2 += line + "\n";
    }
    f1.close();
    f2.close();
    
    // Write temporary files
    ofstream temp1("/tmp/temp1.txt");
    temp1 << content1;
    temp1.close();
    
    ofstream temp2("/tmp/temp2.txt");
    temp2 << content2;
    temp2.close();
    
    ofstream temp_concat("/tmp/temp_concat.txt");
    temp_concat << content1 << content2;
    temp_concat.close();
    
    // Compress files
    string cmd1, cmd2, cmd_concat;
    if (compressor == "gzip") {
        cmd1 = "gzip -c /tmp/temp1.txt > /tmp/temp1.txt.gz 2>/dev/null";
        cmd2 = "gzip -c /tmp/temp2.txt > /tmp/temp2.txt.gz 2>/dev/null";
        cmd_concat = "gzip -c /tmp/temp_concat.txt > /tmp/temp_concat.txt.gz 2>/dev/null";
    } else if (compressor == "bzip2") {
        cmd1 = "bzip2 -c /tmp/temp1.txt > /tmp/temp1.txt.bz2 2>/dev/null";
        cmd2 = "bzip2 -c /tmp/temp2.txt > /tmp/temp2.txt.bz2 2>/dev/null";
        cmd_concat = "bzip2 -c /tmp/temp_concat.txt > /tmp/temp_concat.txt.bz2 2>/dev/null";
    } else { // lzma default
        cmd1 = "lzma -c /tmp/temp1.txt > /tmp/temp1.txt.lzma 2>/dev/null";
        cmd2 = "lzma -c /tmp/temp2.txt > /tmp/temp2.txt.lzma 2>/dev/null";
        cmd_concat = "lzma -c /tmp/temp_concat.txt > /tmp/temp_concat.txt.lzma 2>/dev/null";
    }
    
    system(cmd1.c_str());
    system(cmd2.c_str());
    system(cmd_concat.c_str());
    
    // Get file sizes
    string ext = (compressor == "gzip") ? ".gz" : 
                 (compressor == "bzip2") ? ".bz2" : ".lzma";
    
    ifstream size1("/tmp/temp1.txt" + ext, ios::binary | ios::ate);
    ifstream size2("/tmp/temp2.txt" + ext, ios::binary | ios::ate);
    ifstream size_concat("/tmp/temp_concat.txt" + ext, ios::binary | ios::ate);
    
    long s1 = size1.tellg();
    long s2 = size2.tellg();
    long s12 = size_concat.tellg();
    
    size1.close();
    size2.close();
    size_concat.close();
    
    if (s1 == 0 || s2 == 0 || s12 == 0) return 1.0;
    
    return (double)(s12 - min(s1, s2)) / max(s1, s2);
}

// Genre classification based on filename patterns
string classifyGenre(const string& filename) {
    string lower_name = filename;
    transform(lower_name.begin(), lower_name.end(), lower_name.begin(), ::tolower);
    
    if (lower_name.find("jazz") != string::npos || 
        lower_name.find("swing") != string::npos ||
        lower_name.find("bebop") != string::npos) return "Jazz";
    
    if (lower_name.find("reggae") != string::npos ||
        lower_name.find("bob_marley") != string::npos ||
        lower_name.find("jamaica") != string::npos) return "Reggae";
    
    if (lower_name.find("classical") != string::npos ||
        lower_name.find("mozart") != string::npos ||
        lower_name.find("beethoven") != string::npos ||
        lower_name.find("bach") != string::npos ||
        lower_name.find("symphony") != string::npos ||
        lower_name.find("concerto") != string::npos) return "Classical";
    
    if (lower_name.find("hip") != string::npos ||
        lower_name.find("rap") != string::npos ||
        lower_name.find("hiphop") != string::npos) return "Hip-Hop";
    
    if (lower_name.find("electronic") != string::npos ||
        lower_name.find("edm") != string::npos ||
        lower_name.find("techno") != string::npos ||
        lower_name.find("house") != string::npos) return "Electronic";
    
    if (lower_name.find("rock") != string::npos ||
        lower_name.find("metal") != string::npos) return "Rock";
    
    if (lower_name.find("pop") != string::npos) return "Pop";
    
    if (lower_name.find("blues") != string::npos) return "Blues";
    
    return "Other";
}

// Test with subset of songs
void testSubset(const vector<string>& songs, int num_songs, const string& compressor = "lzma") {
    *dual_out << "\n=== SUBSET TEST: " << num_songs << " songs with " << compressor << " compressor ===\n";
    
    vector<string> subset(songs.begin(), songs.begin() + min(num_songs, (int)songs.size()));
    
    int correct = 0;
    int total = subset.size();
    
    auto start_time = chrono::high_resolution_clock::now();
    
    for (int i = 0; i < subset.size(); i++) {
        double best_ncd = 1.0;
        string best_match;
        
        for (int j = 0; j < subset.size(); j++) {
            if (i == j) continue;
            
            double ncd = calculateNCD(subset[i], subset[j], compressor);
            if (ncd < best_ncd) {
                best_ncd = ncd;
                best_match = subset[j];
            }
        }
        
        // Get song names
        string query_name = filesystem::path(subset[i]).stem().string();
        string match_name = filesystem::path(best_match).stem().string();
        
        if (query_name == match_name) {
            correct++;
            *dual_out << "✓ " << query_name << " (NCD: " << fixed << setprecision(3) << best_ncd << ")\n";
        } else {
            *dual_out << "✗ " << query_name << " → " << match_name << " (NCD: " << fixed << setprecision(3) << best_ncd << ")\n";
        }
    }
    
    auto end_time = chrono::high_resolution_clock::now();
    auto duration = chrono::duration_cast<chrono::seconds>(end_time - start_time);
    
    double accuracy = (double)correct / total * 100.0;
    *dual_out << "\nAccuracy: " << correct << "/" << total << " (" << fixed << setprecision(2) << accuracy << "%)\n";
    *dual_out << "Time: " << duration.count() << " seconds\n";
}

// Compare different compressors
void compareCompressors(const vector<string>& songs, int num_songs = 10) {
    *dual_out << "\n=== COMPRESSOR COMPARISON TEST ===\n";
    
    vector<string> compressors = {"gzip", "bzip2", "lzma"};
    
    for (const string& comp : compressors) {
        testSubset(songs, num_songs, comp);
    }
}

// Genre diversity test
void testGenreDiversity(const vector<string>& songs) {
    *dual_out << "\n=== GENRE DIVERSITY TEST ===\n";
    
    map<string, vector<string>> genre_songs;
    
    // Classify all songs by genre
    for (const string& song : songs) {
        string genre = classifyGenre(song);
        genre_songs[genre].push_back(song);
    }
    
    *dual_out << "Genre distribution:\n";
    for (const auto& pair : genre_songs) {
        *dual_out << "  " << pair.first << ": " << pair.second.size() << " songs\n";
    }
    
    // Select diverse subset (max 3 songs per genre)
    vector<string> diverse_subset;
    for (const auto& pair : genre_songs) {
        int count = 0;
        for (const string& song : pair.second) {
            if (count < 3) {
                diverse_subset.push_back(song);
                count++;
            }
        }
    }
    
    *dual_out << "\nTesting diverse subset of " << diverse_subset.size() << " songs...\n";
    testSubset(diverse_subset, diverse_subset.size());
}

int main() {
    // Initialize dual output (console + file)
    dual_out = new DualOutput("youtube_test_results.txt");
    
    auto start_time = chrono::high_resolution_clock::now();
    
    *dual_out << "YOUTUBE MUSIC IDENTIFICATION COMPREHENSIVE TEST\n";
    *dual_out << "===============================================\n";
    *dual_out << "Test started at: " << chrono::duration_cast<chrono::seconds>(start_time.time_since_epoch()).count() << "\n\n";
    
    // Extract features first
    *dual_out << "Extracting features from YouTube songs...\n";
    string extract_cmd = "cd /home/maria/Desktop/TAI_Project3 && ./scripts/run.sh extract_features data/full_tracks youtube.features";
    int result = system(extract_cmd.c_str());
    if (result != 0) {
        *dual_out << "Error: Feature extraction failed. Make sure songs are in data/full_tracks/\n";
        delete dual_out;
        return 1;
    }
    
    // Get list of feature files
    vector<string> feature_files;
    
    if (filesystem::exists("../youtube.features")) {
        for (const auto& entry : filesystem::directory_iterator("../youtube.features")) {
            if (entry.path().extension() == ".txt") {
                feature_files.push_back(entry.path().string());
            }
        }
    }
    
    if (feature_files.empty()) {
        *dual_out << "No feature files found in youtube.features/ directory\n";
        return 1;
    }
    
    sort(feature_files.begin(), feature_files.end());
    *dual_out << "Found " << feature_files.size() << " YouTube songs\n\n";
    
    // Test 1: Quick subset test (10 songs)
    *dual_out << "1. Running quick subset test...\n";
    testSubset(feature_files, 10);
    
    // Test 2: Medium subset test (25 songs)
    *dual_out << "\n2. Running medium subset test...\n";
    testSubset(feature_files, 25);
    
    // Test 3: Compressor comparison
    *dual_out << "\n3. Comparing compressors...\n";
    compareCompressors(feature_files, 15);
    
    // Test 4: Genre diversity test
    *dual_out << "\n4. Genre diversity analysis...\n";
    testGenreDiversity(feature_files);
    
    // Test 5: Full test (if manageable)
    if (feature_files.size() <= 50) {
        *dual_out << "\n5. Running full test on all " << feature_files.size() << " songs...\n";
        testSubset(feature_files, feature_files.size());
    } else {
        *dual_out << "\n5. Skipping full test (too many songs: " << feature_files.size() << ")\n";
        *dual_out << "   For testing specific songs or the full " << feature_files.size() 
             << " songs, use the main application:\n";
        *dual_out << "./scripts/run.sh music_id query.wav youtube.features/ results.txt\n";
    }
    
    *dual_out << "\n=== TEST SUMMARY ===\n";
    *dual_out << "Total YouTube songs analyzed: " << feature_files.size() << "\n";
    *dual_out << "Tests completed successfully!\n";
    *dual_out << "\nFor production use with specific " << feature_files.size() 
         << " songs, use the main application:\n";
    *dual_out << "./scripts/run.sh music_id query.wav youtube.features/ results.txt\n";
    
    auto end_time = chrono::high_resolution_clock::now();
    auto duration = chrono::duration_cast<chrono::seconds>(end_time - start_time);
    *dual_out << "\nTest completed in " << duration.count() << " seconds\n";
    *dual_out << "Results saved to: youtube_test_results.txt\n";
    
    delete dual_out;
    return 0;
}
