#pragma once

#include <cstdint>

namespace VHRLE7BERR
{

    enum Status : uint32_t
    {
        okstat = 0,

        errSrcOutOfRange,   // Source BMode reader
        errDstOutOfRange,   // Destination BMode writer

        errSrcMemorySize,
        errSrcInvalid,
        errSrcVersion,

        errRLEInvalidHeader,
        errRLESourceInvalid,

        errDestMemorySize,
        errSettings,
        errAlign,
        errWrite,
        errOutOfRange,
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
