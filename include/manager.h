#pragma once
#include "loader.h"

#include <filesystem>
#include <memory>
#include <spdlog/spdlog.h>
#include <unordered_map>
#include <vector>

namespace synthium {
    struct Manager {
        Manager(std::vector<std::filesystem::path> packs);

        const Asset2 get(std::string name);

        static std::string version();
    private:
        std::unordered_map<uint64_t, uint32_t> namehash_to_pack;
        std::vector<std::pair<Pack2, std::unique_ptr<uint8_t[]>>> packs;

        void load(uint32_t index);
        size_t unload(uint32_t index);
        bool is_loaded(uint32_t index);
    };
}