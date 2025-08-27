// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DEVTOOL_LYNX_DEVTOOL_AGENT_INSPECTOR_UTIL_H_
#define DEVTOOL_LYNX_DEVTOOL_AGENT_INSPECTOR_UTIL_H_

#include <string>

#include "third_party/jsoncpp/include/json/json.h"

namespace lynx {
namespace devtool {
#define CHECK_NULL_RETURN(ptr) \
  if ((ptr) == nullptr) {      \
    return;                    \
  }

#define CHECK_NULL_RETURN_VALUE(ptr, res) \
  if ((ptr) == nullptr) {                 \
    return res;                           \
  }

#define CHECK_NULL_AND_LOG_RETURN(ptr, log_msg) \
  if ((ptr) == nullptr) {                       \
    LOGE(log_msg);                              \
    return;                                     \
  }

#define CHECK_NULL_AND_LOG_RETURN_VALUE(ptr, log_msg, res) \
  if ((ptr) == nullptr) {                                  \
    LOGE(log_msg);                                         \
    return res;                                            \
  }

enum CDPErrorCode {
  kParseError = -32700,
  kInvalidRequest = -32600,
  kMethodNotFound = -32601,
  kInvalidParams = -32602,
  kInternalError = -32603,
  kServerError = -32000,
};

class InspectorUtil {
 public:
  static int CompressData(const std::string& tag, const std::string& data,
                          Json::Value& value, const std::string& key);
};

}  // namespace devtool
}  // namespace lynx

#endif  // DEVTOOL_LYNX_DEVTOOL_AGENT_INSPECTOR_UTIL_H_
