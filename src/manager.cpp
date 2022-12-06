#include "crc64.h"
#include "manager.h"
#include "version.h"

#include <algorithm>
#include <execution>
#include <mutex>
#include <fstream>

namespace logger = spdlog;
using namespace synthium;

Manager::Manager(std::vector<std::filesystem::path> paths) {
    std::mutex packs_mutex, namehashes_mutex;
    std::vector<std::unordered_map<uint64_t, uint32_t>> maps;
    std::for_each(std::execution::par, paths.begin(), paths.end(), [&](std::filesystem::path path){
        std::ifstream input(path, std::ios::binary | std::ios::ate);

        size_t length = input.tellg();
        logger::debug("Loading file {} with length: {}", path.filename().string(), length);
        input.seekg(0);
        std::unique_ptr<uint8_t[]> data;
        try {
            data = std::make_unique<uint8_t[]>(length);
        } catch(std::bad_alloc) {
            logger::error("Failed to allocate memory!");
            std::exit(1);
        }
        input.read((char*)data.get(), length);
        input.close();

        Pack2 pack2(path, std::span<uint8_t>(data.get(), length));
        uint32_t i;
        {
            std::scoped_lock<std::mutex> lk(packs_mutex);
            i = (uint32_t)packs.size();
            packs.push_back(std::make_pair(pack2, std::move(data)));
        }
        std::unordered_map<uint64_t, uint32_t> temp;
        for(auto iter = pack2.namehash_to_asset.begin(); iter != pack2.namehash_to_asset.end(); iter++) {
            temp[iter->first] = i;
        }
        {
            std::scoped_lock<std::mutex> lk(namehashes_mutex);
            maps.push_back(temp);
        }
    });
    for(auto& map : maps) {
        namehash_to_pack.merge(map);
    }
}

const Asset2 Manager::get(std::string name) const {
    uint64_t namehash = crc64(name);
    if(namehash_to_pack.find(namehash) == namehash_to_pack.end()) {
        logger::error("{} not found in given packs.", name);
        return Asset2({}, {});
    }

    Pack2 pack = packs.at(namehash_to_pack.at(namehash)).first;
    
    return pack.asset(name);
}

bool Manager::contains(std::string name) {
    return namehash_to_pack.find(crc64(name)) != namehash_to_pack.end();
}