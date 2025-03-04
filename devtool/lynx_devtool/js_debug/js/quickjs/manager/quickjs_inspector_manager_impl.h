// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DEVTOOL_LYNX_DEVTOOL_JS_DEBUG_JS_QUICKJS_MANAGER_QUICKJS_INSPECTOR_MANAGER_IMPL_H_
#define DEVTOOL_LYNX_DEVTOOL_JS_DEBUG_JS_QUICKJS_MANAGER_QUICKJS_INSPECTOR_MANAGER_IMPL_H_

#include <memory>
#include <set>
#include <string>

#include "core/runtime/jsi/quickjs/quickjs_inspector_manager.h"
#include "devtool/js_inspect/quickjs/quickjs_inspector_client_impl.h"

namespace lynx {
namespace piper {

class QuickjsInspectorManagerImpl : public QuickjsInspectorManager {
 public:
  QuickjsInspectorManagerImpl() = default;
  ~QuickjsInspectorManagerImpl() override = default;

  void InitInspector(
      Runtime* runtime,
      const std::shared_ptr<InspectorRuntimeObserverNG>& observer) override;
  void DestroyInspector() override;

  void InsertScript(const std::string& url) override;

  void PrepareForScriptEval() override;

 private:
  std::shared_ptr<devtool::QJSInspectorClientImpl> inspector_client_;
  std::weak_ptr<InspectorRuntimeObserverNG> observer_wp_;

  int64_t runtime_id_{-1};
  std::string group_id_;
  std::string inspector_group_id_;

  std::set<std::string> scripts_;
};

}  // namespace piper
}  // namespace lynx

#endif  // DEVTOOL_LYNX_DEVTOOL_JS_DEBUG_JS_QUICKJS_MANAGER_QUICKJS_INSPECTOR_MANAGER_IMPL_H_
