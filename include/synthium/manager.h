#pragma once
#include "loader.h"

#include <filesystem>
#include <memory>
#include <optional>
#include <unordered_map>
#include <vector>

namespace synthium {
    struct Manager {
        Manager(std::vector<std::filesystem::path> packs);

        const std::optional<Asset2> get(std::string name) const;
        bool contains(std::string name) const;
    private:
        std::unordered_map<uint64_t, uint32_t> namehash_to_pack;
        std::vector<std::pair<Pack2, std::unique_ptr<uint8_t[]>>> packs;
    };
}