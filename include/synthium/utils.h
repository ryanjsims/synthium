#pragma once

#include <string>
#include <spdlog/fmt/fmt.h>
#include <vector>

namespace synthium::utils {
    std::string human_bytes(uint64_t bytecount);
}