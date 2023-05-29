#include "synthium/crc64.h"
#include "synthium/loader.h"
#include "synthium/version.h"
#include "synthium/utils.h"

#include <zlib.h>

#if __cpp_lib_shared_ptr_arrays < 201707L
#error synthium requires a compiler that supports std::make_shared<T[]> (__cpp_lib_shared_ptr_arrays >= 201707L)
#endif

uint32_t ntohl(uint32_t const net) {
    uint8_t data[4] = {};
    memcpy(&data, &net, sizeof(data));

    return ((uint32_t) data[3] << 0)
         | ((uint32_t) data[2] << 8)
         | ((uint32_t) data[1] << 16)
         | ((uint32_t) data[0] << 24);
}

namespace logger = spdlog;
using namespace std::literals::string_view_literals;
using namespace synthium;

Pack2::Pack2(std::filesystem::path path_, std::span<uint8_t> data): buf_(data), path(path_) {
    if(magic() != "PAK\x01") {
        throw std::invalid_argument("Invalid magic, '" + path.string() + "' is not a pack2 file.");
    }
    logger::debug("Loading {} assets from {}", asset_count(), get_name());
    
    std::span<Asset2Raw> assets = raw_assets();
    for(uint32_t i = 0; i < assets.size(); i++) {
        logger::trace("assets[{}].name_hash = 0x{:016x}", i, assets[i].name_hash);
        namehash_to_asset[assets[i].name_hash] = i;
    }
    logger::debug("Loaded.");
}

std::string_view Pack2::magic() const {
    return std::string_view((char*)buf_.data(), 4);
}

Pack2::ref<uint32_t> Pack2::asset_count() const {
    return get<uint32_t>(4);
}

Pack2::ref<uint64_t> Pack2::length() const {
    return get<uint64_t>(8);
}

Pack2::ref<uint64_t> Pack2::map_offset() const {
    return get<uint64_t>(16);
}

std::span<Asset2Raw> Pack2::raw_assets() const {
    std::span<uint8_t> asset_data = buf_.subspan(map_offset(), asset_count() * sizeof(Asset2Raw));
    return std::span<Asset2Raw>((Asset2Raw*)asset_data.data(), asset_count());
}

std::shared_ptr<Asset2> Pack2::asset(uint32_t index, bool cache) {
    std::unordered_map<uint32_t, std::shared_ptr<Asset2>>::iterator iter;
    if((iter = assets_in_use.find(index)) != assets_in_use.end()) {
        logger::debug("Using asset from cache");
        return iter->second;
    }
    std::span<Asset2Raw> assets = raw_assets();
    std::shared_ptr<Asset2> asset = std::make_shared<Asset2>(assets[index], buf_.subspan(assets[index].offset, assets[index].data_length));
    if(cache) {
        assets_in_use[index] = asset;
        return assets_in_use[index];
    }
    return asset;
}

std::shared_ptr<Asset2> Pack2::asset(std::string name, bool cache) {
    uint32_t index;
    try {
        index = namehash_to_asset.at(crc64(name));
    } catch(std::out_of_range &err) {
        logger::error("{} not in Pack {}: {}", name, this->get_name(), err.what());
        return nullptr;
    }

    std::unordered_map<uint32_t, std::shared_ptr<Asset2>>::iterator iter;
    if((iter = assets_in_use.find(index)) != assets_in_use.end()) {
        logger::info("Using asset from cache");
        return iter->second;
    }
    std::span<Asset2Raw> assets = raw_assets();
    std::shared_ptr<Asset2> asset = std::make_shared<Asset2>(assets[index], buf_.subspan(assets[index].offset, assets[index].data_length));
    if(cache) {
        assets_in_use[index] = asset;
        return assets_in_use[index];
    }
    return asset;
}

Asset2 Pack2::asset(std::string name) const {
    uint32_t index;
    try {
        index = namehash_to_asset.at(crc64(name));
    } catch(std::out_of_range &err) {
        logger::error("{} not in Pack {}: {}", name, this->get_name(), err.what());
        return {{}, {}};
    }
    std::span<Asset2Raw> assets = raw_assets();
    return Asset2(assets[index], name, buf_.subspan(assets[index].offset, assets[index].data_length));
}

bool Pack2::contains(std::string name) const {
    logger::debug("{}: 0x{:x}", name, crc64(name));
    return namehash_to_asset.find(crc64(name)) != namehash_to_asset.end();
}

std::vector<uint8_t> Pack2::asset_data(std::string name, bool raw) const {
    return asset(name).get_data(raw);
}

bool Pack2::in_use() const {
    for(auto iter = assets_in_use.begin(); iter != assets_in_use.end(); iter++) {
        if(iter->second.use_count() > 1) {
            return true;
        }
    }
    return false;
}

void Pack2::notify_unloaded() {
    for(auto iter = assets_in_use.begin(); iter != assets_in_use.end(); iter++) {
        iter->second.reset();
    }
    assets_in_use.clear();
}


Asset2::Asset2(Asset2Raw raw, std::span<uint8_t> data): Asset2(raw, "", data) {}

Asset2::Asset2(Asset2Raw raw, std::string name_, std::span<uint8_t> data): name(name_), buf_(data), raw_(raw) {}

uint32_t Asset2::uncompressed_size() const {
    return ntohl((uint32_t)get<uint32_t>(4));
}

std::vector<uint8_t> Asset2::get_data(bool raw) {
    if (raw || !raw_.is_zipped()) {
        return std::vector<uint8_t>(buf_.begin(), buf_.end());
    }
    if(decompressed != nullptr) {
        logger::info("Using data from cache");
        return std::vector<uint8_t>(decompressed.get(), decompressed.get() + uncompressed_size());
    }
    std::span<uint8_t> raw_data = buf_.subspan(8);

    std::shared_ptr<uint8_t[]> out_buffer;
    unsigned long zipped_length = (unsigned long)raw_data.size();
    unsigned long unzipped_length = uncompressed_size();
    try {
        out_buffer = std::make_shared<uint8_t[]>(unzipped_length);
    } catch(std::bad_alloc &err) {
        logger::error("Failed to allocate output buffer: {}", err.what());
        throw err;
    }
    
    if(name != "") {
        logger::info("Decompressing asset '{}' of size {} (compressed size {})", name, utils::human_bytes(unzipped_length), utils::human_bytes(zipped_length));
    }
    int errcode = uncompress2(out_buffer.get(), &unzipped_length, raw_data.data(), &zipped_length);
    if(errcode != Z_OK) {
        switch(errcode) {
        case Z_MEM_ERROR:
            logger::error("uncompress: Not enough memory! ({})", errcode);
            throw std::bad_alloc();
        case Z_BUF_ERROR:
            // Theoretically this shouldn't happen
            logger::error("uncompress: Not enough room in the output buffer! ({})", errcode);
            break;
        case Z_DATA_ERROR:
            logger::error("uncompress: Input data was corrupted or incomplete! ({})", errcode);
            break;
        }
        throw std::runtime_error(zError(errcode));
    }
    decompressed = out_buffer;
    return std::vector<uint8_t>(out_buffer.get(), out_buffer.get() + unzipped_length);
}