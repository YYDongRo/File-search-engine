

#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;

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
        if (entry.is_regular_file()) {
            std::cout << entry.path() << "\n";
        }
    }

    return 0;
}
