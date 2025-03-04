// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_DATA_LYNX_VIEW_DATA_MANAGER_H_
#define CORE_RENDERER_DATA_LYNX_VIEW_DATA_MANAGER_H_

#include "third_party/rapidjson/document.h"

namespace lynx {
namespace lepus {
class Value;
}

namespace tasm {
class LynxViewDataManager {
 public:
  static lepus::Value* ParseData(const char* data);
  static void ReleaseData(lepus::Value* obj);
  static bool UpdateData(lepus::Value* target, lepus::Value* value);
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_DATA_LYNX_VIEW_DATA_MANAGER_H_
