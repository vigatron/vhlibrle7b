/* ======================================================================================
 * Library       : vhlibrle7b
 * Description   : C++ library implementing a 7-bit Run-Length Encoding (RLE) algorithm
 * Revision      : 0.1.0
 * Source        : https://github.com/vigatron/vhlibrle7b
 * Disclaimer    : Provided "AS IS", without warranty.
 * License       : MIT
 * File          : src/vhlibrle7bstrm.hpp
 * Content size  : 4846
 * Date / Time   : 19-09-2026 20:18:21
 * MD5           : 183e27639800c00ecacd42933e036e5b
 * Notes         : MD5 = file content without header/footer
 * Encoding      : UTF-8
 * Author        : Viktor Glebov / V01G04A81
 * Copyright     : © 2026 Viktor Glebov
 * ========================[ BEGIN FILE CONTENT ]====================================== */
#pragma once

#include "vhlibrle7binc.hpp"

class VHRLE7bStreams
{
public:
    // Callback function template for input data
    typedef bool (*CallbackFunc_VHLIBRLE7B_IDATA)(uint8_t *pbv, size_t rpos);

    // Callback function template for output data
    typedef bool (*CallbackFunc_VHLIBRLE7B_ODATA)(uint8_t bv, size_t wpos, void *phdr);

    // Single-byte API: both funcs should be valid for sbyte mode

    /**
     * @brief Constructs the stream objects and initializes them
     * @param rbyte Callback function for input data
     * @param wbyte Callback function for output data
     * @return void - stream initialization completed
     */
    VHRLE7bStreams(
        CallbackFunc_VHLIBRLE7B_IDATA rbyte,
        CallbackFunc_VHLIBRLE7B_ODATA wbyte) : rd_crc_en(false), wr_crc_en(false)
    {
        getbyte = rbyte;
        putbyte = wbyte;
        SetRStreamPos(0);
        SetWStreamPos(0);
    }

    /**
     * @brief Verifies whether streams are correctly initialized
     * @return bool - true if all callback functions are initialized
     */
    bool isInitialized()
    {
        return checkCallbacks(getbyte, putbyte);
    }

    /**
     * @brief Reads one byte from the stream
     * @param databyte Pointer to buffer for output data
     * @return bool - true on successful read
     */
    bool readbyte(uint8_t *databyte)
    {
        bool rd = getbyte(databyte, rpos);
        if (!rd)
            return false;

        if (rd_crc_en)
            rd_crc = VHRLE7bCRC32::calcStep(rd_crc, *databyte);

        rpos++;

        return true;
    }

    /**
     * @brief Writes one byte to the stream
     * @param databyte Data to write
     * @param phdr Additional data
     * @return bool - true on successful write
     */
    bool writebyte(uint8_t databyte, void *phdr)
    {
        bool wr = putbyte(databyte, wpos, phdr);
        if (!wr)
            return false;

        if(wr_crc_en)
            wr_crc = VHRLE7bCRC32::calcStep(wr_crc, databyte);

        wpos++;
        return true;
    }

    /**
     * @brief Sets the read stream position
     * @param pos Read position
     * @return void - position set
     */
    void SetRStreamPos(size_t pos) { rpos = pos; }

    /**
     * @brief Sets the write stream position
     * @param pos Write position
     * @return void - position set
     */
    void SetWStreamPos(size_t pos) { wpos = pos; }

    /**
     * @brief Returns the read stream position
     * @return size_t - current read position
     */
    size_t GetRStreamPos() { return rpos; }

    /**
     * @brief Returns the write stream position
     * @return size_t - current write position
     */
    size_t GetWStreamPos() { return wpos; }

    /**
     * @brief Resets the read stream CRC to default polynomial value
     * @return void - CRC reset completed
     */
    void ResetRdCRC()
    {
        rd_crc = VHRLE7bCRC32::DEF_POLY_VAL;
    }

    /**
     * @brief Enables or disables CRC validation for read operations
     * @param flag Enable (true) or disable (false) CRC checking
     * @return void - CRC setting updated
     */
    void EnableRdCRC(bool flag)
    {
        rd_crc_en = flag;
    }

    /**
     * @brief Checks if read CRC matches expected value
     * @param crc Received CRC value to validate
     * @return bool - true if CRC validation passed
     */
    bool CheckRdCRC(uint32_t crc)
    {
        return crc == ~rd_crc;
    }

    /**
     * @brief Resets the write stream CRC to default polynomial value
     * @return void - CRC reset completed
     */
    void ResetWrCRC()
    {
        wr_crc = VHRLE7bCRC32::DEF_POLY_VAL;
    }

    /**
     * @brief Enables or disables CRC validation for write operations
     * @param flag Enable (true) or disable (false) CRC checking
     * @return void - CRC setting updated
     */
    void EnableWrCRC(bool flag)
    {
        wr_crc_en = flag;
    }

    /**
     * @brief Checks if write CRC matches expected value
     * @param crc Received CRC value to validate
     * @return bool - true if CRC validation passed
     */
    bool CheckWrCRC(uint32_t crc)
    {
        return crc == ~wr_crc;
    }

private:
    //
    CallbackFunc_VHLIBRLE7B_IDATA getbyte;
    CallbackFunc_VHLIBRLE7B_ODATA putbyte;

    size_t rpos;
    size_t wpos;

    uint32_t rd_crc;
    bool rd_crc_en;

    uint32_t wr_crc;
    bool wr_crc_en;

    /**
     * @brief Validates callback function pointers
     * @param funcIn Input function pointer
     * @param funcOut Output function pointer
     * @return bool - true if functions are valid
     */
    bool checkCallbacks(
        CallbackFunc_VHLIBRLE7B_IDATA funcIn,
        CallbackFunc_VHLIBRLE7B_ODATA funcOut)
    {
        if (funcIn == nullptr)
            return false;
        if (funcOut == nullptr)
            return false;
        return true;
    }
};
/* ========================[  END FILE CONTENT  ]========================
 * Library          : vhlibrle7b
 * File             : src/vhlibrle7bstrm.hpp
 * Revision         : 0.1.0
 * Content size     : 4846
 * Date / Time      : 19-09-2026 20:18:21
 * MD5              : 183e27639800c00ecacd42933e036e5b
 * Copyright        : © 2026 Viktor Glebov
 * ====================================================================== */