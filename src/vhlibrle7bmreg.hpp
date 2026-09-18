/* ======================================================================================
 * Library       : vhlibrle7b
 * Description   : C++ library implementing a 7-bit Run-Length Encoding (RLE) algorithm
 * Revision      : 0.1.0
 * Source        : https://github.com/vigatron/vhlibrle7b
 * Disclaimer    : Provided "AS IS", without warranty.
 * License       : MIT
 * File          : src/vhlibrle7bmreg.hpp
 * Content size  : 3006
 * Date / Time   : 18-09-2026 21:38:13
 * MD5           : 2777288eef8256717c91a09587b24896
 * Notes         : MD5 = file content without header/footer
 * Encoding      : UTF-8
 * Author        : Viktor Glebov / V01G04A81
 * Copyright     : © 2026 Viktor Glebov
 * ========================[ BEGIN FILE CONTENT ]====================================== */
#pragma once

#include "vhlibrle7binc.hpp"

//
class VHRLE7bMemRegion
{
public:
    /**
     * @brief Default constructor, initializes memory region to null.
     * @note Creates an empty region with no memory pointer.
     */
    VHRLE7bMemRegion() : _ptr(nullptr), pos(0), sz(0) {}

    /**
     * @brief Constructor that initializes region with memory pointer and size.
     * @param pmem Pointer to source/destination memory buffer.
     * @param size Size of the memory region in bytes.
     */
    VHRLE7bMemRegion(uint8_t *pmem, size_t size) : _ptr(pmem), pos(0), sz(size) {}

    /**
     * @brief Get pointer to memory region.
     * @return Pointer to memory region or nullptr if not initialized.
     */
    const uint8_t *ptr() const { return _ptr; }

    /**
     * @brief Get pointer to memory region (non-const version).
     * @return Pointer to writable memory region.
     */
    uint8_t *ptr() { return _ptr; }

    /**
     * @brief Get size of memory region in bytes.
     * @return Size in bytes.
     */
    size_t size() const { return sz; }

    /**
     * @brief Check if a pointer is byte-aligned.
     * @param bcnt Alignment boundary size in bytes (default sizeof(uint32_t)).
     * @return true if aligned, false otherwise.
     */
    bool checkalign(size_t bcnt = sizeof(uint32_t)) const
    {
        return !(reinterpret_cast<uintptr_t>(_ptr) % bcnt);
    }

    /**
     * @brief Read single byte from compressed RLE block.
     * @param pbyte Output buffer pointer to store the byte value.
     * @return Status::vok on success, or appropriate Status error code on failure.
     */
    verr readByte(uint8_t *pbyte)
    {
        // Validate stream boundaries
        if (pos >= sz)
            return verror(VHRLE7BERR::errSrcOutOfRange);

        *pbyte = _ptr[pos++];
        return vok;
    }

    /**
     * @brief Set current position in memory region.
     * @param position New position index in bytes.
     * @return Status::vok on success, or appropriate Status error code on failure.
     */
    verr setpos(size_t position)
    {
        if (position < sz)
        {
            pos = position;
            return vok;
        }
        return verror(VHRLE7BERR::errOutOfRange);
    }

    /**
     * @brief Get current position in memory region.
     * @return Current position index in bytes.
     */
    size_t getpos() const
    {
        return pos;
    }

    /**
     * @brief Write a single byte to memory region.
     * @param v Byte value to write.
     * @return Status::vok on success, or appropriate Status error code on failure.
     */
    verr writebyte(uint8_t v)
    {
        if (!bytesleft())
            return verror(VHRLE7BERR::errIODestination);

        _ptr[pos++] = v;
        return vok;
    };

    /**
     * @brief Get remaining bytes in memory region.
     * @return Remaining bytes count.
     */
    size_t bytesleft() const
    {
        return sz - pos;
    }

private:
    uint8_t *_ptr;
    size_t pos;
    size_t sz;
};
/* ========================[  END FILE CONTENT  ]========================
 * Library          : vhlibrle7b
 * File             : src/vhlibrle7bmreg.hpp
 * Revision         : 0.1.0
 * Content size     : 3006
 * Date / Time      : 18-09-2026 21:38:13
 * MD5              : 2777288eef8256717c91a09587b24896
 * Copyright        : © 2026 Viktor Glebov
 * ====================================================================== */