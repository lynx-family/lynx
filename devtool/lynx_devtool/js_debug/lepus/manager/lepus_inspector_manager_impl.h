// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DEVTOOL_LYNX_DEVTOOL_JS_DEBUG_LEPUS_MANAGER_LEPUS_INSPECTOR_MANAGER_IMPL_H_
#define DEVTOOL_LYNX_DEVTOOL_JS_DEBUG_LEPUS_MANAGER_LEPUS_INSPECTOR_MANAGER_IMPL_H_

#include <unordered_map>

#include "core/inspector/lepus_inspector_manager.h"
#include "devtool/js_inspect/lepus/lepus_inspector_client_impl.h"

namespace lynx {
namespace lepus {

class LepusInspectorManagerImpl : public LepusInspectorManager {
 public:
  LepusInspectorManagerImpl() = default;
  ~LepusInspectorManagerImpl() override = default;

  void InitInspector(runtime::MTSContext* entry,
                     const std::shared_ptr<InspectorLepusObserver>& observer,
                     const std::string& context_name) override;
  void SetDebugInfo(const std::string& debug_info_url,
                    const std::string& file_name) override;
  void DestroyInspector() override;

 private:
  std::string GenerateInspectorName(const std::string& name);

  std::shared_ptr<devtool::LepusInspectorClientImpl> inspector_client_;
  std::weak_ptr<InspectorLepusObserver> observer_wp_;

  std::string inspector_name_;
  // MTS chunk shares debug-info.json with the main entry or lazy components,
  // eliminating the need for redownloading. We maintain a map to record URLs
  // that have already been downloaded and assign an ID to each debug info. If a
  // URL is found in the map, we retrieve the ID directly.
  std::unordered_map<std::string, int> debug_info_url_to_id_;
};

}  // namespace lepus
}  // namespace lynx

#endif  // DEVTOOL_LYNX_DEVTOOL_JS_DEBUG_LEPUS_MANAGER_LEPUS_INSPECTOR_MANAGER_IMPL_H_
