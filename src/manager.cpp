#include "synthium/crc64.h"
#include "synthium/manager.h"
#include "synthium/version.h"

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

const std::optional<Asset2> Manager::get(std::string name) const {
    uint64_t namehash = crc64(name);
    if(namehash_to_pack.find(namehash) == namehash_to_pack.end()) {
        logger::error("{} not found in given packs.", name);
        return {};
    }

    Pack2 pack = packs.at(namehash_to_pack.at(namehash)).first;
    
    return pack.asset(name);
}

bool Manager::contains(std::string name) const {
    return namehash_to_pack.find(crc64(name)) != namehash_to_pack.end();
}

void Manager::export_by_magic(
    std::span<uint8_t> magic, 
    std::filesystem::path output_directory, 
    std::string extension
) {
    std::stringstream magic_string;
    for(uint32_t i = 0; i < magic.size(); i++) {
        magic_string << std::hex << std::setw(2) << std::setfill('0') << (int)magic[i] << " ";
    }
    logger::info("Got magic {}", magic_string.str());
    std::mutex io_mutex;
    std::for_each(
        std::execution::par,
        packs.begin(), packs.end(), [&](std::pair<Pack2, std::unique_ptr<uint8_t[]>>& pack_pair){
        std::span<Asset2Raw> assets = pack_pair.first.raw_assets();
        for(uint32_t i = 0; i < assets.size(); i++) {
            std::vector<uint8_t> data_container;
            std::span<uint8_t> asset_data;
            if(assets[i].data_length == 0) {
                continue;
            }
            if(!assets[i].is_zipped()) {
                asset_data = pack_pair.first.buf_.subspan(assets[i].offset, assets[i].data_length);
            } else {
                Asset2 asset(assets[i], pack_pair.first.buf_.subspan(assets[i].offset, assets[i].data_length));
                data_container = asset.get_data();
                asset_data = std::span<uint8_t>(data_container.begin(), data_container.end());
            }

            if(std::strncmp((char*)magic.data(), (char*)asset_data.data(), magic.size()) == 0) {
                std::scoped_lock<std::mutex> lk(io_mutex);
                logger::info("{}", std::to_string(assets[i].name_hash));
                std::ofstream output((output_directory / std::to_string(assets[i].name_hash)).replace_extension(extension), std::ios::binary);
                output.write((char*)asset_data.data(), asset_data.size());
                output.close();
            }
        }
    });
}