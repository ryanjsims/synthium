#include "crc64.h"
#include "loader.h"
#include "version.h"

#include <zlib.h>

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
    name = path.stem().string();
    logger::info("Loading {} assets from {}{}", asset_count(), name, path.extension().string());
    const std::span<uint8_t> asset_data = buf_.subspan(map_offset(), asset_count() * sizeof(Asset2));
    assets = std::span<Asset2>((Asset2*)asset_data.data(), asset_count());
    for(uint64_t i = 0; i < assets.size(); i++) {
        logger::debug("assets[{}].name_hash = 0x{:016x}", i, assets[i].name_hash);
        namehash_to_index[assets[i].name_hash] = i;
    }
    logger::info("Loaded.");
}

std::string Pack2::version() {
    return SYNTHIUM_VERSION;
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

std::span<Asset2> Pack2::raw_assets() const {
    return assets;
}

Asset2 Pack2::asset(std::string name) const {
    uint64_t index;
    try {
        index = namehash_to_index.at(crc64(name));
    } catch(std::out_of_range &err) {
        logger::error("{} not in Pack {}: {}", name, this->name, err.what());
        return {};
    }
    return assets[index];
}

bool Pack2::contains(std::string name) const {
    logger::debug("{}: 0x{:x}", name, crc64(name));
    return namehash_to_index.find(crc64(name)) != namehash_to_index.end();
}

std::vector<uint8_t> Pack2::asset_data(std::string name, bool raw) const {
    Asset2 asset = this->asset(name);
    if(asset.name_hash == 0) {
        return {};
    }
    std::span<uint8_t> raw_data = buf_.subspan(asset.offset + 8, asset.data_length);
    if (raw || (asset.zipped != 1 && asset.zipped != 17)) {
        return std::vector<uint8_t>(raw_data.begin(), raw_data.end());
    }
    uLongf unzipped_length = ntohl(get<uint32_t>(asset.offset + 4));
    std::shared_ptr<uint8_t[]> buffer;
    try {
        buffer = std::make_shared<uint8_t[]>(unzipped_length);
    } catch(std::bad_alloc &err) {
        logger::error("Failed to allocate output buffer: {}", err.what());
        std::exit(1);
    }
    int errcode = uncompress(buffer.get(), &unzipped_length, raw_data.data(), (uLong)raw_data.size());
    if(errcode != Z_OK) {
        switch(errcode) {
        case Z_MEM_ERROR:
            logger::error("uncompress: Not enough memory! ({})", errcode);
            break;
        case Z_BUF_ERROR:
            logger::error("uncompress: Not enough room in the output buffer! ({})", errcode);
            break;
        case Z_DATA_ERROR:
            logger::error("uncompress: Input data was corrupted or incomplete! ({})", errcode);
            break;
        }
        std::exit(1);
    }
    return std::vector<uint8_t>(buffer.get(), buffer.get() + unzipped_length);
}
