// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_VM_LEPUS_OUTPUT_STREAM_H_
#define CORE_RUNTIME_VM_LEPUS_OUTPUT_STREAM_H_

#include <cstdint>
#include <cstdio>
#include <memory>
#include <string>
#include <vector>

#include "base/include/cast_util.h"

namespace lynx {
namespace lepus {
class OutputStream {
 public:
  OutputStream() : offset_(0) {}
  virtual ~OutputStream() {}
  void WriteData(const uint8_t* src, size_t size, const char* desc = nullptr);

  void WriteCompactU32(uint32_t value);
  void WriteCompactS32(int32_t value);
  void WriteCompactU64(uint64_t value);
  void WriteCompactD64(double value);
  virtual size_t size() const = 0;
  virtual bool WriteToFile(const std::string& filename) = 0;
  virtual const std::vector<uint8_t>& byte_array() = 0;
  virtual void Move(uint32_t insert_pos, uint32_t start, uint32_t size) = 0;
  size_t Offset();

  // Overwrite the data at a specific offset
  virtual void OverwriteData(const uint8_t* src, size_t size,
                             size_t overwrite_offset) = 0;

 protected:
  virtual void WriteImpl(const uint8_t* buffer, size_t offset,
                         size_t length) = 0;

  size_t offset_;
};

struct OutputBuffer {
  bool WriteToFile(const std::string& filename) const;
  void clear() { data.clear(); }
  size_t size() const { return data.size(); }

  std::vector<uint8_t> data;
};

class ByteArrayOutputStream : public OutputStream {
 public:
  ByteArrayOutputStream() : buf_(new OutputBuffer()) {}

  bool WriteToFile(const std::string& filename) override;
  virtual size_t size() const override { return buf_->size(); }

  virtual void Move(uint32_t insert_pos, uint32_t start,
                    uint32_t size) override;

  virtual const std::vector<uint8_t>& byte_array() override;

  void OverwriteData(const uint8_t* src, size_t size,
                     size_t overwrite_offset) override;

 protected:
  void WriteImpl(const uint8_t* buffer, size_t offset, size_t length) override;

 private:
  std::unique_ptr<OutputBuffer> buf_;
};

}  // namespace lepus
}  // namespace lynx

#endif  // CORE_RUNTIME_VM_LEPUS_OUTPUT_STREAM_H_
