# vhlibrle7b — Embedded 7-bit RLE Compression Library

[![Revision](https://img.shields.io/badge/revision-0.1.0-blue.svg)](https://github.com/vigatron/vhlibrle7b)
[![License](https://img.shields.io/badge/License-MIT-green.svg)](https://github.com/vigatron/vhlibrle7b/blob/main/LICENSE)
[![Language](https://img.shields.io/badge/C%2B%2B-17%2B-orange.svg)](https://isocpp.org/)

**vhlibrle7b** is a lightweight, header-only C++ library implementing a 7-bit Run-Length Encoding (RLE) algorithm tailored for resource-constrained embedded systems and microcontrollers (e.g., ARM Cortex-M, ESP32, STM32).

It features integrated IEEE 802.3 CRC32 checksums, strict memory bounds checking, and forced 32-bit memory alignment validation (`checkalign`) to prevent hardware fault exceptions on alignment-sensitive architectures.


---

## Library Metadata

* **Repository:** [https://github.com/vigatron/vhlibrle7b](https://github.com/vigatron/vhlibrle7b)
* **Revision:** `0.1.0`
* **Main Header:** `src/vhlibrle7b.hpp`
* **Author:** Viktor Glebov (`V01G04A81`)
* **Copyright:** © 2026 Viktor Glebov
* **License:** [MIT](https://opensource.org/licenses/MIT)

---

## Key Features

* **Header-Only Core:** Core algorithms are header-only, requires 4 additional header files (`vhlibrle7binc.hpp`, `vhlibrle7berrs.hpp`, `vhlibrle7bmem.hpp`, `vhlibrle7bstrm.hpp`) for full functionality.
* **Dual-Mode 7-Bit Encoding:** Dynamically splits data streams into **RLE** (run-length) and **Literal (STD)** spans with minimal control overhead.
* **Integrity Protection:** Computes CRC32 checksums for both uncompressed source data and compressed payload, uses standard polynomial 0xEDB88320.
* **Hardware Safe:** Built-in address alignment checks prevent unaligned memory access crashes on RISC/ARM platforms.
* **Configurable Parameters:** Custom thresholds for minimum sequence run-length (`minRLE`) and maximum span length (`maxRLE`).
* **Endianness Support** Native little-endian byte ordering.

---

## Operation Modes

The library provides two distinct API architectures to fit different embedded constraints:

* **BMode (Block Mode):** Memory Block operation. Available since the initial version. Best for in-RAM compression/decompression where both source and destination buffers are fully allocated and aligned.
* **SMode (Stream Mode):** Byte-per-byte I/O stream. Introduced in **rev 0.0.5**, this callback-oriented API minimizes RAM footprint. Ideal for streaming data directly to/from peripherals (e.g., SPI Flash, SD Card, UART) without buffering the entire payload in RAM. 
* **Note:** Compression (`pack_SMode`) is currently under development (returns `errNotImplemented` in v0.1.0).

## API Reference

### Public Methods

| Method | Description |
|--------|-------------|
| `VHRLE7b::pack()` | Pack data array using Block Mode |
| `VHRLE7b::pack_BMode()` | Pack data using Block Mode API |
| `VHRLE7b::pack_SMode()` | Pack data using Stream Mode API *(Not implemented in v0.1.0)* |
| `VHRLE7b::unpack()` | Unpack data using Block Mode |
| `VHRLE7b::unpack_BMode()` | Unpack data using Block Mode API |
| `VHRLE7b::unpack_SMode()` | Unpack data using Stream Mode API |
| `VHRLE7b::checkRLE()` | Validate compressed data integrity |
| `VHRLE7b::checkRLE_BMode()` | Validate compressed data in Block Mode |
| `VHRLE7b::checkRLE_SMode()` | Validate compressed data in Stream Mode |
| `VHRLE7b::ptrhdr()` | Get pointer to header structure |
| `VHRLE7b::isValidHeader()` | Validate header structure |


### Error Codes

| Error Code | Description |
|------------|-------------|
| `errDestMemorySize` | Destination buffer too small |
| `errSettings` | Invalid parameter constraints |
| `errAlign` | Source or destination pointer not 4-byte aligned |
| `errWrite` | Compressed data exceeded destination buffer bounds |
| `errRLEInvalidHeader` | Invalid header structure |
| `errRLESourceInvalid` | Invalid RLE source data |
| `errRLECRC` | CRC32 checksum mismatch |
| `errDSTCRC` | Destination CRC32 mismatch |
| `errSrcInvalid` | Invalid source data |
| `errSrcVersion` | Reserved field not zero |
| `errInternal` | Internal error |
| `errIOSource` | Read from source failed |
| `errIODestination` | Write to destination failed |


---

## Basic Usage Examples

### Block Mode (BMode) - In-RAM Compression

```cpp
#include "vhlibrle7b.hpp"

int main() {
    uint8_t input[] = {0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42};
    uint8_t output[128];
    uint8_t decompressed[128];
    VHRLE7b rle;
    
    // 1. Pack data into output buffer
    verr err = rle.pack(input, sizeof(input), output, sizeof(output), 4, 127);
    if (err != vok) {
        // Handle error
    }
    
    // Get actual compressed payload size from the generated header
    const VHRLE7b::sthdr *hdr = rle.ptrhdr(output);
    uint32_t total_rle_size = sizeof(VHRLE7b::sthdr) + hdr->rlesize;

    // 2. Unpack data: 
    // Arguments: (compressed_input, compressed_size, destination_buffer, destination_capacity)
    verr unerr = rle.unpack(output, total_rle_size, decompressed, sizeof(decompressed));
    if (unerr != vok) {
        // Handle error
    }
    
    return 0;
}
```

### Stream Mode (SMode) - Low Memory Usage

```cpp
#include "vhlibrle7b.hpp"
#include <stdio.h>

// Example callback function `ReadByte` for stream I/O
bool ReadDataCallback(uint8_t *byte, uint32_t pos) {
    // Read byte from peripheral (e.g. SPI Flash / UART) at position 'pos'
    return true; 
}

// Example callback function `WriteByte` for stream I/O
bool WriteDataCallback(uint8_t byte, uint32_t pos, void * phdr) {
    // Write decompressed byte to target storage at position 'pos'
    return true;
}

int main() {

    VHRLE7b rle;
    
    // Pass read and write callbacks directly to the constructor
    VHRLE7bStreams streams(ReadDataCallback, WriteDataCallback);

    // Unpack data using stream mode (checkrle and checkdst are true by default)
    verr unerr = rle.unpack_SMode(streams);
    if (unerr != vok) {
        // Handle decompression error
    }

    return 0;
}
```

---

## Data Format Specification

Every compressed stream begins with a fixed **32-byte header** (`sthdr`), followed by a sequence of control bytes and payload data.

### Header Layout (`sthdr`)

| Offset | Field | Type | Description |
| :--- | :--- | :--- | :--- |
| `0x00` | `pfx[8]` | `uint8_t[8]` | Magic signature (`"VHRLE7b "`) |
| `0x08` | `spans` | `uint32_t` | Total number of spans in the stream |
| `0x0C` | `crc32src` | `uint32_t` | IEEE 802.3 CRC32 checksum of original uncompressed data |
| `0x10` | `srcsize` | `uint32_t` | Original uncompressed data size in bytes |
| `0x14` | `crc32rle` | `uint32_t` | IEEE 802.3 CRC32 checksum of compressed payload |
| `0x18` | `rlesize` | `uint32_t` | Compressed payload size in bytes (excluding header) |
| `0x1C` | `reserved` | `uint32_t` | Reserved for format versioning (must be `0`) |

### Control Byte Encoding

Each data span begins with a 1-byte control header (`ctrl`):

* **MSB (Bit 7 = `0x80`):** Mode indicator
  * `1` = **RLE Span:** Repeated byte sequence. Followed by 1 byte representing the repeated character value.
  * `0` = **Literal (STD) Span:** Uncompressed byte sequence. Followed by $N$ raw payload bytes.
* **Bits 0–6 (`0x7F`):** Span length $N$ ($1 \le N \le 127$).

---

## Integration & Requirements


### Requirements
* C++17 or higher
* BMode: source and destination buffers must be 32-bit aligned
* SMode: alignment is not required

### Configuration Limits
* **minRLE**: Must be ≥ 4 bytes (minimum repeated sequence length)
* **maxRLE**: Must be in range [4, 127] bytes (maximum span length per byte)
* **minRLE ≤ maxRLE**: Minimum must not exceed maximum

### Debug Mode
To enable verbose `printf` debugging during encoding/decoding, define `DEBUG_VHRLE7B` prior to including the header:

```cpp
#define DEBUG_VHRLE7B
#include "vhlibrle7b.hpp"
```

### License
This project is licensed under the MIT License — see the source headers or the LICENSE file for details.

Copyright (c) 2026 Viktor Glebov / V01G04A81
