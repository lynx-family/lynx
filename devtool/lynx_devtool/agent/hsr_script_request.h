// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DEVTOOL_LYNX_DEVTOOL_AGENT_HSR_SCRIPT_REQUEST_H_
#define DEVTOOL_LYNX_DEVTOOL_AGENT_HSR_SCRIPT_REQUEST_H_

#include <string>

#include "third_party/jsoncpp/include/json/json.h"

namespace lynx {
namespace devtool {

// Transport-owned boundary, independent of Runtime ownership and lifecycle.
struct HSRScriptRequest {
  enum class Operation { kLoadScript, kEvaluate };
  enum class SourceType { kInline, kUrl };

  Operation operation = Operation::kLoadScript;
  SourceType source_type = SourceType::kInline;
  // Script text, resource URL, or evaluation expression. Resource URLs are
  // passed unchanged to the platform's existing resource fetcher, which owns
  // scheme handling (including file URLs). Never parse a business envelope.
  std::string source;
};

bool ParseHSRLoadScript(const Json::Value& params, HSRScriptRequest& request,
                        std::string& error);
bool ParseHSREvaluate(const Json::Value& params, HSRScriptRequest& request,
                      std::string& error);
// The host performs URL decoding and rejects duplicate query parameters first.
bool ParseHSRSchemaLoad(const Json::Value& params, HSRScriptRequest& request,
                        std::string& error);

}  // namespace devtool
}  // namespace lynx

#endif  // DEVTOOL_LYNX_DEVTOOL_AGENT_HSR_SCRIPT_REQUEST_H_
