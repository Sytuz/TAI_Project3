#!/bin/bash

# Check if build exists
if [ ! -d "build" ]; then
    echo "❌ Build not found. Running setup script first..."
    ./scripts/setup.sh
    if [ $? -ne 0 ]; then
        echo "❌ Setup failed. Please fix the errors and try again."
        exit 1
    fi
fi

# Function to print usage
print_usage() {
    echo "Usage: ./scripts/run.sh [APP] [APP_OPTIONS]"
    echo ""
    echo "Available applications:"
    echo "  music_id         Full pipeline: extract features, compute NCD, build tree"
    echo "  extract_features Extract frequency features from WAV files"
    echo "  compute_ncd      Compute NCD matrix between feature files"
    echo "  build_tree       Build a similarity tree from NCD matrix"
    echo ""
    echo "Examples:"
    echo "  ./scripts/run.sh music_id --help"
    echo "  ./scripts/run.sh music_id --method fft --compressor gzip data/samples results/run1"
    echo "  ./scripts/run.sh extract_features --method maxfreq data/samples output/features"
    echo ""
    echo "For app-specific options, run: ./scripts/run.sh [APP] --help"
}

# Check for app argument
if [ $# -eq 0 ] || [ "$1" == "--help" ]; then
    print_usage
    exit 0
fi

APP="$1"
shift  # Remove the app name from arguments

# Validate app name
case "$APP" in
    music_id|extract_features|compute_ncd|build_tree)
        # Valid app name
        ;;
    *)
        echo "❌ Unknown application: $APP"
        print_usage
        exit 1
        ;;
esac

# Ensure the app exists
if [ ! -f "./apps/$APP" ]; then
    echo "❌ Application not found: ./apps/$APP"
    echo "Make sure the build completed successfully."
    exit 1
fi

# Execute the requested application with all remaining arguments
echo "===== Running $APP ====="
echo "Command: ./apps/$APP $@"
./apps/$APP "$@"

exit $?