#pragma once
#include <span>
#include <spdlog/spdlog.h>
#include <string_view>
#include <unordered_map>
#include <filesystem>

struct Asset2 {
    uint64_t name_hash;
    uint64_t offset;
    uint64_t data_length;
    uint32_t zipped;
    uint32_t data_hash;
};

struct Pack2 {
    mutable std::span<uint8_t> buf_;

    Pack2(std::filesystem::path path, std::span<uint8_t> data);

    template <typename T>
    struct ref {
        uint8_t * const p_;
        ref (uint8_t *p) : p_(p) {}
        operator T () const { T t; memcpy(&t, p_, sizeof(t)); return t; }
        T operator = (T t) const { memcpy(p_, &t, sizeof(t)); return t; }
    };

    template <typename T>
    ref<T> get (size_t offset) const {
        if (offset + sizeof(T) > buf_.size()) throw std::out_of_range("Pack2: Offset out of range");
        return ref<T>(&buf_[0] + offset);
    }

    size_t size() const {
        return buf_.size();
    }

    std::string_view magic() const;
    ref<uint32_t> asset_count() const;
    ref<uint64_t> length() const;
    ref<uint64_t> map_offset() const;

    std::span<Asset2> raw_assets() const;
    Asset2 asset(std::string name) const;
    std::vector<uint8_t> asset_data(std::string name, bool raw = false) const;

    bool contains(std::string name) const;
    static std::string version();
private:
    std::unordered_map<uint64_t, uint64_t> namehash_to_index;
    std::span<Asset2> assets;
    std::string name;
    std::filesystem::path path;
};
