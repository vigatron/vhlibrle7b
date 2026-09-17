/* ======================================================================================
 * Library       : vhlibrle7b
 * Description   : C++ library implementing a 7-bit Run-Length Encoding (RLE) algorithm
 * Revision      : 0.0.5-rc5
 * Source        : https://github.com/vigatron/vhlibrle7b
 * Disclaimer    : Provided "AS IS", without warranty.
 * License       : MIT
 * File          : src/vhlibrle7binc.hpp
 * Content size  : 218
 * Date / Time   : 17-09-2026 16:17:27
 * MD5           : 308fdf9b15e3b9136c864445e859ac45
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
#define vok 0
#endif

/* ========================[  END FILE CONTENT  ]========================
 * Library          : vhlibrle7b
 * File             : src/vhlibrle7binc.hpp
 * Revision         : 0.0.5-rc5
 * Content size     : 218
 * Date / Time      : 17-09-2026 16:17:27
 * MD5              : 308fdf9b15e3b9136c864445e859ac45
 * Copyright        : © 2026 Viktor Glebov
 * ====================================================================== */