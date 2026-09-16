/* ======================================================================================
 * Library       : vhlibrle7b
 * Description   : C++ library implementing a 7-bit Run-Length Encoding (RLE) algorithm
 * Revision      : 0.0.5-rc3
 * Source        : https://github.com/vigatron/vhlibrle7b
 * Disclaimer    : Provided "AS IS", without warranty.
 * License       : MIT
 * File          : src/vhlibrle7b.hpp
 * Content size  : 21243
 * Date / Time   : 16-09-2026 19:23:43
 * MD5           : 392e900d5bb4253dec883b71fa55f8e5
 * Notes         : MD5 = file content without header/footer
 * Encoding      : UTF-8
 * Author        : Viktor Glebov / V01G04A81
 * Copyright     : © 2026 Viktor Glebov
 * ========================[ BEGIN FILE CONTENT ]====================================== */
#pragma once

#include "vhlibrle7binc.hpp"
#include "vhlibrle7bstrm.hpp"

/**
 * Embedded version RLE-7-bit
 */

class VHRLE7b
{

public:
    VHRLE7b() = default;

#pragma pack(push, 1)
    struct sthdr
    {
        uint8_t pfx[8];    // Default prefix
        uint32_t spans;    // Spans count field
        uint32_t crc32src; // Source data CRC32
        uint32_t srcsize;  // Source block length
        uint32_t crc32rle; // Destination data CRC32
        uint32_t rlesize;  // Destination block length
        uint32_t reserved; // Reserved
    };
#pragma pack(pop)

    static_assert(sizeof(sthdr) == 32);

    struct stblockmode
    {
        uint8_t *srcptr;
        size_t srcpos;
        size_t srcsz;

        uint8_t *dstptr;
        size_t dstpos;
        size_t dstsz;
    };

    enum Status : uint32_t
    {
        okstat = 0,
        
        
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

    enum ChunkType : uint8_t
    {
        chunkSTD = 0,
        chunkRLE
    };

    /**
     * @brief   Pack data array using `Memory Block mode`
     * @param
     * @param
     * @return
     */
    verr pack(
        const uint8_t *srcptr,
        const uint32_t srcsize,
        uint8_t *dstptr,
        const uint32_t dstsize,
        const uint8_t minRLE,
        const uint8_t maxSIZ)
    {
        return pack_BMode(srcptr, srcsize, dstptr, dstsize, minRLE, maxSIZ);
    }

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
    verr pack_BMode(
        const uint8_t *srcptr,
        const uint32_t srcsize,
        uint8_t *dstptr,
        const uint32_t dstsize,
        const uint8_t minRLE,
        const uint8_t maxSIZ)
    {

        // Check before processing
        if (dstsize < sizeof(sthdr))
            return errDestMemorySize;
        if (minRLE < 4)
            return errSettings;
        if (maxSIZ < 4 || maxSIZ >= 128)
            return errSettings;
        if (!checkalign(srcptr))
            return errAlign;
        if (!checkalign(dstptr))
            return errAlign;

        // Setup writer
        uint8_t *ptrbin = dstptr + sizeof(sthdr);
        uint32_t wrleft = dstsize - sizeof(sthdr);
        uint32_t spans_count = 0;
        uint32_t stdcnt = 0;

        // Safe byte writer lambda
        auto putbyte = [&](uint8_t v) -> bool
        {
            if (wrleft == 0)
                return false;
            wrleft--;
            *ptrbin++ = v;
            return true;
        };

        // RLE span writer lambda
        auto writerle = [&](size_t pos, uint8_t cnt, uint8_t sym) -> bool
        {
#if defined(DEBUG_VHRLE7B)
            printf("Write RLE @ %d  `%d`x%d\n", (int)pos, sym, (int)cnt);
#endif
            if (!putbyte(0x80 | cnt))
                return false;
            if (!putbyte(sym))
                return false;
            spans_count++;
            return true;
        };

        // Literal (STD) span writer lambda
        auto writestd = [&](size_t pos, uint8_t cnt, const uint8_t *pbin) -> bool
        {
#if defined(DEBUG_VHRLE7B)
            printf("Write STD @ %d x%d :", (int)pos, (int)cnt);
#endif
            if (!putbyte(cnt))
                return false;
            for (uint8_t i = 0; i < cnt; i++)
            {
#if defined(DEBUG_VHRLE7B)
                printf(" %d", pbin[i]);
#endif
                if (!putbyte(pbin[i]))
                    return false;
            }
            spans_count++;
#if defined(DEBUG_VHRLE7B)
            printf("\n");
#endif
            return true;
        };

        // Main processing loop
        for (uint32_t i = 0; i < srcsize;)
        {

            uint32_t scnt = calcDubsCount(srcptr + i, srcsize - i);

            if (scnt >= minRLE)
            {

                // Force store STD spans if avail
                while (stdcnt)
                {
                    size_t wrcnt = (stdcnt > maxSIZ) ? maxSIZ : stdcnt;
                    if (!writestd(i - stdcnt, wrcnt, srcptr + (i - stdcnt)))
                        return errWrite;
                    stdcnt -= wrcnt;
                }

                // Store RLE spans
                while (scnt)
                {
                    size_t wrcnt = (scnt > maxSIZ) ? maxSIZ : scnt;
                    if (!writerle(i, wrcnt, srcptr[i]))
                        return errWrite;
                    scnt -= wrcnt;
                    i += wrcnt;
                }
            }
            else
            {

                stdcnt += scnt;
                i += scnt;

                // Store STD
                while (stdcnt >= maxSIZ)
                {
                    if (!writestd(i - stdcnt, maxSIZ, srcptr + (i - stdcnt)))
                        return errWrite;
                    stdcnt -= maxSIZ;
                }

                // EOF ? (i++ already)
                if (i == srcsize)
                {

                    // Force store STD spans if avail
                    while (stdcnt)
                    {
                        size_t wrcnt = (stdcnt > maxSIZ) ? maxSIZ : stdcnt;
                        if (!writestd(i - stdcnt, wrcnt, srcptr + (i - stdcnt)))
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
        hdr.crc32src = calcBlockCRC32(srcptr, srcsize);
        hdr.rlesize = compressed_data_size;
        hdr.crc32rle = calcBlockCRC32(dstptr + sizeof(sthdr), compressed_data_size);
        hdr.reserved = 0;

        // Copy header to destination buffer start
        std::memcpy(dstptr, &hdr, sizeof(sthdr));

        // Return result
        return vok;
    }

    /**
     * @brief   Pack data array using `Stream Mode`
     * @param
     * @param
     * @return
     */
    verr pack_SMode(VHRLE7bStreams &streams)
    {
        return verror(errNotImplemented);
    }

    /**
     *
     */
    verr checkRLE(const uint8_t *ptrrle, const uint32_t rleblksize)
    {
        return checkRLE_BMode(ptrrle, rleblksize);
    }

    /**
     * @brief Validates a compressed VHRLE7b data block structure and integrity.
     * @param ptrrle Pointer to the input compressed block (including header).
     * @param rleblksize Total size of the compressed block in bytes.
     * @return Status::vok if valid, error code otherwise
     *          (errAlign, errSrcMemorySize, errSrcVersion, errSrcInvalid, errCRC).
     */
    verr checkRLE_BMode(const uint8_t *ptrrle, const uint32_t rleblksize)
    {

        if (!checkalign(ptrrle))
            return errAlign;

        // Check limit
        if (rleblksize < sizeof(sthdr))
            return errSrcMemorySize;

        sthdr hdr;
        std::memcpy(&hdr, ptrrle, sizeof(sthdr));

        if (vok != checkHeaderIntegrity(&hdr))
            return verror(errRLEInvalidHeader);

        // Check rlesrc size
        if (rleblksize - sizeof(sthdr) != hdr.rlesize)
            return errSrcMemorySize;

        // Check CRC
        uint32_t crcrle = calcBlockCRC32(ptrrle + sizeof(sthdr), hdr.rlesize);
        bool checkcrc = crcrle == hdr.crc32rle;
        return checkcrc ? vok : errCRC;
    }

    /**
     * @brief Check RLE stream integrity / CRC of RLE data block without header
     * @param funcIn Callback function for reading input data
     * @param funcOut Callback function for writing output data
     * @param phdr Pointer to header structure for validation
     * @return Status::vok if valid, or appropriate error code otherwise
     */
    verr checkRLE_SMode(VHRLE7bStreams &streams, sthdr *phdr)
    {
        uint32_t crc_rle;
        if (!calcCRC32_SIn(streams, sizeof(sthdr), phdr->rlesize, &crc_rle))
            return verror(errRLESourceInvalid);

        if (crc_rle != phdr->crc32rle)
            return verror(errInvalidRLESource);

        return vok;
    }

    /**
     * @brief Decompresses a VHRLE7b encoded data in Block mode API and validates CRC32 checksums.
     * @param ptrsrc Pointer to the source compressed data block.
     * @param srcsize Size of the source compressed data block in bytes.
     * @param ptrdst Pointer to the destination output buffer.
     * @param dstsize Maximum capacity of the destination buffer in bytes.
     * @return Status::vok on success, or appropriate Status error code on failure.
     */
    verr unpack(
        const uint8_t *pRLEbin,
        const uint32_t srcsize,
        const uint8_t *pBINout,
        const uint32_t dstsize)
    {

        stblockmode sblk = {
            .srcptr = const_cast<uint8_t *>(pRLEbin),
            .srcpos = 0,
            .srcsz = srcsize,

            .dstptr = const_cast<uint8_t *>(pBINout),
            .dstpos = 0,
            .dstsz = dstsize};

        return unpack_BMode(sblk);
    }

    /**
     * @brief Decompress RLE block / `memory block` mode
     * @param pRLEbin Pointer to compressed RLE data block
     * @param srcsize Size of source compressed data block in bytes
     * @param pDATbin Pointer to destination output buffer
     * @param dstsize Maximum capacity of destination buffer in bytes
     * @return Status::vok on success, or appropriate error code on failure
     */
    verr unpack_BMode(stblockmode & sblk, bool checkrle = true, bool checkdst = true)
    {
        // Check alignment
        if(vok != checkBlockRegions(sblk))
            return verror(errBlockModeParams);

        const sthdr *phdr = ptrhdr(sblk.srcptr);

        if(!checkHeaderIntegrity(phdr))
            return verror(errRLEInvalidHeader);

        // Not enough output buffer space for decompressed data
        if ( sblk.dstsz < phdr->srcsize)
            return verror(errDestMemorySize);

        // Check RLE CRC
        if(checkrle)
        {
            uint32_t crcrle = calcBlockCRC32( sblk.srcptr + sizeof(sthdr), phdr->rlesize);
            if( crcrle != phdr->crc32rle)
                return verror(errRLECRC);
        }

        sblk.srcpos += sizeof(sthdr);

        uint32_t spanscnt = phdr->spans;

        while (spanscnt--)
        {
            uint8_t cbyte;
            if (vok != readDataByte_BMode(sblk, &cbyte))
                return verror(errRLESourceInvalid);

            uint8_t cnt = cbyte & 0x7F;
            if (cnt == 0)
                return verror(errInternal);

            ChunkType ctype = (ChunkType)(cbyte >> 7);

            if (ctype == chunkRLE)
            {
                if (vok != unpack_RLEChunk_BMode(sblk, cnt))
                    return verror(errIOSource);
            }
            else
            {
                if (vok != unpack_STDChunk_BMode(sblk, cnt))
                    return verror(errIODestination);
            }
        }

        // Verify uncompressed byte count matches header
        if (sblk.dstpos != phdr->srcsize)
            return verror(errInternal);

        // Check results CRC32
        if(checkdst)
        {
            uint32_t crc = calcBlockCRC32(sblk.dstptr, phdr->srcsize);
            if(crc != phdr->crc32src)
                return verror(errDSTCRC);
        }

        return vok;
    }

    /**
     * @brief Unpack RLE data array in `Stream Mode`
     * @param funcIn Callback function for reading input data
     * @param funcOut Callback function for writing output data
     * @param checkbefore Optional flag to check before unpacking (default: true)
     * @return Status::vok on success, or appropriate error code on failure
     */
    verr unpack_SMode(VHRLE7bStreams &streams, bool checkrle = true, bool checkdst = true)
    {
        //
        sthdr hdr;

        // Extract header
        if (readHeader_SMode(streams, &hdr) != vok)
            return verror(errRLESourceInvalid);

        // Check RLE source block first
        if (checkrle)
            if (checkRLE_SMode(streams, &hdr) != vok)
                return verr(errCheckFailed);

        streams.SetRStreamPos(sizeof(sthdr));
        streams.SetWStreamPos(0);

        uint32_t spanscnt = hdr.spans;

        while (spanscnt--)
        {
            uint8_t cbyte;
            if (!streams.readbyte(&cbyte))
                return verror(errRLESourceInvalid);

            uint8_t cnt = cbyte & 0x7F;
            if (cnt == 0)
                return errInternal;

            ChunkType ctype = (ChunkType)(cbyte >> 7);

            if (ctype == chunkRLE)
            {
                if (vok != unpack_RLEChunk_SMode(streams, &hdr, cnt))
                    return verror(errWriteError);
            }
            else
            {
                if (vok != unpack_STDChunk_SMode(streams, &hdr, cnt))
                    return verror(errWriteError);
            }
        }

        if (streams.GetRStreamPos() != (hdr.rlesize + sizeof(sthdr)))
            return verror(errUnpackProcessFailed);

        if (streams.GetWStreamPos() != hdr.srcsize)
            return verror(errUnpackProcessFailed);

        // CRC Check ?
        return vok;
    }

    /**
     *
     */
    const sthdr *ptrhdr(const uint8_t *pbin)
    {
        const sthdr *phdr = (const sthdr *)pbin;
        return phdr;
    }

private:
    /**
     *
     */
    static const uint8_t *get_hdrpfx()
    {
        static const uint8_t pfx[8] = {'V', 'H', 'R', 'L', 'E', '7', 'b', ' '};
        return pfx;
    }

    /**
     *
     */
    size_t calcDubsCount(const uint8_t *ptr, size_t sz)
    {
        if (sz < 2)
            return sz;
        size_t cnt = 1;
        uint8_t sym = *ptr;
        for (size_t i = 1; i < sz; i++)
        {
            if (ptr[i] == sym)
                cnt++;
            else
                break;
        }
        return cnt;
    }

    /**
     *
     */
    uint32_t calcBlockCRC32(
        const uint8_t *data,
        size_t len,
        uint32_t crc = 0xFFFFFFFF)
    {
        while (len--)
        {
            crc ^= *data++;
            for (int i = 0; i < 8; i++)
                crc = (crc >> 1) ^ (0xEDB88320 & -(crc & 1));
        }
        return ~crc;
    }

    /**
     *
     */
    bool checkalign(const uint8_t *ptr)
    {
        return !(reinterpret_cast<uintptr_t>(ptr) % sizeof(uint32_t));
    }

    /**
     * 
     */
    verr checkBlockRegions(const stblockmode & sblk)
    {
        if (sblk.srcsz < sizeof(sthdr))
            return verror(errRLESourceInvalid);

        if (!checkalign(sblk.srcptr))
            return verror(errAlign);

        if (!checkalign(sblk.dstptr))
            return verror(errAlign);

        return vok;
    }

        // // Verify total consumed bytes match source size
        // if (sblk.dstpos != sblk.dstsiz)
        //     return errInternal;

    /**
     *
     */
    verr checkHeaderIntegrity(const sthdr * phdr)
    {
        // Check pfx
        for (size_t i = 0; i < sizeof(sthdr::pfx); i++)
            if (phdr->pfx[i] != get_hdrpfx()[i])
                return verror(errSrcInvalid);

        // version check
        if (phdr->reserved != 0)
            return errSrcVersion;

        // Spans non-zero ?
        if (!phdr->spans)
            return verror(errSrcInvalid);

        // enought src bytes ?
        if (phdr->srcsize < sizeof(sthdr))
            return verror(errSrcInvalid);

        return vok;
    }

    
    /**
     *
     */
    bool calcCRC32_SIn(
        VHRLE7bStreams &streams,
        size_t startoffs,
        size_t bytescount,
        uint32_t *pdst,
        uint32_t crc = 0xFFFFFFFF)
    {
        streams.SetRStreamPos(startoffs);

        for (size_t i = 0; i < bytescount; i++)
        {
            uint8_t data;
            if (!streams.readbyte(&data))
                return false;

            crc ^= data;
            for (int i = 0; i < 8; i++)
                crc = (crc >> 1) ^ (0xEDB88320 & -(crc & 1));
        }

        *pdst = ~crc;
        return true;
    }

    // -----------------------------
    //  BMode I/O
    // -----------------------------


    /**
     *
     */
    verr readDataByte_BMode(stblockmode & blk, uint8_t *pbyte)
    {
        // Validate stream boundaries
        if (blk.srcpos >= blk.srcsz)
            return verror(errOutOfRange);

        *pbyte = blk.srcptr[blk.srcpos++];
        return vok;
    }

    /**
     * @brief Read databyte and store dubs `cnt`
     */
    verr unpack_RLEChunk_BMode(stblockmode & blk, uint8_t cnt)
    {
        showline_RLE_cnt(cnt);

        // Enough in writer ?
        if ((blk.dstpos + cnt) > blk.dstsz)
            return errDestMemorySize;

        uint8_t sym;
        if (vok != readDataByte_BMode(blk, &sym))
            return verror(errRLESourceInvalid);

        // Write sequence
        for (uint8_t i = 0; i < cnt; i++)
        {
            blk.dstptr[blk.dstpos++] = sym;
        }

        return vok;
    }

    /**
     *
     */
    verr unpack_STDChunk_BMode(stblockmode & blk, uint8_t cnt)
    {
        showline_STD_cnt(cnt);

        // Enough in reader ?
        if ((blk.srcpos + cnt) > blk.srcsz)
            return errSrcMemorySize;

        // Enough in writer ?
        if ((blk.dstpos + cnt) > blk.dstsz)
            return errDestMemorySize;

        // Unpack STD chunk
        for (uint8_t i = 0; i < cnt; i++)
        {
            blk.dstptr[blk.dstpos++] = blk.srcptr[blk.srcpos++];
        }

        return vok;
    }

    // ------------------------------------------------------
    // *** SMode I/O API for Byte-Reading streams / 0.0.5 ***
    // ------------------------------------------------------

    /**
     *
     */
    verr readHeader_SMode(VHRLE7bStreams &streams, sthdr *phdr)
    {
        streams.SetRStreamPos(0);

        for (size_t i = 0; i < sizeof(sthdr); i++)
        {
            bool rd = streams.readbyte(((uint8_t *)phdr) + i);
            if (!rd)
                return verror(errRLEInvalidHeader);
        }

        if (checkHeaderIntegrity(phdr) != vok)
            return verror(errRLEInvalidHeader);

        return vok;
    }

    /**
     * @brief Read databyte and store dubs `cnt`
     */
    verr unpack_RLEChunk_SMode(VHRLE7bStreams &streams, sthdr *phdr, uint8_t cnt)
    {
        uint8_t sym;

        showline_RLE_cnt(cnt);

        if (!streams.readbyte(&sym))
            return verror(errRLESourceInvalid);

        for (uint8_t i = 0; i < cnt; i++)
        {
            if (!streams.writebyte(sym, phdr))
                return verror(errWriteError);
        }

        return vok;
    }

    /**
     * @brief Transfer `cnt` databytes from RLE src to dst
     */
    verr unpack_STDChunk_SMode(VHRLE7bStreams &streams, sthdr *phdr, uint8_t cnt)
    {
        uint8_t sym;

        showline_STD_cnt(cnt);

        for (uint8_t i = 0; i < cnt; i++)
        {
            if (!streams.readbyte(&sym))
                return verror(errRLESourceInvalid);

            if (!streams.writebyte(sym, phdr))
                return verror(errWriteError);
        }

        return vok;
    }

    // -----------------------------
    // DBG
    // -----------------------------

    /**
     *
     */
    void showline_RLE_cnt(uint8_t cnt)
    {
#if defined(DEBUG_VHRLE7B)
        printf("RLE %d\n", cnt);
#endif
    }

    /**
     *
     */
    void showline_STD_cnt(uint8_t cnt)
    {
#if defined(DEBUG_VHRLE7B)
        printf("STD %d\n", cnt);
#endif
    }
};

/* ========================[  END FILE CONTENT  ]========================
 * Library          : vhlibrle7b
 * File             : src/vhlibrle7b.hpp
 * Revision         : 0.0.5-rc3
 * Content size     : 21243
 * Date / Time      : 16-09-2026 19:23:43
 * MD5              : 392e900d5bb4253dec883b71fa55f8e5
 * Copyright        : © 2026 Viktor Glebov
 * ====================================================================== */