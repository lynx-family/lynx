// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_INSPECTOR_OBSERVER_INSPECTOR_LEPUS_OBSERVER_H_
#define CORE_INSPECTOR_OBSERVER_INSPECTOR_LEPUS_OBSERVER_H_

#include <memory>
#include <string>

#include "core/inspector/lepus_inspector_manager.h"
#include "core/runtime/mts_context.h"

namespace lynx {

namespace devtool {
class InspectorClientNG;
}

namespace lepus {

class InspectorLepusObserver {
 public:
  virtual ~InspectorLepusObserver() = default;

  virtual std::unique_ptr<LepusInspectorManager> CreateLepusInspectorManager(
      runtime::ContextType context_type) {
    return nullptr;
  }
  virtual bool ShouldFetchDebugInfo() { return false; }
  virtual std::string GetDebugInfo(const std::string& url) { return ""; }
  virtual void SetDebugInfoUrl(const std::string& url,
                               const std::string& file_name) = 0;

  virtual void OnInspectorInited(
      const std::string& vm_type, const std::string& name,
      const std::shared_ptr<devtool::InspectorClientNG>& client) = 0;
  virtual void OnContextDestroyed(const std::string& name) = 0;

  virtual void TakeOver(
      const std::shared_ptr<InspectorLepusObserver>& other) = 0;

  virtual void OnConsoleEvent(const std::string& func_name,
                              const std::string& args) = 0;

  virtual void PrepareForScriptEval(const std::string& name) = 0;
};

}  // namespace lepus
}  // namespace lynx

#endif  // CORE_INSPECTOR_OBSERVER_INSPECTOR_LEPUS_OBSERVER_H_
