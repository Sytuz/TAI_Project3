# Comparison between old metaClass with simpler model vs integration with our project 1 model.

Variables: k = 10, alpha = 0.1, t = 20 (default)

## Simpler model

### Using db.txt

Top 20 most similar sequences:
----------------------------------------
Rank |        NRC | Reference Name
----------------------------------------
   1 |   0.170362 | NC_001348.1 Human herpesvirus 3, complete genome
   2 |   0.211057 | Super ISS Si1240
   3 |   0.213665 | NC_005831.2 Human Coronavirus NL63, complete genome
   4 |   0.288031 | OR353425.1 Octopus vulgaris mitochondrion, complete genome
   5 |   0.310735 | Super MUL 720
   6 |   0.557389 | gi|49169782|ref|NC_005831.2| Human Coronavirus NL63, complete genome
   7 |   1.204299 | gi|9629255|ref|NC_001793.1| Oat blue dwarf virus complete genome
   8 |   1.208147 | gi|971748077|ref|NC_028836.1| Pseudomonas phage DL62, complete genome
   9 |   1.209897 | gi|14141972|ref|NC_002786.1| Maize rayado fino virus isolate Costa Rican, complete genome
  10 |   1.214257 | gi|84662653|ref|NC_007710.1| Xanthomonas oryzae phage OP2 DNA, complete genome
  11 |   1.223399 | gi|472339725|ref|NC_020840.1| Tetraselmis viridis virus S20 genomic sequence
  12 |   1.223987 | gi|971752197|ref|NC_028885.1| Pseudomonas phage DL64, complete genome
  13 |   1.224126 | gi|22855189|ref|NC_004168.1| Yellowtail ascites virus segment A, complete sequence
  14 |   1.227563 | gi|927594140|ref|NC_027867.1| Mollivirus sibericum isolate P1084-T, complete genome
  15 |   1.227617 | gi|21427658|ref|NC_004018.1| Gremmeniella abietina RNA virus MS1 RNA 1, complete genome
  16 |   1.227833 | gi|475994049|ref|NC_020902.1| Equine Pegivirus 1 isolate C0035, complete genome
  17 |   1.228948 | gi|33867953|ref|NC_005075.1| Helicobasidium mompa No.17 dsRNA virus small segment, complete sequence
  18 |   1.231738 | gi|98960854|ref|NC_008041.1| Redspotted grouper nervous necrosis virus RNA 2, complete sequence
  19 |   1.232406 | gi|113478394|ref|NC_008311.1| Norovirus GV, complete genome
  20 |   1.232628 | gi|33867950|ref|NC_005074.1| Helicobasidium mompa No.17 dsRNA virus large segment, complete sequence

### Using db_with_meta.txt
- The normal db file with the meta sequence added on purpose as @meta_sequence

Top 20 most similar sequences:
----------------------------------------
Rank |        NRC | Reference Name
----------------------------------------
   1 |   0.170362 | NC_001348.1 Human herpesvirus 3, complete genome
   2 |   0.174182 | meta_sequence
   3 |   0.211057 | Super ISS Si1240
   4 |   0.213665 | NC_005831.2 Human Coronavirus NL63, complete genome
   5 |   0.288031 | OR353425.1 Octopus vulgaris mitochondrion, complete genome
   6 |   0.310735 | Super MUL 720
   7 |   0.557389 | gi|49169782|ref|NC_005831.2| Human Coronavirus NL63, complete genome
   8 |   1.204299 | gi|9629255|ref|NC_001793.1| Oat blue dwarf virus complete genome
   9 |   1.208147 | gi|971748077|ref|NC_028836.1| Pseudomonas phage DL62, complete genome
  10 |   1.209897 | gi|14141972|ref|NC_002786.1| Maize rayado fino virus isolate Costa Rican, complete genome
  11 |   1.214257 | gi|84662653|ref|NC_007710.1| Xanthomonas oryzae phage OP2 DNA, complete genome
  12 |   1.223399 | gi|472339725|ref|NC_020840.1| Tetraselmis viridis virus S20 genomic sequence
  13 |   1.223987 | gi|971752197|ref|NC_028885.1| Pseudomonas phage DL64, complete genome
  14 |   1.224126 | gi|22855189|ref|NC_004168.1| Yellowtail ascites virus segment A, complete sequence
  15 |   1.227563 | gi|927594140|ref|NC_027867.1| Mollivirus sibericum isolate P1084-T, complete genome
  16 |   1.227617 | gi|21427658|ref|NC_004018.1| Gremmeniella abietina RNA virus MS1 RNA 1, complete genome
  17 |   1.227833 | gi|475994049|ref|NC_020902.1| Equine Pegivirus 1 isolate C0035, complete genome
  18 |   1.228948 | gi|33867953|ref|NC_005075.1| Helicobasidium mompa No.17 dsRNA virus small segment, complete sequence
  19 |   1.231738 | gi|98960854|ref|NC_008041.1| Redspotted grouper nervous necrosis virus RNA 2, complete sequence
  20 |   1.232406 | gi|113478394|ref|NC_008311.1| Norovirus GV, complete genome

## FCMModel from project 1

- Note: This newer integration should also allow to save the model like before.

### Using db.txt

Top 20 most similar sequences:
----------------------------------------
Rank |        NRC | Reference Name
----------------------------------------
   1 |   0.220115 | NC_001348.1 Human herpesvirus 3, complete genome
   2 |   0.234870 | Super ISS Si1240
   3 |   0.259817 | NC_005831.2 Human Coronavirus NL63, complete genome
   4 |   0.304065 | Super MUL 720
   5 |   0.328846 | OR353425.1 Octopus vulgaris mitochondrion, complete genome
   6 |   0.436487 | gi|49169782|ref|NC_005831.2| Human Coronavirus NL63, complete genome
   7 |   0.871507 | gi|363539767|ref|NC_016072.1| Megavirus chiliensis, complete genome
   8 |   0.872403 | gi|12084983|ref|NC_002642.1| Yaba-like disease virus, complete genome
   9 |   0.884590 | gi|66396483|ref|NC_007067.1| Ageratum yellow vein China virus-associated DNA beta, complete genome
  10 |   0.886552 | gi|169822550|ref|NC_010437.1| Bat coronavirus 1A, complete genome
  11 |   0.889493 | gi|1003725904|ref|NC_029631.1| Lake Sarah-associated circular virus-44 isolate LSaCV-44-LSCO-2013, complete sequence
  12 |   0.889798 | gi|971745655|ref|NC_028814.1| BtRf-AlphaCoV/HuB2013, complete genome
  13 |   0.890019 | gi|1013949521|ref|NC_029798.1| Iris yellow spot virus segment M, complete sequence
  14 |   0.890714 | gi|163869641|ref|NC_004037.2| Ovine adenovirus 7, complete genome
  15 |   0.891059 | gi|897364658|ref|NC_027575.1| Lutzomyia reovirus 1 isolate piaui segment unknown 1 hypothetical protein gene, complete cds
  16 |   0.891436 | gi|98960844|ref|NC_008037.1| Prune dwarf virus segment RNA2, complete sequence
  17 |   0.891656 | gi|985485914|ref|NC_029052.1| Goose dicistrovirus isolate UW1, complete genome
  18 |   0.891931 | gi|146411809|ref|NC_009450.1| Honeysuckle yellow vein mosaic beta-[Japan:Miyizaki:2001] DNA, complete genome
  19 |   0.892033 | gi|20889306|ref|NC_003887.1| Cabbage leaf curl virus nuclear shuttle movement protein (BV1) and movement/pathogenicity protein (BC1) genes, complete cds
  20 |   0.892390 | gi|1008861384|ref|NC_005030.2| Tobacco leaf curl Yunnan virus satellite DNA beta clone 17, complete sequence

### Using db_with_meta.txt
- The normal db file with the meta sequence added on purpose as @meta_sequence

Top 20 most similar sequences:
----------------------------------------
Rank |        NRC | Reference Name
----------------------------------------
   1 |   0.220115 | NC_001348.1 Human herpesvirus 3, complete genome
   2 |   0.226614 | meta_sequence
   3 |   0.234870 | Super ISS Si1240
   4 |   0.259817 | NC_005831.2 Human Coronavirus NL63, complete genome
   5 |   0.304065 | Super MUL 720
   6 |   0.328846 | OR353425.1 Octopus vulgaris mitochondrion, complete genome
   7 |   0.436487 | gi|49169782|ref|NC_005831.2| Human Coronavirus NL63, complete genome
   8 |   0.871507 | gi|363539767|ref|NC_016072.1| Megavirus chiliensis, complete genome
   9 |   0.872403 | gi|12084983|ref|NC_002642.1| Yaba-like disease virus, complete genome
  10 |   0.884590 | gi|66396483|ref|NC_007067.1| Ageratum yellow vein China virus-associated DNA beta, complete genome
  11 |   0.886552 | gi|169822550|ref|NC_010437.1| Bat coronavirus 1A, complete genome
  12 |   0.889493 | gi|1003725904|ref|NC_029631.1| Lake Sarah-associated circular virus-44 isolate LSaCV-44-LSCO-2013, complete sequence
  13 |   0.889798 | gi|971745655|ref|NC_028814.1| BtRf-AlphaCoV/HuB2013, complete genome
  14 |   0.890019 | gi|1013949521|ref|NC_029798.1| Iris yellow spot virus segment M, complete sequence
  15 |   0.890714 | gi|163869641|ref|NC_004037.2| Ovine adenovirus 7, complete genome
  16 |   0.891059 | gi|897364658|ref|NC_027575.1| Lutzomyia reovirus 1 isolate piaui segment unknown 1 hypothetical protein gene, complete cds
  17 |   0.891436 | gi|98960844|ref|NC_008037.1| Prune dwarf virus segment RNA2, complete sequence
  18 |   0.891656 | gi|985485914|ref|NC_029052.1| Goose dicistrovirus isolate UW1, complete genome
  19 |   0.891931 | gi|146411809|ref|NC_009450.1| Honeysuckle yellow vein mosaic beta-[Japan:Miyizaki:2001] DNA, complete genome
  20 |   0.892033 | gi|20889306|ref|NC_003887.1| Cabbage leaf curl virus nuclear shuttle movement protein (BV1) and movement/pathogenicity protein (BC1) genes, complete cds


## What NRC Values Mean
Lower NRC values indicate that a sequence can be compressed more efficiently using the trained model, suggesting greater similarity between the reference sequence and the training sequence. In theory, values range from:

0: Perfect compression (impossible in practice)
1: No compression benefit compared to raw encoding
>1: Worse than raw encoding (model is actually harmful)

## Interpreting the Differences
What we're seeing is:

- For top matches: The simpler model shows lower NRC values (suggesting stronger compression)
- For distant matches: The FCMModel shows lower NRC values (staying below 1.0)

A good model isn't just about producing the lowest absolute NRC values, but about:

- Correctly ranking sequences by similarity (both models got similar ranking orders)
- Providing discrimination between more and less similar sequences
- Producing theoretically sound compression estimates

## Possible Conclusions
The FCMModel implementation might actually be more theoretically appropriate because it maintains NRC values below 1.0 even for distant matches (as it uses more sophisticated context handling).

The somewhat consistent ranking between both models suggests they are capturing the same underlying similarities.