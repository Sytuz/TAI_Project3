# T.A.I. - Project 3
## Music Identification using Normalized Compression Distance

### Introduction:
This project implements a music identification system based on the Normalized Compression Distance (NCD). The NCD is a general similarity metric between two objects (e.g., audio files), defined by how well they can be compressed together.
It requires no domain-specific features - it measures similarity purely via compression.
*Cilibrasi et al.* demonstrated a "fully automatic" music classification method that uses only string compression of music representations, requiring no musical background knowledge.
In our system, the audio files are converted to feature strings (e.g., frequency sequences), and the NCD between feature files is computed as:
NCD(x, y) = (C(xy) - min{C(x), C(y)}) / (max{C(x), C(y)}),
where C(.) is the compressed length (using zlib).
NCD is universal in the sense that it captures all the effective notions of similarity at once.

### The project is organized as follows:
- [apps/](/apps/): Executables: music_id, extract_features, compute_ncd, run_tests.
- [include/core/](/include/core/): Core classes: WAVReader, FTFExtractor, MaxFreqExtractor, NCD, TreeBuilder.
- [include/utils](/include/utils/): Utility classes: Segmenter, NoiseInjector, CompressorWrapper.
- [src/core/](/src/core/): Implementations of core modules.
- [src/utils/](/src/utils/): Implementations of utilities.
- [data/samples/](/data/samples/): Input WAV files.
- [data/generated](/data/generated/): Generated feature files per method.
- [results/](/results/): Output similarity matrices, Newick trees and images.
- [scripts/](/scripts/): Shell scripts and external calls.
- [visualization/](/visualization/): Python script to plot trees and heatmaps.
- [CMakeLists.txt](/CMakeLists.txt): CMake configuration.

### Overview

- **Input:** WAV files (mono or stereo) in `data/samples/`.
- **Feature Extraction:** Audio is segmented into overlapping frames. Two methods are available:
  - **MaxFreq (provided)**: find dominant frequency per frame.
  - **FFT-based (custom)**: compute FFT magnitudes and select top frequencies.
- **Noise (optional):** Add Gaussian noise at a specified SNR via CLI.
- **Compression:** Use a chosen compressor (`gzip`, `bzip2`, `lzma`, or `zstd`) to compress feature files.
- **NCD Matrix:** Compute pairwise NCD = (C(xy) - min(C(x),C(y))) / max(C(x),C(y)) for all feature pairs, output as CSV in `results/`.
- **Tree Building:** Use a quartet-based method:contentReference[oaicite:7]{index=7} (external script) to build a tree. Output Newick in `results/` and optionally a PNG image of the tree.
- **Visualization:** Python scripts (`visualization/`) plot the tree (PNG) or heatmap from the NCD matrix.


