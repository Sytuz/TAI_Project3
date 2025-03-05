#include <iostream>
#include <fstream>
#include <vector>
#include <chrono>
#include <cstdlib>
#include <sstream>
#include <iomanip>
#include <filesystem>
#include <string>

using namespace std;
using namespace std::chrono;
using namespace std::filesystem;


void getUserInput(vector<string> &inputFiles, int &k_min, int &k_max, double &alpha_min, double &alpha_max, double &alpha_step, string &outputFormat){
    string filename;

    cout << "Do you want to use all files in the 'sequences/' folder as input files? (y/n): ";
    char useDefaultFiles;
    cin >> useDefaultFiles;
    cin.ignore();

    if(useDefaultFiles == 'y' || useDefaultFiles == 'Y'){
        for(const auto& entry : directory_iterator("sequences/")){
            if(entry.is_regular_file()){
                inputFiles.push_back(entry.path().string());
            }
        }

        if(inputFiles.empty()){
            cerr << "Error: No files found in the 'sequences/' folder. Exiting..." << endl;
            exit(1);
        }

        cout << "Using default input files in the 'sequences/' folder: " << endl;
        for(const auto& file : inputFiles){
            cout << "- " << file << endl;
        }
    }
    else{
        cout << "Enter the input file names (saved in the 'sequences/' folder, without .txt) (press Enter to finish):" << endl;

        while(true){
            cout << "> ";
            getline(cin, filename);
            if(filename.empty()){
                break;
            }

            string fullPath = "sequences/" + filename + ".txt";

            if (!filesystem::exists(fullPath)) {
                cerr << "Error: File '" << fullPath << "' not found. Please enter a valid file name." << endl;
                continue;
            }

        inputFiles.push_back(fullPath);
        }

        if(inputFiles.empty()){
            cerr << "Error: No input files provided. Exiting..." << endl;
            exit(1);
        }
    }

    while(true){
        cout << "Enter the range for k (min max) or press Enter to use default [1, 6]: ";
        string k_input;
        getline(cin, k_input);

        if(k_input.empty()){
            k_min = 1;
            k_max = 6;
            cout << "Using default k range: [1, 6]" << endl;
            break;
        }
        else{
            stringstream ss(k_input);
            ss >> k_min >> k_max;

            if(k_min < 0 || k_max < 0){
                cout << "Error: k values cannot be negative. Please enter valid values." << endl;
                continue;
            }
            else if(k_min > k_max){
                cout << "Error: k values must be in ascending order. Please enter valid values." << endl;
                continue;
            }

            cout << "Using k range: [" << k_min << "," << k_max << "]" << endl;
            break;
        }
    }

    while(true){
        cout << "Enter the range for alpha (min max) or press Enter to use default [0.01, 1.0]: ";
        string alpha_input;
        getline(cin, alpha_input);

        if(alpha_input.empty()){
            alpha_min = 0.0;
            alpha_max = 1.0;
            cout << "Using default alpha range: [0.0, 1.0]" << endl;
            break;
        }
        else{
            stringstream ss(alpha_input);
            ss >> alpha_min >> alpha_max;

            if(alpha_min <= 0.0 || alpha_max <= 0.0){
                cout << "Error: alpha values cannot be negative. Please enter valid values." << endl;
                continue;
            }
            else if(alpha_min > alpha_max){
                cout << "Error: alpha values must be in ascending order. Please enter valid values." << endl;
                continue;
            }

            cout << "Using alpha range: [" << alpha_min << "," << alpha_max << "]" << endl;
            break;
        }
    }

    while(true){
        cout << "Enter the alpha step size or press Enter to use default (0.10): ";
        string alpha_step_input;
        getline(cin, alpha_step_input);
        if(alpha_step_input.empty()){
            alpha_step = 0.10;
            cout << "Using default alpha step size: 0.10" << endl;
            break;
        }
        else{
            stringstream ss(alpha_step_input);
            ss >> alpha_step;

            if(alpha_step < 0.0){
                cout << "Error: alpha step size cannot be negative. Please enter a valid value." << endl;
                continue;
            }

            cout << "Using alpha step size: " << alpha_step << endl;
            break;
        }
    }

    cout << "Choose the output format: JSON (j), BSON (b), or both (press other letter or Enter): ";
    getline(cin, outputFormat);
}

string getAverageInfoContent(const string &command) {
    FILE* pipe = popen(command.c_str(), "r");
    if(!pipe){
        cerr << "Error opening pipe to execute command." << endl;
        return "N/A";
    }

    string avgInfoContent = "N/A";
    char buffer[256];
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        cout << buffer << endl;
        string line(buffer);
        if(line.find("Average Information Content:") != string::npos) {
            avgInfoContent = line.substr(line.find(":") + 1);
            size_t pos = avgInfoContent.find(" bits per symbol");
            if(pos != string::npos){
                avgInfoContent = avgInfoContent.substr(0, pos);
            }
            break;
        }
    }


    pclose(pipe);
    return avgInfoContent;
}

void runTests(const vector<string> &inputFiles, int k_min, int k_max, double alpha_min, double alpha_max, double alpha_step, const string &outputFormat, vector<vector<string>> &results){
    int testIndex = 0;
    for(const auto &inputFile : inputFiles){
        for(int k = k_min; k <= k_max; ++k){
            for(double alpha = alpha_min; alpha <= alpha_max + alpha_step/2; alpha+=alpha_step){
                testIndex++;

                vector<string> modelFiles;

                if(outputFormat == "j" || outputFormat.empty()){
                    modelFiles.push_back("temp_model" + to_string(testIndex) + ".json");
                }
                if(outputFormat == "b" || outputFormat.empty()){
                    modelFiles.push_back("temp_model" + to_string(testIndex) + ".bson");
                }

                for(const auto& modelFile : modelFiles){
                    string last4 = modelFile.substr(modelFile.length() - 4);
                    string modelFileName = modelFile;
                    modelFileName.erase(modelFile.length() - 5);
                    if(last4 == "json"){
                        modelFileName += " --json";
                    }

                    string command = "./fcm " + inputFile + " -k " + to_string(k) + " -a " + to_string(alpha) + " -o " + modelFileName;
                    cout << "Running: " << command <<endl;

                    int ret = system(command.c_str());
                    if(ret != 0) {
                        cerr << "Error executing command." << endl;
                    }

                    auto start = high_resolution_clock::now();
                    string avgInfoContent = getAverageInfoContent(command);
                    auto end = high_resolution_clock::now();
                    double execTime = duration<double, milli>(end - start).count();

                    long fileSize = 0;
                    bool fileCreated = false;

                    try{
                        fileCreated = filesystem::exists(modelFile);
                        if(fileCreated){
                            fileSize = filesystem::file_size(modelFile);
                        }
                    }
                    catch(const filesystem::filesystem_error& e){
                        cerr << "Error: Unable to read file size for " << modelFile << endl;
                        fileSize = -1;
                    }

                    if(!fileCreated || fileSize <= 0){
                        cerr << "WARNING: Model generation failed for parameters:" << endl;
                        cerr << "  File: " << inputFile << endl;
                        cerr << "  k: " << k << endl;
                        cerr << "  alpha: " << alpha << endl;
                        cerr << "  Model File: " << modelFile << endl;
                    }

                    results.push_back({
                        inputFile,
                        to_string(k),
                        to_string(alpha),
                        modelFile,
                        avgInfoContent,
                        to_string(execTime),
                        to_string(fileSize)
                    });
                }
            }
        }
    }
}

void saveToCSV(const string &filename, const vector<vector<string>> &data){
    ofstream file(filename);

    if(!file){
        cerr << "Error: Unable to create file " << filename << endl;
        return;
    }

    for(const auto &row : data){
        for(size_t i = 0; i < row.size(); ++i){
            file << row[i];
            if(i != row.size() - 1){
                file << ",";
            }
        }
        file << "\n";
    }

    cout << "Results saved to: " << filename << endl;
}

int main(){
    vector<string> inputFiles;
    int k_min, k_max;
    double alpha_min, alpha_max, alpha_step;
    string outputFormat;
    getUserInput(inputFiles, k_min, k_max, alpha_min, alpha_max, alpha_step, outputFormat);

    vector<vector<string>> results = {{"File", "k", "alpha", "ModelFile", "AvgInfoContent", "ExecTime(ms)", "ModelSize"}};
    runTests(inputFiles, k_min, k_max, alpha_min, alpha_max, alpha_step, outputFormat, results);

    string outputFile = "results/test_results.csv";
    saveToCSV(outputFile, results);

    return 0;
}
