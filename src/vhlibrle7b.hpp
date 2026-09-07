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
#define verr uint32_t
#define verror(X) (X)
#define vok verror(0)
#endif

// * Callback-oriented API introduced in rev 0.0.5 for minimizing RAM usage

// Callback function template for input data
typedef bool (*CallbackFunc_VHLIBRLE7B_IDATA)(uint8_t *pbv, size_t pos);

// Callback function template for output data
typedef bool (*CallbackFunc_VHLIBRLE7B_ODATA)(uint8_t bv, size_t pos);

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
        size_t srcmax;

        uint8_t *dst;
        size_t dstpos;
        size_t dstmax;
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
        errCheckFailed
    };

    enum ChunkType : uint8_t
    {
        chunkSTD = 0,
        chunkRLE
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
     * @brief Validates a compressed VHRLE7b data block structure and integrity.
     * @param ptrrle Pointer to the input compressed block (including header).
     * @param rleblksize Total size of the compressed block in bytes.
     * @return Status::vok if valid, error code otherwise
     *          (errAlign, errSrcMemorySize, errSrcVersion, errSrcInvalid, errCRC).
     */
    verr checkRLE(
        const uint8_t *ptrrle,
        const uint32_t rleblksize,

        // Single-byte APIs: both should be valid for sbyte mode
        CallbackFunc_VHLIBRLE7B_IDATA getbyte = nullptr,
        CallbackFunc_VHLIBRLE7B_ODATA putbyte = nullptr)
    {

        if (!checkalign(ptrrle))
            return errAlign;

        // Check limit
        if (rleblksize < sizeof(sthdr))
            return errSrcMemorySize;

        sthdr hdr;
        std::memcpy(&hdr, ptrrle, sizeof(sthdr));

        if (!internalCheckHeaderStruct(&hdr))
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
     * @brief Decompresses a VHRLE7b encoded data block and validates CRC32 checksums.
     * @param ptrsrc Pointer to the source compressed data block.
     * @param srcsize Size of the source compressed data block in bytes.
     * @param ptrdst Pointer to the destination output buffer.
     * @param dstsize Maximum capacity of the destination buffer in bytes.
     * @return Status::vok on success, or appropriate Status error code on failure.
     */
    verr unpack(

        // Block mode API
        const uint8_t *pRLEbin,
        const uint32_t srcsize,
        const uint8_t *pBINout,
        const uint32_t dstsize,

        // Single-byte APIs: both should be valid for sbyte mode
        CallbackFunc_VHLIBRLE7B_IDATA getbyte = nullptr,
        CallbackFunc_VHLIBRLE7B_ODATA putbyte = nullptr)
    {
        // Mode: block mode or single-byte ?
        bool modeAPIblock = getbyte == nullptr || putbyte == nullptr;

        verr status;

        if (modeAPIblock)
        {
            status = unpackWithAPIBlockMode(pRLEbin, srcsize, pBINout, dstsize);
        }
        else
        {
            status = unpackWithAPICallback(getbyte, putbyte);
        }

        return status;
    }

    /**
     * @brief
     * @param
     * @param
     * @return
     */
    verr packWithAPICallback(
        CallbackFunc_VHLIBRLE7B_IDATA funcIn,
        CallbackFunc_VHLIBRLE7B_ODATA funcOut)
    {
        return verror(errNotImplemented);
    }

    /**
     * @brief
     * @param
     * @param
     * @return
     */
    verr checkRLESourceWithAPICallback(
        CallbackFunc_VHLIBRLE7B_IDATA funcIn,
        CallbackFunc_VHLIBRLE7B_ODATA funcOut)
    {
        //
        sthdr hdr;

        if (!readHeaderWithAPI(funcIn, funcOut, &hdr))
            return verror(errRleSourceInvalid);

        if (!internalCheckHeaderStruct(&hdr))
            return verror(errInvalidHeader);

        uint32_t crc_rle;
        if (!calcBlockCRC32WithAPI(funcIn, 0, &crc_rle, sizeof(sthdr)))
            return verror(errRleSourceInvalid);

        if (crc_rle != hdr.crc32rle)
            return verror(errInvalidRLESource);

        return verror(errNotImplemented);
    }

    /**
     *
     */
    verr unpackWithAPIBlockMode(
        const uint8_t *pRLEbin,
        const uint32_t srcsize,
        const uint8_t *pDATbin,
        const uint32_t dstsize)
    {
        stblockmode sblk =
            {
                .src = const_cast<uint8_t *>(pRLEbin),
                .srcpos = 0,
                .srcmax = srcsize,

                .dst = const_cast<uint8_t *>(pDATbin),
                .dstpos = 0,
                .dstmax = dstsize};

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
            if (vok != readSourceByte(&sblk, &cbyte))
                return errRleSourceInvalid;

            uint8_t cnt = cbyte & 0x7F;
            if (cnt == 0)
                return errInternal;

            if(!spanscnt)
                asm("nop");

            ChunkType ctype = (ChunkType)(cbyte >> 7);

            if (ctype == chunkRLE)
            {
                if (vok != unpackRLEChunk(&sblk, cnt))
                    return verror(1);
            }
            else
            {
                if (vok != unpackSTDChunk(&sblk, cnt))
                    return verror(1);
            }
        }

        // Verify total consumed bytes match source size
        if (sblk.dstpos != sblk.dstmax)
            return errInternal;

        // Verify uncompressed byte count matches header
        // uint32_t produced = dstsize - wrleft;
        if ( sblk.dstpos != hdr.srcsize)
            return errInternal;

        // Check results CRC32
        uint32_t crc = calcBlockCRC32(sblk.dst, hdr.srcsize);
        bool valid = crc == hdr.crc32src;

        // Return CRC verification result
        return valid ? vok : errCRC;
    }

    /**
     * @brief
     * @param
     * @param
     * @return
     */
    verr unpackWithAPICallback(
        CallbackFunc_VHLIBRLE7B_IDATA funcIn,
        CallbackFunc_VHLIBRLE7B_ODATA funcOut,
        bool checkbefore = true)
    {
        if (!checkRLESourceWithAPICallback(funcIn, funcOut))
            return verr(errCheckFailed);

        //
        sthdr hdr;

        if (!readHeaderWithAPI(funcIn, funcOut, &hdr))
            return verror(errRleSourceInvalid);

        if (!internalCheckHeaderStruct(&hdr))
            return verror(errInvalidHeader);

        for (size_t i = 0; i < sizeof(sthdr); i++)
        {
            uint8_t *rptr = (uint8_t *)&hdr;
            if (!funcIn(rptr + i, i))
                return errRleSourceInvalid;
        }

        // if (!writeByte())
        //     return errDestMemorySize;

        return verror(errNotImplemented);
    }

private:
    /**
     *
     */
    static const uint8_t *get_hdrpfx()
    {
        static const uint8_t pfx[8] = {
            'V', 'H', 'R', 'L', 'E', '7', 'b', ' '};
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
    bool internalCheckHeaderStruct(sthdr *phdr)
    {
        if (phdr->reserved != 0)
            return errSrcVersion;

        // Check pfx
        for (size_t i = 0; i < sizeof(sthdr::pfx); i++)
            if (phdr->pfx[i] != get_hdrpfx()[i])
                return errSrcInvalid;

        return true;
    }

    // *** API for Byte-Reading streams / 0.0.5 ***

    /**
     *
     */
    bool checkCallbacks(
        CallbackFunc_VHLIBRLE7B_IDATA *funcIn,
        CallbackFunc_VHLIBRLE7B_ODATA *funcOut)
    {
        if (funcIn == nullptr)
            return false;
        if (funcOut == nullptr)
            return false;
        return true;
    }

    /**
     *
     */
    bool calcBlockCRC32WithAPI(
        CallbackFunc_VHLIBRLE7B_IDATA funcIn,
        size_t len,
        uint32_t *pdst,
        size_t rdoffset,
        uint32_t crc = 0xFFFFFFFF)
    {
        for (size_t i = 0; i < len; i++)
        {
            uint8_t data;
            if (!funcIn(&data, rdoffset++))
                return false;

            crc ^= data;
            for (int i = 0; i < 8; i++)
                crc = (crc >> 1) ^ (0xEDB88320 & -(crc & 1));
        }

        *pdst = ~crc;
        return true;
    }

    /**
     *
     */
    bool readHeaderWithAPI(
        CallbackFunc_VHLIBRLE7B_IDATA funcIn,
        CallbackFunc_VHLIBRLE7B_ODATA funcOut,
        sthdr *hdr)
    {

        uint8_t *pout = (uint8_t *)hdr;

        for (size_t i = 0; i < sizeof(sthdr); i++)
        {
            bool rd = funcIn(pout + i, i);
            if (!rd)
                return false;
        }

        return true;
    }

    // *** Block I/O

    /**
     *
     */
    verr readSourceByte(stblockmode *pblk, uint8_t *pbyte)
    {

        // Validate stream boundaries
        if (pblk->srcpos >= pblk->srcmax)
            return errOutOfRange;

        *pbyte = pblk->src[pblk->srcpos++];
        return vok;
    }

    /**
     *
     */
    verr unpackRLEChunk(stblockmode *pblk, uint8_t cnt)
    {
        showline_RLE_cnt(cnt);

        uint8_t sym;
        if (vok != readSourceByte(pblk, &sym))
            return errRleSourceInvalid;

        // Enough in writer ?
        if ((pblk->dstpos + cnt) > pblk->dstmax)
            return errDestMemorySize;

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
    verr unpackSTDChunk(stblockmode *pblk, uint8_t cnt)
    {
        showline_STD_cnt(cnt);

        // Enough in reader ?
        if ((pblk->srcpos + cnt) >= pblk->srcmax)
            return errSrcMemorySize;

        // Enough in writer ?
        if ((pblk->dstpos + cnt) >= pblk->dstmax)
            return errDestMemorySize;

        // Unpack STD chunk
        for (uint8_t i = 0; i < cnt; i++)
        {
            pblk->dst[ pblk->dstpos++ ] = pblk->src[pblk->srcpos++];
        }

        return vok;
    }

    // *** SByte I/O

    // *** DBG

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
 * Revision         : 0.0.4
 * Content size     : 13142
 * Date / Time      : 12-08-2026 20:36:06
 * MD5              : 39c7a25dde4d7341111f91f9200dfa75
 * Copyright        : © 2026 Viktor Glebov
 * ====================================================================== */