#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <mutex>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "thread_pool.h"

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

static void print_results(
    const std::unordered_map<std::string, std::unordered_map<std::string, int>>& index,
    const std::string& query
) {
    auto it = index.find(query);
    if (it == index.end()) {
        std::cout << "No results for: " << query << "\n";
        return;
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
}

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <folder>\n";
        return 1;
    }

    fs::path root = argv[1];
    if (!fs::exists(root)) {
        std::cerr << "Error: path does not exist: " << root << "\n";
        return 1;
    }

    std::unordered_map<std::string, std::unordered_map<std::string, int>> index;
    std::mutex index_mutex;

    ThreadPool pool(4);

    for (const auto& entry : fs::recursive_directory_iterator(root)) {
        if (!entry.is_regular_file()) continue;
        if (entry.path().filename() == ".DS_Store") continue;

        std::string filepath = entry.path().string();

        pool.enqueue([filepath, &index, &index_mutex]() {
            std::ifstream file(filepath);
            if (!file.is_open()) return;

            std::unordered_map<std::string, int> local;

            std::string token;
            char ch;
            while (file.get(ch)) {
                unsigned char uc = (unsigned char)ch;
                if (is_word_char(uc)) {
                    token.push_back(ch);
                } else {
                    if (!token.empty()) {
                        local[normalize_word(token)]++;
                        token.clear();
                    }
                }
            }
            if (!token.empty()) {
                local[normalize_word(token)]++;
            }

            std::lock_guard<std::mutex> lock(index_mutex);
            for (const auto& [word, cnt] : local) {
                index[word][filepath] += cnt;
            }
        });
    }

    pool.shutdown();

    std::cout << "Indexed with 4 threads. Type a single word to search. Type 'quit' to exit.\n";

    std::string input;
    while (true) {
        std::cout << "> ";
        if (!std::getline(std::cin, input)) break;

        input = normalize_word(input);
        if (input.empty()) continue;
        if (input == "quit" || input == "exit") break;

        print_results(index, input);
    }

    return 0;
}
