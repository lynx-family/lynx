// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "core/runtime/lepus/binary_reader.h"

#include <assert.h>

#include "base/include/log/logging.h"
#include "base/trace/native/trace_event.h"
#include "core/runtime/lepus/binary_input_stream.h"
#include "core/runtime/trace/runtime_trace_event_def.h"

namespace lynx {
namespace lepus {

bool BinaryReader::ReadStringDirectly(std::string* out_value) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, READ_STRING_DIRECTLY);
  uint32_t length = 0;
  ERROR_UNLESS(ReadCompactU32(&length));
  ERROR_UNLESS(stream_->ReadString(*out_value, length));
  return true;
}

bool BinaryReader::ReadStringDirectly(base::String& out_value) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, READ_STRING_DIRECTLY);
  uint32_t length = 0;
  ERROR_UNLESS(ReadCompactU32(&length));
  ERROR_UNLESS(stream_->ReadString(out_value, length));
  return true;
}

void BinaryReader::PrintError(const char* format, const char* func, int line) {
  char buffer[1024];
  snprintf(buffer, sizeof(buffer), format, func, line);
  LOGE(error_message_ << ", " << buffer);
  // TODO ...
}

bool BinaryReader::CheckSize(int len, uint32_t maxOffset) {
  size_t curMax = stream_->size();
  if (maxOffset > 0 && maxOffset < curMax) {
    curMax = maxOffset;
  }
  if (stream_->offset() + len > curMax) {
    return false;
  }
  return true;
}

void BinaryReader::Skip(uint32_t size) {
  size_t offset = stream_->offset();
  stream_->Seek(offset + size);
}

size_t BinaryReader::Offset() { return stream_->offset(); }

}  // namespace lepus
}  // namespace lynx
