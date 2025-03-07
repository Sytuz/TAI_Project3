#include <iostream>
#include <fstream>
#include <vector>
#include <chrono>
#include <cstdlib>
#include <sstream>
#include <iomanip>
#include <filesystem>
#include <string>
#include <algorithm>

using namespace std;
using namespace std::chrono;
using namespace std::filesystem;


void getUserInput(vector<string> &inputFiles, int &k_min, int &k_max, double &alpha_min, double &alpha_max, double &alpha_step, string &outputFormat, int &recursiveSteps, string &generationSize){
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

        for(const auto& entry : directory_iterator("sequences/camilo_castelo/")){
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

    while(true){
        cout << "Enter the number of recursive steps or press Enter to use default (10): ";
        string recursive_steps_input;
        getline(cin, recursive_steps_input);
        if(recursive_steps_input.empty()){
            recursiveSteps = 10;
            cout << "Using default recursive steps: 10" << endl;
            break;
        }
        else{
            stringstream ss(recursive_steps_input);
            ss >> recursiveSteps;

            if(recursiveSteps < 1){
                cout << "Error: recursive steps must be at least 1. Please enter a valid value." << endl;
                continue;
            }

            cout << "Using recursive steps: " << recursiveSteps << endl;
            break;
        }
    }

    while(true){
        cout << "Enter the text generation size or press Enter to use default (1000): ";
        string gen_size_input;
        getline(cin, gen_size_input);
        if(gen_size_input.empty()){
            generationSize = "1000";
            cout << "Using default generation size: 1000" << endl;
            break;
        }
        else{
            generationSize = gen_size_input;
            int size = stoi(generationSize);

            if(size < 10){
                cout << "Error: generation size must be at least 10. Please enter a valid value." << endl;
                continue;
            }

            cout << "Using generation size: " << generationSize << endl;
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

void writeToFile(const string &content, const string &filename) {
    ofstream file(filename);
    if(!file){
        cerr << "Error: Unable to create file " << filename << endl;
        return;
    }
    file << content;
}

string getGeneratedText(const string &command) {
    FILE* pipe = popen(command.c_str(), "r");
    if(!pipe){
        cerr << "Error opening pipe to execute command." << endl;
        return "";
    }

    string generatedText;
    bool captureText = false;
    char buffer[4096];

    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        string line(buffer);

        if(line.find("Generated Text:") != string::npos) {
            captureText = true;
            continue;
        }

        if(captureText) {
            generatedText += line;
        }
    }

    pclose(pipe);
    return generatedText;
}

void runRecursiveTests(const vector<string> &inputFiles, int k_min, int k_max, double alpha_min, double alpha_max,
                        double alpha_step, const string &outputFormat, int recursiveSteps, const string &generationSize,
                        vector<vector<string>> &results) {
    if(!filesystem::exists("temp")) {
        filesystem::create_directory("temp");
    }

    for(const auto &inputFile : inputFiles) {
        for(int k = k_min; k <= k_max; ++k) {
            for(double alpha = alpha_min; alpha <= alpha_max + alpha_step/2; alpha += alpha_step) {

                vector<string> modelFiles;

                if(outputFormat == "j" || outputFormat.empty()) {
                    modelFiles.push_back("temp/temp_model.json");
                }
                if(outputFormat == "b" || outputFormat.empty()) {
                    modelFiles.push_back("temp/temp_model.bson");
                }

                for(const auto& modelFile : modelFiles) {
                    string modelType = modelFile.substr(modelFile.length() - 4);
                    transform(modelType.begin(), modelType.end(), modelType.begin(), ::toupper);

                    string modelFileName = modelFile;
                    modelFileName.erase(modelFile.length() - 5);

                    string modelFormatFlag = "";
                    if(modelType == "JSON") {
                        modelFormatFlag = " --json";
                    }

                    // Initial run with the original input file
                    cout << "\n======== Starting recursive test for " << inputFile << " (k=" << k << ", alpha=" << alpha << ") ========\n";

                    string currentInputFile = inputFile;
                    string tempInputFile;

                    for(int step = 0; step <= recursiveSteps; ++step) {
                        cout << "\n--- Step " << step << " ---\n";

                        // Run FCM to create model and get average information content
                        string fcmCommand = "./fcm " + currentInputFile + " -k " + to_string(k) + " -a " + to_string(alpha) + " -o " + modelFileName + modelFormatFlag;
                        cout << "Running FCM: " << fcmCommand << endl;

                        auto startTime = high_resolution_clock::now();
                        system(fcmCommand.c_str());
                        string avgInfoContent = getAverageInfoContent(fcmCommand);
                        auto endTime = high_resolution_clock::now();
                        double execTime = duration<double, milli>(endTime - startTime).count();

                        // Get file size of the model
                        long fileSize = 0;
                        bool fileCreated = false;

                        try {
                            fileCreated = filesystem::exists(modelFile);
                            if(fileCreated) {
                                fileSize = filesystem::file_size(modelFile);
                            }
                        }
                        catch(const filesystem::filesystem_error& e) {
                            cerr << "Error: Unable to read file size for " << modelFile << endl;
                            fileSize = -1;
                        }

                        // Record the results for this step
                        results.push_back({
                            inputFile,
                            to_string(k),
                            to_string(alpha),
                            modelType,
                            to_string(step),
                            avgInfoContent,
                            to_string(execTime),
                            to_string(fileSize)
                        });

                        if(step == recursiveSteps) {
                            break;
                        }

                        // Generate text from the model
                        string priorContext;
                        ifstream inputFileStream(currentInputFile);
                        if(inputFileStream) {
                            char c;
                            for(int i = 0; i < k && inputFileStream.get(c); ++i) {
                                priorContext += c;
                            }
                            inputFileStream.close();
                        }

                        if(priorContext.length() < k) {
                            // Pad with spaces if not enough characters
                            while(priorContext.length() < k) {
                                priorContext += " ";
                            }
                        }

                        tempInputFile = "temp/temp_file.txt";

                        // Run generator to create new text
                        string generatorCommand = "./generator -m " + modelFile + " -p \"" + priorContext + "\" -s " + generationSize;
                        cout << "Running Generator: " << generatorCommand << endl;

                        // Get the generated text and write it to a file
                        string generatedText = getGeneratedText(generatorCommand);
                        writeToFile(generatedText, tempInputFile);

                        // Set the generated text as the new input for the next iteration
                        currentInputFile = tempInputFile;

                        filesystem::remove(modelFile);
                    }
                }
            }
        }
    }
}

void saveToCSV(const string &filename, const vector<vector<string>> &data) {
    ofstream file(filename);

    if(!file) {
        cerr << "Error: Unable to create file " << filename << endl;
        return;
    }

    for(const auto &row : data) {
        for(size_t i = 0; i < row.size(); ++i) {
            file << row[i];
            if(i != row.size() - 1) {
                file << ",";
            }
        }
        file << "\n";
    }

    cout << "Results saved to: " << filename << endl;
}

int main() {
    vector<string> inputFiles;
    int k_min, k_max;
    double alpha_min, alpha_max, alpha_step;
    string outputFormat;
    int recursiveSteps;
    string generationSize;

    getUserInput(inputFiles, k_min, k_max, alpha_min, alpha_max, alpha_step, outputFormat, recursiveSteps, generationSize);

    vector<vector<string>> results = {{"File", "k", "alpha", "ModelType", "RecursiveStep", "AvgInfoContent", "ExecTime(ms)", "ModelSize"}};

    runRecursiveTests(inputFiles, k_min, k_max, alpha_min, alpha_max, alpha_step, outputFormat, recursiveSteps, generationSize, results);

    string outputFile = "results/test_results.csv";
    saveToCSV(outputFile, results);

    // Clean up temporary files
    cout << "Cleaning up temporary files..." << endl;
    for(const auto& entry : directory_iterator("temp/")) {
        filesystem::remove(entry.path());
    }

    return 0;
}
