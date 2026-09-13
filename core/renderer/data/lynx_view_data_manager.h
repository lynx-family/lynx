// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_DATA_LYNX_VIEW_DATA_MANAGER_H_
#define CORE_RENDERER_DATA_LYNX_VIEW_DATA_MANAGER_H_

namespace lynx {
namespace lepus {
class Value;
}

namespace tasm {
class LynxViewDataManager {
 public:
  static lepus::Value* ParseData(const char* data);
  static void ReleaseData(lepus::Value* obj);
  static void UpdateData(lepus::Value& dest, const lepus::Value& src);

  // Merges the first-level keys from src into dest when both values are
  // tables. Values with the same key are replaced as a whole instead of being
  // merged recursively.
  static void MergeData(lepus::Value& dest, const lepus::Value& src);
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_DATA_LYNX_VIEW_DATA_MANAGER_H_
