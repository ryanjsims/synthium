#include <iostream>
#include <fstream>
#include <string>
#include <spdlog/spdlog.h>

#include "crc64.h"

namespace logger = spdlog;

int main(int argc, char* argv[]) {
    std::string namelist = "{NAMELIST}";
    std::cout << std::hex << crc64(namelist) << std::endl;

    std::ifstream test_hashes("hashes.txt");
    std::string line, hashstr, name;
    std::getline(test_hashes, line);
    while(test_hashes.good()) {
        auto colon = std::find(line.begin(), line.end(), ':');
        hashstr = std::string(line.begin(), colon);
        name = std::string(colon + 1, line.end());
        uint64_t hash = std::strtoull(hashstr.c_str(), nullptr, 16);
        logger::info("0x{:016x} == crc64(\"{}\"): {}", hash, name, hash == crc64(name));
        std::getline(test_hashes, line);
    }
    return 0;
}