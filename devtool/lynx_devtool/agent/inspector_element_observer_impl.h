// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DEVTOOL_LYNX_DEVTOOL_AGENT_INSPECTOR_ELEMENT_OBSERVER_IMPL_H_
#define DEVTOOL_LYNX_DEVTOOL_AGENT_INSPECTOR_ELEMENT_OBSERVER_IMPL_H_

#include "core/inspector/observer/inspector_element_observer.h"
#include "devtool/lynx_devtool/agent/inspector_tasm_executor.h"

namespace lynx {
namespace devtool {
class InspectorElementObserverImpl
    : public lynx::tasm::InspectorElementObserver {
 public:
  InspectorElementObserverImpl(
      const std::shared_ptr<InspectorTasmExecutor> &element_executor);
  virtual ~InspectorElementObserverImpl() = default;

 public:
  virtual void OnDocumentUpdated() override;

  virtual void OnElementNodeAdded(lynx::tasm::Element *ptr) override;
  virtual void OnElementNodeRemoved(lynx::tasm::Element *ptr) override;
  virtual void OnCharacterDataModified(lynx::tasm::Element *ptr) override;
  virtual void OnElementDataModelSet(lynx::tasm::Element *ptr) override;
  virtual void OnElementManagerWillDestroy() override;
  virtual void OnSetNativeProps(lynx::tasm::Element *ptr,
                                const std::string &name,
                                const std::string &value,
                                bool is_style) override;

  virtual void OnCSSStyleSheetAdded(lynx::tasm::Element *ptr) override;
  virtual void OnComponentUselessUpdate(
      const std::string &component_name,
      const lynx::lepus::Value &properties) override;

  virtual std::map<lynx::devtool::DevToolFunction,
                   std::function<void(const base::any &)>>
  GetDevToolFunction() override;

 private:
  std::weak_ptr<InspectorTasmExecutor> element_executor_wp_;
};
}  // namespace devtool
}  // namespace lynx

#endif  // DEVTOOL_LYNX_DEVTOOL_AGENT_INSPECTOR_ELEMENT_OBSERVER_IMPL_H_
