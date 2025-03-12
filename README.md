# TAI_Project1
This repository contains the implementation of a Finite-Context Model (FCM) and its recursive extension (RFCM) for text analysis, average information content measurement, and text generation. 

It was developed in the Teoria Algorítmica da Informação (TAI) course.

---

## Table of Contents
1. [Introduction](#introduction)  
2. [Dependencies](#dependencies)  
3. [Installation](#installation)  
4. [Usage](#usage)  
   1. [Running the Programs](#running-the-programs)  
   2. [Running Tests](#running-tests)  
   3. [Generating Graphs](#generating-graphs)
5. [Data Sources](#data-sources)
6. [Project Structure](#project-structure)
7. [Authors](#authors)

---

## Introduction
This project provides:
- **fcm**: A C++ program for training a Finite-Context Model on text data, calculating symbol probabilities, and determining the Average Information Content (in bits per symbol).  
- **generator**: A C++ program that generates text from a trained model, optionally performing its own training if no pre-trained model is provided.  
- **editor**: An interactive text-based menu for creating, importing, and manipulating models without requiring extensive command-line knowledge.  
- **tests**: A set of automated tests (in `tests.cpp`) that verify core functionalities such as training, prediction, and model export/import.

---

## Dependencies
1. **C++ Compiler**  
   - A modern C++ compiler (e.g., GCC 9+, Clang 10+, or MSVC 2019+) with C++17 (or later) support.
   - Features such as `std::make_unique`, and filesystem operations require C++17.
2. **Make** 
   - Required for compiling the project using the provided `Makefile`.
3. **Python 3** (Optional, for generating graphs)  
   - Python 3.6+ to run the graph generation scripts.  
   - Required libraries are listed in `requirements.txt`.

---

## Installation

1. **Clone the Repository**  
   ```bash
   git clone https://github.com/miguel-silva48/TAI_Project1.git
   cd TAI_Project1
   ```

2. **Install C++ Dependencies**  
    - Ensure you have a compatible C++17 compiler installed.

    - On Ubuntu/Debian:
    ```bash
    sudo apt update
    sudo apt install build-essential
    ``` 
    
    - On macOS (using Homebrew):
    ```bash
    brew install gcc make
    ``` 

2. **Install Python Dependencies** (Optional, for graph generation) 
    - Set up a virtual environment (recommended)
    ```bash
    python3 -m venv venv
    source venv/bin/activate  # On Windows: venv\Scripts\activate
    ```

    - Install required packages:
    ```bash
    pip install -r requirements.txt
    ```

---

## Usage

### Running the Programs
A simple Makefile is provided. From the project root directory, compile and build all executables by running:
```bash
make
```
This command will generate the following binaries:
- `fcm`
- `generator`
- `editor`
- `tests`

**Note:** It will also generate binaries for the model files and inject the required dependencies.

To remove compiled files and binaries:
```bash
make clean
```

#### Example Commands
1. **fcm**  
   ```bash
   ./fcm <input_file> -k 3 -a 0.1 -o <output_model> [--json]
   ```
   Trains a model of order `k=3` with smoothing `alpha=0.1` on `input_file`, then saves the model to `<output_model>.(bson|json)`, depending on your export format.

    For complete usage information:
    ```bash
   ./fcm -h
   ```

2. **generator**  
   ```bash
   ./generator -m <model_file> -p "the" -s 100
   ```
   Loads a pre-trained model from `<model_file>` and generates 100 symbols starting from the context `"the"`.

   **Note:** The generator also supports doing the full process (learning + predicting), by adding the same parameters as in fcm.

    For complete usage information:
    ```bash
   ./generator -h
   ```

3. **editor**  
   ```bash
   ./editor
   ```
   Launches an interactive menu-driven interface for creating or importing models, learning from text, generating predictions, performing syntactic analysis, and more.

### Running Tests
Execute automated tests (which validate training, prediction, model export/import, etc.) by running:
```bash
./tests
```
This will open a different text menu and prompt the user which sequences they wish to test and the corresponding ranges for the parameters.

### Generating Graphs
The project includes Python scripts for generating visualization graphs in the `generate_plots` directory:
```bash
# Activate virtual environment if you created one
source venv/bin/activate  # On Windows: venv\Scripts\activate
# Generate plots
python3 generate_plots/plots.py
```

**Note:** Pre-generated plots and test results are already included in the `results` directory for reference, and in order to generate plot, having test results is required.

---

## Data Sources
- The text sequences used in this project are derived from works available at [Project Gutenberg](https://www.gutenberg.org/).
- The syntactic analysis wordlist for Portuguese language (`ptWords.txt`) is sourced from [GitHub Repository](https://github.com/jfoclpf/words-pt).


## Project Structure
- `FCMModel.*`: Implementation of the Finite Context Model
- `RFCMModel.*`: Implementation of the Recursive Finite Context Model
- `fcm.cpp`, `generator.cpp`, `editor.cpp`: Main program entry points
- `tests.cpp`: Test suite for validating model functionality
- `generate_plots/`: Python scripts for visualization
- `results/`: Directory containing test outputs and generated plots
- `sequences/`: Text samples for training and testing
- `assets/`: Directory containing, the report PDF and source files, the presentation PDF and the diagram's code.

---

## Authors
<table>
  <tr>
    <td align="center">
        <a href="https://github.com/Sytuz">
            <img src="https://avatars0.githubusercontent.com/Sytuz?v=3" width="100px;" alt="Alexandre"/>
            <br />
            <sub>
                <b>Alexandre Ribeiro</b>
                <br>
                <i>108122</i>
            </sub>
        </a>
    </td>
    <td align="center">
        <a href="https://github.com/mariiajoao">
            <img src="https://avatars0.githubusercontent.com/mariiajoao?v=3" width="100px;" alt="Maria"/>
            <br />
            <sub>
                <b>Maria Sardinha</b>
                <br>
                <i>108756</i>
            </sub>
        </a>
    </td>
    <td align="center">
        <a href="https://github.com/miguel-silva48">
            <img src="https://avatars0.githubusercontent.com/miguel-silva48?v=3" width="100px;" alt="Miguel"/>
            <br />
            <sub>
                <b>Miguel Pinto</b>
                <br>
                <i>107449</i>
            </sub>
        </a>
    </td>
  </tr>
</table>
