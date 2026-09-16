/* ======================================================================================
 * Library       : vhlibrle7b
 * Description   : C++ library implementing a 7-bit Run-Length Encoding (RLE) algorithm
 * Revision      : 0.0.5-rc2
 * Source        : https://github.com/vigatron/vhlibrle7b
 * Disclaimer    : Provided "AS IS", without warranty.
 * License       : MIT
 * File          : src/vhlibrle7b.hpp
 * Content size  : 20412
 * Date / Time   : 16-09-2026 15:00:43
 * MD5           : 1c36cdec5df88d99d7fb933e325f393c
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
        uint8_t *src;
        size_t srcpos;
        size_t srcsiz;

        uint8_t *dst;
        size_t dstpos;
        size_t dstsiz;
    };

    enum Status : uint32_t
    {
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
        errInternal,

        errNotImplemented,
        errInvalidCallback,
        errInvalidHeader,
        errInvalidRLESource,
        errWriteError,
        errUnpackProcessFailed,

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

            uint32_t scnt = calcscnt(srcptr + i, srcsize - i);

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

        if (vok != internalCheckHeaderStruct(&hdr))
            return verror(errInvalidHeader);

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
            return verror(errRleSourceInvalid);

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
        return unpack_BMode(pRLEbin, srcsize, pBINout, dstsize);
    }

    /**
     * @brief Decompress RLE block / `memory block` mode
     * @param pRLEbin Pointer to compressed RLE data block
     * @param srcsize Size of source compressed data block in bytes
     * @param pDATbin Pointer to destination output buffer
     * @param dstsize Maximum capacity of destination buffer in bytes
     * @return Status::vok on success, or appropriate error code on failure
     */
    verr unpack_BMode(
        const uint8_t *pRLEbin,
        const uint32_t srcsize,
        const uint8_t *pDATbin,
        const uint32_t dstsize)
    {
        stblockmode sblk =
            {
                .src = const_cast<uint8_t *>(pRLEbin),
                .srcpos = 0,
                .srcsiz = srcsize,

                .dst = const_cast<uint8_t *>(pDATbin),
                .dstpos = 0,
                .dstsiz = dstsize};

        size_t hdrlen = sizeof(sthdr);

        if (checkRLE(pRLEbin, srcsize) != vok)
            return errRleSourceInvalid;

        if (!checkalign(pDATbin))
            return errAlign;

        if (srcsize < hdrlen)
            return errRleSourceInvalid;

        // Copy RLE header struct
        sthdr hdr;
        std::memcpy(&hdr, pRLEbin, hdrlen);
        sblk.srcpos += hdrlen;

        // Not enough output buffer space for decompressed data
        if (dstsize < hdr.srcsize)
            return errDestMemorySize;

        uint32_t spanscnt = hdr.spans;

        while (spanscnt--)
        {
            uint8_t cbyte;
            if (vok != readDataByte_BMode(&sblk, &cbyte))
                return errRleSourceInvalid;

            uint8_t cnt = cbyte & 0x7F;
            if (cnt == 0)
                return errInternal;

            ChunkType ctype = (ChunkType)(cbyte >> 7);

            if (ctype == chunkRLE)
            {
                if (vok != unpack_RLEChunk_BMode(&sblk, cnt))
                    return verror(errIOSource);
            }
            else
            {
                if (vok != unpack_STDChunk_BMode(&sblk, cnt))
                    return verror(errIODestination);
            }
        }

        // Verify total consumed bytes match source size
        if (sblk.dstpos != sblk.dstsiz)
            return errInternal;

        // Verify uncompressed byte count matches header
        // uint32_t produced = dstsize - wrleft;
        if (sblk.dstpos != hdr.srcsize)
            return errInternal;

        // Check results CRC32
        uint32_t crc = calcBlockCRC32(sblk.dst, hdr.srcsize);
        bool valid = crc == hdr.crc32src;

        // Return CRC verification result
        return valid ? vok : errCRC;
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
            return verror(errRleSourceInvalid);

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
                return errRleSourceInvalid;

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
    size_t calcscnt(const uint8_t *ptr, size_t sz)
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
    verr internalCheckHeaderStruct(sthdr *phdr)
    {
        if (phdr->reserved != 0)
            return errSrcVersion;

        // Check pfx
        for (size_t i = 0; i < sizeof(sthdr::pfx); i++)
            if (phdr->pfx[i] != get_hdrpfx()[i])
                return verror(errSrcInvalid);

        if (!phdr->spans)
            return verror(errSrcInvalid);

        if (!phdr->srcsize)
            return verror(errSrcInvalid);

        return vok;
    }

    // *** API for Byte-Reading streams / 0.0.5 ***

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
            if (! streams.readbyte(&data))
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
    verr readDataByte_BMode(stblockmode *pblk, uint8_t *pbyte)
    {

        // Validate stream boundaries
        if (pblk->srcpos >= pblk->srcsiz)
            return errOutOfRange;

        *pbyte = pblk->src[pblk->srcpos++];
        return vok;
    }

    /**
     * @brief Read databyte and store dubs `cnt`
     */
    verr unpack_RLEChunk_BMode(stblockmode *pblk, uint8_t cnt)
    {
        showline_RLE_cnt(cnt);

        // Enough in writer ?
        if ((pblk->dstpos + cnt) > pblk->dstsiz)
            return errDestMemorySize;

        uint8_t sym;
        if (vok != readDataByte_BMode(pblk, &sym))
            return errRleSourceInvalid;

        // Write sequence
        for (uint8_t i = 0; i < cnt; i++)
        {
            pblk->dst[pblk->dstpos++] = sym;
        }

        return vok;
    }

    /**
     *
     */
    verr unpack_STDChunk_BMode(stblockmode *pblk, uint8_t cnt)
    {
        showline_STD_cnt(cnt);

        // Enough in reader ?
        if ((pblk->srcpos + cnt) > pblk->srcsiz)
            return errSrcMemorySize;

        // Enough in writer ?
        if ((pblk->dstpos + cnt) > pblk->dstsiz)
            return errDestMemorySize;

        // Unpack STD chunk
        for (uint8_t i = 0; i < cnt; i++)
        {
            pblk->dst[pblk->dstpos++] = pblk->src[pblk->srcpos++];
        }

        return vok;
    }

    // -----------------------------
    //  SMode I/O
    // -----------------------------

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
                return verror(errInvalidHeader);
        }

        if (internalCheckHeaderStruct(phdr) != vok)
            return verror(errInvalidHeader);

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
            return verror(errRleSourceInvalid);

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
                return verror(errRleSourceInvalid);

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
 * Revision         : 0.0.5-rc2
 * Content size     : 20412
 * Date / Time      : 16-09-2026 15:00:43
 * MD5              : 1c36cdec5df88d99d7fb933e325f393c
 * Copyright        : © 2026 Viktor Glebov
 * ====================================================================== */