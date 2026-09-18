#pragma once

#include "vhlibrle7binc.hpp"

//
class VHRLE7bMemRegion
{
public:
    /**
     *
     */
    VHRLE7bMemRegion() : _ptr(nullptr), pos(0), sz(0) {}

    /**
     *
     */
    VHRLE7bMemRegion(uint8_t *pmem, size_t size) : _ptr(pmem), pos(0), sz(size) {}

    /**
     *
     */
    const uint8_t *ptr() const { return _ptr; }

    /**
     * 
     */
    uint8_t *ptr() { return _ptr; }

    /**
     *
     */
    const size_t size() const { return sz; }

    /**
     * @brief Check if a pointer is 4-byte aligned.
     * @param ptr Pointer to check.
     * @return true if aligned, false otherwise.
     */
    bool checkalign(size_t bcnt = sizeof(uint32_t)) const
    {
        return !(reinterpret_cast<uintptr_t>(_ptr) % bcnt);
    }

    /**
     * @brief Read single byte from compressed RLE block
     * @param blk stblockmode reference for source/destination buffers
     * @param pbyte Pointer to output buffer
     * @return Status::vok on success, or appropriate Status error code
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
     * 
     */
    verr setpos(size_t position)
    {
        if(position < sz) {
            pos = position;
            return vok;
        }
        return verror(1);
    }

    /**
     * 
     */
    size_t getpos() const
    {
        return pos;
    }

    /**
     *
     */
    verr writebyte(uint8_t v)
    {
        if (!bytesleft())
            return verror(VHRLE7BERR::errSrcOutOfRange);

        _ptr[pos++] = v;
        return vok;
    };

    /**
     *
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

//
class VHRLE7bMemRegions
{
public:
    /**
     *
     */
    VHRLE7bMemRegions(const VHRLE7bMemRegion &inblk, const VHRLE7bMemRegion &outblk)
    {
        srcmem = inblk;
        dstmem = outblk;
    }

    /**
     *
     */
    const VHRLE7bMemRegion &src() const { return srcmem; }

    /**
     *
     */
    VHRLE7bMemRegion &src() { return srcmem; }

    /**
     *
     */
    const VHRLE7bMemRegion &dst() const { return dstmem; }

    /**
     *
     */
    VHRLE7bMemRegion &dst() { return dstmem; }

private:
    //
    VHRLE7bMemRegion srcmem;

    //
    VHRLE7bMemRegion dstmem;
};
