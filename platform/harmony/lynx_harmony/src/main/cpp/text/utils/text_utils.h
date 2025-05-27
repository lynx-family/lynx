// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_TEXT_UTILS_TEXT_UTILS_H_
#define PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_TEXT_UTILS_TEXT_UTILS_H_

#include <string>

#include "base/include/value/base_value.h"
#include "core/public/pub_value.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/lynx_context.h"

namespace lynx {
namespace tasm {
namespace harmony {
class TextUtils {
 public:
  static lepus::Value GetTextInfo(const std::string& content,
                                  const pub::Value& info, LynxContext* ctx);
};
}  // namespace harmony
}  // namespace tasm
}  // namespace lynx

#endif  // PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_TEXT_UTILS_TEXT_UTILS_H_
