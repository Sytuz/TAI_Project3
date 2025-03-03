import os
import pandas as pd
import seaborn as sns
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt


def load_data(csv_path):
    df = pd.read_csv(csv_path)

    df["alpha"] = df["alpha"].astype(float)
    df["k"] = df["k"].astype(int)
    df["AvgInfoContent"] = pd.to_numeric(df["AvgInfoContent"], errors="coerce")
    df["ExecTime(ms)"] = pd.to_numeric(df["ExecTime(ms)"], errors="coerce")

    df["File"] = df["File"].apply(lambda x: os.path.splitext(os.path.basename(x))[0])
    return df

def plot_avg_info_cont_vs_alpha(df_seq, sequence, seq_folder):
    plt.figure(figsize=(10, 6))
    sns.lineplot(data=df_seq, x="alpha", y="AvgInfoContent", hue="k", marker="o")
    plt.title(f"Avg Information Content VS Alpha - {sequence}")
    plt.xlabel("Alpha")
    plt.ylabel("Average Information Content")
    plt.legend(title="k")
    plt.savefig(f"{seq_folder}/avg_info_vs_alpha.png")
    plt.close()

def plot_exec_time_vs_alpha(df_seq, sequence, seq_folder):
    plt.figure(figsize=(10, 6))
    sns.lineplot(data=df_seq, x="alpha", y="ExecTime(ms)", hue="k", marker="o")
    plt.title(f"Execution Time VS Alpha - {sequence}")
    plt.xlabel("Alpha")
    plt.ylabel("Execution Time (ms)")
    plt.legend(title="k")
    plt.savefig(f"{seq_folder}/exec_time_vs_alpha.png")
    plt.close()

def plot_exec_time_vs_k(df_seq, sequence, seq_folder):
    plt.figure(figsize=(10, 6))
    sns.lineplot(data=df_seq, x="k", y="ExecTime(ms)", hue="alpha", marker="o", palette="coolwarm")
    plt.title(f"Execution Time VS k - {sequence}")
    plt.xlabel("k")
    plt.ylabel("Execution Time (ms)")
    plt.legend(title="Alpha", bbox_to_anchor=(1,1))
    plt.savefig(f"{seq_folder}/exec_time_vs_k.png")
    plt.close()

def plot_avg_info_cont_vs_k(df_seq, sequence, seq_folder):
    plt.figure(figsize=(10, 6))
    sns.lineplot(data=df_seq, x="k", y="AvgInfoContent", hue="alpha", marker="o", palette="coolwarm")
    plt.title(f"Avg Information Content VS k - {sequence}")
    plt.xlabel("k")
    plt.ylabel("Average Information Content")
    plt.legend(title="Alpha", bbox_to_anchor=(1,1))
    plt.savefig(f"{seq_folder}/avg_info_vs_k.png")
    plt.close()

def generate_plots(df, base_folder):
    sns.set_theme(style="whitegrid")
    os.makedirs(base_folder, exist_ok=True)

    for sequence in df["File"].unique():
        seq_folder = f"results/plots/{sequence}"
        os.makedirs(seq_folder, exist_ok=True)
        df_seq = df[df["File"] == sequence]

        # Plot 1: Average Information Content VS Alpha for different k
        plot_avg_info_cont_vs_alpha(df_seq, sequence, seq_folder)

        # Plot 2: Execution Time VS Alpha for different k
        plot_exec_time_vs_alpha(df_seq, sequence, seq_folder)

        # Plot 3: Execution Time VS k for different alpha values
        plot_exec_time_vs_k(df_seq, sequence, seq_folder)

        # Plot 4: Average Information Content VS k for different alpha values
        plot_avg_info_cont_vs_k(df_seq, sequence, seq_folder)

def main():
    base_folder = "../results/plots"
    df = load_data("../results/test_results.csv")
    generate_plots(df, base_folder)
    print("All plots saved successfully!")


if __name__ == "__main__":
    main()
