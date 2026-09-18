/* ======================================================================================
 * Library       : vhlibrle7b
 * Description   : C++ library implementing a 7-bit Run-Length Encoding (RLE) algorithm
 * Revision      : 0.1.0
 * Source        : https://github.com/vigatron/vhlibrle7b
 * Disclaimer    : Provided "AS IS", without warranty.
 * License       : MIT
 * File          : src/vhlibrle7berrs.hpp
 * Content size  : 829
 * Date / Time   : 18-09-2026 21:38:13
 * MD5           : beba1802664244668f7f596c9ec63652
 * Notes         : MD5 = file content without header/footer
 * Encoding      : UTF-8
 * Author        : Viktor Glebov / V01G04A81
 * Copyright     : © 2026 Viktor Glebov
 * ========================[ BEGIN FILE CONTENT ]====================================== */
#pragma once

#include <cstdint>

namespace VHRLE7BERR
{

    enum Status : uint32_t
    {
        okstat = 0,

        errSrcOutOfRange,   // Source BMode reader
        errDstOutOfRange,   // Destination BMode writer
        errOutOfRange,

        errSrcMemorySize,
        errSrcInvalid,
        errSrcVersion,

        errRLEInvalidHeader,
        errRLESourceInvalid,

        errDestMemorySize,
        errSettings,
        errAlign,
        errWrite,
        errInternal,

        // Wrong CRC
        errCRC,
        errRLECRC,
        errDSTCRC,

        errNotImplemented,
        errInvalidCallback,
        errInvalidRLESource,
        errWriteError,
        errUnpackProcessFailed,

        //
        errBlockModeParams,

        //
        errIOSource,
        errIODestination,

        errCheckFailed
    };

};
/* ========================[  END FILE CONTENT  ]========================
 * Library          : vhlibrle7b
 * File             : src/vhlibrle7berrs.hpp
 * Revision         : 0.1.0
 * Content size     : 829
 * Date / Time      : 18-09-2026 21:38:13
 * MD5              : beba1802664244668f7f596c9ec63652
 * Copyright        : © 2026 Viktor Glebov
 * ====================================================================== */