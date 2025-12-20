#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

namespace fs = std::filesystem;

static bool is_word_char(unsigned char c) {
    return std::isalnum(c) || c == '_';
}

static std::string normalize_word(const std::string& w) {
    std::string out;
    out.reserve(w.size());
    for (unsigned char c : w) out.push_back((char)std::tolower(c));
    return out;
}

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <folder>\n";
        return 1;
    }

    fs::path root = argv[1];
    if (!fs::exists(root)) {
        std::cerr << "Path does not exist\n";
        return 1;
    }

    for (const auto& entry : fs::recursive_directory_iterator(root)) {
        if (!entry.is_regular_file()) continue;
        if (entry.path().filename() == ".DS_Store") continue;

        std::ifstream file(entry.path());
        if (!file.is_open()) continue;

        std::unordered_map<std::string, int> counts;

        std::string token;
        char ch;
        while (file.get(ch)) {
            unsigned char uc = (unsigned char)ch;
            if (is_word_char(uc)) {
                token.push_back(ch);
            } else {
                if (!token.empty()) {
                    std::string w = normalize_word(token);
                    counts[w]++;
                    token.clear();
                }
            }
        }
        if (!token.empty()) { // last token at EOF
            std::string w = normalize_word(token);
            counts[w]++;
        }

        // Sort top words by frequency
        std::vector<std::pair<std::string, int>> items(counts.begin(), counts.end());
        std::sort(items.begin(), items.end(),
                  [](const auto& a, const auto& b) {
                      if (a.second != b.second) return a.second > b.second;
                      return a.first < b.first;
                  });

        std::cout << "==== " << entry.path() << " ====\n";
        int shown = 0;
        for (const auto& [word, cnt] : items) {
            if (word.empty()) continue;
            std::cout << word << ": " << cnt << "\n";
            if (++shown >= 10) break;
        }
    }

    return 0;
}
