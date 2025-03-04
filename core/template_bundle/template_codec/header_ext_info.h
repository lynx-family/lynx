// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef CORE_TEMPLATE_BUNDLE_TEMPLATE_CODEC_HEADER_EXT_INFO_H_
#define CORE_TEMPLATE_BUNDLE_TEMPLATE_CODEC_HEADER_EXT_INFO_H_

#include "base/include/vector.h"
#include "core/runtime/vm/lepus/binary_reader.h"
#include "core/template_bundle/template_codec/compile_options.h"

#define HEADER_EXT_INFO_MAGIC 0x494e464f
namespace lynx {
namespace tasm {
struct HeaderExtInfo {
  uint32_t header_ext_info_size;
  uint32_t header_ext_info_magic_;
  uint32_t header_ext_info_field_numbers_;

#pragma pack(push)
#pragma pack(4)
  struct HeaderExtInfoField {
    uint8_t type_;
    uint8_t key_id_;
    uint16_t payload_size_;
    void* payload_;
  };
#pragma pack(pop)

  static constexpr uint8_t TYPE_STRING = 0;
  static constexpr uint8_t TYPE_UINT8 = 1;
  static constexpr uint8_t TYPE_UINT16 = 2;
  static constexpr uint8_t TYPE_UINT32 = 3;
  static constexpr uint8_t TYPE_UINT64 = 4;
  static constexpr uint8_t TYPE_INT8 = 5;
  static constexpr uint8_t TYPE_INT16 = 6;
  static constexpr uint8_t TYPE_INT32 = 7;
  static constexpr uint8_t TYPE_INT64 = 8;
  static constexpr uint8_t TYPE_FLOAT = 9;
  static constexpr uint8_t TYPE_DOUBLE = 10;

  static constexpr uint8_t SIZE_UINT8 = sizeof(uint8_t);
  static constexpr uint8_t SIZE_UINT16 = sizeof(uint16_t);
  static constexpr uint8_t SIZE_UINT32 = sizeof(uint32_t);
  static constexpr uint8_t SIZE_UINT64 = sizeof(uint64_t);
  static constexpr uint8_t SIZE_INT8 = sizeof(int8_t);
  static constexpr uint8_t SIZE_INT16 = sizeof(int16_t);
  static constexpr uint8_t SIZE_INT32 = sizeof(int32_t);
  static constexpr uint8_t SIZE_INT64 = sizeof(int64_t);
  static constexpr uint8_t SIZE_FLOAT = sizeof(float);
  static constexpr uint8_t SIZE_DOUBLE = sizeof(double);
};

// Use InlineVector to accommodate most extension fields without allocating
// additional memory.
using HeaderExtInfoByteArray =
    base::InlineVector<uint8_t, HeaderExtInfo::SIZE_DOUBLE>;

}  // namespace tasm
}  // namespace lynx
#endif  // CORE_TEMPLATE_BUNDLE_TEMPLATE_CODEC_HEADER_EXT_INFO_H_
