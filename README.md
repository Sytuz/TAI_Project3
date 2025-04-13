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
5. [Data Sources](#data-sources)
6. [Project Structure](#project-structure)
7. [Authors](#authors)

---

## Introduction
This project provides:
- **MetaClass**: A C++ program that implements the Normalized Relative Compression (NRC) method to analyze similarities between metagenomic samples and reference sequences. It trains a Finite-Context Model on a metagenomic sample and uses this model to evaluate the compression of reference sequences, ranking them by similarity.

The program takes a metagenomic sample that may contain unknown genetic sequences and attempts to identify which known organisms from a reference database have the highest similarity to the sample.

---

## Dependencies
1. **C++ Compiler**  
   - A modern C++ compiler (e.g., GCC 9+, Clang 10+, or MSVC 2019+) with C++17 (or later) support.
   - Features such as JSON handling and file operations require C++17.
2. **Make** 
   - Required for compiling the project using the provided `Makefile`.

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
    sudo apt install build-essential
    ``` 
    
    - On macOS (using Homebrew):
    ```bash
    brew install gcc make
    ``` 

---

## Usage

## TEMPORARY INSTRUCTIONS
- During refactor process to cmake, the project is now ran this way
```bash
# Make sure to have cmake installed (should be by default)
sudo apt install cmake

# Building the project
mkdir -p build
cd build
cmake ..
cmake --build .

# Running the application
./apps/MetaClass -d ../data/samples/db.txt -s ../data/samples/meta.txt -k 10 -a 0.1 -t 20
```
- Note: tests.cpp will be changed, so no command given, but default path for input have been updated


### Running MetaClass - TO BE CHANGED
A simple Makefile is provided. From the project root directory, compile and build all executables by running:
```bash
make
```
This command will generate the following binaries:
- `MetaClass`
- `tests`

To remove compiled files and binaries:
```bash
make clean
```

#### Example Commands for MetaClass
```bash
./MetaClass -d db.txt -s meta.txt -k 10 -a 0.1 -t 20
```
Trains a model of order `k=10` with smoothing `alpha=0.1` on the metagenomic sample from `meta.txt`, then calculates NRC for each sequence in `db.txt`, and displays the top 20 most similar sequences.

You can also save and load models:
```bash
# Save a model after training
./MetaClass -d db.txt -s meta.txt -m model.bson

# Load a previously trained model
./MetaClass -d db.txt -l model.bson
```

For complete usage information:
```bash
./MetaClass -h
```

### Running Tests
[TBD]

---

## Data Sources
- The genetic sequences used in this project are synthetic DNA sequences for testing purposes.

## Project Structure - TO BE CHANGED
- `FCMModel.*`: Implementation of the Finite Context Model (inherited from Project 1)
- `MetaClass.cpp`: Main implementation of the metagenomic classifier using NRC
- `tests.cpp`: Test suite for validating model functionality and NRC calculations
- `samples/`: Directory containing sample metagenomic files and reference databases

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