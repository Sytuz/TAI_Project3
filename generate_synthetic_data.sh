#!/bin/bash

# Default parameters
SEQ_COUNT=100
RANDOM_COUNT=80
META_COUNT=10
MUTATED_COUNT=10
MIN_LENGTH=100
MAX_LENGTH=5000
MUT_RATE_MIN=0.05
MUT_RATE_MAX=0.25
DEL_RATE_MAX=0.05
INS_RATE_MAX=0.05
OUTPUT_DIR="samples/generated"
LINE_WIDTH=72
GTO_PATH="../../gto/bin" # Path to GTO tools, due to the cd command
GTO_REPO_FILES="gto/bin" # Path to GTO repository files

# Display usage information
usage() {
    echo "Usage: $0 [OPTIONS]"
    echo "Generate synthetic DNA sequences for testing FCM models"
    echo ""
    echo "Options:"
    echo "  --seq-count NUMBER     Total number of sequences to generate (default: $SEQ_COUNT)"
    echo "  --random-count NUMBER  Number of random sequences (default: $RANDOM_COUNT)"
    echo "  --meta-count NUMBER    Number of reference sequences (default: $META_COUNT)"
    echo "  --mutated-count NUMBER Number of mutated sequences (default: $MUTATED_COUNT)"
    echo "  --min-length NUMBER    Minimum sequence length (default: $MIN_LENGTH)"
    echo "  --max-length NUMBER    Maximum sequence length (default: $MAX_LENGTH)"
    echo "  --mut-rate-min NUMBER  Minimum mutation rate (default: $MUT_RATE_MIN)"
    echo "  --mut-rate-max NUMBER  Maximum mutation rate (default: $MUT_RATE_MAX)"
    echo "  --del-rate-max NUMBER  Maximum deletion rate (default: $DEL_RATE_MAX)"
    echo "  --ins-rate-max NUMBER  Maximum insertion rate (default: $INS_RATE_MAX)"
    echo "  --line-width NUMBER    Characters per line (default: $LINE_WIDTH)"
    echo "  --output-dir PATH      Output directory (default: $OUTPUT_DIR)"
    echo "  --help                 Display this help message and exit"
    echo ""
    echo "Original GTO repository: https://github.com/bioinformatics-ua/gto.git"
    exit 1
}

# Parse command line arguments
while [ "$1" != "" ]; do
    case $1 in
        --seq-count )        shift; SEQ_COUNT=$1 ;;
        --random-count )     shift; RANDOM_COUNT=$1 ;;
        --meta-count )       shift; META_COUNT=$1 ;;
        --mutated-count )    shift; MUTATED_COUNT=$1 ;;
        --min-length )       shift; MIN_LENGTH=$1 ;;
        --max-length )       shift; MAX_LENGTH=$1 ;;
        --mut-rate-min )     shift; MUT_RATE_MIN=$1 ;;
        --mut-rate-max )     shift; MUT_RATE_MAX=$1 ;;
        --del-rate-max )     shift; DEL_RATE_MAX=$1 ;;
        --ins-rate-max )     shift; INS_RATE_MAX=$1 ;;
        --line-width )       shift; LINE_WIDTH=$1 ;;
        --output-dir )       shift; OUTPUT_DIR=$1 ;;
        --help )             usage ;;
        * )                  echo "Unknown parameter: $1"; usage ;;
    esac
    shift
done

# Validate parameters
if [ $RANDOM_COUNT -lt 0 ] || [ $META_COUNT -lt 0 ] || [ $MUTATED_COUNT -lt 0 ]; then
    echo "Error: Counts cannot be negative"
    exit 1
fi

if [ $(($RANDOM_COUNT + $META_COUNT + $MUTATED_COUNT)) -ne $SEQ_COUNT ]; then
    echo "Error: The sum of random, meta, and mutated counts must equal total sequence count"
    exit 1
fi

if [ $MIN_LENGTH -gt $MAX_LENGTH ]; then
    echo "Error: Minimum length cannot be greater than maximum length"
    exit 1
fi

# Check if GTO is available
if [ ! -f "$GTO_REPO_FILES/gto_genomic_gen_random_dna" ]; then
    echo "Error: GTO tools not found at $GTO_REPO_FILES"
    echo "Original GTO repository: https://github.com/bioinformatics-ua/gto.git"
    exit 1
fi

# Create output directory if it doesn't exist
if [ ! -d "$OUTPUT_DIR" ]; then
    mkdir -p "$OUTPUT_DIR"
fi
cd "$OUTPUT_DIR" || exit 1

# Clear any existing output files
> db_synthetic.txt
> meta_synthetic.txt

echo "Generating $SEQ_COUNT sequences (${RANDOM_COUNT} random, ${META_COUNT} reference, ${MUTATED_COUNT} mutated)..."

echo "Generating $RANDOM_COUNT random sequences..."
# Generate random sequences
for i in $(seq 1 $RANDOM_COUNT); do
    # Add sequence header with just the number for random sequences
    echo "@sequence_$i" >> db_synthetic.txt
    
    # Generate a random length between MIN_LENGTH and MAX_LENGTH
    range=$(($MAX_LENGTH - $MIN_LENGTH + 1))
    length=$(($RANDOM % $range + $MIN_LENGTH))
    
    # Generate a random DNA sequence with unique seed
    $GTO_PATH/gto_genomic_gen_random_dna -s $i -n $length | 
    head -n $length | 
    tr -d '\n' | 
    fold -w $LINE_WIDTH >> db_synthetic.txt
    # No longer adding extra newlines with sed
    
    # Add a single newline after each sequence
    echo "" >> db_synthetic.txt
done

echo "Generating $META_COUNT reference sequences (META)..."
# Generate reference sequences
meta_start=$(($RANDOM_COUNT + 1))
meta_end=$(($RANDOM_COUNT + $META_COUNT))
for i in $(seq $meta_start $meta_end); do
    # Calculate meta sequence number (1-based)
    meta_num=$((i - $RANDOM_COUNT))
    
    # Add sequence header to database file with _meta suffix
    echo "@sequence_${i}_meta" >> db_synthetic.txt
    
    # Generate a random length
    range=$(($MAX_LENGTH - $MIN_LENGTH + 1))
    length=$(($RANDOM % $range + $MIN_LENGTH))
    
    # Generate reference sequence to temporary file
    $GTO_PATH/gto_genomic_gen_random_dna -s $i -n $length |
    head -n $length |
    tr -d '\n' |
    fold -w $LINE_WIDTH > meta_seq_$i.txt
    
    # Add the sequence to database file
    cat meta_seq_$i.txt >> db_synthetic.txt
    
    # Add header and sequence to meta file with _meta suffix
    echo "@sequence_${i}_meta" >> meta_synthetic.txt
    cat meta_seq_$i.txt >> meta_synthetic.txt
    
    # Add a single newline after each sequence in both files
    echo "" >> db_synthetic.txt
    echo "" >> meta_synthetic.txt
done

echo "Generating $MUTATED_COUNT mutated sequences from reference sequences..."
# Generate mutated sequences
mutated_start=$(($RANDOM_COUNT + $META_COUNT + 1))
mutated_end=$SEQ_COUNT
for i in $(seq $mutated_start $mutated_end); do
    # Calculate which reference sequence to mutate (cycling through references)
    meta_offset=$(( (i - $mutated_start) % $META_COUNT ))
    meta_index=$(($meta_start + $meta_offset))
    
    # Add sequence header with sequence_<i>_mut_<meta_index> format
    echo "@sequence_${i}_mut_${meta_index}" >> db_synthetic.txt
    
    # Seed random number generator differently for each iteration
    RANDOM=$i
    
    # Calculate random mutation rates using more variation
    # Convert min/max values to integers (multiply by 100)
    mut_min_int=$(echo "$MUT_RATE_MIN * 100" | bc | cut -d. -f1)
    mut_max_int=$(echo "$MUT_RATE_MAX * 100" | bc | cut -d. -f1)
    del_max_int=$(echo "$DEL_RATE_MAX * 100" | bc | cut -d. -f1)
    ins_max_int=$(echo "$INS_RATE_MAX * 100" | bc | cut -d. -f1)
    
    # Generate random integers and convert back to decimal
    mut_int=$(($RANDOM % ($mut_max_int - $mut_min_int + 1) + $mut_min_int))
    del_int=$(($RANDOM % ($del_max_int + 1)))
    ins_int=$(($RANDOM % ($ins_max_int + 1)))
    
    # Convert to decimal with 2 decimal places
    mut=$(echo "scale=2; $mut_int / 100" | bc)
    del=$(echo "scale=2; $del_int / 100" | bc)
    ins=$(echo "scale=2; $ins_int / 100" | bc)
    
    echo "  Mutating seq. $meta_index -> $i (mut=$mut, del=$del, ins=$ins)"
    
    # Apply mutation and format correctly
    $GTO_PATH/gto_genomic_dna_mutate -m $mut -d $del -i $ins < meta_seq_$meta_index.txt |
    tr -d '\n' |
    fold -w $LINE_WIDTH >> db_synthetic.txt
    # No longer adding extra newlines with sed
    
    # Add a single newline after each sequence
    echo "" >> db_synthetic.txt
done

# Clean up temporary files
rm -f meta_seq_*.txt

echo "Generation complete!"
echo "Database with $SEQ_COUNT sequences created in '$OUTPUT_DIR/db_synthetic.txt'"
echo "Reference sequences ($META_COUNT) saved in '$OUTPUT_DIR/meta_synthetic.txt'"