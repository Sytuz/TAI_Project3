#ifndef CLIPARSER_H
#define CLIPARSER_H

#include <string>
#include <vector>

using namespace std;

/**
 * @brief CLIParser parses command-line arguments for the applications.
 * Supports flags for compressor, noise, feature method, etc.
 */
class CLIParser {
public:
    CLIParser(int argc, char* argv[]);
    ~CLIParser() = default;

    bool flagExists(const string& flag) const;
    string getOption(const string& option, const string& defaultVal = "") const;
    vector<string> getArgs() const;  // Non-option arguments

private:
    vector<string> tokens;
};

#endif // CLIPARSER_H
