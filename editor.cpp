#include "FCMModel.h"
#include <iostream>
#include <fstream>
#include <string>
#include <limits>
#include <cstdlib>
#include <filesystem>


/* --- Screen Manipulation --- */

void clearScreen() {
    #ifdef _WIN32
        system("cls");
    #else
        system("clear");
    #endif
}

void pressEnterToContinue() {
    std::cout << "\nPress Enter to continue...";
    //std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    std::cin.get();
}


/* --- Creation & Importing --- */

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
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}

std::string importModel(FCMModel& model) {
    std::string filename;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    
    std::cout << "Enter the filename to import the model from: ";
    std::getline(std::cin, filename);
    
    try {
        model.importModel(filename);
        std::cout << "Model imported successfully from " << filename << std::endl;
        filename = filename.substr(0, filename.find_last_of('.'));
        return filename;
    } catch (const std::exception& e) {
        std::cerr << "Error importing model: " << e.what() << std::endl;
        return "";
    }
}


/* --- Core Model Operations --- */

void batchLearnFromDirectory(FCMModel& model) {
    std::string directoryPath;
    
    //std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    std::cout << "Enter the directory path to learn from (type '0' to cancel): ";
    std::getline(std::cin, directoryPath);
    
    int successCount = 0;
    int fileCount = 0;

    if (directoryPath == "0") {
        std::cout << "Operation cancelled." << std::endl;
        return;
    }
    
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

void learnFromText(FCMModel& model) {
    std::string choice;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    
    std::cout << "Choose input method:\n";
    std::cout << "1. Enter text directly\n";
    std::cout << "2. Load from file\n";
    std::cout << "3. Batch learn from directory\n";
    std::cout << "4. Word List\n";
    std::cout << "0. Cancel\n";
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
    } else if (choice == "3") {
        batchLearnFromDirectory(model);
    } else if (choice == "4") {
        // The word list option requires providing a text file where each line is a word
        std::string filename;
        std::cout << "Enter the filename to load the word list from: ";
        std::getline(std::cin, filename);
        
        // For each word in the word list, learn the word followed and preceded by a space
        try {
            std::string text = readFile(filename);
            std::istringstream iss(text);
            std::string word;
            while (iss >> word) {
                model.learn(" " + word + " ", true);
            }
            std::cout << "Model learned from word list successfully." << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "Error learning from word list: " << e.what() << std::endl;
        }
    } else if (choice == "0") {
        std::cout << "Operation cancelled." << std::endl;
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
        std::cout << "Prediction:\n" << prediction << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Error during prediction: " << e.what() << std::endl;
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        return;
    }

    std::cout << "\n=============================================\n";
    std::cout << "Do you want to evaluate the prediction? (y/n): ";
    std::string evaluate;
    std::cin >> evaluate;

    if (evaluate == "y" || evaluate == "Y") {
        // The prediction evaluation shows the percentage of real words in the prediction
    }

    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); 
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


/* --- Model Manipulation --- */

void lockUnlockModel(FCMModel& model) {
    if (model.isLocked()) {
        model.unlockModel();
        std::cout << "Model unlocked successfully." << std::endl;
    } else {
        model.lockModel();
        std::cout << "Model locked successfully." << std::endl;
    }
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}

void renameModel(std::string& modelName) {
    std::string newName;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    
    std::cout << "Enter the new model name: ";
    std::getline(std::cin, newName);
    modelName = newName;
    std::cout << "Model renamed successfully." << std::endl;
}

void clearModel(FCMModel& model) {
    model.clearModel();
    std::cout << "Model cleared successfully." << std::endl;
}

void exportModel(FCMModel& model, std::string modelName) {
    std::cout << "Exporting model..." << std::endl;
    std::string filename = modelName + ".json";
    
    try {
        model.exportModel(filename);
        std::cout << "Model exported successfully to " << filename << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Error exporting model: " << e.what() << std::endl;
    }

    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}


/* --- Main Program Loop --- */

void displayMenu(const std::string& modelName, bool modelInitialized, FCMModel& model) {
    clearScreen();
    std::cout << "=============================================\n";
    std::cout << "              FCM MODEL EDITOR               \n";
    std::cout << "=============================================\n";
    if (modelInitialized) {
        std::cout << "Current Model: " << modelName << "\n";
        model.printModelSummary();
    } else {
        std::cout << "No model loaded.\n";
    }
    std::cout << "1. Create New Model\n";
    std::cout << "2. Import Model\n";
    
    if (modelInitialized) {
        std::cout << "=============================================\n";
        std::cout << "----        CORE MODEL OPERATIONS        ----\n";
        std::cout << "3. Learn From Text\n";
        std::cout << "4. Predict Next Symbols\n";
        std::cout << "5. Compute Information Content\n";
        std::cout << "=============================================\n";
        std::cout << "----         MODEL MANIPULATION          ----\n";
        std::cout << "6. Lock/Unlock Model\n";
        std::cout << "7. Rename Model\n";
        std::cout << "8. Clear Model\n";
        std::cout << "9. Export Model\n";
        std::cout << "=============================================\n";
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
        displayMenu(modelName, modelInitialized, model);
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
                if (modelInitialized) {
                    std::cout << "A model is already loaded. Do you want to overwrite it? (y/n): ";
                    char overwrite;
                    std::cin >> overwrite;
                    if (overwrite != 'y' && overwrite != 'Y') {
                        break;
                    }
                    model.clearModel();
                }
                createNewModel(model);
                modelInitialized = true;
                modelName = "New Model";
                break;
            
            case 2:
                modelName = importModel(model);
                if (!modelName.empty()) {
                    modelInitialized = true;
                }
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
                            lockUnlockModel(model);
                            break;
                        case 7:
                            renameModel(modelName);
                            break;
                        case 8:
                            clearModel(model);
                            break;
                        case 9:
                            exportModel(model, modelName);
                            break;
                        default:
                            std::cout << "Invalid choice. Please try again." << std::endl;
                    }
                }
        }
        pressEnterToContinue();
    }
}
