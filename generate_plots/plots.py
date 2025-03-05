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
    df["ModelSize"] = pd.to_numeric(df["ModelSize"], errors="coerce")

    df["File"] = df["File"].apply(lambda x: os.path.splitext(os.path.basename(x))[0])
    df["Format"] = df["ModelFile"].apply(lambda x: "bson" if x.endswith(".bson") else "json")

    files_to_remove = df.loc[df["ModelSize"] == 0, "File"].unique()
    df = df[~df["File"].isin(files_to_remove)]

    return df

def plot_metric_vs_alpha(df_seq, sequence, seq_folder, metric, y_label):
    plt.figure(figsize=(10, 6))
    sns.lineplot(data=df_seq, x="alpha", y=metric, hue="k", marker="o", palette="coolwarm")
    plt.title(f"{y_label} VS Alpha - {sequence}")
    plt.xlabel("Alpha")
    plt.ylabel(y_label)
    plt.legend(title="k")
    plt.savefig(f"{seq_folder}/{metric.lower()}_vs_alpha.png")
    plt.close()

def plot_metric_vs_k(df_seq, sequence, seq_folder, metric, y_label):
    plt.figure(figsize=(10, 6))
    sns.lineplot(data=df_seq, x="k", y=metric, hue="alpha", marker="o", palette="coolwarm")
    plt.title(f"{y_label} VS k - {sequence}")
    plt.xlabel("k")
    plt.ylabel(y_label)
    plt.legend(title="Alpha", bbox_to_anchor=(1,1))
    plt.savefig(f"{seq_folder}/{metric.lower()}_vs_k.png")
    plt.close()

def plot_bson_json_comparison_alpha(df_seq, sequence, seq_folder, metric, y_label):
    plt.figure(figsize=(10, 6))
    sns.lineplot(data=df_seq, x="alpha", y=metric, hue="Format", marker="o")
    plt.title(f"{y_label} Comparison (BSON VS JSON) - {sequence}")
    plt.xlabel("Alpha")
    plt.ylabel(y_label)
    plt.legend(title="Format")
    plt.savefig(f"{seq_folder}/{metric.lower()}_bson_vs_json.png")
    plt.close()

def plot_bson_json_comparison_k(df_seq, sequence, seq_folder, metric, y_label):
    plt.figure(figsize=(10, 6))
    sns.lineplot(data=df_seq, x="k", y=metric, hue="Format", marker="o")
    plt.title(f"{y_label} Comparison (BSON VS JSON) - {sequence}")
    plt.xlabel("k")
    plt.ylabel(y_label)
    plt.legend(title="Format")
    plt.savefig(f"{seq_folder}/{metric.lower()}_bson_vs_json.png")
    plt.close()

def save_metrics(df, base_folder):
    os.makedirs(base_folder, exist_ok=True)
    metrics_file = os.path.join(base_folder, "metrics_summary.txt")
    with open(metrics_file, "w") as f:
        for sequence in df["File"].unique():
            df_seq = df[df["File"] == sequence]
            avg_exec_json = df_seq[df_seq["Format"] == "json"]["ExecTime(ms)"].mean()
            avg_exec_bson = df_seq[df_seq["Format"] == "bson"]["ExecTime(ms)"].mean()
            avg_size_json = df_seq[df_seq["Format"] == "json"]["ModelSize"].mean()
            avg_size_bson = df_seq[df_seq["Format"] == "bson"]["ModelSize"].mean()
            f.write(f"{sequence}:\n")
            f.write(f"  Avg Execution Time (JSON): {avg_exec_json:.2f} ms\n")
            f.write(f"  Avg Execution Time (BSON): {avg_exec_bson:.2f} ms\n")
            f.write(f"  Avg Model Size (JSON): {avg_size_json:.2f} bytes\n")
            f.write(f"  Avg Model Size (BSON): {avg_size_bson:.2f} bytes\n\n")

def plot_avg_exec_time_comparison(df, base_folder):
    plt.figure(figsize=(10, 6))
    df_avg_exec = df.groupby(["File", "Format"])["ExecTime(ms)"].mean().reset_index()
    sns.barplot(data=df_avg_exec, x="File", y="ExecTime(ms)", hue="Format")
    plt.title("Average Execution Time Comparison (BSON vs JSON)")
    plt.xlabel("Sequence")
    plt.ylabel("Execution Time (ms)")
    plt.legend(title="Format")
    plt.xticks(rotation=15, ha="right")
    plt.savefig(os.path.join(base_folder, "avg_exec_time_comparison.png"))
    plt.close()

def plot_avg_model_size_comparison(df, base_folder):
    plt.figure(figsize=(12, 6))
    df_avg_size = df.groupby(["File", "Format"])["ModelSize"].mean().reset_index()
    sns.barplot(data=df_avg_size, x="File", y="ModelSize", hue="Format")
    plt.title("Average Model Size Comparison (BSON vs JSON)")
    plt.xlabel("Sequence")
    plt.ylabel("Model Size (bytes)")
    plt.legend(title="Format")
    plt.xticks(rotation=15, ha="right")
    plt.savefig(os.path.join(base_folder, "avg_model_size_comparison.png"))
    plt.close()

def generate_plots(df, base_folder):
    sns.set_theme(style="whitegrid")
    os.makedirs(base_folder, exist_ok=True)

    for sequence in df["File"].unique():
        seq_folder = os.path.join(base_folder, sequence)
        os.makedirs(seq_folder, exist_ok=True)
        df_seq = df[df["File"] == sequence]

        plot_metric_vs_alpha(df_seq, sequence, seq_folder, "AvgInfoContent", "Average Information Content")
        plot_metric_vs_k(df_seq, sequence, seq_folder, "AvgInfoContent", "Average Information Content")
        plot_bson_json_comparison_alpha(df_seq, sequence, seq_folder, "ExecTime(ms)", "Execution Time (ms)")
        plot_bson_json_comparison_alpha(df_seq, sequence, seq_folder, "ModelSize", "Model Size (bytes)")
        plot_bson_json_comparison_k(df_seq, sequence, seq_folder, "ExecTime(ms)", "Execution Time (ms)")
        plot_bson_json_comparison_k(df_seq, sequence, seq_folder, "ModelSize", "Model Size (bytes)")

    save_metrics(df, base_folder)
    plot_avg_exec_time_comparison(df, base_folder)
    plot_avg_model_size_comparison(df, base_folder)

def main():
    base_folder = "../results/plots"
    df = load_data("../results/test_results.csv")
    generate_plots(df, base_folder)
    print("All plots and metrics saved successfully!")


if __name__ == "__main__":
    main()
