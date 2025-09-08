// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef BASE_INCLUDE_STRING_QUICKJS_DTOA_H_
#define BASE_INCLUDE_STRING_QUICKJS_DTOA_H_

#include "base/include/base_defines.h"
#include "base/include/base_export.h"

BASE_EXTERN_C_BEGIN

BASE_EXPORT void js_dtoa(char* buf, double val);

BASE_EXTERN_C_END

#endif  // BASE_INCLUDE_STRING_QUICKJS_DTOA_H_
