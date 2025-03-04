// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "base/include/log/log_stream.h"

#include <algorithm>
#include <cstdio>
#include <limits>

#include "third_party/rapidjson/internal/dtoa.h"
#include "third_party/rapidjson/internal/itoa.h"

#if defined(OS_WIN)
#include "base/include/string/string_conversion_win.h"
#endif

namespace lynx {
namespace base {
namespace logging {

namespace detail {

constexpr char kDigitHex[] = "0123456789ABCDEF";
constexpr int32_t kMaxNumericSize = 48;
#if defined(__LP64__) || defined(_WIN64)
constexpr int32_t kAddressSignificantBit = 16;
#else
constexpr int32_t kAddressSignificantBit = 8;
#endif

#define TRANSFER_FUNCTION(type, value, buffer)      \
  char* transfer_start = buffer;                    \
  char* transfer_end = type(value, transfer_start); \
  *transfer_end = '\0';                             \
  return static_cast<uint32_t>(transfer_end - transfer_start);

inline int32_t Integer32ToString(int32_t value, char* buffer) {
  TRANSFER_FUNCTION(rapidjson::internal::i32toa, value, buffer);
}

inline int32_t UnsignedInteger32ToString(uint32_t value, char* buffer) {
  TRANSFER_FUNCTION(rapidjson::internal::u32toa, value, buffer);
}

inline int32_t Integer64ToString(int64_t value, char* buffer) {
  TRANSFER_FUNCTION(rapidjson::internal::i64toa, value, buffer);
}

inline int32_t UnsignedInteger64ToString(uint64_t value, char* buffer) {
  TRANSFER_FUNCTION(rapidjson::internal::u64toa, value, buffer);
}

inline int32_t DoubleToString(double value, char* buffer) {
  TRANSFER_FUNCTION(rapidjson::internal::dtoa, value, buffer);
}

void StaticCheck() {
  static_assert(kMaxNumericSize - 10 > std::numeric_limits<double>::digits10,
                "kMaxNumericSize is large enough");
  static_assert(
      kMaxNumericSize - 10 > std::numeric_limits<long double>::digits10,
      "kMaxNumericSize is large enough");
  static_assert(kMaxNumericSize - 10 > std::numeric_limits<long>::digits10,
                "kMaxNumericSize is large enough");
  static_assert(kMaxNumericSize - 10 > std::numeric_limits<int64_t>::digits10,
                "kMaxNumericSize is large enough");
}
}  // namespace detail

// looping convert address type to string as hex
// if null, the output is 0x00000000 in 32bits OS
// and 0x0000000000000000 in 64bits OS
// have a out of bundary check at function operator<<(const void* address)
// have a fixed length output
void ConvertAddressToHexString(char buffer[], uintptr_t value) {
  uintptr_t base = value;
  char* target = buffer;
  int32_t index = detail::kAddressSignificantBit - 1;

  do {
    int32_t least_significant_bit = static_cast<int32_t>(base % 16);
    base /= 16;
    target[index] = detail::kDigitHex[least_significant_bit];
    --index;
  } while (base != 0 && index >= 0);

  // fill zero at higher bits
  while (index >= 0) {
    target[index--] = '0';
  }
}

LogStream& LogStream::operator<<(bool value) {
  return value ? operator<<("true") : operator<<("false");
}

// integer
LogStream& LogStream::operator<<(int8_t value) {
  return operator<<(static_cast<int32_t>(value));
}

LogStream& LogStream::operator<<(uint8_t value) {
  return operator<<(static_cast<uint32_t>(value));
}

LogStream& LogStream::operator<<(int16_t value) {
  return operator<<(static_cast<int32_t>(value));
}

LogStream& LogStream::operator<<(uint16_t value) {
  return operator<<(static_cast<uint32_t>(value));
}

LogStream& LogStream::operator<<(int32_t value) {
  if (buffer_.Available() > detail::kMaxNumericSize) {
    char numeric_container[detail::kMaxNumericSize];
    int32_t length = detail::Integer32ToString(value, numeric_container);
    Append(numeric_container, length);
  }
  return *this;
}

LogStream& LogStream::operator<<(uint32_t value) {
  if (buffer_.Available() > detail::kMaxNumericSize) {
    char numeric_container[detail::kMaxNumericSize];
    int32_t length =
        detail::UnsignedInteger32ToString(value, numeric_container);
    Append(numeric_container, length);
  }
  return *this;
}

LogStream& LogStream::operator<<(int64_t value) {
  if (buffer_.Available() > detail::kMaxNumericSize) {
    char numeric_container[detail::kMaxNumericSize];
    int32_t length = detail::Integer64ToString(value, numeric_container);
    Append(numeric_container, length);
  }
  return *this;
}

LogStream& LogStream::operator<<(uint64_t value) {
  if (buffer_.Available() > detail::kMaxNumericSize) {
    char numeric_container[detail::kMaxNumericSize];
    int32_t length =
        detail::UnsignedInteger64ToString(value, numeric_container);
    Append(numeric_container, length);
  }
  return *this;
}

// convert address to hex string,
// if null, the output is 0x00000000 in 32bits OS
// and 0x0000000000000000 in 64bits OS
// have a fixed length output
LogStream& LogStream::operator<<(const void* address) {
  if (buffer_.Available() > detail::kMaxNumericSize) {
    constexpr int32_t hex_prefix_size = 2;
    constexpr int32_t hex_address_size =
        detail::kAddressSignificantBit + hex_prefix_size;
    char numeric_container[hex_address_size];
    uintptr_t value = reinterpret_cast<uintptr_t>(address);
    numeric_container[0] = '0';
    numeric_container[1] = 'x';
    ConvertAddressToHexString(numeric_container + hex_prefix_size, value);
    Append(numeric_container, hex_address_size);
  }
  return *this;
}

// 6 precision default
LogStream& LogStream::operator<<(float value) {
  if (buffer_.Available() > detail::kMaxNumericSize) {
    char numeric_container[detail::kMaxNumericSize];
    int32_t length =
        snprintf(numeric_container, detail::kMaxNumericSize, "%.6g", value);
    Append(numeric_container, length);
  }
  return *this;
}

LogStream& LogStream::operator<<(double value) {
  if (buffer_.Available() > detail::kMaxNumericSize) {
    char numeric_container[detail::kMaxNumericSize];
    int32_t length = detail::DoubleToString(value, numeric_container);
    Append(numeric_container, length);
  }
  return *this;
}

LogStream& LogStream::operator<<(const char& value) {
  Append(&value, 1);
  return *this;
}

// convert const char * to string
// if null, the output will be truncated
LogStream& LogStream::operator<<(const char* value) {
  if (value) {
    Append(value, strlen(value));
  } else {
    constexpr char terminator = '\0';
    Append(&terminator, 1);
  }
  return *this;
}

LogStream& LogStream::operator<<(const std::string& value) {
  Append(value.c_str(), value.length());
  return *this;
}

LogStream& LogStream::operator<<(const std::string_view& value) {
  Append(value.data(), value.length());
  return *this;
}

// overload for wchar_t, std::wstring
#if defined(OS_WIN)
LogStream& LogStream::operator<<(wchar_t value) {
  return operator<<(Utf8FromUtf16(&value, 1));
}

LogStream& LogStream::operator<<(const wchar_t* value) {
  return operator<<(Utf8FromUtf16(value, wcslen(value)));
}

LogStream& LogStream::operator<<(const std::wstring& value) {
  return operator<<(Utf8FromUtf16(value));
}

LogStream& LogStream::operator<<(const std::wstring_view& value) {
  return operator<<(Utf8FromUtf16(value));
}
#endif

}  // namespace logging
}  // namespace base
}  // namespace lynx
