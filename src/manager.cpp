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

const Asset2 Manager::get(std::string name) {
    uint64_t namehash = crc64(name);
    if(namehash_to_pack.find(namehash) == namehash_to_pack.end()) {
        logger::error("{} not found in given packs.", name);
        return Asset2({}, {});
    }

    Pack2 pack = packs.at(namehash_to_pack.at(namehash)).first;
    if(!is_loaded(namehash_to_pack.at(namehash))) {
        logger::warn("{} not loaded, attempting to reload...", pack.get_name());
        try {
            load(namehash_to_pack.at(namehash));
        } catch (std::bad_alloc) {
            logger::warn("Failed to allocate memory! Unloading some packs and trying again.");
            size_t length = pack.size();
            size_t unloaded = 0;
            uint32_t index = 0;
            while(unloaded < length) {
                if(is_loaded(index)) {
                    unloaded += unload(index);
                }
                index++;
            }

            try {
                load(namehash_to_pack.at(namehash));
            } catch (std::bad_alloc &err) {
                logger::error("Failed to allocate memory on retry! {}", err.what());
                std::exit(1);
            }
            logger::warn("Unloaded {} bytes to make room for {}", unloaded, pack.get_name());
        }
    }
    return pack.asset(name);
}

void Manager::load(uint32_t index) {
    if(is_loaded(index))
        return;
    std::ifstream input(packs.at(index).first.path, std::ios::binary | std::ios::ate);
    size_t length = input.tellg();
    input.seekg(0);
    packs.at(index).second = std::make_unique<uint8_t[]>(length);
    input.read((char*)packs.at(index).second.get(), length);
    packs.at(index).first.buf_ = std::span<uint8_t>(packs.at(index).second.get(), length);
    input.close();
}

size_t Manager::unload(uint32_t index) {
    if(!is_loaded(index))
        return 0;
    size_t length = packs.at(index).first.size();
    packs.at(index).second.reset();
    return length;
}

bool Manager::is_loaded(uint32_t index) {
    return packs.at(index).second != nullptr;
}
