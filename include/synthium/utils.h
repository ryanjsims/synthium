#pragma once

#include <string>
#include <spdlog/fmt/fmt.h>
#include <vector>

namespace synthium::utils {
    std::string human_bytes(uint64_t bytecount) {
        std::vector<std::string> suffixes = {"B", "KiB", "MiB", "GiB", "TiB", "PiB"};
    
        uint32_t i = 0;
        double bytes = (double)bytecount;
        if(bytecount > 1024) {
            for(i; (bytecount / 1024) > 0 && i < suffixes.size(); i++, bytecount /= 1024) {
                bytes = (double)bytecount / 1024.0;
            }
        }

        return fmt::format("{:.2f}{}", bytes, suffixes[i]);
    }
}