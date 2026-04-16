// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_INSPECTOR_RTS_INSPECTOR_MANAGER_H_
#define CORE_INSPECTOR_RTS_INSPECTOR_MANAGER_H_

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>

#include "core/inspector/lepus_inspector_manager.h"

struct RNIEnv_;
typedef RNIEnv_ RNIEnv;

namespace lynx {
namespace lepus {

class RTSInspectorManager : public LepusInspectorManager {
 public:
  ~RTSInspectorManager() override = default;

  virtual bool LoadScriptWithSource(RNIEnv* env, const uint8_t* buf,
                                    size_t buf_len, const std::string& filename,
                                    int* result) = 0;
  virtual std::unordered_map<std::string, std::string> GetSource(
      RNIEnv* env, const std::string& url) = 0;
};

}  // namespace lepus
}  // namespace lynx

#endif  // CORE_INSPECTOR_RTS_INSPECTOR_MANAGER_H_
