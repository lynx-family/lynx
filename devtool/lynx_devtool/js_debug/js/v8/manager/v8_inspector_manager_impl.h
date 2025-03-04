// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DEVTOOL_LYNX_DEVTOOL_JS_DEBUG_JS_V8_MANAGER_V8_INSPECTOR_MANAGER_IMPL_H_
#define DEVTOOL_LYNX_DEVTOOL_JS_DEBUG_JS_V8_MANAGER_V8_INSPECTOR_MANAGER_IMPL_H_

#include "core/runtime/jsi/v8/v8_inspector_manager.h"
#include "devtool/js_inspect/v8/v8_inspector_client_impl.h"

namespace lynx {
namespace piper {

class V8InspectorManagerImpl : public V8InspectorManager {
 public:
  V8InspectorManagerImpl() = default;
  ~V8InspectorManagerImpl() override = default;

  void InitInspector(
      Runtime* runtime,
      const std::shared_ptr<InspectorRuntimeObserverNG>& observer) override;
  void DestroyInspector() override;

  void PrepareForScriptEval() override;

 private:
  std::shared_ptr<devtool::V8InspectorClientImpl> inspector_client_;
  std::weak_ptr<InspectorRuntimeObserverNG> observer_wp_;

  int64_t runtime_id_{-1};
  int inspector_group_id_{0};
  std::string group_id_;
};

}  // namespace piper
}  // namespace lynx

#endif  // DEVTOOL_LYNX_DEVTOOL_JS_DEBUG_JS_V8_MANAGER_V8_INSPECTOR_MANAGER_IMPL_H_
