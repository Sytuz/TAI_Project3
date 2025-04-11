#!/bin/bash

# create directory if it does not exist
if [ ! -d "samples/generated" ]; then
    mkdir -p samples/generated
fi
cd samples/generated

# clear any existing output files
> db_synthetic.txt
> meta_synthetic.txt

echo "Generating 100 sequences for database ..."

# create a temporary file to store sequence numbers for later selection
> seq_numbers.txt

# extract original sequences as base templates
cat ../meta.txt | head -500 > base_sequences.txt

# generate 100 sequences with varying mutation rates
for i in $(seq 1 100); do
    # calculate a random mutation rate between 0.05 and 0.25
    mut=$(echo "scale=2; (($RANDOM % 20) + 5) / 100" | bc)

    # random small values for deletion and insertion rates
    del=$(echo "scale=2; ($RANDOM % 5) / 100" | bc)
    ins=$(echo "scale=2; ($RANDOM % 5) / 100" | bc)

    echo "Generating sequence $i with mutation rate $mut"

    # create header for this sequence
    echo "@Sequence_$i" >> db_synthetic.txt

    # generate mutated sequence from base sequences
    cat base_sequences.txt | ../../gto/bin/gto_genomic_dna_mutate -m $mut -d $del -i $ins |
    head -100 >> db_synthetic.txt

    # add blank line after sequence
    echo "" >> db_synthetic.txt

    # add sequence number to the list for later selection
    echo $i >> seq_numbers.txt
done

echo "Selecting 20 sequences for meta file..."

# randomly select 20 sequence numbers
shuf -n 20 seq_numbers.txt > selected_seq_numbers.txt

# process each selected sequence
while read seq_num; do
    echo "Adding sequence $seq_num to meta file"

    # extract sequence header and add _meta suffix
    grep -A 1 "@Sequence_$seq_num" db_synthetic.txt |
    sed "s/@Sequence_$seq_num/@Sequence_${seq_num}_meta/" >> meta_synthetic.txt

    # add black line
    echo "" >> meta_synthetic.txt
done < selected_seq_numbers.txt

# clean up temporary files
rm seq_numbers.txt selected_seq_numbers.txt base_sequences.txt

echo "Done! Generated 100 database sequences and selected 20 for meta file."
