#include "FCMModel.h"
#include <iostream>
#include <fstream>
#include <string>
#include <limits>
#include <cstdlib>
#include <filesystem>

void clearScreen() {
    #ifdef _WIN32
        system("cls");
    #else
        system("clear");
    #endif
}

void pressEnterToContinue() {
    std::cout << "\nPress Enter to continue...";
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    std::cin.get();
}

void createNewModel(FCMModel& model) {
    int k;
    double alpha;
    
    std::cout << "Enter the order (k) of the model: ";
    while(!(std::cin >> k) || k < 1) {
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        std::cout << "Invalid input. Please enter a positive integer: ";
    }
    
    std::cout << "Enter the smoothing parameter (alpha): ";
    while(!(std::cin >> alpha) || alpha <= 0) {
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        std::cout << "Invalid input. Please enter a positive number: ";
    }
    
    model = FCMModel(k, alpha);
    std::cout << "New model created successfully with k=" << k << " and alpha=" << alpha << std::endl;
}

void importModel(FCMModel& model) {
    std::string filename;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    
    std::cout << "Enter the filename to import the model from: ";
    std::getline(std::cin, filename);
    
    try {
        model.importModel(filename);
        std::cout << "Model imported successfully from " << filename << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Error importing model: " << e.what() << std::endl;
    }
}

void learnFromText(FCMModel& model) {
    std::string choice;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    
    std::cout << "Choose input method:\n";
    std::cout << "1. Enter text directly\n";
    std::cout << "2. Load from file\n";
    std::cout << "Enter your choice: ";
    std::getline(std::cin, choice);
    
    if (choice == "1") {
        std::string text;
        std::cout << "Enter the text (end with a line containing only 'END'):\n";
        
        std::string line;
        while (std::getline(std::cin, line) && line != "END") {
            text += line + "\n";
        }
        
        try {
            model.learn(text);
            std::cout << "Model learned from the entered text successfully." << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "Error learning from text: " << e.what() << std::endl;
        }
    } else if (choice == "2") {
        std::string filename;
        std::cout << "Enter the filename to load the text from: ";
        std::getline(std::cin, filename);
        
        try {
            std::string text = readFile(filename);
            model.learn(text);
            std::cout << "Model learned from file " << filename << " successfully." << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "Error learning from file: " << e.what() << std::endl;
        }
    } else {
        std::cout << "Invalid choice." << std::endl;
    }
}

void predictNextSymbols(const FCMModel& model) {
    std::string context;
    int n;
    
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    std::cout << "Enter the context to predict from: ";
    std::getline(std::cin, context);
    
    // Fixed warning: compare unsigned with unsigned
    int k = model.getK();
    if (context.length() < static_cast<std::size_t>(k)) {
        std::cout << "Warning: Context length is shorter than model's order k=" << k << std::endl;
        std::cout << "Prediction may not be accurate." << std::endl;
    }
    
    std::cout << "Enter the number of symbols to predict: ";
    while(!(std::cin >> n) || n < 1) {
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        std::cout << "Invalid input. Please enter a positive integer: ";
    }
    
    try {
        std::string prediction = model.predict(context, n);
        std::cout << "Prediction: " << prediction << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Error during prediction: " << e.what() << std::endl;
    }
}

void computeInformationContent(const FCMModel& model) {
    std::string text;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    
    std::cout << "Choose input method:\n";
    std::cout << "1. Enter text directly\n";
    std::cout << "2. Load from file\n";
    std::cout << "Enter your choice: ";
    
    std::string choice;
    std::getline(std::cin, choice);
    
    if (choice == "1") {
        std::cout << "Enter the text (end with a line containing only 'END'):\n";
        
        std::string line;
        while (std::getline(std::cin, line) && line != "END") {
            text += line + "\n";
        }
    } else if (choice == "2") {
        std::string filename;
        std::cout << "Enter the filename to load the text from: ";
        std::getline(std::cin, filename);
        
        try {
            text = readFile(filename);
        } catch (const std::exception& e) {
            std::cerr << "Error reading file: " << e.what() << std::endl;
            return;
        }
    } else {
        std::cout << "Invalid choice." << std::endl;
        return;
    }
    
    try {
        double averageInfo = model.computeAverageInformationContent(text);
        std::cout << "Average information content: " << averageInfo << " bits per symbol" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Error computing information content: " << e.what() << std::endl;
    }
}

void exportModel(FCMModel& model) {  // Changed from const FCMModel& to FCMModel&
    std::string filename;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    
    std::cout << "Enter the filename to export the model to: ";
    std::getline(std::cin, filename);
    
    try {
        model.exportModel(filename);
        std::cout << "Model exported successfully to " << filename << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Error exporting model: " << e.what() << std::endl;
    }
}

void displayModelProperties(const FCMModel& model) {
    std::cout << "Model Properties:" << std::endl;
    std::cout << "- Order (k): " << model.getK() << std::endl;
    // Additional properties can be added if we expand the FCMModel class to expose more information
}

void batchLearnFromFiles(FCMModel& model) {
    std::string directoryPath;
    
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    std::cout << "Enter the directory path to learn from: ";
    std::getline(std::cin, directoryPath);
    
    int successCount = 0;
    int fileCount = 0;
    
    try {
        for (const auto& entry : std::filesystem::directory_iterator(directoryPath)) {
            if (entry.is_regular_file()) {
                fileCount++;
                try {
                    std::string text = readFile(entry.path().string());
                    model.learn(text);
                    std::cout << "✓ Learned from " << entry.path().string() << std::endl;
                    successCount++;
                } catch (const std::exception& e) {
                    std::cerr << "✗ Error learning from " << entry.path().string() << ": " << e.what() << std::endl;
                }
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Error accessing directory: " << e.what() << std::endl;
        return;
    }
    
    std::cout << "Batch learning completed. Successfully processed " 
              << successCount << " out of " << fileCount << " files." << std::endl;
}

void displayMenu(const std::string& modelName, bool modelInitialized) {
    clearScreen();
    std::cout << "=============================================\n";
    std::cout << "             FCM MODEL EDITOR               \n";
    std::cout << "=============================================\n";
    if (modelInitialized) {
        std::cout << "Current Model: " << modelName << "\n";
    } else {
        std::cout << "No model loaded.\n";
    }
    std::cout << "1. Create New Model\n";
    std::cout << "2. Import Model\n";
    
    if (modelInitialized) {
        std::cout << "3. Learn From Text\n";
        std::cout << "4. Predict Next Symbols\n";
        std::cout << "5. Compute Information Content\n";
        std::cout << "6. Lock Model\n";
        std::cout << "7. Unlock Model\n";
        std::cout << "8. Clear Model\n";
        std::cout << "9. Export Model\n";
        std::cout << "10. Display Model Properties\n";
        std::cout << "11. Batch Learn From Files\n";
    }
    
    std::cout << "0. Exit\n";
    std::cout << "=============================================\n";
    std::cout << "Enter your choice: ";
}

int main() {
    FCMModel model;
    bool modelInitialized = false;
    std::string modelName;
    int choice;
    
    while (true) {
        displayMenu(modelName, modelInitialized);
        while (!(std::cin >> choice)) {
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::cout << "Invalid input. Please enter a number: ";
        }
        clearScreen();
        
        switch (choice) {
            case 0:
                std::cout << "Exiting program. Goodbye!" << std::endl;
                return 0;
            
            case 1:
                createNewModel(model);
                modelInitialized = true;
                modelName = "New Model";
                break;
            
            case 2:
                importModel(model);
                modelInitialized = true;
                std::cout << "Enter model filename: ";
                std::cin.ignore();
                std::getline(std::cin, modelName);
                break;
            
            default:
                if (!modelInitialized) {
                    std::cout << "No model is currently loaded. Please create or import a model first." << std::endl;
                } else {
                    switch (choice) {
                        case 3:
                            learnFromText(model);
                            break;
                        case 4:
                            predictNextSymbols(model);
                            break;
                        case 5:
                            computeInformationContent(model);
                            break;
                        case 6:
                            model.lockModel();
                            std::cout << "Model locked successfully." << std::endl;
                            break;
                        case 7:
                            model.unlockModel();
                            std::cout << "Model unlocked successfully." << std::endl;
                            break;
                        case 8:
                            model.clearModel();
                            std::cout << "Model cleared successfully." << std::endl;
                            break;
                        case 9:
                            exportModel(model);
                            break;
                        case 10:
                            displayModelProperties(model);
                            break;
                        case 11:
                            batchLearnFromFiles(model);
                            break;
                        default:
                            std::cout << "Invalid choice. Please try again." << std::endl;
                    }
                }
        }
        pressEnterToContinue();
    }
}
