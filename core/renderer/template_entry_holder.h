// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_TEMPLATE_ENTRY_HOLDER_H_
#define CORE_RENDERER_TEMPLATE_ENTRY_HOLDER_H_

#include <atomic>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>

#include "base/include/closure.h"
#include "core/renderer/template_entry.h"
#include "core/template_bundle/lynx_template_bundle.h"
#include "lynx/core/renderer/js_bundle_holder_impl.h"

namespace lynx {
namespace tasm {

/**
 * Holder of template entries
 * 1. base class of TemplateAssembler to ensure vm context released after lepus
 * value
 * 2. provide js bundle holder for js app
 */
class TemplateEntryHolder {
 public:
  TemplateEntryHolder() = default;
  virtual ~TemplateEntryHolder() = default;

 public:
  const std::shared_ptr<TemplateEntry>& FindEntry(
      const std::string& entry_name);

  std::shared_ptr<TemplateEntry> FindTemplateEntry(
      const std::string& entry_name);

  /**
   * insert bundle for preloading lazy bundle
   */
  void InsertLynxTemplateBundle(const std::string& url,
                                LynxTemplateBundle&& bundle);

  std::shared_ptr<piper::JsBundleHolder> GetJsBundleHolder() const;

 protected:
  std::unique_ptr<JsBundleHolderImpl::RequestScope> CreateRequestScope(
      const std::string& url) const;

  void InsertEntry(const std::string& name,
                   std::shared_ptr<TemplateEntry> entry);

  void ForEachEntry(
      base::MoveOnlyClosure<void, const std::shared_ptr<TemplateEntry>&> func);

  std::optional<LynxTemplateBundle> GetPreloadTemplateBundle(
      const std::string& name);

  void SetEnableQueryComponentSync(bool enable);

 private:
  std::unordered_map<std::string, std::shared_ptr<TemplateEntry>>
      template_entries_;

  // template bundles for preloading lazy bundle
  std::unordered_map<std::string, LynxTemplateBundle> preload_template_bundles_;

  const std::shared_ptr<JsBundleHolderImpl> js_bundle_holder_{
      std::make_shared<JsBundleHolderImpl>()};
};
}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_TEMPLATE_ENTRY_HOLDER_H_
