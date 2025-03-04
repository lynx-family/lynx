// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/template_entry_holder.h"

#include <utility>

#include "base/include/debug/lynx_assert.h"
#include "base/include/log/logging.h"
#include "base/trace/native/trace_event.h"
#include "core/build/gen/lynx_sub_error_code.h"

namespace lynx {
namespace tasm {

void TemplateEntryHolder::InsertEntry(const std::string& name,
                                      std::shared_ptr<TemplateEntry> entry) {
  template_entries_.emplace(std::make_pair(name, entry));
  js_bundle_holder_->InsertJSBundle(name, entry->GetJsBundle());
}

const std::shared_ptr<TemplateEntry>& TemplateEntryHolder::FindEntry(
    const std::string& entry_name) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "TemplateEntryHolder::FindEntry");
  auto entry = template_entries_.find(entry_name);
  LynxFatal(entry != template_entries_.end(),
            error::E_APP_BUNDLE_LOAD_RENDER_FAILED,
            "Lynx Must registered card or component which name is:%s",
            entry_name.c_str());
  return entry->second;
}

std::shared_ptr<TemplateEntry> TemplateEntryHolder::FindTemplateEntry(
    const std::string& entry_name) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "TemplateEntryHolder::FindTemplateEntry");
  auto entry_iter = template_entries_.find(entry_name);
  return entry_iter == template_entries_.end() ? nullptr : entry_iter->second;
}

void TemplateEntryHolder::ForEachEntry(
    base::MoveOnlyClosure<void, const std::shared_ptr<TemplateEntry>&> func) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "TemplateEntryHolder::ForEachEntry");
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
    preload_template_bundles_.insert_or_assign(url, std::move(bundle));
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

void TemplateEntryHolder::SetEnableQueryComponentSync(bool enable) {
  js_bundle_holder_->SetEnable(enable);
}

std::shared_ptr<piper::JsBundleHolder> TemplateEntryHolder::GetJsBundleHolder()
    const {
  return js_bundle_holder_;
}

std::unique_ptr<JsBundleHolderImpl::RequestScope>
TemplateEntryHolder::CreateRequestScope(const std::string& url) const {
  return js_bundle_holder_->CreateRequestScope(url);
}

}  // namespace tasm
}  // namespace lynx
