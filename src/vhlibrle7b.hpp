/* ======================================================================================
 * Library       : vhlibrle7b
 * Description   : C++ library implementing a 7-bit Run-Length Encoding (RLE) algorithm
 * Revision      : 0.0.5-rc5
 * Source        : https://github.com/vigatron/vhlibrle7b
 * Disclaimer    : Provided "AS IS", without warranty.
 * License       : MIT
 * File          : src/vhlibrle7b.hpp
 * Content size  : 25338
 * Date / Time   : 17-09-2026 16:17:27
 * MD5           : 2faf44289a11fd464e8f2ed6018f431f
 * Notes         : MD5 = file content without header/footer
 * Encoding      : UTF-8
 * Author        : Viktor Glebov / V01G04A81
 * Copyright     : © 2026 Viktor Glebov
 * ========================[ BEGIN FILE CONTENT ]====================================== */
#pragma once

#include "vhlibrle7binc.hpp"
#include "vhlibrle7berrs.hpp"
#include "vhlibrle7bmem.hpp"
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

    enum ChunkType : uint8_t
    {
        chunkSTD = 0,
        chunkRLE
    };

    /**
     * @brief Pack data array using `Memory Block mode`
     * @param srcptr Pointer to the raw input data buffer (must be 32-bit aligned).
     * @param srcsize Size of the raw input data in bytes.
     * @param dstptr Pointer to the output destination buffer (must be 32-bit aligned).
     * @param dstsize Total capacity of the destination buffer in bytes.
     * @param minRLE Minimum repeating sequence length to trigger RLE encoding (must be >= 4).
     * @param maxSIZ Maximum allowed span size in bytes (must be in range [4, 127]).
     * @return Status::vok on success, or appropriate Status error code on failure.
     */
    verr pack(
        uint8_t *srcptr,
        uint32_t srcsize,
        uint8_t *dstptr,
        uint32_t dstsize,
        uint8_t minRLE,
        uint8_t maxRLE)
    {
        //
        VHRLE7bMemRegion inpmem(srcptr, srcsize);
        VHRLE7bMemRegion outmem(dstptr, dstsize);
        VHRLE7bMemRegions regions(inpmem, outmem);
        return pack_BMode(regions, minRLE, maxRLE);
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
     * @param[in]  maxRLE   Maximum allowed span size in bytes (must be in range [4, 127]).
     *
     * @return Status::vok on success, or an appropriate Status error code on failure:
     *         - errDestMemorySize : Destination buffer is too small to fit the header.
     *         - errSettings       : Invalid parameter constraints (minRLE < 4, or maxSIZ outside [4, 127]).
     *         - errAlign          : Source or destination pointer is not 4-byte aligned.
     *         - errWrite          : Compressed data exceeded destination buffer bounds.
     */
    verr pack_BMode(
        VHRLE7bMemRegions &regions,
        uint8_t minRLE,
        uint8_t maxRLE)
    {
        // Check before processing
        if (regions.dst().size() < sizeof(sthdr))
            return verror(VHRLE7BERR::errDestMemorySize);

        // Check pack parameters
        if (minRLE > maxRLE)
            return verror(VHRLE7BERR::errSettings);
        if (minRLE < 4 || minRLE > 127)
            return verror(VHRLE7BERR::errSettings);
        if (maxRLE < 4 || maxRLE > 127)
            return verror(VHRLE7BERR::errSettings);

        // Check align
        if (!regions.src().checkalign())
            return verror(VHRLE7BERR::errAlign);
        if (!regions.dst().checkalign())
            return verror(VHRLE7BERR::errAlign);

        // Setup writer
        if (vok != regions.dst().setpos(sizeof(sthdr)))
            return verror(VHRLE7BERR::errDestMemorySize);

        uint32_t spans_count = 0;
        uint32_t stdcnt = 0;

        // Main processing loop
        for (uint32_t i = 0; i < regions.src().size();)
        {
            uint32_t scnt = calcDubsCount(regions.src().ptr() + i, regions.src().size() - i);

            if (scnt >= minRLE)
            {
                // Force store STD spans if avail
                while (stdcnt)
                {
                    size_t wrcnt = (stdcnt > maxRLE) ? maxRLE : stdcnt;
                    if (vok != writestd(regions.dst(), i - stdcnt, wrcnt, regions.src().ptr() + (i - stdcnt)))
                        return verror(VHRLE7BERR::errWrite);
                    spans_count++;
                    stdcnt -= wrcnt;
                }

                // Store RLE spans
                while (scnt)
                {
                    size_t wrcnt = (scnt > maxRLE) ? maxRLE : scnt;
                    if (vok != writerle(regions.dst(), i, wrcnt, regions.src().ptr()[i]))
                        return verror(VHRLE7BERR::errWrite);
                    spans_count++;
                    scnt -= wrcnt;
                    i += wrcnt;
                }
            }
            else
            {
                stdcnt += scnt;
                i += scnt;

                // Store STD
                while (stdcnt >= maxRLE)
                {
                    if (vok != writestd(regions.dst(), i - stdcnt, maxRLE, regions.src().ptr() + (i - stdcnt)))
                        return verror(VHRLE7BERR::errWrite);
                    spans_count++;
                    stdcnt -= maxRLE;
                }

                // EOF ? (i++ already)
                if (i == regions.src().size())
                {
                    // Force store STD spans if avail
                    while (stdcnt)
                    {
                        size_t wrcnt = (stdcnt > maxRLE) ? maxRLE : stdcnt;
                        if (vok != writestd(regions.dst(), i - stdcnt, wrcnt, regions.src().ptr() + (i - stdcnt)))
                            return verror(VHRLE7BERR::errWrite);
                        spans_count++;
                        stdcnt -= wrcnt;
                    }
                }
            }
        }

        // Compute payload size
        uint32_t compressed_data_size = (regions.dst().size() - sizeof(sthdr)) - regions.dst().bytesleft();

        // Copy header to destination buffer start
        sthdr hdr;
        std::memcpy(hdr.pfx, get_hdrpfx(), sizeof(hdr.pfx));
        hdr.spans = spans_count;
        hdr.srcsize = regions.src().size();
        hdr.crc32src = calcBlockCRC32(regions.src().ptr(), hdr.srcsize);
        hdr.rlesize = compressed_data_size;
        hdr.crc32rle = calcBlockCRC32(regions.dst().ptr() + sizeof(sthdr), compressed_data_size);
        hdr.reserved = 0;

        // Copy header to destination buffer start
        std::memcpy(regions.dst().ptr(), &hdr, sizeof(sthdr));

        // Return result
        return vok;
    }

    /**
     * @brief Pack data array using `Stream Mode`
     * @param streams VHRLE7bStreams reference for stream operations.
     * @return Status::errNotImplemented (currently not implemented).
     */
    verr pack_SMode(VHRLE7bStreams &streams)
    {
        return verror(VHRLE7BERR::errNotImplemented);
    }

    /**
     * @brief Validates a compressed VHRLE7b data block structure and integrity.
     * @param ptrrle Pointer to the input compressed block (including header).
     * @param rleblksize Total size of the compressed block in bytes.
     * @return Status::vok if valid, error code otherwise.
     */
    verr checkRLE(uint8_t *ptrrle, const uint32_t rleblksize)
    {
        VHRLE7bMemRegion mem(ptrrle, rleblksize);
        return checkRLE_BMode(mem);
    }

    /**
     * @brief Validates a compressed VHRLE7b data block structure and integrity.
     * @param ptrrle Pointer to the input compressed block (including header).
     * @param rleblksize Total size of the compressed block in bytes.
     * @return Status::vok if valid, error code otherwise
     *          (errAlign, errSrcMemorySize, errSrcVersion, errSrcInvalid, errCRC).
     */
    verr checkRLE_BMode(VHRLE7bMemRegion &mem)
    {
        if (!mem.checkalign())
            return VHRLE7BERR::errAlign;

        // Check limit
        if (mem.size() < sizeof(sthdr))
            return VHRLE7BERR::errSrcMemorySize;

        const VHRLE7b::sthdr *phdr = (VHRLE7b::sthdr *)mem.ptr();

        if (isValidHeader(phdr) != vok)
            return verror(VHRLE7BERR::errRLEInvalidHeader);

        // Check rlesrc size
        if (mem.size() - sizeof(sthdr) != phdr->rlesize)
            return verror(VHRLE7BERR::errSrcMemorySize);

        // Check CRC
        uint32_t crcrle = calcBlockCRC32(mem.ptr() + sizeof(sthdr), phdr->rlesize);
        bool checkcrc = crcrle == phdr->crc32rle;
        return checkcrc ? vok : verror(VHRLE7BERR::errCRC);
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
            return verror(VHRLE7BERR::errRLESourceInvalid);

        if (crc_rle != phdr->crc32rle)
            return verror(VHRLE7BERR::errInvalidRLESource);

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
        uint8_t *pRLEbin,
        uint32_t rlesize,
        uint8_t *pBINout,
        uint32_t dstsize)
    {

        VHRLE7bMemRegion inpmem(pRLEbin, rlesize);
        VHRLE7bMemRegion outmem(pBINout, dstsize);
        VHRLE7bMemRegions memregions(inpmem, outmem);

        return unpack_BMode(memregions);
    }

    /**
     * @brief Decompress RLE block / `memory block` mode
     * @param pRLEbin Pointer to compressed RLE data block
     * @param srcsize Size of source compressed data block in bytes
     * @param pDATbin Pointer to destination output buffer
     * @param dstsize Maximum capacity of destination buffer in bytes
     * @return Status::vok on success, or appropriate error code on failure
     */
    verr unpack_BMode(VHRLE7bMemRegions &mem, bool checkrle = true, bool checkdst = true)
    {
        // Check alignment
        if (vok != checkBlockRegions(mem))
            return verror(VHRLE7BERR::errBlockModeParams);

        const sthdr *phdr = ptrhdr(mem.src().ptr());

        if (isValidHeader(phdr) != vok)
            return verror(VHRLE7BERR::errRLEInvalidHeader);

        // Check RLE source block size
        if (mem.src().size() - sizeof(sthdr) != phdr->rlesize)
            return verror(VHRLE7BERR::errSrcInvalid);

        // Not enough output buffer space for decompressed data
        if (mem.dst().size() < phdr->srcsize)
            return verror(VHRLE7BERR::errDestMemorySize);

        // Check RLE CRC
        if (checkrle)
        {
            uint32_t crcrle = calcBlockCRC32(mem.src().ptr() + sizeof(sthdr), phdr->rlesize);
            if (crcrle != phdr->crc32rle)
                return verror(VHRLE7BERR::errRLECRC);
        }

        // Set reader pos
        if (vok != mem.src().setpos(sizeof(sthdr)))
            return verror(VHRLE7BERR::errSrcMemorySize);

        uint32_t spanscnt = phdr->spans;

        while (spanscnt--)
        {
            uint8_t cbyte;
            if (vok != mem.src().readByte(&cbyte))
                return verror(VHRLE7BERR::errRLESourceInvalid);

            uint8_t cnt = cbyte & 0x7F;
            if (cnt == 0)
                return verror(VHRLE7BERR::errInternal);

            ChunkType ctype = (ChunkType)(cbyte >> 7);

            if (ctype == chunkRLE)
            {
                if (vok != unpack_RLEChunk_BMode(mem, cnt))
                    return verror(VHRLE7BERR::errIOSource);
            }
            else
            {
                if (vok != unpack_STDChunk_BMode(mem, cnt))
                    return verror(VHRLE7BERR::errIODestination);
            }
        }

        // Verify uncompressed byte count matches header
        if (mem.dst().getpos() != phdr->srcsize)
            return verror(VHRLE7BERR::errInternal);

        // Check results CRC32
        if (checkdst)
        {
            uint32_t crc = calcBlockCRC32(mem.dst().ptr(), phdr->srcsize);
            if (crc != phdr->crc32src)
                return verror(VHRLE7BERR::errDSTCRC);
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
        // Callbacks initialized ?
        if (!streams.isInitialized())
            return verror(VHRLE7BERR::errInternal);

        //
        sthdr hdr;

        // Extract header
        if (readHeader_SMode(streams, &hdr) != vok)
            return verror(VHRLE7BERR::errRLESourceInvalid);

        // Check RLE source block first
        if (checkrle)
        {
            if (checkRLE_SMode(streams, &hdr) != vok)
                return verror(VHRLE7BERR::errCheckFailed);
        }

        streams.SetRStreamPos(sizeof(sthdr));
        streams.SetWStreamPos(0);

        uint32_t spanscnt = hdr.spans;

        while (spanscnt--)
        {
            uint8_t cbyte;
            if (!streams.readbyte(&cbyte))
                return verror(VHRLE7BERR::errRLESourceInvalid);

            uint8_t cnt = cbyte & 0x7F;
            if (cnt == 0)
                return VHRLE7BERR::errInternal;

            ChunkType ctype = (ChunkType)(cbyte >> 7);

            if (ctype == chunkRLE)
            {
                if (vok != unpack_RLEChunk_SMode(streams, &hdr, cnt))
                    return verror(VHRLE7BERR::errWriteError);
            }
            else
            {
                if (vok != unpack_STDChunk_SMode(streams, &hdr, cnt))
                    return verror(VHRLE7BERR::errWriteError);
            }
        }

        if (streams.GetRStreamPos() != (hdr.rlesize + sizeof(sthdr)))
            return verror(VHRLE7BERR::errUnpackProcessFailed);

        if (streams.GetWStreamPos() != hdr.srcsize)
            return verror(VHRLE7BERR::errUnpackProcessFailed);

        // CRC Check ?
        return vok;
    }

    /**
     * @brief Get pointer to header structure from compressed block
     * @param pbin Pointer to compressed data block
     * @return Pointer to sthdr structure
     */
    const sthdr *ptrhdr(const uint8_t *pbin)
    {
        const sthdr *phdr = (const sthdr *)pbin;
        return phdr;
    }

    /**
     * @brief Validates VHRLE7b header structure
     * @param phdr Pointer to header structure
     * @return Status::vok if valid, or appropriate Status error code
     */
    verr isValidHeader(const sthdr *phdr)
    {
        // Check pfx
        for (size_t i = 0; i < sizeof(sthdr::pfx); i++)
            if (phdr->pfx[i] != get_hdrpfx()[i])
                return verror(VHRLE7BERR::errSrcInvalid);

        // version check
        if (phdr->reserved != 0)
            return verror(VHRLE7BERR::errSrcVersion);

        // Spans non-zero ?
        if (!phdr->spans)
            return verror(VHRLE7BERR::errSrcInvalid);

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
     * @brief Calculate duplicate count starting from pointer
     * @param ptr Pointer to the start of the byte sequence
     * @param sz Size of the sequence in bytes
     * @return Number of consecutive identical bytes
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
     * @brief Calculate CRC32 checksum for a block of data.
     * @param data Pointer to the data buffer.
     * @param len Size of the data buffer in bytes.
     * @param crc Initial CRC value (default: 0xFFFFFFFF).
     * @return Calculated CRC32 value.
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
     * @brief Validate block mode parameters (alignment and size)
     * @param sblk stblockmode reference for source/destination buffers
     * @return Status::vok if valid, or appropriate Status error code
     */
    verr checkBlockRegions(const VHRLE7bMemRegions &regions)
    {
        if (regions.src().size() < sizeof(sthdr))
            return verror(VHRLE7BERR::errRLESourceInvalid);

        if (!regions.src().checkalign())
            return verror(VHRLE7BERR::errAlign);

        if (!regions.dst().checkalign())
            return verror(VHRLE7BERR::errAlign);

        return vok;
    }

    /**
     * @brief Calculate CRC32 checksum for stream data
     * @param streams VHRLE7bStreams reference for stream operations
     * @param startoffs Start offset in stream for CRC calculation
     * @param bytescount Number of bytes to calculate CRC for
     * @param pdst Pointer to output buffer for CRC value
     * @param crc Initial CRC value (default: 0xFFFFFFFF)
     * @return true on success, false otherwise
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
     * RLE span writer
     */
    verr writerle(VHRLE7bMemRegion &mem, size_t pos, uint8_t cnt, uint8_t sym)
    {
#if defined(DEBUG_VHRLE7B)
        printf("Write RLE @ %d  `%d`x%d\n", (int)pos, sym, (int)cnt);
#endif
        if (vok != mem.writebyte(0x80 | cnt))
            return verror(1);

        if (vok != mem.writebyte(sym))
            return verror(2);

        return vok;
    };

    /**
     * Literal (STD) span writer
     */
    verr writestd(VHRLE7bMemRegion &mem, size_t pos, uint8_t cnt, const uint8_t *pbin)
    {
#if defined(DEBUG_VHRLE7B)
        printf("Write STD @ %d x%d :", (int)pos, (int)cnt);
#endif
        if (vok != mem.writebyte(cnt))
            return verror(1);

        for (uint8_t i = 0; i < cnt; i++)
        {
#if defined(DEBUG_VHRLE7B)
            printf(" %d", pbin[i]);
#endif
            if (vok != mem.writebyte(pbin[i]))
                return verror(2);
        }

#if defined(DEBUG_VHRLE7B)
        printf("\n");
#endif
        return vok;
    };

    /**
     * @brief Unpack RLE chunk with count `cnt` into destination buffer
     * @param blk stblockmode reference for source/destination buffers
     * @param cnt Number of bytes to copy from the RLE symbol
     * @return Status::vok on success, or appropriate error code on failure
     */
    verr unpack_RLEChunk_BMode(VHRLE7bMemRegions &mem, uint8_t cnt)
    {
        showline_RLE_cnt(cnt);

        uint8_t sym;
        if (vok != mem.src().readByte(&sym))
            return verror(VHRLE7BERR::errRLESourceInvalid);

        // Enough in writer ?
        if (mem.dst().bytesleft() < cnt)
            return verror(VHRLE7BERR::errDestMemorySize);

        // Write sequence
        for (uint8_t i = 0; i < cnt; i++)
        {
            mem.dst().writebyte(sym);
        }

        return vok;
    }

    /**
     * @brief Unpack STD (Literal) chunk with count `cnt` into destination buffer
     * @param blk stblockmode reference for source/destination buffers
     * @param cnt Number of bytes to copy from source directly
     * @return Status::vok on success, or appropriate error code on failure
     */
    verr unpack_STDChunk_BMode(VHRLE7bMemRegions &mem, uint8_t cnt)
    {
        showline_STD_cnt(cnt);

        // Enough in reader ?
        if (mem.src().bytesleft() < cnt)
            return verror(VHRLE7BERR::errSrcMemorySize);

        // Enough in writer ?
        if (mem.dst().bytesleft() < cnt)
            return verror(VHRLE7BERR::errDestMemorySize);

        uint8_t sym;

        // Unpack STD chunk
        for (uint8_t i = 0; i < cnt; i++)
        {
            if(vok != mem.src().readByte(&sym))
                return verror(VHRLE7BERR::errSrcMemorySize);
            
            if(vok != mem.dst().writebyte(sym))
                return verror(VHRLE7BERR::errDestMemorySize);
        }

        return vok;
    }

    // ------------------------------------------------------
    // *** SMode I/O API for Byte-Reading streams / 0.0.5 ***
    // ------------------------------------------------------

    /**
     * @brief Read header structure from VHRLE7b stream
     * @param streams VHRLE7bStreams reference for stream operations
     * @param phdr Pointer to header structure for output
     * @return Status::vok on success, or appropriate Status error code
     */
    verr readHeader_SMode(VHRLE7bStreams &streams, sthdr *phdr)
    {
        streams.SetRStreamPos(0);

        for (size_t i = 0; i < sizeof(sthdr); i++)
        {
            bool rd = streams.readbyte(((uint8_t *)phdr) + i);
            if (!rd)
                return verror(VHRLE7BERR::errRLEInvalidHeader);
        }

        if (isValidHeader(phdr) != vok)
            return verror(VHRLE7BERR::errRLEInvalidHeader);

        return vok;
    }

    /**
     * @brief Unpack RLE chunk with count `cnt` into destination buffer
     * @param streams VHRLE7bStreams reference for stream operations
     * @param phdr Pointer to header structure
     * @param cnt Number of bytes to copy from the RLE symbol
     * @return Status::vok on success, or appropriate Status error code
     */
    verr unpack_RLEChunk_SMode(VHRLE7bStreams &streams, sthdr *phdr, uint8_t cnt)
    {
        uint8_t sym;

        showline_RLE_cnt(cnt);

        if (!streams.readbyte(&sym))
            return verror(VHRLE7BERR::errRLESourceInvalid);

        for (uint8_t i = 0; i < cnt; i++)
        {
            if (!streams.writebyte(sym, phdr))
                return verror(VHRLE7BERR::errWriteError);
        }

        return vok;
    }

    /**
     * @brief Unpack STD (Literal) chunk with count `cnt` into destination buffer
     * @param streams VHRLE7bStreams reference for stream operations
     * @param phdr Pointer to header structure
     * @param cnt Number of bytes to copy from source directly
     * @return Status::vok on success, or appropriate Status error code
     */
    verr unpack_STDChunk_SMode(VHRLE7bStreams &streams, sthdr *phdr, uint8_t cnt)
    {
        uint8_t sym;

        showline_STD_cnt(cnt);

        for (uint8_t i = 0; i < cnt; i++)
        {
            if (!streams.readbyte(&sym))
                return verror(VHRLE7BERR::errRLESourceInvalid);

            if (!streams.writebyte(sym, phdr))
                return verror(VHRLE7BERR::errWriteError);
        }

        return vok;
    }

    // -----------------------------
    // DBG
    // -----------------------------

    /**
     * @brief Debug: Print RLE count to console
     * @param cnt Number of bytes in RLE chunk
     */
    void showline_RLE_cnt(uint8_t cnt)
    {
#if defined(DEBUG_VHRLE7B)
        printf("RLE %d\n", cnt);
#endif
    }

    /**
     * @brief Debug: Print STD count to console
     * @param cnt Number of bytes in RLE chunk
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
 * Revision         : 0.0.5-rc5
 * Content size     : 25338
 * Date / Time      : 17-09-2026 16:17:27
 * MD5              : 2faf44289a11fd464e8f2ed6018f431f
 * Copyright        : © 2026 Viktor Glebov
 * ====================================================================== */