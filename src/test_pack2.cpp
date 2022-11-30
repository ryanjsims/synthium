#include "loader.h"

#include <iostream>
#include <fstream>
#include <filesystem>
#include <spdlog/spdlog.h>

namespace logger = spdlog;

int main() {
    logger::set_level(logger::level::info);
    logger::info("Test");
    std::filesystem::path path("C:/Users/Public/Daybreak Game Company/Installed Games/PlanetSide 2 Test/Resources/Assets/Amerish_x64_0.pack2");
    std::ifstream input(path, std::ios::binary | std::ios::ate);

    size_t length = input.tellg();
    logger::info("Length: {}", length);
    input.seekg(0);
    std::unique_ptr<uint8_t[]> data = std::make_unique<uint8_t[]>(length);
    if(!data) {
        logger::error("Failed to allocate memory!");
        std::exit(1);
    }
    input.read((char*)data.get(), length);

    std::span<uint8_t> span_data = std::span<uint8_t>(data.get(), length);
    std::size_t count = 4;
    logger::info("Data at 0 = {}", std::string((char*)span_data.first(4).data(), count));

    Pack2 pack2(path, span_data);
    logger::info("Test");
    if(pack2.contains("Amerish.zone")) {
        logger::info("Amerish.zone found.");
        std::ofstream output("Amerish.zone", std::ios::binary);
        std::vector<uint8_t> zone_data = pack2.asset_data("Amerish.zone", false);
        output.write((char*)zone_data.data(), zone_data.size());
        logger::info("Wrote {} bytes", zone_data.size());
        output.close();
    } else {
        logger::info("Amerish.zone not found.");
    }
    return 0;
}