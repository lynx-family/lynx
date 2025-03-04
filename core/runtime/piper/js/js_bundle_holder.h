// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_PIPER_JS_JS_BUNDLE_HOLDER_H_
#define CORE_RUNTIME_PIPER_JS_JS_BUNDLE_HOLDER_H_

#include <optional>
#include <string>

#include "core/runtime/piper/js/js_bundle.h"

namespace lynx {
namespace piper {
/**
 * an interface to help js app to get dynamic component sources from main thread
 */
class JsBundleHolder {
 public:
  virtual ~JsBundleHolder() = default;

  virtual std::optional<JsBundle> GetJSBundleFromBT(const std::string& url) = 0;
};
}  // namespace piper
}  // namespace lynx

#endif  // CORE_RUNTIME_PIPER_JS_JS_BUNDLE_HOLDER_H_
