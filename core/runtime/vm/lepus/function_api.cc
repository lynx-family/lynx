// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "core/runtime/vm/lepus/function_api.h"

#include <string>
#include <utility>

#include "base/include/string/string_number_convert.h"
#include "core/runtime/vm/lepus/array.h"
#include "core/runtime/vm/lepus/vm_context.h"

namespace lynx {
namespace lepus {

static bool ParseStringToInt(const std::string& str, int32_t radix,
                             int64_t& ret) {
  errno = 0;
  char* end_ptr = nullptr;
  ret = std::strtoll(str.c_str(), &end_ptr, radix);

  if (end_ptr == str.c_str() || errno == ERANGE) {
    errno = 0;
    return false;
  }
  return true;
}

static bool ParseStringToDouble(const std::string& str, double& ret) {
  errno = 0;
  char* end_ptr = nullptr;
  ret = std::strtod(str.c_str(), &end_ptr);

  if (end_ptr == str.c_str() || errno == ERANGE) {
    errno = 0;
    return false;
  }
  return true;
}

static void getArrayNumber(const Value* param, std::string& str) {
  if (param->IsArray() && param->Array()->size() > 0) {
    const auto& array0 = param->Array()->get(0);
    if (array0.IsString()) {
      str = array0.StdString();
    } else if (array0.IsNumber()) {
      str = std::to_string(array0.Number());
    } else if (array0.IsBool()) {
      if (array0.IsTrue()) {
        str = "true";
      } else {
        str = "false";
      }
    } else {
      getArrayNumber(&array0, str);
    }
  }
}

static Value ParseInt(VMContext* context) {
  auto params_count = context->GetParamsSize();
  DCHECK(params_count == 1 || params_count == 2);
  int64_t ret = 0;
  int radix = 0;
  if (params_count == 2) {
    radix = static_cast<int32_t>(context->GetParam(1)->Number());
    if (radix < 2 || radix > 36) {
      return Value(true, true);
    }
  }
  if (context->GetParam(0)->IsString()) {
    // Avoid string copy for most common case.
    if (ParseStringToInt(context->GetParam(0)->StdString(), radix, ret)) {
      return Value(ret);
    }
    return Value(true, true);
  }

  std::string str;
  if (context->GetParam(0)->IsNumber()) {
    str = std::to_string(context->GetParam(0)->Number());
  } else if (context->GetParam(0)->IsBool()) {
    if (context->GetParam(0)->IsTrue()) {
      str = "true";
    } else {
      str = "false";
    }
  } else {
    getArrayNumber(context->GetParam(0), str);
  }

  if (ParseStringToInt(str, radix, ret)) {
    return Value(ret);
  }
  return Value(true, true);
}

static Value ParseFloat(VMContext* context) {
  LOGI("lepus::parseFloat");
  auto params_count = context->GetParamsSize();
  DCHECK(params_count == 1);
  double ret = 0;
  if (context->GetParam(0)->IsString()) {
    // Avoid string copy for most common case.
    if (ParseStringToDouble(context->GetParam(0)->StdString(), ret)) {
      if (ret != static_cast<int64_t>(ret)) {
        return Value(ret);
      } else {
        return Value(static_cast<int64_t>(ret));
      }
    }
    return Value(true, true);
  }

  std::string str;
  if (context->GetParam(0)->IsNumber()) {
    str = std::to_string(context->GetParam(0)->Number());
  } else {
    getArrayNumber(context->GetParam(0), str);
  }
  if (ParseStringToDouble(str, ret)) {
    if (ret != static_cast<int64_t>(ret)) {
      return Value(ret);
    } else {
      return Value(static_cast<int64_t>(ret));
    }
  }
  return Value(true, true);
}

static Value IsNan(VMContext* context) {
  auto param_count = context->GetParamsSize();
  DCHECK(param_count == 1);
  return Value(context->GetParam(0)->NaN());
}

static int isURIReserved(int c) {
  return c < 0x100 &&
         memchr(";/?:@&=+$,#", c, sizeof(";/?:@&=+$,#") - 1) != nullptr;
}

static int isURIUnescaped(int c, int isComponent) {
  return c < 0x100 &&
         ((c >= 0x61 && c <= 0x7a) || (c >= 0x41 && c <= 0x5a) ||
          (c >= 0x30 && c <= 0x39) ||
          memchr("-_.!~*'()", c, sizeof("-_.!~*'()") - 1) != nullptr ||
          (!isComponent && isURIReserved(c)));
}

static int encodeURI_hex(std::string& result, int c) {
  uint8_t buf[6];
  int n = 0;
  const char* hex = "0123456789ABCDEF";

  buf[n++] = '%';
  if (c >= 256) {
    buf[n++] = 'u';
    buf[n++] = hex[(c >> 12) & 15];
    buf[n++] = hex[(c >> 8) & 15];
  }
  buf[n++] = hex[(c >> 4) & 15];
  buf[n++] = hex[(c >> 0) & 15];
  for (int i = 0; i < n; i++) {
    result += buf[i];
  }
  return 0;
}

static Value EncodeURIComponent(VMContext* context) {
  auto params_count = context->GetParamsSize();
  DCHECK(params_count == 1);
  const std::string& param = context->GetParam(0)->StdString();

  std::string result;
  result.reserve(param.size() * 3);

  for (size_t i = 0; i < param.size(); i++) {
    int p = static_cast<signed char>(param[i]);
    if (isURIUnescaped(p, 1)) {
      result += param[i];
    } else {
      DCHECK(!(p >= 0xdc00 && p <= 0xdfff));
      if (p >= 0xd800 && p <= 0xdbff) {
        DCHECK(i < param.size());
        int c1 = static_cast<signed char>(param[i]);
        DCHECK(!(p < 0xdc00 || p > 0xdfff));
        p = (((p & 0x3ff) << 10) | (c1 & 0x3ff)) + 0x10000;
      }
      if (p < 0x80) {
        encodeURI_hex(result, p);
      } else {
        /* XXX: use C UTF-8 conversion ? */
        if (p < 0x800) {
          encodeURI_hex(result, (p >> 6) | 0xc0);
        } else {
          if (p < 0x10000) {
            encodeURI_hex(result, (p >> 12) | 0xe0);
          } else {
            encodeURI_hex(result, (p >> 18) | 0xf0);
            encodeURI_hex(result, ((p >> 12) & 0x3f) | 0x80);
          }
          encodeURI_hex(result, ((p >> 6) & 0x3f) | 0x80);
        }
        encodeURI_hex(result, (p & 0x3f) | 0x80);
      }
    }
  }
  return Value(std::move(result));
}

static inline int from_hex(int c) {
  if (c >= '0' && c <= '9')
    return c - '0';
  else if (c >= 'A' && c <= 'F')
    return c - 'A' + 10;
  else if (c >= 'a' && c <= 'f')
    return c - 'a' + 10;
  else
    return -1;
}

static int string_get_hex(const std::string& param, size_t k, size_t n) {
  int c = 0, h;
  while (n-- > 0) {
    if ((h = from_hex(param[k++])) < 0) return -1;
    c = (c << 4) | h;
  }
  return c;
}

static int hex_decode(const std::string& param, size_t k) {
  int c;
  if (k >= param.length() || param[k] != '%') {
    return -1;
  }
  if (k + 2 >= param.length() || (c = string_get_hex(param, k + 1, 2)) < 0) {
    return -1;
  }

  return c;
}

static void unicode_to_utf8(std::string& result, unsigned int c) {
  if (c < 0x80) {
    result += static_cast<char>(c);
  } else {
    if (c < 0x800) {
      result += static_cast<char>((c >> 6) | 0xc0);
    } else {
      if (c < 0x10000) {
        result += static_cast<char>((c >> 12) | 0xe0);
      } else {
        if (c < 0x00200000) {
          result += static_cast<char>((c >> 18) | 0xf0);
        } else {
          if (c < 0x04000000) {
            result += static_cast<char>((c >> 24) | 0xf8);
          } else if (c < 0x80000000) {
            result += static_cast<char>((c >> 30) | 0xfc);
            result += static_cast<char>(((c >> 24) & 0x3f) | 0x80);
          } else {
            result += "";
          }
          result += static_cast<char>(((c >> 18) & 0x3f) | 0x80);
        }
        result += static_cast<char>(((c >> 12) & 0x3f) | 0x80);
      }
      result += static_cast<char>(((c >> 6) & 0x3f) | 0x80);
    }
    result += static_cast<char>((c & 0x3f) | 0x80);
  }
}

static Value DecodeURIComponent(VMContext* context) {
  auto params_count = context->GetParamsSize();
  DCHECK(params_count == 1);
  const std::string& param = context->GetParam(0)->StdString();

  std::string result;
  result.reserve(param.size() * 3);
  int c, c1, n, c_min;
  for (size_t k = 0; k < param.size();) {
    c = static_cast<signed char>(param[k]);
    if (c == '%') {
      c = hex_decode(param, k);
      DCHECK(c >= 0);
      k += 3;
      if (c < 0x80) {
      } else {
        /* Decode URI-encoded UTF-8 sequence */
        if (c >= 0xc0 && c <= 0xdf) {
          n = 1;
          c_min = 0x80;
          c &= 0x1f;
        } else if (c >= 0xe0 && c <= 0xef) {
          n = 2;
          c_min = 0x800;
          c &= 0xf;
        } else if (c >= 0xf0 && c <= 0xf7) {
          n = 3;
          c_min = 0x10000;
          c &= 0x7;
        } else {
          n = 0;
          c_min = 1;
          c = 0;
        }
        while (n-- > 0) {
          c1 = hex_decode(param, k);
          DCHECK(c1 >= 0);
          k += 3;
          if ((c1 & 0xc0) != 0x80) {
            c = 0;
            break;
          }
          c = (c << 6) | (c1 & 0x3f);
        }
        DCHECK(!(c < c_min || c > 0x10FFFF));
      }
    } else {
      k++;
    }

    std::string tmp = "";
    if (c < 0) {
      tmp = static_cast<char>(c);
    } else {
      unicode_to_utf8(tmp, c);
    }
    result += tmp;
  }
  return Value(std::move(result));
}

void RegisterFunctionAPI(Context* ctx) {
  RegisterBuiltinFunction(ctx, "parseInt", &ParseInt);
  RegisterBuiltinFunction(ctx, "parseFloat", &ParseFloat);
  RegisterBuiltinFunction(ctx, "isNaN", &IsNan);
  RegisterBuiltinFunction(ctx, "encodeURIComponent", &EncodeURIComponent);
  RegisterBuiltinFunction(ctx, "decodeURIComponent", &DecodeURIComponent);
}
}  // namespace lepus
}  // namespace lynx
