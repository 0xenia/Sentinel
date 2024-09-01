//
// Created by Xenia on 1.09.2024.
//

#include "CRC32.h"

uint32_t CRC32::CalculateCRC32(const std::vector<uint8_t> &data) {
    return crc32(0, data.data(), data.size());
}



