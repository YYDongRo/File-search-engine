#pragma once

#include <string>
#include <unordered_map>
#include <vector>

using Index = std::unordered_map<std::string, std::unordered_map<std::string, int>>;

// Build index for folder using num_threads. Returns time in ms.
long long build_index(const std::string& folder, int num_threads, Index& index_out);

// Search a single word in the index. Returns top results (file, count).
std::vector<std::pair<std::string, int>> search_word(const Index& index,
                                                     const std::string& query,
                                                     int top_k = 20);

// Utility: normalize word (lowercase)
std::string normalize_word(const std::string& w);
