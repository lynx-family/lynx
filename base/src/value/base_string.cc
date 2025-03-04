// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "base/include/value/base_string.h"

#include <cstring>
#include <iostream>

#include "base/include/cast_util.h"
#include "base/include/string/string_utils.h"

namespace lynx {
namespace base {

RefCountedStringImpl RefCountedStringImpl::Unsafe::kEmptyString("", 0);

RefCountedStringImpl& RefCountedStringImpl::Unsafe::kTrueString() {
  static RefCountedStringImpl kTrueString(
      "true", std::char_traits<char>::length("true"));
  return kTrueString;
}

RefCountedStringImpl& RefCountedStringImpl::Unsafe::kFalseString() {
  static RefCountedStringImpl kFalseString(
      "false", std::char_traits<char>::length("false"));
  return kFalseString;
}

RefCountedStringImpl::RefCountedStringImpl(const char* str)
    : RefCountedStringImpl(str, str == nullptr ? 0 : std::strlen(str)) {}

RefCountedStringImpl::RefCountedStringImpl(const char* str, std::size_t len) {
  length_ = static_cast<uint32_t>(len);
  str_.resize(len);
  if (str == nullptr || len == 0) {
    hash_ = std::hash<std::string>()(str_);
    return;
  }
  std::memcpy(&str_[0], str, len);
  hash_ = std::hash<std::string>()(str_);
}

RefCountedStringImpl::RefCountedStringImpl(std::string str) {
  str_ = std::move(str);
  length_ = static_cast<uint32_t>(str_.size());
  hash_ = std::hash<std::string>()(str_);
}

size_t RefCountedStringImpl::length_utf8() {
  return base::SizeOfUtf8(str_.c_str(), str_.length());
}

size_t RefCountedStringImpl::length_utf16() {
  if (!utf16_len_calculated_) {
    utf16_length_ = static_cast<uint32_t>(base::SizeOfUtf16(str_));
    utf16_len_calculated_ = 1;
  }
  return utf16_length_;
}

bool StringConvertHelper::IsMinusZero(double value) {
  return base::BitCast<int64_t>(value) == base::BitCast<int64_t>(-0.0);
}

}  // namespace base
}  // namespace lynx
