#include <iostream>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>
#include <zlib.h>

bool decompressData(std::vector<uint8_t>& compressedData, std::vector<uint8_t>& decompressedData) {
    z_stream stream;
    stream.avail_in = compressedData.size();
    stream.next_in = const_cast<Bytef*>(compressedData.data());
    stream.avail_out = 0;

    if (inflateInit(&stream) != Z_OK) {
        return false;
    }

    int ret;
    char outbuffer[32768]; // 32KB buffer for decompressed data
    do {
        stream.avail_out = sizeof(outbuffer);
        stream.next_out = reinterpret_cast<Bytef*>(outbuffer);
        ret = inflate(&stream, 0);

        if (decompressedData.size() < stream.total_out) {
            decompressedData.insert(decompressedData.end(), outbuffer, outbuffer + sizeof(outbuffer) - stream.avail_out);
        }
    } while (ret == Z_OK);
    inflateEnd(&stream);
    return ret == Z_STREAM_END;
}

int main(int argc, char *argv[])
{
    // Flush after every std::cout / std::cerr
    std::cout << std::unitbuf;
    std::cerr << std::unitbuf;

    // You can use print statements as follows for debugging, they'll be visible when running tests.
    std::cout << "Logs from your program will appear here!\n";

    // Uncomment this block to pass the first stage

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

        std::string blob_sha = argv[argc - 1];
        std::string object_path = ".git/objects/" + blob_sha.substr(0, 2) + "/" + blob_sha.substr(2);

        std::ifstream objectFile(object_path, std::ios::binary);
        if (!objectFile.is_open()) {
            std::cerr << "Unable to open the blob object file.\n";
            return EXIT_FAILURE;
        } 

        // Get the size of the file
        objectFile.seekg(0, std::ios::end);
        size_t size = objectFile.tellg();
        objectFile.seekg(0, std::ios::beg);

        // Read the file into a buffer
        std::vector<uint8_t> buffer(size);
        objectFile.read(reinterpret_cast<char*>(buffer.data()), size);

        // Close the file
        objectFile.close();

        // Decompress the data
        std::vector<uint8_t> decompressedData;
        if (!decompressData(buffer, decompressedData)) {
            std::cerr << "Failed to decompress data.\n";
            return EXIT_FAILURE;
        }

        // Read the decompressed data
        std::string blobStr(decompressedData.begin(), decompressedData.end());
        size_t nullPos = blobStr.find('\0');
        std::cout << blobStr.substr(nullPos + 1); // Skip the header

    } else {
        std::cerr << "Unknown command " << command << '\n';
        return EXIT_FAILURE;
    }
    
    return EXIT_SUCCESS;
}
