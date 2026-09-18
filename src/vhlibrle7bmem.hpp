/* ======================================================================================
 * Library       : vhlibrle7b
 * Description   : C++ library implementing a 7-bit Run-Length Encoding (RLE) algorithm
 * Revision      : 0.1.0
 * Source        : https://github.com/vigatron/vhlibrle7b
 * Disclaimer    : Provided "AS IS", without warranty.
 * License       : MIT
 * File          : src/vhlibrle7bmem.hpp
 * Content size  : 1276
 * Date / Time   : 18-09-2026 21:38:13
 * MD5           : 8cef1a1bd597e49a744bc93d68b4a939
 * Notes         : MD5 = file content without header/footer
 * Encoding      : UTF-8
 * Author        : Viktor Glebov / V01G04A81
 * Copyright     : © 2026 Viktor Glebov
 * ========================[ BEGIN FILE CONTENT ]====================================== */
#pragma once

#include "vhlibrle7binc.hpp"

#include "vhlibrle7bmreg.hpp"

//
class VHRLE7bMemRegions
{
public:
    /**
     * @brief Constructor that initializes regions with source and destination memory blocks.
     * @param inblk Source memory region.
     * @param outblk Destination memory region.
     */
    VHRLE7bMemRegions(const VHRLE7bMemRegion &inblk, const VHRLE7bMemRegion &outblk)
    {
        srcmem = inblk;
        dstmem = outblk;
    }

    /**
     * @brief Get constant reference to source memory region.
     * @return Constant reference to source memory region.
     */
    const VHRLE7bMemRegion &src() const { return srcmem; }

    /**
     * @brief Get reference to source memory region.
     * @return Reference to source memory region.
     */
    VHRLE7bMemRegion &src() { return srcmem; }

    /**
     * @brief Get constant reference to destination memory region.
     * @return Constant reference to destination memory region.
     */
    const VHRLE7bMemRegion &dst() const { return dstmem; }

    /**
     * @brief Get reference to destination memory region.
     * @return Reference to destination memory region.
     */
    VHRLE7bMemRegion &dst() { return dstmem; }

private:
    VHRLE7bMemRegion srcmem;
    VHRLE7bMemRegion dstmem;
};
/* ========================[  END FILE CONTENT  ]========================
 * Library          : vhlibrle7b
 * File             : src/vhlibrle7bmem.hpp
 * Revision         : 0.1.0
 * Content size     : 1276
 * Date / Time      : 18-09-2026 21:38:13
 * MD5              : 8cef1a1bd597e49a744bc93d68b4a939
 * Copyright        : © 2026 Viktor Glebov
 * ====================================================================== */