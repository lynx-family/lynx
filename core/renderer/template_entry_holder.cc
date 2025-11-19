// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/template_entry_holder.h"

#include <utility>

#include "base/include/debug/lynx_assert.h"
#include "base/include/log/logging.h"
#include "base/trace/native/trace_event.h"
#include "core/build/gen/lynx_sub_error_code.h"
#include "core/renderer/trace/renderer_trace_event_def.h"

namespace lynx {
namespace tasm {

void TemplateEntryHolder::InsertEntry(const std::string& name,
                                      std::shared_ptr<TemplateEntry> entry) {
  if (!entry->IsCard()) {
    TryPostJSBundle(name, entry->template_bundle());
  }
  template_entries_.try_emplace(name, std::move(entry));
}

void TemplateEntryHolder::SetLazyBundleLoader(
    const std::shared_ptr<LazyBundleLoader>& loader) {
  component_loader_ = loader;
  js_bundle_holder_ = std::make_shared<JsBundleHolderImpl>(*this);
}

const std::shared_ptr<TemplateEntry>& TemplateEntryHolder::FindEntry(
    const std::string& entry_name) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, TEMPLATE_ENTRY_HOLDER_FIND_ENTRY);
  auto entry = template_entries_.find(entry_name);
  LynxFatal(entry != template_entries_.end(),
            error::E_APP_BUNDLE_LOAD_RENDER_FAILED,
            "Lynx Must registered card or component which name is:%s",
            entry_name.c_str());
  return entry->second;
}

std::shared_ptr<TemplateEntry> TemplateEntryHolder::FindTemplateEntry(
    const std::string& entry_name) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, TEMPLATE_ENTRY_HOLDER_FIND_TEMPLATE_ENTRY);
  auto entry_iter = template_entries_.find(entry_name);
  return entry_iter == template_entries_.end() ? nullptr : entry_iter->second;
}

std::optional<LynxTemplateBundle> TemplateEntryHolder::FindTemplateBundle(
    const std::string& entry_name) {
  auto bundle_iter = template_entries_.find(entry_name);
  if (bundle_iter != template_entries_.end()) {
    return bundle_iter->second->template_bundle();
  }

  auto preload_bundle_iter = preload_template_bundles_.find(entry_name);
  if (preload_bundle_iter != preload_template_bundles_.end()) {
    return preload_bundle_iter->second;
  }

  return component_loader_->GetTemplateBundle(entry_name);
}

void TemplateEntryHolder::ForEachEntry(
    base::MoveOnlyClosure<void, const std::shared_ptr<TemplateEntry>&> func) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, TEMPLATE_ENTRY_HOLDER_FOR_EACH_ENTRY);
  for (const auto& [name, entry] : template_entries_) {
    func(entry);
  }
}

void TemplateEntryHolder::InsertLynxTemplateBundle(
    const std::string& url, LynxTemplateBundle&& bundle) {
  /**
   * Currently, preload_template_bundles_ is used to store preloaded template
   * entries which will eventually be loaded into template_entries_. So existing
   * items in template_entries_ do not need to be inserted.
   */
  if (template_entries_.find(url) == template_entries_.end()) {
    // TODD(yangguangzhao.solace): JS query component should use preloaded
    // bundle too.
    preload_template_bundles_.try_emplace(url, bundle);
  }

  // TODO: It's reasonable that using component_loader as bundle_manager
  // so we need to insert pre registered bundle into bundle manager.
  // refactoring it later!
  if (component_loader_) {
    component_loader_->InsertTemplateBundle(url, std::move(bundle));
  }
}

void TemplateEntryHolder::TryPostJSBundle(const std::string& url,
                                          const LynxTemplateBundle& bundle) {
  // Only needed by Fiber because Fiber will not fetch lazy bundle resources
  // from the Engine Thread except for the first screen.
  if (bundle.EnableFiberArch() && js_bundle_holder_) {
    js_bundle_holder_->InsertJSBundle(url, bundle.GetJsBundle());
  }
}

std::optional<LynxTemplateBundle> TemplateEntryHolder::GetPreloadTemplateBundle(
    const std::string& name) {
  std::optional<LynxTemplateBundle> bundle = std::nullopt;
  auto iter = preload_template_bundles_.find(name);
  if (iter != preload_template_bundles_.end()) {
    bundle = std::move(iter->second);
    preload_template_bundles_.erase(iter);
  }
  return bundle;
}

lepus::Value TemplateEntryHolder::GetCustomSection(const std::string& url) {
  auto bundle = component_loader_->GetTemplateBundle(url);
  if (bundle) {
    return bundle->GetCustomSections();
  }
  return lepus::Value();
}

void TemplateEntryHolder::SetEnableQueryComponentSync(bool enable) {
  if (js_bundle_holder_) {
    js_bundle_holder_->SetEnable(enable);
  }
}

std::shared_ptr<piper::JsBundleHolder> TemplateEntryHolder::GetJsBundleHolder()
    const {
  return js_bundle_holder_;
}

std::unique_ptr<JsBundleHolderImpl::RequestScope>
TemplateEntryHolder::CreateRequestScope(const std::string& url) const {
  if (js_bundle_holder_) {
    return js_bundle_holder_->CreateRequestScope(url);
  }
  return nullptr;
}

}  // namespace tasm
}  // namespace lynx
