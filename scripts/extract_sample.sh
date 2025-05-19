#!/bin/bash

# Check if sox is installed
if ! command -v sox &> /dev/null; then
    echo "Error: sox is not installed. Please install it first."
    echo "On Debian/Ubuntu: sudo apt-get install sox"
    echo "On macOS with Homebrew: brew install sox"
    exit 1
fi

# Default values
input_path="data/full_tracks"
output_dir="data/samples"
start_time=0
segment_length=10
noise_type=""
noise_level=0.1 # Default noise level is 10%

# Display usage information
function show_help() {
    echo "Usage: $0 [OPTIONS]"
    echo "Options:"
    echo "  -i, --input <path>    Input directory or WAV file [default: data/full_tracks]"
    echo "  -o, --output <dir>    Output directory for samples [default: data/samples]"
    echo "  -l, --length <sec>    Length of segment in seconds [default: 10]"
    echo "  -s, --start <sec>     Start time in seconds [default: 0]"
    echo "  -n, --noise <type>    Noise type: white, pink, or brown"
    echo "  -m, --mix <level>     Noise level (0.0-1.0) [default: 0.1]"
    echo "  -h, --help            Show this help message"
    echo
    echo "The script extracts segments from audio files and optionally adds noise."
    echo "Examples:"
    echo "  $0 -i music/song.wav -l 15 -n pink -m 0.2 -o query_samples"
    echo "  $0 -i music_directory/ -s 30"
}

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        -i|--input)
            input_path="$2"
            shift 2
            ;;
        -o|--output)
            output_dir="$2"
            shift 2
            ;;
        -l|--length)
            segment_length="$2"
            shift 2
            ;;
        -s|--start)
            start_time="$2"
            shift 2
            ;;
        -n|--noise)
            noise_type="$2"
            if [[ ! "$noise_type" =~ ^(white|pink|brown)$ ]]; then
                echo "Error: Invalid noise type. Use white, pink, or brown."
                exit 1
            fi
            shift 2
            ;;
        -m|--mix)
            noise_level="$2"
            # Simple validation (assumes decimal point notation)
            if (( $(echo "$noise_level < 0.0 || $noise_level > 1.0" | bc -l) )); then
                echo "Error: Noise level must be between 0.0 and 1.0"
                exit 1
            fi
            shift 2
            ;;
        -h|--help)
            show_help
            exit 0
            ;;
        -*)
            echo "Error: Unknown option $1"
            show_help
            exit 1
            ;;
        *)
            echo "Error: Unexpected argument $1"
            show_help
            exit 1
            ;;
    esac
done

# Create output directory if it doesn't exist
mkdir -p "$output_dir"

# Function to save configuration info to a file
save_config_info() {
    local files_processed="$1"
    local config_file="${output_dir}/extraction_config.txt"
    
    echo "Saving configuration information to ${config_file}"
    
    {
        echo "Sample Extraction Configuration"
        echo "=============================="
        echo "Date: $(date '+%Y-%m-%d %H:%M:%S')"
        echo "Input path: ${input_path}"
        echo "Output directory: ${output_dir}"
        echo "Segment length: ${segment_length} seconds"
        echo "Start time: ${start_time} seconds"
        
        if [ -n "$noise_type" ]; then
            echo "Noise type: ${noise_type}"
            echo "Noise level: ${noise_level}"
        else
            echo "Noise: None"
        fi
        
        echo "Files processed: ${files_processed}"
        echo "Command: $0 $original_args"
    } > "$config_file"
}

# Function to process a single file
process_file() {
    local input_file="$1"
    local base_name=$(basename "$input_file" | sed 's/\.[^.]*$//')
    local output_file
    
    # Create output filename based on whether noise is being added
    if [ -n "$noise_type" ]; then
        output_file="${output_dir}/sample_${base_name}_${noise_type}_noise.wav"
    else
        output_file="${output_dir}/sample_${base_name}.wav"
    fi
    
    echo "Processing: $input_file"
    
    # Extract segment
    if [ -n "$noise_type" ]; then
        # Generate temporary files for the segment and noise
        temp_segment=$(mktemp --suffix=.wav)
        temp_noise=$(mktemp --suffix=.wav)
        
        # Extract segment
        sox "$input_file" "$temp_segment" trim "$start_time" "$segment_length"
        
        # Get audio properties for noise generation
        channels=$(soxi -c "$temp_segment")
        rate=$(soxi -r "$temp_segment")
        
        # Generate noise
        sox -n -r "$rate" -c "$channels" "$temp_noise" synth "$segment_length" \
            $noise_type vol "$noise_level"
        
        # Mix original segment with noise
        sox -m "$temp_segment" "$temp_noise" "$output_file"
        
        # Clean up temporary files
        rm "$temp_segment" "$temp_noise"
    else
        # Just extract the segment
        sox "$input_file" "$output_file" trim "$start_time" "$segment_length"
    fi
    
    echo "Created: $output_file"
}

# Store original command-line arguments for logging
original_args="$@"

# Check if input exists
if [ ! -e "$input_path" ]; then
    echo "Error: Input path does not exist: $input_path"
    exit 1
fi

# Process input (file or directory)
if [ -f "$input_path" ]; then
    # Process single file
    process_file "$input_path"
    # Save configuration for single files too - helpful for reproducibility
    save_config_info 1
elif [ -d "$input_path" ]; then
    # Process all WAV files in directory
    found_files=0
    for file in "$input_path"/*.wav; do
        if [ -f "$file" ]; then
            process_file "$file"
            ((found_files++))
        fi
    done
    
    # Save configuration information for directory processing
    save_config_info "$found_files"
    
    if [ $found_files -eq 0 ]; then
        echo "Warning: No WAV files found in $input_path"
    else
        echo "Processed $found_files files."
    fi
else
    echo "Error: Input path is neither a file nor a directory: $input_path"
    exit 1
fi

echo "Processing complete."