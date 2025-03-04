// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_PUBLIC_TEXT_UTILS_H_
#define CORE_PUBLIC_TEXT_UTILS_H_

#include <memory>
#include <string>

#include "core/public/pub_value.h"

namespace lynx {
namespace tasm {

class TextUtils {
 public:
  // For now, only support px and rpx, and do not support font scale, screen
  // matrix.
  static std::unique_ptr<pub::Value> GetTextInfo(const std::string& content,
                                                 const pub::Value& info);
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_PUBLIC_TEXT_UTILS_H_
