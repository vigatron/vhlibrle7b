/* ======================================================================================
 * Library       : vhlibrle7b
 * Description   : C++ library implementing a 7-bit Run-Length Encoding (RLE) algorithm
 * Revision      : 0.0.5-rc2
 * Source        : https://github.com/vigatron/vhlibrle7b
 * Disclaimer    : Provided "AS IS", without warranty.
 * License       : MIT
 * File          : src/vhlibrle7binc.hpp
 * Content size  : 226
 * Date / Time   : 16-09-2026 15:00:43
 * MD5           : 5f4bedee92ae0addb6eac16af1031b63
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

/* ========================[  END FILE CONTENT  ]========================
 * Library          : vhlibrle7b
 * File             : src/vhlibrle7binc.hpp
 * Revision         : 0.0.5-rc2
 * Content size     : 226
 * Date / Time      : 16-09-2026 15:00:43
 * MD5              : 5f4bedee92ae0addb6eac16af1031b63
 * Copyright        : © 2026 Viktor Glebov
 * ====================================================================== */