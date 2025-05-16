#include "../../include/utils/CLIParser.h"
#include <algorithm>

using namespace std;

CLIParser::CLIParser(int argc, char* argv[]) {
    for (int i = 1; i < argc; ++i) {
        tokens.emplace_back(argv[i]);
    }
}

bool CLIParser::flagExists(const string& flag) const {
    return find(tokens.begin(), tokens.end(), flag) != tokens.end();
}

string CLIParser::getOption(const string& option, const string& defaultVal) const {
    auto it = find(tokens.begin(), tokens.end(), option);
    if (it != tokens.end() && ++it != tokens.end()) {
        return *it;
    }
    return defaultVal;
}

vector<string> CLIParser::getArgs() const {
    vector<string> args;
    for (const auto& t : tokens) {
        if (t.size() > 0 && t[0] != '-') args.push_back(t);
    }
    return args;
}
