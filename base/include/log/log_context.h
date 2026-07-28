// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef BASE_INCLUDE_LOG_LOG_CONTEXT_H_
#define BASE_INCLUDE_LOG_LOG_CONTEXT_H_

#include <cstdint>

#include "base/include/log/log_stream.h"

namespace lynx::base {

using LynxEntityId = int32_t;

inline constexpr LynxEntityId kUnavailableLynxEntityId = -1;

struct LogContext {
  LynxEntityId view_id = kUnavailableLynxEntityId;
  LynxEntityId engine_id = kUnavailableLynxEntityId;
  LynxEntityId runtime_id = kUnavailableLynxEntityId;

  friend logging::LogStream& operator<<(logging::LogStream& stream,
                                        const LogContext& context) {
    return stream << '[' << context.view_id << ',' << context.engine_id << ','
                  << context.runtime_id << ']';
  }
};

}  // namespace lynx::base

#endif  // BASE_INCLUDE_LOG_LOG_CONTEXT_H_
