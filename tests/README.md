# Tests

The tests were performed on:
- [small_samples](/data/small_samples/): in order to test if the tests were working.
- [samples](/data/samples/): Songs from the internet (36).
- [full_tracks](/data/full_tracks/): Songs from youtube (100 known songs across 10 different music genres).

For the small samples and samples songs, the tests were done with all possible combinations of:
- Compressors: gzip, bzip2, lzma, zstd
- Methods: spectral, maxfreq
- Formats: text, binary
- Noise Types: clean (no noise), white, brown, pink

For the full_tracks, all the previous combinations were also all done for the clean noise type. However, due to computational time reasons, for the different types of noise (white, brown, pink), the tests were only made for the following possible combinations:
- Compressors: gzip, bzip2, lzma, zstd
- Methods: maxfreq
- Formats: binary
- Noise Types: white, brown, pink

These combinations were chosed having in account the results of the previous tests for the clean noise of these songs and from the results of the other songs.

All results are saved in the folders inside this folder.

