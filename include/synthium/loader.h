#pragma once
#include <span>
#include <spdlog/spdlog.h>
#include <string_view>
#include <unordered_map>
#include <filesystem>

namespace synthium {
    struct Asset2Raw {
        uint64_t name_hash;
        uint64_t offset;
        uint64_t data_length;
        uint32_t zipped;
        uint32_t data_hash;

        bool is_zipped() const {
            return zipped == 1 || zipped == 17;
        }
    };

    struct Asset2 {
        mutable std::span<uint8_t> buf_;

        Asset2(Asset2Raw raw, std::span<uint8_t> data);
        Asset2(Asset2Raw raw, std::string name, std::span<uint8_t> data);

        template <typename T>
        struct ref {
            uint8_t * const p_;
            ref (uint8_t *p) : p_(p) {}
            operator T () const { T t; memcpy(&t, p_, sizeof(t)); return t; }
            T operator = (T t) const { memcpy(p_, &t, sizeof(t)); return t; }
        };

        template <typename T>
        ref<T> get (size_t offset) const {
            if (offset + sizeof(T) > buf_.size()) throw std::out_of_range("Asset2: Offset out of range");
            return ref<T>(&buf_[0] + offset);
        }

        size_t size() const {
            return buf_.size();
        }

        uint64_t length() const {
            return raw_.data_length;
        }

        uint32_t uncompressed_size() const;
        std::vector<uint8_t> get_data(bool raw = false) const;
        std::string_view get_name() const {
            return name;
        }

    private:
        std::string name;
        Asset2Raw raw_;
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

        std::span<Asset2Raw> raw_assets() const;
        Asset2 asset(std::string name) const;
        std::vector<uint8_t> asset_data(std::string name, bool raw = false) const;

        bool contains(std::string name) const;

        std::string get_name() const {
            return path.filename().string();
        }

        const std::filesystem::path get_path() const {
            return path;
        }

        friend struct Manager;
    private:
        std::unordered_map<uint64_t, uint32_t> namehash_to_asset;
        std::filesystem::path path;
    };
}