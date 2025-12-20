#include "engine.h"
#include "thread_pool.h"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace fs = std::filesystem;

static bool is_word_char(unsigned char c) {
    return std::isalnum(c) || c == '_';
}

std::string normalize_word(const std::string& w) {
    std::string out;
    out.reserve(w.size());
    for (unsigned char c : w) out.push_back((char)std::tolower(c));
    return out;
}

static std::string lower_ext(const fs::path& p) {
    std::string e = p.extension().string();
    for (auto& ch : e) ch = (char)std::tolower((unsigned char)ch);
    return e;
}

static bool should_index_file(const fs::directory_entry& entry) {
    // Skip obvious junk
    auto name = entry.path().filename().string();
    if (name == ".DS_Store") return false;

    // Extension allow-list (edit freely later)
    static const std::unordered_set<std::string> kAllowed = {
        ".txt", ".md", ".log", ".csv", ".json",
        ".cpp", ".cc", ".cxx", ".c", ".h", ".hpp",
        ".py", ".java", ".js", ".ts",
        ".tex"
    };

    std::string ext = lower_ext(entry.path());
    if (kAllowed.find(ext) == kAllowed.end()) return false;

    // Size cap (avoid huge files that make indexing feel “stuck”)
    // 5 MB default
    constexpr std::uintmax_t kMaxBytes = 5ULL * 1024 * 1024;

    try {
        auto sz = entry.file_size();
        if (sz > kMaxBytes) return false;
    } catch (...) {
        // If we can't read size, skip (safer)
        return false;
    }

    return true;
}

long long build_index(const std::string& folder, int num_threads, Index& index_out) {
    index_out.clear();
    std::mutex index_mutex;

    auto t0 = std::chrono::steady_clock::now();
    ThreadPool pool((size_t)num_threads);

    for (const auto& entry : fs::recursive_directory_iterator(folder)) {
        if (!entry.is_regular_file()) continue;
        if (!should_index_file(entry)) continue;

        std::string filepath = entry.path().string();

        pool.enqueue([filepath, &index_out, &index_mutex]() {
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
            if (!token.empty()) local[normalize_word(token)]++;

            std::lock_guard<std::mutex> lock(index_mutex);
            for (const auto& [word, cnt] : local) {
                index_out[word][filepath] += cnt;
            }
        });
    }

    pool.shutdown();

    auto t1 = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
}

std::vector<std::pair<std::string, int>> search_word(const Index& index,
                                                     const std::string& query,
                                                     int top_k) {
    std::string q = normalize_word(query);

    auto it = index.find(q);
    if (it == index.end()) return {};

    std::vector<std::pair<std::string, int>> results;
    results.reserve(it->second.size());
    for (const auto& [file, count] : it->second) results.push_back({file, count});

    std::sort(results.begin(), results.end(),
              [](const auto& a, const auto& b) {
                  if (a.second != b.second) return a.second > b.second;
                  return a.first < b.first;
              });

    if ((int)results.size() > top_k) results.resize(top_k);
    return results;
}
