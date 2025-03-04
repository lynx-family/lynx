// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_VM_LEPUS_BINARY_INPUT_STREAM_H_
#define CORE_RUNTIME_VM_LEPUS_BINARY_INPUT_STREAM_H_

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/include/cast_util.h"
#include "base/include/value/base_string.h"

namespace lynx {
namespace lepus {

class InputStream {
 public:
  InputStream() : offset_(0) {}

  virtual ~InputStream() {}
  virtual uint8_t* begin() = 0;
  virtual uint8_t* end() = 0;
  virtual size_t size() = 0;
  uint8_t* cursor() { return begin() + offset_; }

  inline bool CheckSize(size_t len) {
    if (size() == 0 || cursor() + len > end()) {
      return false;
    }
    return true;
  }

  size_t Seek(size_t offset) {
    if (offset >= size()) {
      offset_ = size() - 1;
    } else {
      offset_ = offset;
    }
    return offset_;
  }

  template <typename T>
  inline bool ReadUx(T* out_value) {
    if (!CheckSize(sizeof(T))) {
      return false;
    }
    if (out_value) {
      memcpy(out_value, cursor(), sizeof(T));
    }
    offset_ += sizeof(T);
    return true;
  }

  bool ReadData(uint8_t* dst, int len) {
    if (!CheckSize(len)) {
      return false;
    }
    if (dst) {
      memcpy(dst, cursor(), len);
    }
    offset_ += len;
    return true;
  }

  bool ReadString(std::string& str, size_t len) {
    if (!CheckSize(len)) {
      return false;
    }

    str = std::string(reinterpret_cast<const char*>(cursor()), len);
    offset_ += len;
    return true;
  }

  bool ReadString(base::String& str, size_t len) {
    if (!CheckSize(len)) {
      return false;
    }

    str = base::String(reinterpret_cast<const char*>(cursor()), len);
    offset_ += len;
    return true;
  }

  size_t offset() { return offset_; }

  // Returns the length of the leb128.
  size_t ReadCompactU32(uint32_t* out_value);
  size_t ReadCompactS32(int32_t* out_value);
  size_t ReadCompactU64(uint64_t* out_value);

 protected:
  size_t offset_;
};

struct InputBuffer {
  InputBuffer() = default;
  explicit InputBuffer(std::vector<uint8_t> data) : data(std::move(data)) {}

  bool ReadFromFile(const char* filename) const;

  void clear() { data.clear(); }
  size_t size() const { return data.size(); }

  std::vector<uint8_t> data;
};

class ByteArrayInputStream : public InputStream {
 public:
  ByteArrayInputStream(const uint8_t* data, int len) {
    buf_.reset(new InputBuffer());
    buf_->data.assign(data, data + len);
  }

  ByteArrayInputStream(std::vector<uint8_t> data)
      : buf_(std::make_unique<InputBuffer>(std::move(data))) {}

  bool ReadFromFile(const char* filename);
  inline const std::vector<uint8_t>& byte_array() { return buf_->data; }

  virtual uint8_t* begin() override { return &buf_->data[0]; }
  virtual uint8_t* end() override {
    return (&buf_->data[buf_->size() - 1]) + 1;
  }
  virtual size_t size() override { return buf_->size(); }

 private:
  std::unique_ptr<InputBuffer> buf_;
};

}  // namespace lepus
}  // namespace lynx

#endif  // CORE_RUNTIME_VM_LEPUS_BINARY_INPUT_STREAM_H_
