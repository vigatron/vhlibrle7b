/* ======================================================================================
 * Library       : vhlibrle7b
 * Description   : C++ library implementing a 7-bit Run-Length Encoding (RLE) algorithm
 * Revision      : 0.1.0
 * Source        : https://github.com/vigatron/vhlibrle7b
 * Disclaimer    : Provided "AS IS", without warranty.
 * License       : MIT
 * File          : src/vhlibrle7bcrc.hpp
 * Content size  : 900
 * Date / Time   : 19-09-2026 20:06:17
 * MD5           : 1422d2eb965e885bbfe789db9703bffc
 * Notes         : MD5 = file content without header/footer
 * Encoding      : UTF-8
 * Author        : Viktor Glebov / V01G04A81
 * Copyright     : © 2026 Viktor Glebov
 * ========================[ BEGIN FILE CONTENT ]====================================== */
#pragma once

#include "vhlibrle7binc.hpp"

class VHRLE7bCRC32
{
public:
    /**
     *
     */
    static const uint32_t DEF_POLY_VAL = 0xFFFFFFFF;

    /**
     *
     */
    static uint32_t calcStep(uint32_t crc, uint8_t bval)
    {
        crc ^= bval;
        for (int i = 0; i < 8; i++)
            crc = (crc >> 1) ^ (0xEDB88320 & -(crc & 1));
        return crc;
    }

    /**
     * @brief Calculate CRC32 checksum for a block of data.
     * @param data Pointer to the data buffer.
     * @param len Size of the data buffer in bytes.
     * @param crc Initial CRC value (default: 0xFFFFFFFF).
     * @return Calculated CRC32 value.
     */
    static uint32_t calcBlockCRC32(
        const uint8_t *data,
        size_t len,
        uint32_t crc = 0xFFFFFFFF)
    {
        while (len--)
        {
            crc = calcStep(crc, *data++);
        }
        return ~crc;
    }

private:
};
/* ========================[  END FILE CONTENT  ]========================
 * Library          : vhlibrle7b
 * File             : src/vhlibrle7bcrc.hpp
 * Revision         : 0.1.0
 * Content size     : 900
 * Date / Time      : 19-09-2026 20:06:17
 * MD5              : 1422d2eb965e885bbfe789db9703bffc
 * Copyright        : © 2026 Viktor Glebov
 * ====================================================================== */