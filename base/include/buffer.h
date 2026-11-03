// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef BASE_INCLUDE_BUFFER_H_
#define BASE_INCLUDE_BUFFER_H_

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/include/base_export.h"
#include "base/include/value/base_string.h"

namespace lynx {
namespace base {

class BASE_EXPORT Buffer {
 public:
  virtual ~Buffer() = default;
  virtual size_t size() const = 0;
  virtual const uint8_t* data() const = 0;

  // Returns an owned buffer, or nullptr if reading fails or exceeds max_size.
  [[nodiscard]] static std::unique_ptr<Buffer> ReadFromFile(
      const std::string& path,
      size_t max_size = (std::numeric_limits<size_t>::max)());

  [[nodiscard]] bool WriteToFile(const std::string& path) const;
};

class StringBuffer : public Buffer {
 public:
  StringBuffer(std::string s) : s_(std::move(s)) {}
  size_t size() const override { return s_.size(); }
  const uint8_t* data() const override {
    return reinterpret_cast<const uint8_t*>(s_.data());
  }
  std::string& GetString() { return s_; }

 private:
  std::string s_;
};

class StringRefBuffer : public Buffer {
 public:
  explicit StringRefBuffer(const std::string& s) : s_(s) {}
  size_t size() const override { return s_.size(); }
  const uint8_t* data() const override {
    return reinterpret_cast<const uint8_t*>(s_.data());
  }

 private:
  const std::string& s_;
};

// The caller must keep the referenced null-terminated string alive and its
// length unchanged.
class CStringRefBuffer : public Buffer {
 public:
  explicit CStringRefBuffer(const char* s)
      : s_(s), length_(s == nullptr ? 0 : std::strlen(s)) {}
  size_t size() const override { return length_; }
  const uint8_t* data() const override {
    return reinterpret_cast<const uint8_t*>(s_);
  }

 private:
  const char* s_;
  size_t length_;
};

class BaseStringBuffer : public Buffer {
 public:
  explicit BaseStringBuffer(const String& s) : s_(s) {}
  size_t size() const override { return s_.length(); }
  const uint8_t* data() const override {
    return reinterpret_cast<const uint8_t*>(s_.c_str());
  }

 private:
  String s_;
};

class ByteBuffer : public Buffer {
 public:
  explicit ByteBuffer(std::shared_ptr<const std::vector<uint8_t>> data)
      : data_(data == nullptr ? std::make_shared<const std::vector<uint8_t>>()
                              : std::move(data)) {}
  size_t size() const override { return data_->size(); }
  const uint8_t* data() const override { return data_->data(); }

 private:
  std::shared_ptr<const std::vector<uint8_t>> data_;
};

// A non-owning view; the caller must keep the bytes alive.
class ByteRefBuffer : public Buffer {
 public:
  ByteRefBuffer(const uint8_t* data, size_t size) : data_(data), size_(size) {}
  size_t size() const override { return size_; }
  const uint8_t* data() const override { return data_; }

 private:
  const uint8_t* data_;
  size_t size_;
};

}  // namespace base
}  // namespace lynx

#endif  // BASE_INCLUDE_BUFFER_H_
