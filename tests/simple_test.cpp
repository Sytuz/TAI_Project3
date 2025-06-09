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

int main() {
    // Initialize dual output (console + file)
    dual_out = new DualOutput("simple_test_results.txt");
    
    auto start_time = chrono::high_resolution_clock::now();
    
    *dual_out << "SIMPLE MUSIC IDENTIFICATION TEST\n";
    *dual_out << "================================\n";
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
    vector<string> songs = {
        "Colorful campus-Main-version_spectral.feat",
        "Sweet morning-Main-version_spectral.feat",
        "Jazz bar-Main-version_spectral.feat",
        "Positive energy enterprise promotion-Main-version_spectral.feat",
        "The detective is crazy-Main-version_spectral.feat"
    };
    
    // Test 1: Self-similarity (should be very low NCD)
    *dual_out << "=== Self-similarity test ===\n";
    for (const auto& song : songs) {
        string filepath = feature_dir + song;
        if (filesystem::exists(filepath)) {
            double self_ncd = calculateNCD(filepath, filepath);
            *dual_out << song << " vs itself: " << self_ncd << "\n";
        }
    }
    *dual_out << "\n";
    
    // Test 2: Cross-song comparison matrix
    *dual_out << "=== Cross-song comparison matrix ===\n";
    *dual_out << "Query\\Database\t";
    for (const auto& song : songs) {
        *dual_out << song.substr(0, 15) << "\t";
    }
    *dual_out << "\n";
    
    for (const auto& query : songs) {
        string query_path = feature_dir + query;
        if (!filesystem::exists(query_path)) continue;
        
        *dual_out << query.substr(0, 15) << "\t";
        
        for (const auto& db_song : songs) {
            string db_path = feature_dir + db_song;
            if (!filesystem::exists(db_path)) {
                *dual_out << "N/A\t\t";
                continue;
            }
            
            double ncd = calculateNCD(query_path, db_path);
            *dual_out << fixed << setprecision(6) << ncd << "\t";
        }
        *dual_out << "\n";
    }
    *dual_out << "\n";
    
    // Test 3: Music identification ranking
    *dual_out << "=== Music identification ranking ===\n";
    for (const auto& query : songs) {
        string query_path = feature_dir + query;
        if (!filesystem::exists(query_path)) continue;
        
        *dual_out << "Query: " << query << "\n";
        *dual_out << "Ranked matches:\n";
        
        vector<pair<double, string>> matches;
        for (const auto& db_song : songs) {
            string db_path = feature_dir + db_song;
            if (!filesystem::exists(db_path)) continue;
            
            double ncd = calculateNCD(query_path, db_path);
            matches.push_back({ncd, db_song});
        }
        
        // Sort by NCD (lower is better)
        sort(matches.begin(), matches.end());
        
        for (size_t i = 0; i < matches.size(); i++) {
            *dual_out << "  " << (i+1) << ". " << matches[i].second 
                 << " (NCD: " << fixed << setprecision(6) << matches[i].first << ")";
            if (i == 0 && matches[i].second == query) {
                *dual_out << " ← CORRECT!";
            } else if (i == 0) {
                *dual_out << " ← INCORRECT!";
            }
            *dual_out << "\n";
        }
        *dual_out << "\n";
    }
    
    // Test 4: Performance summary
    *dual_out << "=== Performance Summary ===\n";
    int correct_identifications = 0;
    int total_queries = 0;
    
    double min_self_ncd = 1.0, max_self_ncd = 0.0;
    double min_diff_ncd = 1.0, max_diff_ncd = 0.0;
    
    for (const auto& query : songs) {
        string query_path = feature_dir + query;
        if (!filesystem::exists(query_path)) continue;
        
        total_queries++;
        
        vector<pair<double, string>> matches;
        for (const auto& db_song : songs) {
            string db_path = feature_dir + db_song;
            if (!filesystem::exists(db_path)) continue;
            
            double ncd = calculateNCD(query_path, db_path);
            matches.push_back({ncd, db_song});
            
            if (query == db_song) {
                min_self_ncd = min(min_self_ncd, ncd);
                max_self_ncd = max(max_self_ncd, ncd);
            } else {
                min_diff_ncd = min(min_diff_ncd, ncd);
                max_diff_ncd = max(max_diff_ncd, ncd);
            }
        }
        
        sort(matches.begin(), matches.end());
        if (!matches.empty() && matches[0].second == query) {
            correct_identifications++;
        }
    }
    
    *dual_out << "Accuracy: " << correct_identifications << "/" << total_queries 
         << " (" << (100.0 * correct_identifications / total_queries) << "%)\n";
    *dual_out << "Self-match NCD range: " << min_self_ncd << " - " << max_self_ncd << "\n";
    *dual_out << "Different songs NCD range: " << min_diff_ncd << " - " << max_diff_ncd << "\n";
    *dual_out << "Average discrimination: " << (min_diff_ncd - max_self_ncd) << "\n";
    
    auto end_time = chrono::high_resolution_clock::now();
    auto duration = chrono::duration_cast<chrono::seconds>(end_time - start_time);
    *dual_out << "\nTest completed in " << duration.count() << " seconds\n";
    *dual_out << "Results saved to: simple_test_results.txt\n";
    
    delete dual_out;
    return 0;
}
