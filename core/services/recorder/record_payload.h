// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SERVICES_RECORDER_RECORD_PAYLOAD_H_
#define CORE_SERVICES_RECORDER_RECORD_PAYLOAD_H_

#include <sstream>
#include <string>

#include "base/include/log/log_context.h"
#include "base/include/log/logging.h"
#include "core/base/lynx_export.h"

namespace lynx::tasm::recorder {
// Optional DevTool callback. Returns the ID accepted for asynchronous delivery.
using RecordPayloadCallback = std::string (*)(const base::LogContext&,
                                              const char*, std::string);
LYNX_EXPORT_FOR_DEVTOOL void SetObservationPayloadEnabled(bool enabled);
LYNX_EXPORT_FOR_DEVTOOL void SetRecordPayloadCallback(
    RecordPayloadCallback send);
LYNX_EXPORT_FOR_DEVTOOL RecordPayloadCallback GetRecordPayloadCallback();

// Call inside LOGO so the log and payload share the same business conditions.
// Serialization stays on the calling thread: JS values and platform data may
// be thread-affine. Only the resulting owned string is sent asynchronously.
template <typename... Args>
std::string ObservePayload(const base::LogContext& context, const char* version,
                           const char* field, const Args&... values) {
  if (!LOGO_IS_ON()) return {};
  auto send = GetRecordPayloadCallback();
  if (!send) return {};
  std::ostringstream output;
  (output << ... << values);
  auto id = send(context, version, output.str());
  return id.empty() ? std::string{}
                    : " " + std::string(field) + "PayloadId:" + id;
}
}  // namespace lynx::tasm::recorder

#endif  // CORE_SERVICES_RECORDER_RECORD_PAYLOAD_H_
