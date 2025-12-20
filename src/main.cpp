#include <filesystem>
#include <iostream>
#include <string>

#include "thread_pool.h"

namespace fs = std::filesystem;

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

    ThreadPool pool(4);

    for (const auto& entry : fs::recursive_directory_iterator(root)) {
        if (!entry.is_regular_file()) continue;
        if (entry.path().filename() == ".DS_Store") continue;

        std::string path = entry.path().string();
        pool.enqueue([path]() {
            std::cout << "Worker got: " << path << "\n";
        });
    }

    pool.shutdown();
    return 0;
}
