#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <filesystem>
#include <cstdlib>

using namespace std;

// Check if a command exists
bool command_exists(const string& command) {
    string check_cmd = "which " + command + " > /dev/null 2>&1";
    return system(check_cmd.c_str()) == 0;
}

// Install dependencies
bool install_dependencies() {
    cout << "Checking dependencies..." << endl;

    // Check ffmpeg
    if (!command_exists("ffmpeg")) {
        cout << "Installing ffmpeg..." << endl;
        int ret = system("sudo apt update && sudo apt install -y ffmpeg");
        if (ret != 0) {
            cerr << "Failed to install ffmpeg. Please run: sudo apt install ffmpeg" << endl;
            return false;
        }
    }

    // Check yt-dlp
    if (!command_exists("yt-dlp")) {
        cout << "Installing yt-dlp..." << endl;
        int ret = system("pip3 install yt-dlp");
        if (ret != 0) {
            cerr << "Failed to install yt-dlp. Please run: pip install yt-dlp" << endl;
            return false;
        }
    }

    cout << "All dependencies are installed!" << endl;
    return true;
}

// Configuration
const string SONG_LIST_FILE  = "songs.txt";
const string OUTPUT_DIR      = "../data/full_tracks/youtube";
const string AUDIO_FORMAT    = "wav";
const string FFMPEG_LOCATION = "/usr/bin/ffmpeg";

// Ensure output directory exists
void ensure_output_dir() {
    filesystem::create_directories(OUTPUT_DIR);
}

// Read song titles from file
vector<string> load_song_list(const string& path) {
    vector<string> songs;
    ifstream infile(path);
    if (!infile) {
        cerr << "Error: Unable to open song list file: " << path << endl;
        return songs;
    }
    string line;
    while (getline(infile, line)) {
        if (!line.empty()) songs.push_back(line);
    }
    return songs;
}

// Download a single song via yt-dlp
void download_song(const string& title) {
    // Let yt-dlp substitute %(ext)s for “wav” after conversion
    string output_template = OUTPUT_DIR + "/%(title)s.%(ext)s";

    string cmd =
        "yt-dlp "
        "-f bestaudio "
        "-x "
        "--audio-format "   + AUDIO_FORMAT + " "
        "--audio-quality 0 "
        "--no-playlist "
        "--ffmpeg-location \"" + FFMPEG_LOCATION + "\" "
        "--embed-metadata "
        "--add-metadata "
        "-o \"" + output_template + "\" "
        "\"ytsearch1:" + title + "\"";

    cout << "Downloading (WAV): " << title << endl;
    int ret = system(cmd.c_str());
    if (ret != 0) {
        cerr << "  → Failed: '" << title
                << "' (exit code " << ret << ")\n";
    }
}

int main() {
    // Install dependencies if needed
    if (!install_dependencies()) {
        return 1;
    }

    // 1) Verify ffmpeg is present
    if (system((FFMPEG_LOCATION + " -version > /dev/null 2>&1").c_str()) != 0) {
        cerr << "Error: ffmpeg not found at " << FFMPEG_LOCATION
                << ". Please install ffmpeg or adjust FFMPEG_LOCATION.\n";
        return 1;
    }

    ensure_output_dir();
    auto songs = load_song_list(SONG_LIST_FILE);
    if (songs.empty()) {
        cerr << "No songs to download. Please check " << SONG_LIST_FILE << endl;
        return 1;
    }
    for (const auto& song : songs) {
        download_song(song);
    }
    return 0;
}
