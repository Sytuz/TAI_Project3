import pandas as pd
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
import seaborn as sns
import os

df = pd.read_csv("results/test_results.csv")

df["alpha"] = df["alpha"].astype(float)
df["k"] = df["k"].astype(int)
df["AvgInfoContent"] = pd.to_numeric(df["AvgInfoContent"], errors='coerce')
df["ExecTime(ms)"] = pd.to_numeric(df["ExecTime(ms)"], errors='coerce')

df["File"] = df["File"].apply(lambda x: os.path.splitext(os.path.basename(x))[0])

sns.set_theme(style="whitegrid")
os.makedirs("results/plots", exist_ok=True)

for sequence in df["File"].unique():
    seq_folder = f"results/plots/{sequence}"
    os.makedirs(seq_folder, exist_ok=True)
    df_seq = df[df["File"] == sequence]

    # Plot 1: Average Information Content VS Alpha for different k
    plt.figure(figsize=(10, 6))
    sns.lineplot(data=df_seq, x="alpha", y="AvgInfoContent", hue="k", marker="o")
    plt.title(f"Avg Information Content VS Alpha - {sequence}")
    plt.xlabel("Alpha")
    plt.ylabel("Average Information Content")
    plt.legend(title="k")
    plt.savefig(f"{seq_folder}/avg_info_vs_alpha.png")
    plt.close()

    # Plot 2: Execution Time VS Alpha for different k
    plt.figure(figsize=(10, 6))
    sns.lineplot(data=df_seq, x="alpha", y="ExecTime(ms)", hue="k", marker="o")
    plt.title(f"Execution Time VS Alpha - {sequence}")
    plt.xlabel("Alpha")
    plt.ylabel("Execution Time (ms)")
    plt.legend(title="k")
    plt.savefig(f"{seq_folder}/exec_time_vs_alpha.png")
    plt.close()

    # Plot 3: Execution Time VS k for different alpha values
    plt.figure(figsize=(10, 6))
    sns.lineplot(data=df_seq, x="k", y="ExecTime(ms)", hue="alpha", marker="o", palette="coolwarm")
    plt.title(f"Execution Time VS k - {sequence}")
    plt.xlabel("k")
    plt.ylabel("Execution Time (ms)")
    plt.legend(title="Alpha", bbox_to_anchor=(1,1))
    plt.savefig(f"{seq_folder}/exec_time_vs_k.png")
    plt.close()

    # Plot 4: Average Information Content VS k for different alpha values
    plt.figure(figsize=(10, 6))
    sns.lineplot(data=df_seq, x="k", y="AvgInfoContent", hue="alpha", marker="o", palette="coolwarm")
    plt.title(f"Avg Information Content VS k - {sequence}")
    plt.xlabel("k")
    plt.ylabel("Average Information Content")
    plt.legend(title="Alpha", bbox_to_anchor=(1,1))
    plt.savefig(f"{seq_folder}/avg_info_vs_k.png")
    plt.close()

print("All the plots saved in individual sequence folders within the results/plots folder.")
