#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <unordered_map>
#include <utility>
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
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " <folder> <query_word>\n";
        return 1;
    }

    fs::path root = argv[1];
    std::string query = normalize_word(argv[2]);

    if (!fs::exists(root)) {
        std::cerr << "Error: path does not exist: " << root << "\n";
        return 1;
    }

    // word -> (file -> count)
    std::unordered_map<std::string, std::unordered_map<std::string, int>> index;

    // Build index
    for (const auto& entry : fs::recursive_directory_iterator(root)) {
        if (!entry.is_regular_file()) continue;
        if (entry.path().filename() == ".DS_Store") continue;

        std::ifstream file(entry.path());
        if (!file.is_open()) continue;

        const std::string filepath = entry.path().string();

        std::string token;
        char ch;
        while (file.get(ch)) {
            unsigned char uc = (unsigned char)ch;
            if (is_word_char(uc)) {
                token.push_back(ch);
            } else {
                if (!token.empty()) {
                    std::string w = normalize_word(token);
                    index[w][filepath]++;  // add count
                    token.clear();
                }
            }
        }
        if (!token.empty()) {
            std::string w = normalize_word(token);
            index[w][filepath]++;
        }
    }

    // Search
    auto it = index.find(query);
    if (it == index.end()) {
        std::cout << "No results for: " << query << "\n";
        return 0;
    }

    std::vector<std::pair<std::string, int>> results;
    results.reserve(it->second.size());
    for (const auto& [file, count] : it->second) {
        results.push_back({file, count});
    }

    std::sort(results.begin(), results.end(),
              [](const auto& a, const auto& b) {
                  if (a.second != b.second) return a.second > b.second;
                  return a.first < b.first;
              });

    std::cout << "Results for: " << query << "\n";
    int shown = 0;
    for (const auto& [file, count] : results) {
        std::cout << count << "  " << file << "\n";
        if (++shown >= 20) break;
    }

    return 0;
}
