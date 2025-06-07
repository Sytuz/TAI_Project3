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
start_time="" # Empty means random selection (always enabled)
segment_length=10
noise_type=""
noise_level=0.1 # Default noise level is 10%
avoid_start_seconds=5 # Always avoid the first N seconds of the song
avoid_end_seconds=5 # Always avoid the last N seconds of the song
output_binary=false

# Display usage information
function show_help() {
    echo "Usage: $0 [OPTIONS]"
    echo "Options:"
    echo "  -i, --input <path>    Input directory or WAV file [default: data/full_tracks]"
    echo "  -o, --output <dir>    Output directory for samples [default: data/samples]"
    echo "  -l, --length <sec>    Length of segment in seconds [default: 10]"
    echo "  -s, --start <sec>     Start time in seconds [default: random]"
    echo "  -n, --noise <type>    Noise type: white, pink, or brown"
    echo "  -m, --mix <level>     Noise level (0.0-1.0) [default: 0.1]"
    echo "  -b, --binary          Output sample with .bin extension (raw float32 PCM)"
    echo "  -h, --help            Show this help message"
    echo
    echo "The script extracts segments from audio files and optionally adds noise."
    echo "By default, it selects a random segment avoiding the first and last 5 seconds of songs."
    echo "Examples:"
    echo "  $0 -i music/song.wav -l 15 -n pink -m 0.2 -o query_samples"
    echo "  $0 -i music_directory/ -s 30"
    echo "  $0 -b -i music/song.wav -o bin_samples"
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
        -b|--binary)
            output_binary=true
            shift
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

# Always use truly random seed based on current time and process ID for maximum randomness
RANDOM=$(( $(date +%s%N) % 32767 + $$ ))

# Function to get random start time for a file
get_random_start_time() {
    local input_file="$1"
    local duration
    
    # Get the duration of the audio file
    duration=$(soxi -D "$input_file" 2>/dev/null)
    if [ $? -ne 0 ] || [ -z "$duration" ]; then
        echo "Error: Could not determine duration of $input_file" >&2
        return 1
    fi
    
    # Convert to integer seconds (floor)
    duration_int=$(echo "$duration" | cut -d'.' -f1)
    
    # Calculate available range for random selection
    # Ensure we have enough space for the segment plus avoid zones
    min_required_duration=$((avoid_start_seconds + segment_length + avoid_end_seconds))
    
    if [ "$duration_int" -lt "$min_required_duration" ]; then
        echo "Warning: File $input_file is too short ($duration_int s) for random selection" >&2
        echo "Required: $min_required_duration s (avoid_start + segment + avoid_end)" >&2
        echo "Using start time: $avoid_start_seconds" >&2
        echo "$avoid_start_seconds"
        return 0
    fi
    
    # Calculate the range for random start time
    max_start_time=$((duration_int - segment_length - avoid_end_seconds))
    range=$((max_start_time - avoid_start_seconds))
    
    if [ "$range" -le 0 ]; then
        echo "Warning: No valid range for random selection in $input_file" >&2
        echo "Using start time: $avoid_start_seconds" >&2
        echo "$avoid_start_seconds"
        return 0
    fi
    
    # Generate random start time within the valid range
    random_offset=$((RANDOM % range))
    random_start=$((avoid_start_seconds + random_offset))
    
    echo "$random_start"
    return 0
}

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
        
        if [ -z "$start_time" ]; then
            echo "Start time: Random selection (avoiding first/last 5 seconds)"
        else
            echo "Start time: ${start_time} seconds"
        fi
        
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
    local actual_start_time="$start_time"
    
    # If start_time is not specified, calculate random start time
    if [ -z "$start_time" ]; then
        actual_start_time=$(get_random_start_time "$input_file")
        if [ $? -ne 0 ]; then
            echo "Error: Failed to calculate random start time for $input_file"
            return 1
        fi
        echo "Selected random start time: ${actual_start_time}s"
    else
        echo "Using specified start time: ${actual_start_time}s"
    fi
    
    # Create output filename based on whether noise is being added and if time is random
    if [ -n "$noise_type" ]; then
        output_file="${output_dir}/sample_${base_name}_${noise_type}_noise.wav"
    else
        output_file="${output_dir}/sample_${base_name}.wav"
    fi
    if [ "$output_binary" = true ]; then
        output_file="${output_file}.bin"
    else
        output_file="${output_file}.wav"
    fi
    
    echo "Processing: $input_file"
    
    # Extract segment
    if [ "$output_binary" = true ]; then
        # Output as raw float32 PCM (little-endian)
        sox "$input_file" -t f32 "$output_file" trim "$actual_start_time" "$segment_length"
    else
        if [ -n "$noise_type" ]; then
            # Generate temporary files for the segment and noise
            temp_segment=$(mktemp --suffix=.wav)
            temp_noise=$(mktemp --suffix=.wav)
            
            # Extract segment
            sox "$input_file" "$temp_segment" trim "$actual_start_time" "$segment_length"
            
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
            sox "$input_file" "$output_file" trim "$actual_start_time" "$segment_length"
        fi
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