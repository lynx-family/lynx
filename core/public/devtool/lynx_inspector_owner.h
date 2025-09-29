// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_PUBLIC_DEVTOOL_LYNX_INSPECTOR_OWNER_H_
#define CORE_PUBLIC_DEVTOOL_LYNX_INSPECTOR_OWNER_H_

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "base/include/fml/task_runner.h"

namespace lynx {
namespace piper {
class InspectorRuntimeObserverNG;
}

namespace tasm {
class TemplateData;
}  // namespace tasm

namespace devtool {
class LynxDevToolProxy;

class LynxInspectorOwner {
 public:
  virtual ~LynxInspectorOwner() = default;
  virtual void SetUITaskRunner(
      const fml::RefPtr<fml::TaskRunner>& task_runner) {}
  virtual void Init(LynxDevToolProxy* proxy,
                    const std::shared_ptr<LynxInspectorOwner>& shared_self) = 0;
  // life cycle
  virtual void OnTemplateAssemblerCreated(intptr_t ptr) = 0;
  virtual void OnLoaded(const std::string& url) = 0;
  virtual void OnLoadTemplate(
      const std::string& url, const std::vector<uint8_t>& tem,
      const std::shared_ptr<tasm::TemplateData>& data) = 0;
  virtual void OnShow() = 0;
  virtual void OnHide() = 0;
  virtual void InvokeCDPFromSDK(
      const std::string& cdp_msg,
      std::function<void(const std::string&)>&& callback) = 0;
  virtual std::shared_ptr<lynx::piper::InspectorRuntimeObserverNG>
  OnBackgroundRuntimeCreated(const std::string& group_thread_name) = 0;
};

}  // namespace devtool
}  // namespace lynx

#endif  // CORE_PUBLIC_DEVTOOL_LYNX_INSPECTOR_OWNER_H_
