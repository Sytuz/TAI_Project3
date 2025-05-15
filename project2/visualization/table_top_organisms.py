import json
import pandas as pd
import matplotlib.pyplot as plt
from matplotlib.backends.backend_pdf import PdfPages


def load_json_data(json_path):
    with open(json_path, "r") as f:
        data = json.load(f)
    return data

def convert_to_dataframe(data, target_k, target_alpha):
    # Extract the best configuration
    entry = next((item for item in data if item["k"] == target_k and item["alpha"] == target_alpha), None)

    if entry is None:
        raise ValueError(f"No data found for k={target_k} and alpha={target_alpha}")

    k = entry["k"]
    alpha = entry["alpha"]
    references = entry["references"][:10]  # Top 10 results

    # Convert to DataFrame
    df = pd.DataFrame(references)
    df = df[["rank", "name", "nrc", "kld"]]
    df.columns = ["Rank", "Organism", "NRC", "KLD"]

    # Clean and shorten names for formatting
    df["Organism"] = df["Organism"].str.replace(r"gi\|\d+\|ref\|", "", regex=True)
    df["Organism"] = df["Organism"].str.replace("_", " ").str.slice(0, 60) + "..."

    return df, k, alpha

def generate_latex_table(df, k, alpha, latex_path):
    latex_code = []
    latex_code.append("\\begin{table}[H]")
    latex_code.append("\\centering")
    latex_code.append(f"\\caption{{Top 10 NRC-ranked organisms (k = {k}, $\\alpha$ = {alpha:.2f})}}")
    latex_code.append("\\label{tab:nrc_top}")
    latex_code.append("\\resizebox{\columnwidth}{!}{%")
    latex_code.append("\\begin{tabular}{|c|p{6.5cm}|c|c|}")
    latex_code.append("\\hline")
    latex_code.append("Rank & Organism Name & NRC & KLD \\\\ \\hline")

    for _, row in df.iterrows():
        name = row["Organism"].replace("_", "\\_")
        latex_code.append(f"{int(row['Rank'])} & {name} & {row['NRC']:.6f} & {row['KLD']:.6f} \\\\ \\hline")

    latex_code.append("\\end{tabular}")
    latex_code.append("}")
    latex_code.append("\\end{table}")

    # Save LaTeX to a .tex file
    with open(latex_path, "w") as f:
        f.write("\n".join(latex_code))

def generate_table_image(df, k, alpha, image_path):
    fig, ax = plt.subplots(figsize=(12, 4))
    ax.axis('off')
    table = ax.table(cellText=df.values,
                    colLabels=df.columns,
                    colColours=["#f2f2f2"]*4,
                    loc='center',
                    cellLoc='left')
    table.auto_set_font_size(False)
    table.set_fontsize(9)
    table.scale(1.2, 1.5)

    plt.title(f"Top 10 NRC-ranked organisms (k = {k}, $\\alpha$ = {alpha:.2f})", fontsize=14)
    plt.tight_layout()
    plt.savefig(image_path, dpi=300, bbox_inches='tight')

def create_table_top_organisms():
    # Load JSON data
    json_path = "../results/latest/top_organisms_results.json"
    data = load_json_data(json_path)

    # Convert to DataFrame
    target_k = 17
    target_alpha = 0.01
    df, k, alpha = convert_to_dataframe(data, target_k, target_alpha)

    # --- Generate LaTeX table code ---
    latex_path = "visualization_results/latest/top_organisms_table.tex"
    generate_latex_table(df, k, alpha, latex_path)

    # --- Generate Table Image ---
    image_path = "visualization_results/latest/top_organisms_table.png"
    generate_table_image(df, k, alpha, image_path)

    print(f"Table LaTeX code saved to {latex_path}")
    print(f"Table image saved to {image_path}")
