# TAI_Project2
This [GitHub Repository](https://github.com/mariiajoao/TAI_Project2) contains the implementation of a DNA sequence classifier using Normalized Relative Compression (NRC) based on Finite-Context Models (FCM).

This project is a fork of our [previous work](https://github.com/miguel-silva48/TAI_Project1/) on Finite-Context Models, developed specifically for metagenome analysis in the context of Teoria Algorítmica da Informação (TAI) course.

---

## Table of Contents
1. [Introduction](#introduction)  
2. [Dependencies](#dependencies)  
3. [Installation](#installation)  
4. [Usage](#usage)  
   1. [Running MetaClass](#running-metaclass)  
   2. [Running Tests](#running-tests)
   3. [Running Visualizations](#running-visualizations)
5. [Data Sources](#data-sources)
6. [Project Structure](#project-structure)
7. [Authors](#authors)

---

## Introduction
This project provides:
- **MetaClass**: A C++ program that implements the Normalized Relative Compression (NRC) method to analyze similarities between metagenomic samples and reference sequences. It trains a Finite-Context Model on a metagenomic sample and uses this model to evaluate the compression of reference sequences, ranking them by similarity.

- **Test Framework**: A text-based interface that provides advanced operations for metagenome analysis including grid testing of FCM parameters (k and alpha), chunk analysis, symbol information analysis, and cross-comparison between top organisms.

- **Visualization Tools**: Python-based visualization framework that processes the test results to generate insightful graphs and charts for easier interpretation of metagenome analysis.

The core functionality takes a metagenomic sample that may contain unknown genetic sequences and attempts to identify which known organisms from a reference database have the highest similarity to the sample.

---

## Dependencies
1. **C++ Compiler**  
   - A modern C++ compiler (e.g., GCC 9+, Clang 10+, or MSVC 2019+) with C++17 (or later) support.
   - Features such as JSON handling and file operations require C++17.
2. **CMake** 
   - Required for building the project (version 3.10 or higher).
3. **Python 3.6+** (optional)
   - Required for the visualization framework.
   - Additional Python packages: matplotlib, pandas, seaborn, numpy

---

## Installation

1. **Clone the Repository**  
   ```bash
   git clone https://github.com/mariiajoao/TAI_Project2.git
   cd TAI_Project2
   ```

2. **Install C++ Dependencies**  
    - Ensure you have a compatible C++17 compiler installed.

    - On Ubuntu/Debian:
    ```bash
    sudo apt update
    sudo apt install build-essential cmake
    ``` 
    
    - On macOS (using Homebrew):
    ```bash
    brew install gcc cmake
    ``` 

3. **Build the Project with CMake**
    ```bash
    mkdir -p build
    cd build
    cmake ..
    cmake --build .
    ```

    *The installation for the visualization tools in included [here](#running-visualizations), due to the need of being in adifferent directory*

---

## Usage

### Running MetaClass
After building the project with CMake, you can run the MetaClass application with various options:

```bash
# Basic usage with DNA database and metagenomic sample
./apps/MetaClass -d ../data/samples/db.txt -s ../data/samples/meta.txt -k 10 -a 0.1 -t 20
```

Parameter options:
- `-d, --database <file>`: Specify the reference database file
- `-s, --sample <file>`: Specify the metagenomic sample file
- `-k, --context <value>`: Set the context size (order of the model)
- `-a, --alpha <value>`: Set the smoothing parameter
- `-t, --top <value>`: Set the number of top matches to display
- `-m, --save-model <file>`: Save trained model to a file
- `-l, --load-model <file>`: Load a previously trained model
- `-h, --help`: Display help information

---

### Running Tests
After building the project, you can use the test framework to perform parameter optimization and analysis:

    ```bash
    # Basic interactive usage
    ./apps/tests

    # Running with a configuration file
    ./apps/tests --config ../configs/test_config.json

    # Running synthetic data evaluation
    ./apps/tests --synthetic
    ```
The test framework provides:

- **Grid Testing**: Tests multiple combinations of k-order and alpha values

- **Results Visualization**: Automatically saves results to JSON/CSV for later analysis
    - Results are stored in the `results/` directory
    - Each test run creates a timestamp subfolder (e.g., `results/20250414_101430/`)
    - The most recent results are also available to `results/latest/` for convenience (will be renamed to the previous format as soon as new tests are ran)

- **Advanced Analysis**:
    - Symbol Information Analysis: Examines probabilities for each symbol in context
    - Chunk Analysis: Divides samples into chunks and analyzes each separately
    - Cross-Comparison: Compares top organisms against each other

    *Example configuration file available at [config_template.json](configs/config_template.json)*

---

### Running Visualizations

1. **Install Python Dependencies** (optional, for visualization)
    ```bash
    # a) Navigate back to the project root directory (if you are in the build directory)
    cd ..
    
    # b) Create a Python virtual environment
    python3 -m venv venv
    
    # c) Activate the virtual environment
    # On Linux/macOS:
    source venv/bin/activate
    # On Windows:
    # venv\Scripts\activate
    
    # d) Navigate to the visualization directory and install requirements
    cd visualization
    pip install -r requirements.txt
    ```

2. **Run the main script**
    ```bash
    # Navigate to the visualization directory (if not already there)
    cd visualization
    
    # Run the visualization script
    python3 visualize_results.py
    ```

3. **View results**
    The generated visualizations will be saved in the `visualization_results/` directory. Navigate there to see the charts and graphs.

---

## Data Sources
- The genetic sequences used in this project are synthetic DNA sequences for testing purposes.

## Project Structure
The project is organized with the following directory structure:

- `apps/`: Application source files
    - `MetaClass.cpp`: Main application for NRC-based classification
    - `tests.cpp`: Test framework for parameter optimization and analysis

- `configs/`: Configuration files for running tests (templates)

- `data/`: Sample data files
    - `samples/`: Metagenomic samples and reference databases
    - `generated/`: Synthetic data for testing

- `external/`*: External binary dependencies used for synthetic data generation

- `include/`: Header file for json operations

- `results/`: Directory for storing test results

- `scripts/`: Helper scripts to generate synthetic data

- `src/`: Source files for core functionality
    - `core/`: Implementation of the Finite Context Model
    - `utils/`: Utility functions for I/O, compression, and testing

- `visualization/`: Python-based visualization tools
        - `visualization_results/`: Directory for storing visualization results

\* External tools originated from [Cobilab GTO Repository](https://github.com/cobilab/gto)

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