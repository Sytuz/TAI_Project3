import os
import pandas as pd
import seaborn as sns
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
from collections import Counter


def read_sequence(file_path):
    with open(file_path, "r") as f:
        return f.read().replace("\n", "")

def compute_complexity_profile_matrix(sequence, k):
    if k == 0:
        print("k cannot be 0. Please provide a positive integer value for k.")
        return None
    if k > len(sequence):
        print(f"Skipping k={k} as it is larger than the sequence length.")
        return None

    counter = Counter(sequence[i:i+k+1] for i in range(len(sequence) - k + 1))
    unique_chars = set("".join(counter.keys()))
    matrix = {sub[:-1]: {c: 0 for c in unique_chars} for sub in counter.keys()}

    for sub, count in counter.items():
        if sub[:-1] in matrix and sub[-1] in matrix[sub[:-1]]:
            matrix[sub[:-1]][sub[-1]] += count

    df_matrix = pd.DataFrame(matrix).fillna(0).astype(int).T
    df_matrix["Sum"] = df_matrix.sum(axis=1)

    return df_matrix


def plot_complexity_profile_matrix(cp_matrix, sequence_name, k, output_folder):
    os.makedirs(output_folder, exist_ok=True)

    plt.figure(figsize=(12, 8))
    sns.heatmap(cp_matrix, annot=True, fmt="d", cmap="YlOrBr")
    plt.title(f"K Frequency Matrix (k={k}) - {sequence_name}")
    plt.xlabel("Next Char")
    plt.ylabel(f"{k} Context")
    plt.savefig(f"{output_folder}/complexity_profile_matrix_k{k}.png")
    plt.close()

def process_complexity_profile(sequence_file, k, output_folder):
    if not os.path.exists(sequence_file):
        print(f"Sequence file {sequence_file} not found.")
        return

    sequence = read_sequence(sequence_file)
    cp_matrix = compute_complexity_profile_matrix(sequence, k)
    plot_complexity_profile_matrix(cp_matrix, os.path.basename(sequence_file), k, output_folder)
    print(f"Complexity Profile Matrix for k={k} saved successfully!")


def main():
    print(f"Warning: a large 'k' may take a long time and use a lot of memory! Proceed with caution.")

    while True:
        print("\n--- Complexity Profile Matrix ---")
        sequence_name = input("Enter the sequence filename (in the 'sequences/' folder, without .txt) or press Enter to quit: ")
        if not sequence_name:
            break

        sequence_file = f"../sequences/{sequence_name}.txt"
        k_value = input("Enter the value of k: ")

        try:
            k = int(k_value)
            if k < 0:
                print("Invalid k value. Please enter a non negative number (>= 0).")
                continue
        except ValueError:
            print("Invalid input. Please enter a valid integer (>= 0).")

        output_folder = f"../results/plots/{sequence_name}/complexity_profiles"
        process_complexity_profile(sequence_file, k, output_folder)


if __name__ == "__main__":
    main()
