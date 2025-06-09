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
    string cmd1 = "lzma -c /tmp/temp1.txt > /tmp/temp1.txt.lzma 2>/dev/null";
    string cmd2 = "lzma -c /tmp/temp2.txt > /tmp/temp2.txt.lzma 2>/dev/null";
    string cmd_concat = "lzma -c /tmp/temp_concat.txt > /tmp/temp_concat.txt.lzma 2>/dev/null";
    
    system(cmd1.c_str());
    system(cmd2.c_str());
    system(cmd_concat.c_str());
    
    // Get sizes
    auto size1 = filesystem::file_size("/tmp/temp1.txt.lzma");
    auto size2 = filesystem::file_size("/tmp/temp2.txt.lzma");
    auto size_concat = filesystem::file_size("/tmp/temp_concat.txt.lzma");
    
    // Calculate NCD
    double ncd = (double)(size_concat - min(size1, size2)) / max(size1, size2);
    
    // Cleanup
    system("rm -f /tmp/temp*.txt* 2>/dev/null");
    
    return ncd;
}

// Function to get all .feat files from directory
vector<string> getAllFeatureFiles(const string& directory) {
    vector<string> files;
    for (const auto& entry : filesystem::directory_iterator(directory)) {
        if (entry.path().extension() == ".feat") {
            files.push_back(entry.path().filename().string());
        }
    }
    sort(files.begin(), files.end());
    return files;
}

// Function to get a clean song name for display
string getCleanSongName(const string& filename) {
    string name = filename;
    // Remove "_spectral.feat" suffix
    size_t pos = name.find("_spectral.feat");
    if (pos != string::npos) {
        name = name.substr(0, pos);
    }
    // Remove "-Main-version" suffix
    pos = name.find("-Main-version");
    if (pos != string::npos) {
        name = name.substr(0, pos);
    }
    return name;
}

int main() {
    // Initialize dual output (console + file)
    dual_out = new DualOutput("complete_test_results.txt");
    
    auto start_time = chrono::high_resolution_clock::now();
    
    *dual_out << "COMPLETE MUSIC IDENTIFICATION TEST - ALL SONGS\n";
    *dual_out << "===============================================\n";
    *dual_out << "Test started at: " << chrono::duration_cast<chrono::seconds>(start_time.time_since_epoch()).count() << "\n\n";
    
    // Extract features first to ensure they exist
    *dual_out << "Extracting features from sample files...\n";
    string extract_cmd = "cd /home/maria/Desktop/TAI_Project3 && ./scripts/run.sh extract_features data/samples test.features";
    int result = system(extract_cmd.c_str());
    if (result != 0) {
        *dual_out << "Warning: Feature extraction failed. Some tests may not work.\n";
    } else {
        *dual_out << "Feature extraction completed successfully.\n";
    }
    *dual_out << "\n";
    
    string feature_dir = "../test.features/";
    vector<string> songs = getAllFeatureFiles(feature_dir);
    
    *dual_out << "Found " << songs.size() << " songs for testing:\n";
    for (size_t i = 0; i < songs.size(); i++) {
        *dual_out << "  " << (i+1) << ". " << getCleanSongName(songs[i]) << "\n";
    }
    *dual_out << "\n";
    
    // Test 1: Self-similarity test for all songs
    *dual_out << "=== SELF-SIMILARITY TEST (ALL " << songs.size() << " SONGS) ===\n";
    vector<double> self_ncds;
    for (const auto& song : songs) {
        string filepath = feature_dir + song;
        if (filesystem::exists(filepath)) {
            double self_ncd = calculateNCD(filepath, filepath);
            self_ncds.push_back(self_ncd);
            *dual_out << getCleanSongName(song) << ": " << fixed << setprecision(6) << self_ncd << "\n";
        }
    }
    
    // Statistics for self-similarity
    if (!self_ncds.empty()) {
        double avg_self = accumulate(self_ncds.begin(), self_ncds.end(), 0.0) / self_ncds.size();
        double min_self = *min_element(self_ncds.begin(), self_ncds.end());
        double max_self = *max_element(self_ncds.begin(), self_ncds.end());
        *dual_out << "\nSelf-similarity statistics:\n";
        *dual_out << "  Average: " << fixed << setprecision(6) << avg_self << "\n";
        *dual_out << "  Range: " << min_self << " - " << max_self << "\n";
    }
    *dual_out << "\n";
    
    // Test 2: Music identification ranking for all songs
    *dual_out << "=== MUSIC IDENTIFICATION TEST (ALL VS ALL) ===\n";
    int correct_identifications = 0;
    int total_queries = 0;
    vector<double> different_ncds;
    
    *dual_out << "Testing each song as query against all " << songs.size() << " songs...\n\n";
    
    for (size_t q = 0; q < songs.size(); q++) {
        const auto& query = songs[q];
        string query_path = feature_dir + query;
        if (!filesystem::exists(query_path)) continue;
        
        total_queries++;
        *dual_out << "Query " << total_queries << "/" << songs.size() << ": " << getCleanSongName(query) << "\n";
        
        vector<pair<double, string>> matches;
        for (const auto& db_song : songs) {
            string db_path = feature_dir + db_song;
            if (!filesystem::exists(db_path)) continue;
            
            double ncd = calculateNCD(query_path, db_path);
            matches.push_back({ncd, db_song});
            
            // Collect different song NCDs for statistics
            if (query != db_song) {
                different_ncds.push_back(ncd);
            }
        }
        
        // Sort by NCD (lower is better)
        sort(matches.begin(), matches.end());
        
        // Show top 5 matches
        *dual_out << "  Top 5 matches:\n";
        for (size_t i = 0; i < min(5, (int)matches.size()); i++) {
            *dual_out << "    " << (i+1) << ". " << getCleanSongName(matches[i].second) 
                 << " (NCD: " << fixed << setprecision(6) << matches[i].first << ")";
            if (i == 0 && matches[i].second == query) {
                *dual_out << " ← CORRECT!";
                correct_identifications++;
            } else if (i == 0) {
                *dual_out << " ← INCORRECT!";
            }
            *dual_out << "\n";
        }
        *dual_out << "\n";
    }
    
    // Test 3: Comprehensive statistics
    *dual_out << "=== COMPREHENSIVE PERFORMANCE ANALYSIS ===\n";
    
    // Accuracy
    double accuracy = (double)correct_identifications / total_queries * 100.0;
    *dual_out << "Overall Accuracy: " << correct_identifications << "/" << total_queries 
         << " (" << fixed << setprecision(2) << accuracy << "%)\n";
    
    // Self vs Different NCD statistics
    if (!self_ncds.empty() && !different_ncds.empty()) {
        double avg_self = accumulate(self_ncds.begin(), self_ncds.end(), 0.0) / self_ncds.size();
        double avg_diff = accumulate(different_ncds.begin(), different_ncds.end(), 0.0) / different_ncds.size();
        double min_self = *min_element(self_ncds.begin(), self_ncds.end());
        double max_self = *max_element(self_ncds.begin(), self_ncds.end());
        double min_diff = *min_element(different_ncds.begin(), different_ncds.end());
        double max_diff = *max_element(different_ncds.begin(), different_ncds.end());
        
        *dual_out << "\nNCD Statistics:\n";
        *dual_out << "  Self-matches:\n";
        *dual_out << "    Average: " << fixed << setprecision(6) << avg_self << "\n";
        *dual_out << "    Range: " << min_self << " - " << max_self << "\n";
        *dual_out << "  Different songs:\n";
        *dual_out << "    Average: " << fixed << setprecision(6) << avg_diff << "\n";
        *dual_out << "    Range: " << min_diff << " - " << max_diff << "\n";
        *dual_out << "  Discrimination gap: " << (min_diff - max_self) << "\n";
        *dual_out << "  Separation ratio: " << (avg_diff / avg_self) << "x\n";
    }
    
    // Timing
    auto end_time = chrono::high_resolution_clock::now();
    auto duration = chrono::duration_cast<chrono::seconds>(end_time - start_time);
    *dual_out << "\nTotal test duration: " << duration.count() << " seconds\n";
    
    // Test 4: Confusion analysis (find most confusing pairs)
    *dual_out << "\n=== CONFUSION ANALYSIS ===\n";
    vector<tuple<double, string, string>> confusions;
    
    for (size_t i = 0; i < songs.size(); i++) {
        string query_path = feature_dir + songs[i];
        if (!filesystem::exists(query_path)) continue;
        
        vector<pair<double, string>> matches;
        for (size_t j = 0; j < songs.size(); j++) {
            if (i == j) continue; // Skip self-comparison
            string db_path = feature_dir + songs[j];
            if (!filesystem::exists(db_path)) continue;
            
            double ncd = calculateNCD(query_path, db_path);
            matches.push_back({ncd, songs[j]});
        }
        
        sort(matches.begin(), matches.end());
        if (!matches.empty()) {
            // Most similar different song
            confusions.push_back({matches[0].first, songs[i], matches[0].second});
        }
    }
    
    sort(confusions.begin(), confusions.end());
    *dual_out << "Most confusing song pairs (lowest NCD between different songs):\n";
    for (size_t i = 0; i < min(10, (int)confusions.size()); i++) {
        *dual_out << "  " << (i+1) << ". " << getCleanSongName(get<1>(confusions[i])) 
             << " ↔ " << getCleanSongName(get<2>(confusions[i]))
             << " (NCD: " << fixed << setprecision(6) << get<0>(confusions[i]) << ")\n";
    }
    
    *dual_out << "\n=== TEST COMPLETED ===\n";
    *dual_out << "Tested " << total_queries << " songs with " << correct_identifications 
         << " correct identifications (" << fixed << setprecision(2) << accuracy << "% accuracy)\n";
    
    *dual_out << "Test completed in " << duration.count() << " seconds\n";
    *dual_out << "Results saved to: complete_test_results.txt\n";
    
    delete dual_out;
    return 0;
}
