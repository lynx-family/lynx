// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/shell/ios/data_utils.h"

#import <Foundation/Foundation.h>

#include "base/trace/native/trace_event.h"
#include "core/shell/common/shell_trace_event_def.h"

std::vector<uint8_t> ConvertNSBinary(NSData* binary) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, CONVERT_NS_BINARY);

  std::vector<uint8_t> result;
  auto len = binary.length;
  if (len > 0) {
    // Leave one extra byte capacity so downstream can append a '\0' sentinel
    // without reallocating/memcpy'ing large buffers (e.g. JS sources).
    result.reserve(static_cast<size_t>(len) + 1);
    auto begin = reinterpret_cast<const uint8_t*>(binary.bytes);
    result.assign(begin, begin + len);
  }
  return result;
}
