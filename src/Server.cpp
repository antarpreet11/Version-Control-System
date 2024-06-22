#include <iostream>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>
#include <zlib.h>

int main(int argc, char *argv[])
{
    // Flush after every std::cout / std::cerr
    std::cout << std::unitbuf;
    std::cerr << std::unitbuf;

    if (argc < 2) {
        std::cerr << "No command provided.\n";
        return EXIT_FAILURE;
    }
    
    std::string command = argv[1];
    
    if (command == "init") {
        try {
            std::filesystem::create_directory(".git");
            std::filesystem::create_directory(".git/objects");
            std::filesystem::create_directory(".git/refs");
    
            std::ofstream headFile(".git/HEAD");
            if (headFile.is_open()) {
                headFile << "ref: refs/heads/main\n";
                headFile.close();
            } else {
                std::cerr << "Failed to create .git/HEAD file.\n";
                return EXIT_FAILURE;
            }
    
            std::cout << "Initialized git directory\n";
        } catch (const std::filesystem::filesystem_error& e) {
            std::cerr << e.what() << '\n';
            return EXIT_FAILURE;
        }
    } else if (command == "cat-file") {
        if (argc < 3) {
            std::cerr << "No blob SHA provided.\n";
            return EXIT_FAILURE;
        }

        const std::string_view blob_sha(argv[argc - 1]);
        const auto object_path = std::filesystem::path(".git") / ("objects/") / blob_sha.substr(0, 2) / blob_sha.substr(2);

        std::ifstream objectFile(object_path, std::ios::binary);
        if (!objectFile.is_open()) {
            std::cerr << "Unable to open the blob object file.\n";
            return EXIT_FAILURE;
        } 

        std::string blobData = std::string(std::istreambuf_iterator<char>(objectFile), std::istreambuf_iterator<char>());

        auto buf = std::string();
        buf.resize(blobData.size());

        while(true) {
            uLongf len = buf.size();

            if (auto res = uncompress((uint8_t*)buf.data(), &len, (const uint8_t*)blobData.data(), blobData.size()); res == Z_BUF_ERROR) {
                buf.resize(buf.size() * 2);
            } else if (res != Z_OK) {
                std::cerr << "Failed to uncompress Zlib. (code: " << res << ")\n";
                return EXIT_FAILURE;
            } else {
                buf.resize(len);
                break;
            }
        }

        std::cout << std::string_view(buf).substr(buf.find('\0') + 1); // Skip the header

    } else {
        std::cerr << "Unknown command " << command << '\n';
        return EXIT_FAILURE;
    }
    
    return EXIT_SUCCESS;
}
