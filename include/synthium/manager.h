#pragma once
#ifdef _WIN32
typedef int64_t ssize_t;
#endif

#include "loader.h"

#include <filesystem>
#include <memory>
#include <optional>
#include <span>
#include <unordered_map>
#include <vector>

namespace synthium {
    struct Manager {
        Manager(std::vector<std::filesystem::path> packs);

        const std::shared_ptr<Asset2> get(std::string name);
        const std::optional<Asset2Raw> get_raw(std::string name);
        bool contains(std::string name) const;
        void export_by_magic(std::span<uint8_t> magic, std::filesystem::path output_directory = std::filesystem::path("export"), std::string extension = ".bin");
        void deallocate(ssize_t bytes);
    
    private:
        std::unordered_map<uint64_t, uint32_t> namehash_to_pack;
        std::vector<std::pair<Pack2, std::unique_ptr<uint8_t[]>>> packs;

        /**
         * @brief Opens a pack from file
         * 
         * @param path The path where the pack is located on disk
         * @param length Will be filled with the number of bytes loaded
         * @return std::unique_ptr<uint8_t[]> The pack data
         */
        std::unique_ptr<uint8_t[]> open_pack(std::filesystem::path path, size_t* length);

        /**
         * @brief Loads a previously opened pack into memory
         * 
         * @param index The index of the pack to load
         * @return true The pack was loaded successfully
         * @return false The pack failed to load
         */
        bool load_pack(uint32_t index);

        /**
         * @brief Unloads a pack from memory
         * 
         * @param index The index of the pack to unload
         * @return size_t The size in bytes unloaded. 0 if the pack is not loaded.
         */
        size_t unload_pack(uint32_t index);

        /**
         * @brief Checks if a pack is loaded
         * 
         * @param index The index of the pack to check
         * @return true The pack is in memory
         * @return false The pack is not in memory
         */
        bool pack_is_loaded(uint32_t index);
    };
}