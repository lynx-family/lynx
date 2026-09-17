// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/lynx_devtool/agent/hsr_script_request.h"

#include <utility>

namespace lynx {
namespace devtool {

bool ParseHSRLoadScript(const Json::Value& params, HSRScriptRequest& request,
                        std::string& error) {
  if (!params.isObject() || !params["source"].isObject()) {
    error = "Expected object source";
    return false;
  }
  const auto& source = params["source"];
  if (!source["type"].isString()) {
    error = "Expected source.type: inline or url";
    return false;
  }
  const auto type = source["type"].asString();
  HSRScriptRequest parsed;
  if (type == "inline") {
    if (!source["script"].isString() || source.isMember("url")) {
      error = "Expected source.script without source.url for inline scripts";
      return false;
    }
    parsed.source = source["script"].asString();
  } else if (type == "url") {
    const auto& url = source["url"];
    if (url.isString() && !source.isMember("script")) {
      parsed.source = url.asString();
    }
    if (parsed.source.empty()) {
      error = "Expected non-empty source.url without source.script";
      return false;
    }
    parsed.source_type = HSRScriptRequest::SourceType::kUrl;
  } else {
    error = "Expected source.type: inline or url";
    return false;
  }
  request = std::move(parsed);
  return true;
}

bool ParseHSREvaluate(const Json::Value& params, HSRScriptRequest& request,
                      std::string& error) {
  if (!params.isObject() || !params["expression"].isString()) {
    error = "Expected string expression";
    return false;
  }
  request = {HSRScriptRequest::Operation::kEvaluate,
             HSRScriptRequest::SourceType::kInline,
             params["expression"].asString()};
  return true;
}

}  // namespace devtool
}  // namespace lynx
