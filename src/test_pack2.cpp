#include "synthium.h"

#include <iostream>
#include <fstream>
#include <filesystem>
#include <spdlog/spdlog.h>

namespace logger = spdlog;
using namespace synthium;

int main() {
    logger::set_level(logger::level::info);
    logger::info("Pack2 Test Program (using pack2lib {})", Pack2::version());
    std::filesystem::path path("C:/Users/Public/Daybreak Game Company/Installed Games/Planetside 2 Test/Resources/Assets/Amerish_x64_7.pack2");
    std::ifstream input(path, std::ios::binary | std::ios::ate);

    size_t length = input.tellg();
    logger::info("Length: {}", length);
    input.seekg(0);
    std::unique_ptr<uint8_t[]> data;
    try {
        data = std::make_unique<uint8_t[]>(length);
    } catch(std::bad_alloc &err) {
        logger::error("Failed to allocate memory! {}", err.what());
        std::exit(1);
    }
    input.read((char*)data.get(), length);

    std::span<uint8_t> span_data = std::span<uint8_t>(data.get(), length);

    Pack2 pack2(path, span_data);

    logger::info("Magic = {}", pack2.magic());
    std::vector<uint8_t> namelist;
    if(pack2.contains("{NAMELIST}")) {
        logger::info("Found namelist:");
        namelist = pack2.asset_data("{NAMELIST}");
        auto start = namelist.begin();
        auto end = std::find(namelist.begin(), namelist.end(), (uint8_t)'\n');
        while(start != namelist.end()) {
            logger::info("    {}", std::string((char*)(namelist.data() + (start - namelist.begin())), end - start));
            start = end + 1;
            end = std::find(start, namelist.end(), (uint8_t)'\n');
        }
    }

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