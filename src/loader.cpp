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
    const std::span<uint8_t> asset_data = buf_.subspan(map_offset(), asset_count() * sizeof(Asset2Raw));
    assets = std::span<Asset2Raw>((Asset2Raw*)asset_data.data(), asset_count());
    for(uint64_t i = 0; i < assets.size(); i++) {
        logger::debug("assets[{}].name_hash = 0x{:016x}", i, assets[i].name_hash);
        namehash_to_asset[assets[i].name_hash] = i;
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

std::span<Asset2Raw> Pack2::raw_assets() const {
    return assets;
}

Asset2 Pack2::asset(std::string name) const {
    uint32_t index;
    try {
        index = namehash_to_asset.at(crc64(name));
    } catch(std::out_of_range &err) {
        logger::error("{} not in Pack {}: {}", name, this->name, err.what());
        return {{}, {}};
    }
    return Asset2(assets[index], name, buf_.subspan(assets[index].offset, assets[index].data_length));
}

bool Pack2::contains(std::string name) const {
    logger::debug("{}: 0x{:x}", name, crc64(name));
    return namehash_to_asset.find(crc64(name)) != namehash_to_asset.end();
}

std::vector<uint8_t> Pack2::asset_data(std::string name, bool raw) const {
    return asset(name).get_data(raw);
}


Asset2::Asset2(Asset2Raw raw, std::span<uint8_t> data): Asset2(raw, "", data) {}

Asset2::Asset2(Asset2Raw raw, std::string name_, std::span<uint8_t> data): name(name_), buf_(data), raw_(raw) {}

uint32_t Asset2::uncompressed_size() const {
    return ntohl(get<uint32_t>(4));
}

std::vector<uint8_t> Asset2::get_data(bool raw) const {
    std::span<uint8_t> raw_data = buf_.subspan(8);
    if (raw || (raw_.zipped != 1 && raw_.zipped != 17)) {
        return std::vector<uint8_t>(raw_data.begin(), raw_data.end());
    }

    std::shared_ptr<uint8_t[]> buffer;
    uLongf unzipped_length = (uLongf)uncompressed_size();
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