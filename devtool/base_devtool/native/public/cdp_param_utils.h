// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DEVTOOL_BASE_DEVTOOL_NATIVE_PUBLIC_CDP_PARAM_UTILS_H_
#define DEVTOOL_BASE_DEVTOOL_NATIVE_PUBLIC_CDP_PARAM_UTILS_H_

#include <string>

#include "devtool/base_devtool/native/public/base_devtool_export.h"
#include "third_party/jsoncpp/include/json/json.h"

namespace lynx {
namespace devtool {

// Read a JSON integer representable as int, without coercing other JSON types.
// All readers leave result unchanged on failure, including missing/null
// values.
BASE_DEVTOOL_EXPORT bool ReadIntParam(const Json::Value& value, int& result);

// Read a JSON boolean without coercing other JSON types.
BASE_DEVTOOL_EXPORT bool ReadBoolParam(const Json::Value& value, bool& result);

// Read a JSON string without coercing other JSON types.
BASE_DEVTOOL_EXPORT bool ReadStringParam(const Json::Value& value,
                                         std::string& result);

// Read a complete decimal integer string (optional sign), representable as int.
// Field-specific constraints such as non-negativity belong to the caller.
BASE_DEVTOOL_EXPORT bool ReadIntStringParam(const Json::Value& value,
                                            int& result);

// Read a finite JSON number as double, without coercing other JSON types.
BASE_DEVTOOL_EXPORT bool ReadFiniteNumberParam(const Json::Value& value,
                                               double& result);

// Read a finite JSON number within float range, without coercing other JSON
// types.
BASE_DEVTOOL_EXPORT bool ReadFiniteFloatParam(const Json::Value& value,
                                              float& result);

}  // namespace devtool
}  // namespace lynx

#endif  // DEVTOOL_BASE_DEVTOOL_NATIVE_PUBLIC_CDP_PARAM_UTILS_H_
