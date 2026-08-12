/* ======================================================================================
 * Library       : vhlibrle7b
 * Description   : C++ library implementing a 7-bit Run-Length Encoding (RLE) algorithm
 * Revision      : 0.0.4
 * Source        : https://github.com/vigatron/vhlibrle7b
 * Disclaimer    : Provided "AS IS", without warranty.
 * License       : MIT
 * File          : src/vhlibrle7b.hpp
 * Content size  : 13142
 * Date / Time   : 12-08-2026 20:36:06
 * MD5           : 39c7a25dde4d7341111f91f9200dfa75
 * Notes         : MD5 = file content without header/footer
 * Encoding      : UTF-8
 * Author        : Viktor Glebov / V01G04A81
 * Copyright     : © 2026 Viktor Glebov
 * ========================[ BEGIN FILE CONTENT ]====================================== */
#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>

#if defined(DEBUG_VHRLE7B)
#include <cstdio>
#endif

#ifndef VHPLATFORM_INCLUDED
#define verr        uint32_t
#define verror(X)   (X)
#define vok         verror(0)
#endif

/**
 * Embedded version RLE-7-bit
 */

class VHRLE7b {

    public:

        VHRLE7b() = default;

        #pragma pack(push, 1)
        struct sthdr {
            uint8_t     pfx[8];     // Default prefix
            uint32_t    spans;      // Spans count field
            uint32_t    crc32src;   // Source data CRC32
            uint32_t    srcsize;    // Source block length
            uint32_t    crc32rle;   // Destination data CRC32
            uint32_t    rlesize;    // Destination block length
            uint32_t    reserved;   // Reserved
        };
        #pragma pack(pop)

        static_assert(sizeof(sthdr) == 32);

        enum Status : uint32_t {
            okstat = 0,
            errSrcMemorySize,
            errSrcInvalid,
            errRleSourceInvalid,
            errSrcVersion,
            errDestMemorySize,
            errSettings,
            errAlign,
            errWrite,
            errCRC,
            errOutOfRange,
            errInternal
        };

        /**
         * @brief Compresses data using the VHRLE7b run-length encoding algorithm.
         * 
         * Packs raw source bytes into 7-bit RLE and Literal (STD) spans, prepending a 32-byte
         * header containing stream metadata and CRC32 checksums.
         * 
         * @param[in]  srcptr   Pointer to the raw input data buffer (must be 32-bit aligned).
         * @param[in]  srcsize  Size of the raw input data in bytes.
         * @param[out] dstptr   Pointer to the output destination buffer (must be 32-bit aligned).
         * @param[in]  dstsize  Total capacity of the destination buffer in bytes.
         * @param[in]  minRLE   Minimum repeating sequence length to trigger RLE encoding (must be >= 4).
         * @param[in]  maxSIZ   Maximum allowed span size in bytes (must be in range [4, 127]).
         * 
         * @return Status::vok on success, or an appropriate Status error code on failure:
         *         - errDestMemorySize : Destination buffer is too small to fit the header.
         *         - errSettings       : Invalid parameter constraints (minRLE < 4, or maxSIZ outside [4, 127]).
         *         - errAlign          : Source or destination pointer is not 4-byte aligned.
         *         - errWrite          : Compressed data exceeded destination buffer bounds.
         */
        verr pack(
            const uint8_t *srcptr,
            const uint32_t srcsize,
            uint8_t *dstptr,
            const uint32_t dstsize,
            const uint8_t minRLE,
            const uint8_t maxSIZ
        ) {

            // Check before processing
            if(dstsize < sizeof(sthdr)) return errDestMemorySize;
            if(minRLE < 4) return errSettings;
            if(maxSIZ < 4 || maxSIZ >= 128) return errSettings;
            if(!checkalign(srcptr)) return errAlign;
            if(!checkalign(dstptr)) return errAlign;

            // Setup writer
            uint8_t * ptrbin = dstptr    + sizeof(sthdr);
            uint32_t  wrleft = dstsize   - sizeof(sthdr);
            uint32_t  spans_count = 0;
            uint32_t  stdcnt = 0;

            // Safe byte writer lambda
            auto putbyte = [&](uint8_t v) -> bool {
                if (wrleft == 0) return false;
                wrleft--;
                *ptrbin++ = v;
                return true;
            };

            // RLE span writer lambda
            auto writerle = [&](size_t pos, uint8_t cnt, uint8_t sym) -> bool {
                #if defined(DEBUG_VHRLE7B)
                printf("Write RLE @ %d  `%d`x%d\n", (int)pos, sym, (int)cnt);
                #endif
                if (!putbyte(0x80 | cnt)) return false;
                if (!putbyte(sym)) return false;
                spans_count++;
                return true;
            };

            // Literal (STD) span writer lambda
            auto writestd = [&](size_t pos, uint8_t cnt, const uint8_t *pbin) -> bool {
                #if defined(DEBUG_VHRLE7B)
                printf("Write STD @ %d x%d :", (int)pos, (int)cnt);
                #endif
                if (!putbyte(cnt)) return false;
                for (uint8_t i = 0; i < cnt; i++) {
                    #if defined(DEBUG_VHRLE7B)
                    printf(" %d", pbin[i]);
                    #endif
                    if (!putbyte(pbin[i])) return false;
                }
                spans_count++;
                #if defined(DEBUG_VHRLE7B)
                printf("\n");
                #endif
                return true;
            };

            // Main processing loop
            for(uint32_t i = 0; i < srcsize;) {

                uint32_t scnt = calcscnt(srcptr + i, srcsize - i);

                if(scnt >= minRLE) {

                    // Force store STD spans if avail
                    while(stdcnt) {
                        size_t wrcnt = (stdcnt > maxSIZ) ? maxSIZ : stdcnt;
                        if(!writestd(i - stdcnt, wrcnt, srcptr + (i - stdcnt)))
                            return errWrite;
                        stdcnt -= wrcnt;
                    }

                    // Store RLE spans
                    while(scnt) {
                        size_t wrcnt = (scnt > maxSIZ) ? maxSIZ : scnt;
                        if(!writerle(i, wrcnt, srcptr[i]))
                            return errWrite;
                        scnt -= wrcnt;
                        i += wrcnt;
                    }

                } else {

                    stdcnt += scnt;
                    i += scnt;

                    // Store STD
                    while(stdcnt >= maxSIZ) {
                        if(!writestd(i - stdcnt, maxSIZ, srcptr + (i - stdcnt)))
                            return errWrite;
                        stdcnt -= maxSIZ;
                    }

                    // EOF ? (i++ already)
                    if(i == srcsize) {

                        // Force store STD spans if avail
                        while(stdcnt) {
                            size_t wrcnt = (stdcnt > maxSIZ) ? maxSIZ : stdcnt;
                            if(!writestd(i - stdcnt, wrcnt, srcptr + (i - stdcnt)))
                                return errWrite;
                            stdcnt -= wrcnt;
                        }

                    }

                }

            }

            // Compute payload size
            uint32_t compressed_data_size = (dstsize - sizeof(sthdr)) - wrleft;

            // Copy header to destination buffer start
            sthdr hdr;
            std::memcpy(hdr.pfx, get_hdrpfx(), sizeof(hdr.pfx));
            hdr.spans = spans_count;
            hdr.srcsize = srcsize;
            hdr.crc32src = crc32(srcptr, srcsize);
            hdr.rlesize = compressed_data_size;
            hdr.crc32rle = crc32(dstptr + sizeof(sthdr), compressed_data_size);
            hdr.reserved = 0;

            // Copy header to destination buffer start
            std::memcpy(dstptr, &hdr, sizeof(sthdr));

            // Return result
            return vok;
        }

        /**
         * @brief Validates a compressed VHRLE7b data block structure and integrity.
         * @param ptrrle Pointer to the input compressed block (including header).
         * @param rleblksize Total size of the compressed block in bytes.
         * @return Status::vok if valid, error code otherwise
         *          (errAlign, errSrcMemorySize, errSrcVersion, errSrcInvalid, errCRC).
         */
        verr check(
            const uint8_t * ptrrle,
            const uint32_t rleblksize) {

            if(!checkalign(ptrrle))
                return errAlign;

            // Check limit
            if(rleblksize < sizeof(sthdr))
                return errSrcMemorySize;

            sthdr hdr;
            std::memcpy(&hdr, ptrrle, sizeof(sthdr));

            if (hdr.reserved != 0)
                return errSrcVersion;

            // Check pfx
            for(size_t i = 0; i < sizeof(sthdr::pfx); i++)
                if(hdr.pfx[i] != get_hdrpfx()[i])
                    return errSrcInvalid;

            // Check rlesrc size
            if(rleblksize - sizeof(sthdr) != hdr.rlesize)
                return errSrcMemorySize;

            // Check CRC
            uint32_t crcrle = crc32(ptrrle + sizeof(sthdr), hdr.rlesize);
            bool checkcrc = crcrle == hdr.crc32rle;
            return checkcrc ? vok : errCRC;
        }

        /**
         * @brief Decompresses a VHRLE7b encoded data block and validates CRC32 checksums.
         * @param ptrsrc Pointer to the source compressed data block.
         * @param srcsize Size of the source compressed data block in bytes.
         * @param ptrdst Pointer to the destination output buffer.
         * @param dstsize Maximum capacity of the destination buffer in bytes.
         * @return Status::vok on success, or appropriate Status error code on failure.
         */
        verr unpack(
            const uint8_t * ptrsrc,
            const uint32_t  srcsize,
            uint8_t * ptrdst,
            const uint32_t  dstsize
        ) {

            if(check(ptrsrc, srcsize) != vok)
                return errRleSourceInvalid;

            if(!checkalign(ptrdst))
                return errAlign;

            uint8_t* ptrbin = ptrdst;
            uint32_t wrleft = dstsize;

            // Safe byte writer lambda
            auto putbyte = [&](uint8_t v) -> bool {
                if (wrleft == 0) return false;
                wrleft--;
                *ptrbin++ = v;
                return true;
            };

            sthdr hdr;
            std::memcpy(&hdr, ptrsrc, sizeof(sthdr));

            // Not enough output buffer space for decompressed data
            if (dstsize < hdr.srcsize)
                return errDestMemorySize;

            uint32_t spanscnt = hdr.spans;
            uint32_t offs = sizeof(sthdr);

            while(spanscnt--) {

                // Validate stream boundaries
                if(offs >= srcsize)
                    return errOutOfRange;

                uint8_t ctrl = ptrsrc[offs++];
                uint8_t mod = ctrl >> 7;
                uint8_t cnt = ctrl & 0x7F;

                if (cnt == 0)
                    return errInternal;

                if(mod) { // RLE

                    if (offs >= srcsize)
                        return errOutOfRange;

                     uint8_t sym = ptrsrc[offs++];

                    #if defined(DEBUG_VHRLE7B)
                    printf("RLE %d\n", cnt);
                    #endif

                    for(uint8_t i=0; i < cnt; i++)
                        if(!putbyte(sym))
                            return errDestMemorySize;

                } else { // STD

                    if (srcsize - offs < cnt)
                        return errOutOfRange;

                    #if defined(DEBUG_VHRLE7B)
                    printf("STD %d\n", cnt);
                    #endif

                    for(uint8_t i=0; i < cnt; i++)
                        if(!putbyte(ptrsrc[offs++]))
                            return errDestMemorySize;
                }

            }

            // Verify total consumed bytes match source size
            if(offs != srcsize)
                return errInternal;

            // Verify uncompressed byte count matches header
            uint32_t produced = dstsize - wrleft;
            if (produced != hdr.srcsize)
                return errInternal;

            // Check CRC32
            uint32_t crc = crc32(ptrdst, hdr.srcsize);
            bool valid = crc == hdr.crc32src;

            // Return CRC verification result
            return valid ? vok : errCRC;
        }

        private:

            /**
             *
             */
            static const uint8_t* get_hdrpfx() {
                    static const uint8_t pfx[8] = {
                        'V', 'H', 'R', 'L', 'E', '7', 'b', ' '
                    };
                    return pfx;
                }

            /**
             *
             */
            size_t calcscnt(const uint8_t *ptr, size_t sz) {
                if(sz < 2) return sz;
                size_t cnt = 1;
                uint8_t sym = *ptr;
                for(size_t i = 1; i <sz; i++) {
                    if(ptr[i] == sym) cnt++;
                    else break; }
                return cnt;
            }

            /**
             *
             */
            uint32_t crc32(const uint8_t *data, size_t len, uint32_t crc = 0xFFFFFFFF) {
                while (len--) {
                    crc ^= *data++;
                    for (int i = 0; i < 8; i++)
                        crc = (crc >> 1) ^ (0xEDB88320 & -(crc & 1));
                }
                return ~crc;
            }

            /**
             *
             */
            bool checkalign(const uint8_t *ptr) {
                return ! (reinterpret_cast<uintptr_t>(ptr) % sizeof(uint32_t));
            }

};
/* ========================[  END FILE CONTENT  ]========================
 * Library          : vhlibrle7b
 * File             : src/vhlibrle7b.hpp
 * Revision         : 0.0.4
 * Content size     : 13142
 * Date / Time      : 12-08-2026 20:36:06
 * MD5              : 39c7a25dde4d7341111f91f9200dfa75
 * Copyright        : © 2026 Viktor Glebov
 * ====================================================================== */