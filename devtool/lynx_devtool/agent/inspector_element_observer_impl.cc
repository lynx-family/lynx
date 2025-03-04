// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/lynx_devtool/agent/inspector_element_observer_impl.h"

#include "devtool/lynx_devtool/agent/inspector_util.h"

namespace lynx {
namespace devtool {

InspectorElementObserverImpl::InspectorElementObserverImpl(
    const std::shared_ptr<InspectorTasmExecutor> &element_executor)
    : element_executor_wp_(element_executor) {}

void InspectorElementObserverImpl::OnDocumentUpdated() {
  TRACE_EVENT_BEGIN(LYNX_TRACE_CATEGORY, "OnDocumentUpdated");
  auto element_executor = element_executor_wp_.lock();
  CHECK_NULL_AND_LOG_RETURN(element_executor, "element_executor is null");
  element_executor->OnDocumentUpdated();
  TRACE_EVENT_END(LYNX_TRACE_CATEGORY);
}

void InspectorElementObserverImpl::OnElementNodeAdded(
    lynx::tasm::Element *ptr) {
  TRACE_EVENT_BEGIN(LYNX_TRACE_CATEGORY, "OnElementNodeAdded");
  auto element_executor = element_executor_wp_.lock();
  CHECK_NULL_AND_LOG_RETURN(element_executor, "element_executor is null");
  element_executor->OnElementNodeAdded(ptr);
  TRACE_EVENT_END(LYNX_TRACE_CATEGORY);
}

void InspectorElementObserverImpl::OnElementNodeRemoved(
    lynx::tasm::Element *ptr) {
  TRACE_EVENT_BEGIN(LYNX_TRACE_CATEGORY, "OnElementNodeRemoved");
  auto element_executor = element_executor_wp_.lock();
  CHECK_NULL_AND_LOG_RETURN(element_executor, "element_executor is null");
  element_executor->OnElementNodeRemoved(ptr);
  TRACE_EVENT_END(LYNX_TRACE_CATEGORY);
}

void InspectorElementObserverImpl::OnCharacterDataModified(
    lynx::tasm::Element *ptr) {
  TRACE_EVENT_BEGIN(LYNX_TRACE_CATEGORY, "OnCharacterDataModified");
  auto element_executor = element_executor_wp_.lock();
  CHECK_NULL_AND_LOG_RETURN(element_executor, "element_executor is null");
  element_executor->OnCharacterDataModified(ptr);
  TRACE_EVENT_END(LYNX_TRACE_CATEGORY);
}

void InspectorElementObserverImpl::OnElementDataModelSet(
    lynx::tasm::Element *ptr) {
  TRACE_EVENT_BEGIN(LYNX_TRACE_CATEGORY, "OnElementDataModelSet");
  auto element_executor = element_executor_wp_.lock();
  CHECK_NULL_AND_LOG_RETURN(element_executor, "element_executor is null");
  element_executor->OnElementDataModelSet(ptr);
  TRACE_EVENT_END(LYNX_TRACE_CATEGORY);
}

void InspectorElementObserverImpl::OnElementManagerWillDestroy() {
  TRACE_EVENT_BEGIN(LYNX_TRACE_CATEGORY, "OnElementManagerWillDestroy");
  auto element_executor = element_executor_wp_.lock();
  CHECK_NULL_AND_LOG_RETURN(element_executor, "element_executor is null");
  element_executor->OnElementManagerWillDestroy();
  TRACE_EVENT_END(LYNX_TRACE_CATEGORY);
}

void InspectorElementObserverImpl::OnSetNativeProps(lynx::tasm::Element *ptr,
                                                    const std::string &name,
                                                    const std::string &value,
                                                    bool is_style) {
  TRACE_EVENT_BEGIN(LYNX_TRACE_CATEGORY, "OnSetNativeProps");
  auto element_executor = element_executor_wp_.lock();
  CHECK_NULL_AND_LOG_RETURN(element_executor, "element_executor is null");
  element_executor->OnSetNativeProps(ptr, name, value, is_style);
  TRACE_EVENT_END(LYNX_TRACE_CATEGORY);
}

void InspectorElementObserverImpl::OnCSSStyleSheetAdded(
    lynx::tasm::Element *ptr) {
  TRACE_EVENT_BEGIN(LYNX_TRACE_CATEGORY, "OnCSSStyleSheetAdded");
  auto element_executor = element_executor_wp_.lock();
  CHECK_NULL_AND_LOG_RETURN(element_executor, "element_executor is null");
  element_executor->OnCSSStyleSheetAdded(ptr);
  TRACE_EVENT_END(LYNX_TRACE_CATEGORY);
}

void InspectorElementObserverImpl::OnComponentUselessUpdate(
    const std::string &component_name, const lynx::lepus::Value &properties) {}

std::map<lynx::devtool::DevToolFunction, std::function<void(const base::any &)>>
InspectorElementObserverImpl::GetDevToolFunction() {
  auto element_executor = element_executor_wp_.lock();
  CHECK_NULL_AND_LOG_RETURN_VALUE(element_executor, "element_executor is null",
                                  {});
  return element_executor->GetFunctionForElementMap();
}

}  // namespace devtool
}  // namespace lynx
