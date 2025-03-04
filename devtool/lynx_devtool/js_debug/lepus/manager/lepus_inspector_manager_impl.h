// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DEVTOOL_LYNX_DEVTOOL_JS_DEBUG_LEPUS_MANAGER_LEPUS_INSPECTOR_MANAGER_IMPL_H_
#define DEVTOOL_LYNX_DEVTOOL_JS_DEBUG_LEPUS_MANAGER_LEPUS_INSPECTOR_MANAGER_IMPL_H_

#include "core/inspector/lepus_inspector_manager.h"
#include "devtool/js_inspect/lepus/lepus_inspector_client_impl.h"

namespace lynx {
namespace lepus {

class LepusInspectorManagerImpl : public LepusInspectorManager {
 public:
  LepusInspectorManagerImpl() = default;
  ~LepusInspectorManagerImpl() override = default;

  void InitInspector(
      Context* entry,
      const std::shared_ptr<InspectorLepusObserver>& observer) override;
  void DestroyInspector() override;

 private:
  std::string GenerateInspectorName(const std::string& name);

  std::shared_ptr<devtool::LepusInspectorClientImpl> inspector_client_;
  std::weak_ptr<InspectorLepusObserver> observer_wp_;

  std::string inspector_name_;
  std::string debug_info_url_;
};

}  // namespace lepus
}  // namespace lynx

#endif  // DEVTOOL_LYNX_DEVTOOL_JS_DEBUG_LEPUS_MANAGER_LEPUS_INSPECTOR_MANAGER_IMPL_H_
