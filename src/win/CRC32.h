//
// Created by Xenia on 1.09.2024.
//

#ifndef CRC32_H
#define CRC32_H
#include <vector>
#include <zlib.h>
#include <cstdint>


class CRC32 {
public:
    static uint32_t CalculateCRC32(const std::vector<uint8_t>& data);
};



#endif //CRC32_H
