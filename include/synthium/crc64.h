#ifndef CRC64_H
#define CRC64_H

#include <cstdint>
#include <span>
#include <cctype>
#include <string>

uint64_t crc64(std::span<uint8_t> data);
uint64_t crc64(std::string data);

#endif //CRC64_H