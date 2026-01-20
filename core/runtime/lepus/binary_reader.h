// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef CORE_RUNTIME_LEPUS_BINARY_READER_H_
#define CORE_RUNTIME_LEPUS_BINARY_READER_H_

#include <stdlib.h>

#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/include/base_defines.h"
#include "base/include/value/base_string.h"
#include "core/runtime/lepus/binary_input_stream.h"

#define ERROR_UNLESS_CODE(expr, code, errorCode)                    \
  do {                                                              \
    if (!(expr)) {                                                  \
      PrintError("Function:%s, Line:%d\n", __FUNCTION__, __LINE__); \
      code = errorCode;                                             \
      return false;                                                 \
    }                                                               \
  } while (0)

#define ERROR_UNLESS(expr, ...)                                     \
  do {                                                              \
    if (!(expr)) {                                                  \
      PrintError("Function:%s, Line:%d\n", __FUNCTION__, __LINE__); \
      __VA_ARGS__;                                                  \
      return false;                                                 \
    }                                                               \
  } while (0)

namespace lynx {
namespace lepus {

class BinaryReader {
 public:
  explicit BinaryReader(std::unique_ptr<InputStream> stream)
      : error_message_("unknown Decode Error \n"), stream_(std::move(stream)) {}
  virtual ~BinaryReader() = default;

  BinaryReader(const BinaryReader&) = delete;
  BinaryReader& operator=(const BinaryReader&) = delete;

  BinaryReader(BinaryReader&&) = default;
  BinaryReader& operator=(BinaryReader&&) = default;

  BASE_INLINE bool ReadU32(uint32_t* out_value) {
    return stream_->ReadUx<uint32_t>(out_value);
  }

  BASE_INLINE bool ReadU8(uint8_t* out_value) {
    return stream_->ReadUx<uint8_t>(out_value);
  }

  BASE_INLINE bool ReadCompactU32(uint32_t* out_value) {
    return stream_->ReadCompactU32(out_value);
  }

  BASE_INLINE bool ReadCompactS32(int32_t* out_value) {
    return stream_->ReadCompactS32(out_value);
  }

  BASE_INLINE bool ReadCompactU64(uint64_t* out_value) {
    return stream_->ReadCompactU64(out_value);
  }

  BASE_INLINE bool ReadCompactD64(double* out_value) {
    uint64_t data = 0;
    if (!stream_->ReadCompactU64(&data)) {
      return false;
    }
    *out_value = base::BitCast<double>(data);
    return true;
  }

  bool ReadStringDirectly(std::string* out_value);
  bool ReadStringDirectly(base::String& out_value);
  void PrintError(const char* format, const char* func, int line);
  bool CheckSize(int len, uint32_t maxOffset = 0);
  void Skip(uint32_t size);
  size_t Offset();
  void Seek(uint32_t offset) { stream_->Seek(offset); }

  bool ReadData(uint8_t* dst, int len) { return stream_->ReadData(dst, len); }

  InputStream* GetStream() { return stream_.get(); }

  std::string error_message_;

 protected:
  std::unique_ptr<InputStream> stream_;
};

}  // namespace lepus
}  // namespace lynx

#endif  // CORE_RUNTIME_LEPUS_BINARY_READER_H_
