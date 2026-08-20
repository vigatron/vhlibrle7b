<head>
  <title>vhlibrle7b — Embedded 7-bit RLE Compression Library</title>
  <meta name="description" content="Lightweight header-only C++11 library for 7-bit RLE compression on ARM Cortex-M, ESP32, and STM32.">
  <meta property="og:title" content="vhlibrle7b">
  <meta property="og:description" content="Embedded 7-bit RLE Compression Library">
  <meta property="og:image" content="https://raw.githubusercontent.com/vigatron/vhlibrle7b/main/docs/vhlibrle7b_logo_1000x1000_transparent.png">
</p>


# vhlibrle7b — Embedded 7-bit RLE Compression Library

[![Revision](https://img.shields.io/badge/revision-0.0.4-blue.svg)](https://github.com/vigatron/vhlibrle7b)
[![License](https://img.shields.io/badge/License-MIT-green.svg)](https://github.com/vigatron/vhlibrle7b/blob/main/LICENSE)
[![Language](https://img.shields.io/badge/C%2B%2B-11%2B-orange.svg)]()

**vhlibrle7b** is a lightweight, header-only C++ library implementing a 7-bit Run-Length Encoding (RLE) algorithm tailored for resource-constrained embedded systems and microcontrollers (e.g., ARM Cortex-M, ESP32, STM32).

It features integrated IEEE 802.3 CRC32 checksums, strict memory bounds checking, and forced 32-bit memory alignment validation (`checkalign`) to prevent hardware fault exceptions on alignment-sensitive architectures.


---

## Library Metadata

* **Repository:** [https://github.com/vigatron/vhlibrle7b](https://github.com/vigatron/vhlibrle7b)
* **Revision:** `0.0.4`
* **Header Path:** `src/vhlibrle7b.hpp`
* **Author:** Viktor Glebov (`V01G04A81`)
* **Copyright:** © 2026 Viktor Glebov
* **License:** [MIT](https://opensource.org/licenses/MIT)

---

## Key Features

* **Header-Only:** Zero external dependencies beyond standard C++ headers (`<cstdint>`, `<cstddef>`, `<cstring>`).
* **Dual-Mode 7-Bit Encoding:** Dynamically splits data streams into **RLE** (run-length) and **Literal (STD)** spans with minimal control overhead.
* **Integrity Protection:** Computes dual IEEE 802.3 CRC32 checksums for both uncompressed source data and compressed payload.
* **Hardware Safe:** Built-in address alignment checks prevent unaligned memory access crashes on RISC/ARM platforms.
* **Configurable Parameters:** Custom thresholds for minimum sequence run-length (`minRLE`) and maximum span length (`maxSIZ`).

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
* C++11 or higher
* 32-bit aligned memory buffers for source and destination arrays

### Debug Mode
To enable verbose `printf` debugging during encoding/decoding, define `DEBUG_VHRLE7B` prior to including the header:

```cpp
#define DEBUG_VHRLE7B
#include "VHRLE7b.hpp"
```

### License
This project is licensed under the MIT License — see the source headers or the LICENSE file for details.

Copyright (c) 2026 Viktor Glebov / V01G04A81
