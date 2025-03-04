// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/utils/test/text_utils_mock.h"

#include <stdint.h>

#include "core/renderer/utils/base/tasm_constants.h"
#include "core/value_wrapper/value_impl_lepus.h"

namespace lynx {
namespace tasm {

// The input info contains fontSize and fontFamily, both of which are string
// types. The unit of fontSize can be px or rpx.
std::unique_ptr<pub::Value> TextUtils::GetTextInfo(const std::string& content,
                                                   const pub::Value& info) {
  lepus::Value result = lepus::Value::CreateObject();
  // TODO(wangyanyi): impl this later
  result.SetProperty(kWidth,
                     lepus::Value(static_cast<int32_t>(content.size())));
  return std::make_unique<PubLepusValue>(lepus::Value(result));
}

}  // namespace tasm
}  // namespace lynx
