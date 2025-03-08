import os
import numpy as np
import pandas as pd
import seaborn as sns
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt


def load_data(csv_path):
    df = pd.read_csv(csv_path)

    df["alpha"] = df["alpha"].astype(float)
    df["k"] = df["k"].astype(int)
    df["RecursiveStep"] = df["RecursiveStep"].astype(int)
    df["AvgInfoContent"] = pd.to_numeric(df["AvgInfoContent"], errors="coerce")
    df["ExecTime(ms)"] = pd.to_numeric(df["ExecTime(ms)"], errors="coerce")
    df["ModelSize"] = pd.to_numeric(df["ModelSize"], errors="coerce")

    df["File"] = df["File"].apply(lambda x: os.path.splitext(os.path.basename(x))[0])
    df["Format"] = df["ModelType"]

    files_to_remove = df.loc[df["ModelSize"] == 0, "File"].unique()
    df = df[~df["File"].isin(files_to_remove)]

    return df

def plot_metric_vs_alpha(df_seq, sequence, seq_folder, metric, y_label):
    plt.figure(figsize=(10, 6))
    # Filter for just step 0 (initial run)
    df_step0 = df_seq[df_seq["RecursiveStep"] == 0]
    sns.lineplot(data=df_step0, x="alpha", y=metric, hue="k", marker="o", palette="coolwarm")
    plt.title(f"{y_label} VS Alpha - {sequence}")
    plt.xlabel("Alpha")
    plt.ylabel(y_label)
    plt.legend(title="k")
    plt.savefig(f"{seq_folder}/{metric.lower()}_vs_alpha.png")
    plt.close()

def plot_metric_vs_k(df_seq, sequence, seq_folder, metric, y_label):
    plt.figure(figsize=(10, 6))
    # Filter for just step 0 (initial run)
    df_step0 = df_seq[df_seq["RecursiveStep"] == 0]
    sns.lineplot(data=df_step0, x="k", y=metric, hue="alpha", marker="o", palette="coolwarm")
    plt.title(f"{y_label} VS k - {sequence}")
    plt.xlabel("k")
    plt.ylabel(y_label)
    plt.legend(title="Alpha", bbox_to_anchor=(1,1))
    plt.savefig(f"{seq_folder}/{metric.lower()}_vs_k.png")
    plt.close()

def plot_bson_json_comparison_alpha(df_seq, sequence, seq_folder, metric, y_label):
    plt.figure(figsize=(10, 6))
    # Filter for just step 0 (initial run)
    df_step0 = df_seq[df_seq["RecursiveStep"] == 0]
    sns.lineplot(data=df_step0, x="alpha", y=metric, hue="Format", marker="o")
    plt.title(f"{y_label} Comparison (BSON VS JSON) - {sequence}")
    plt.xlabel("Alpha")
    plt.ylabel(y_label)
    plt.legend(title="Format")
    plt.savefig(f"{seq_folder}/{metric.lower()}_bson_vs_json.png")
    plt.close()

def plot_bson_json_comparison_k(df_seq, sequence, seq_folder, metric, y_label):
    plt.figure(figsize=(10, 6))
    # Filter for just step 0 (initial run)
    df_step0 = df_seq[df_seq["RecursiveStep"] == 0]
    sns.lineplot(data=df_step0, x="k", y=metric, hue="Format", marker="o")
    plt.title(f"{y_label} Comparison (BSON VS JSON) - {sequence}")
    plt.xlabel("k")
    plt.ylabel(y_label)
    plt.legend(title="Format")
    plt.savefig(f"{seq_folder}/{metric.lower()}_bson_vs_json.png")
    plt.close()

def plot_recursive_info_content(df_seq, sequence, seq_folder):
    recursive_folder = os.path.join(seq_folder, "recurive_folder")
    os.makedirs(recursive_folder, exist_ok=True)
    k_values = sorted(df_seq["k"].unique())
    alpha_values = sorted(df_seq["alpha"].unique())

    for k in k_values:
        plt.figure(figsize=(12, 7))
        df_k = df_seq[df_seq["k"] == k]
        sns.lineplot(data=df_k, x="RecursiveStep", y="AvgInfoContent", hue="alpha", marker="o", palette="viridis")
        plt.title(f"Average Information Content Over Recursive Steps (k={k}) - {sequence}")
        plt.xlabel("Recursive Step")
        plt.ylabel("Average Information Content")
        plt.legend(title="Alpha", bbox_to_anchor=(1.05, 1), loc='upper left')
        plt.grid(True, linestyle='--', alpha=0.7)
        plt.tight_layout()
        plt.savefig(f"{recursive_folder}/recursive_info_content_k{k}.png")
        plt.close()

    for alpha in alpha_values:
        plt.figure(figsize=(12, 7))
        df_alpha = df_seq[df_seq["alpha"] == alpha]
        sns.lineplot(data=df_alpha, x="RecursiveStep", y="AvgInfoContent", hue="k", marker="o", palette="coolwarm")
        plt.title(f"Average Information Content Over Recursive Steps (alpha={alpha}) - {sequence}")
        plt.xlabel("Recursive Step")
        plt.ylabel("Average Information Content")
        plt.legend(title="k", bbox_to_anchor=(1.05, 1), loc='upper left')
        plt.grid(True, linestyle='--', alpha=0.7)
        plt.tight_layout()
        plt.savefig(f"{recursive_folder}/recursive_info_content_alpha{alpha:.2f}.png")
        plt.close()

    # Heatmap showing average information content across all recursive steps and k values
    # for a specific alpha (using the median alpha)
    median_alpha = np.median(alpha_values)
    df_med_alpha = df_seq[df_seq["alpha"].round(2) == round(median_alpha, 2)]

    if not df_med_alpha.empty:
        plt.figure(figsize=(12, 8))
        heatmap_data = df_med_alpha.pivot_table(
            values="AvgInfoContent",
            index="k",
            columns="RecursiveStep"
        )
        sns.heatmap(heatmap_data, annot=True, fmt=".2f", cmap="viridis", linewidths=.5)
        plt.title(f"Average Information Content Heatmap (alpha≈{median_alpha:.2f}) - {sequence}")
        plt.xlabel("Recursive Step")
        plt.ylabel("k")
        plt.tight_layout()
        plt.savefig(f"{seq_folder}/info_content_heatmap_alpha{median_alpha:.2f}.png")
        plt.close()

def plot_complexity_profile(df_seq, sequence, seq_folder):
    # Filter for initial step
    df_step0 = df_seq[df_seq["RecursiveStep"] == 0]

    # Plot complexity profile for different alpha values
    plt.figure(figsize=(12, 7))
    sns.lineplot(data=df_step0, x="k", y="AvgInfoContent", hue="alpha", marker="o", palette="viridis")
    plt.title(f"Complexity Profile - {sequence}")
    plt.xlabel("Context Length (k)")
    plt.ylabel("Average Information Content (bits/symbol)")
    plt.legend(title="Alpha", bbox_to_anchor=(1.05, 1), loc='upper left')
    plt.grid(True, linestyle='--', alpha=0.7)
    plt.tight_layout()
    plt.savefig(f"{seq_folder}/complexity_profile.png")
    plt.close()

    # Plot for different recursive steps (using median alpha)
    alpha_values = sorted(df_seq["alpha"].unique())
    median_alpha = np.median(alpha_values)
    df_med_alpha = df_seq[df_seq["alpha"].round(2) == round(median_alpha, 2)]

    if not df_med_alpha.empty:
        plt.figure(figsize=(12, 7))
        recursive_steps = sorted(df_med_alpha["RecursiveStep"].unique())
        # Select a subset of steps if there are too many
        steps_to_plot = recursive_steps if len(recursive_steps) <= 5 else [0, 1, 2, 5, max(recursive_steps)]

        df_selected = df_med_alpha[df_med_alpha["RecursiveStep"].isin(steps_to_plot)]
        sns.lineplot(data=df_selected, x="k", y="AvgInfoContent", hue="RecursiveStep", marker="o", palette="coolwarm")
        plt.title(f"Complexity Profile Evolution (alpha≈{median_alpha:.2f}) - {sequence}")
        plt.xlabel("Context Length (k)")
        plt.ylabel("Average Information Content (bits/symbol)")
        plt.legend(title="Step", bbox_to_anchor=(1.05, 1), loc='upper left')
        plt.grid(True, linestyle='--', alpha=0.7)
        plt.tight_layout()
        plt.savefig(f"{seq_folder}/complexity_profile_evolution.png")
        plt.close()

def plot_convergence_analysis(df_seq, sequence, seq_folder):
    variance_data = []

    k_values = sorted(df_seq["k"].unique())
    alpha_values = sorted(df_seq["alpha"].unique())

    for k in k_values:
        for alpha in alpha_values:
            df_subset = df_seq[(df_seq["k"] == k) & (df_seq["alpha"] == alpha)]

            if len(df_subset) > 1:  # Need at least 2 steps to measure variance
                df_subset = df_subset.sort_values("RecursiveStep")
                df_subset["InfoContentChange"] = df_subset["AvgInfoContent"].diff().abs()

                df_subset = df_subset.dropna(subset=["InfoContentChange"])

                for _, row in df_subset.iterrows():
                    variance_data.append({
                        "k": k,
                        "alpha": alpha,
                        "RecursiveStep": row["RecursiveStep"],
                        "InfoContentChange": row["InfoContentChange"]
                    })

    if variance_data:
        df_variance = pd.DataFrame(variance_data)

        plt.figure(figsize=(12, 7))
        sns.lineplot(
            data=df_variance,
            x="RecursiveStep",
            y="InfoContentChange",
            hue="k",
            style="alpha",
            markers=True,
            palette="coolwarm"
        )
        plt.title(f"Convergence Analysis - Change in Information Content - {sequence}")
        plt.xlabel("Recursive Step")
        plt.ylabel("Absolute Change in Avg. Information Content")
        plt.yscale("log")
        plt.legend(title="Parameters", bbox_to_anchor=(1.05, 1), loc='upper left')
        plt.grid(True, linestyle='--', alpha=0.7)
        plt.tight_layout()
        plt.savefig(f"{seq_folder}/convergence_analysis.png")
        plt.close()

        # Heatmap of average change by k and alpha
        plt.figure(figsize=(10, 8))
        pivot_data = df_variance.groupby(["k", "alpha"])["InfoContentChange"].mean().reset_index()
        heatmap_data = pivot_data.pivot_table(index="k", columns="alpha", values="InfoContentChange", aggfunc="mean")
        sns.heatmap(heatmap_data, annot=True, fmt=".3f", cmap="viridis", linewidths=.5)
        plt.title(f"Average Change in Information Content by k and alpha - {sequence}")
        plt.tight_layout()
        plt.savefig(f"{seq_folder}/convergence_heatmap.png")
        plt.close()

def save_metrics(df, base_folder):
    os.makedirs(base_folder, exist_ok=True)
    metrics_file = os.path.join(base_folder, "metrics_summary.txt")
    with open(metrics_file, "w") as f:
        for sequence in df["File"].unique():
            df_seq = df[df["File"] == sequence]
            # Filter to just initial step for format comparisons
            df_seq_step0 = df_seq[df_seq["RecursiveStep"] == 0]

            # Format comparison metrics
            avg_exec_json = df_seq_step0[df_seq_step0["Format"] == "JSON"]["ExecTime(ms)"].mean()
            avg_exec_bson = df_seq_step0[df_seq_step0["Format"] == "BSON"]["ExecTime(ms)"].mean()
            avg_size_json = df_seq_step0[df_seq_step0["Format"] == "JSON"]["ModelSize"].mean()
            avg_size_bson = df_seq_step0[df_seq_step0["Format"] == "BSON"]["ModelSize"].mean()

            # Recursive analysis metrics
            max_steps = df_seq["RecursiveStep"].max()

            # Get mean, min, max information content by step
            step_metrics = df_seq.groupby("RecursiveStep")["AvgInfoContent"].agg(["mean", "min", "max"]).reset_index()

            # Calculate convergence metrics - average change between consecutive steps
            convergence_data = []
            for k in df_seq["k"].unique():
                for alpha in df_seq["alpha"].unique():
                    df_params = df_seq[(df_seq["k"] == k) & (df_seq["alpha"] == alpha)]
                    if len(df_params) > 1:
                        df_params = df_params.sort_values("RecursiveStep")
                        changes = df_params["AvgInfoContent"].diff().abs().dropna()
                        if not changes.empty:
                            avg_change = changes.mean()
                            convergence_data.append({
                                "k": k,
                                "alpha": alpha,
                                "avg_change": avg_change
                            })

            f.write(f"{sequence}:\n")
            f.write(f"  Format Comparison:\n")
            f.write(f"    Avg Execution Time (JSON): {avg_exec_json:.2f} ms\n")
            f.write(f"    Avg Execution Time (BSON): {avg_exec_bson:.2f} ms\n")
            f.write(f"    Avg Model Size (JSON): {avg_size_json:.2f} bytes\n")
            f.write(f"    Avg Model Size (BSON): {avg_size_bson:.2f} bytes\n\n")

            f.write(f"  Recursive Analysis:\n")
            f.write(f"    Total Recursive Steps: {max_steps}\n")
            f.write(f"    Information Content by Step:\n")

            for _, row in step_metrics.iterrows():
                f.write(f"      Step {int(row['RecursiveStep'])}: Mean={row['mean']:.4f}, Min={row['min']:.4f}, Max={row['max']:.4f}\n")

            f.write(f"\n    Convergence Analysis (Avg Change in Info Content):\n")
            for item in sorted(convergence_data, key=lambda x: (x["k"], x["alpha"])):
                f.write(f"      k={item['k']}, alpha={item['alpha']:.2f}: {item['avg_change']:.6f}\n")

            f.write("\n\n")

def plot_avg_exec_time_comparison(df, base_folder):
    plt.figure(figsize=(12, 7))
    # Filter for step 0
    df_step0 = df[df["RecursiveStep"] == 0]
    df_avg_exec = df_step0.groupby(["File", "Format"])["ExecTime(ms)"].mean().reset_index()
    sns.barplot(data=df_avg_exec, x="File", y="ExecTime(ms)", hue="Format")
    plt.title("Average Execution Time Comparison (BSON vs JSON)")
    plt.xlabel("Sequence")
    plt.ylabel("Execution Time (ms)")
    plt.legend(title="Format")
    plt.xticks(rotation=15, ha="right")
    plt.tight_layout()
    plt.savefig(os.path.join(base_folder, "avg_exec_time_comparison.png"))
    plt.close()

def plot_avg_model_size_comparison(df, base_folder):
    plt.figure(figsize=(12, 7))
    # Filter for step 0
    df_step0 = df[df["RecursiveStep"] == 0]
    df_avg_size = df_step0.groupby(["File", "Format"])["ModelSize"].mean().reset_index()
    sns.barplot(data=df_avg_size, x="File", y="ModelSize", hue="Format")
    plt.title("Average Model Size Comparison (BSON vs JSON)")
    plt.xlabel("Sequence")
    plt.ylabel("Model Size (bytes)")
    plt.legend(title="Format")
    plt.xticks(rotation=15, ha="right")
    plt.tight_layout()
    plt.savefig(os.path.join(base_folder, "avg_model_size_comparison.png"))
    plt.close()

def plot_all_sequences_complexity(df, base_folder):
    plt.figure(figsize=(14, 8))
    # Filter for step 0 and a specific alpha (using median)
    df_step0 = df[df["RecursiveStep"] == 0]
    alpha_values = sorted(df["alpha"].unique())
    median_alpha = np.median(alpha_values)
    df_filtered = df_step0[df_step0["alpha"].round(2) == round(median_alpha, 2)]

    if not df_filtered.empty:
        sns.lineplot(data=df_filtered, x="k", y="AvgInfoContent", hue="File", marker="o")
        plt.title(f"Complexity Profiles Comparison (alpha≈{median_alpha:.2f})")
        plt.xlabel("Context Length (k)")
        plt.ylabel("Average Information Content (bits/symbol)")
        plt.legend(title="Sequence", bbox_to_anchor=(1.05, 1), loc='upper left')
        plt.grid(True, linestyle='--', alpha=0.7)
        plt.tight_layout()
        plt.savefig(os.path.join(base_folder, "all_sequences_complexity.png"))
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
        plot_recursive_info_content(df_seq, sequence, seq_folder)
        plot_complexity_profile(df_seq, sequence, seq_folder)
        plot_convergence_analysis(df_seq, sequence, seq_folder)

    save_metrics(df, base_folder)
    plot_avg_exec_time_comparison(df, base_folder)
    plot_avg_model_size_comparison(df, base_folder)
    plot_all_sequences_complexity(df, base_folder)

def main():
    base_folder = "../results/plots"
    df = load_data("../results/test_results.csv")
    generate_plots(df, base_folder)
    print("All plots and metrics saved successfully!")


if __name__ == "__main__":
    main()
