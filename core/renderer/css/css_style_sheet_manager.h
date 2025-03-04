// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_CSS_CSS_STYLE_SHEET_MANAGER_H_
#define CORE_RENDERER_CSS_CSS_STYLE_SHEET_MANAGER_H_

#include <memory>
#include <mutex>
#include <unordered_map>
#include <unordered_set>
#include <utility>

#include "core/renderer/css/shared_css_fragment.h"
#include "core/renderer/page_config.h"
#include "core/template_bundle/template_codec/moulds.h"
#include "core/template_bundle/template_codec/template_binary.h"

namespace lynx {
namespace tasm {

class CSSStyleSheetDelegate {
 public:
  CSSStyleSheetDelegate() = default;
  virtual ~CSSStyleSheetDelegate() = default;
  virtual bool DecodeCSSFragmentById(int32_t fragmentId) = 0;
};

class CSSStyleSheetManager {
 public:
  using CSSFragmentMap =
      std::unordered_map<int32_t, std::unique_ptr<SharedCSSFragment>>;

  CSSStyleSheetManager(CSSStyleSheetDelegate* delegate)
      : raw_fragments_(std::make_shared<CSSFragmentMap>()),
        delegate_(delegate){};

  SharedCSSFragment* GetCSSStyleSheetForComponent(int32_t id);
  SharedCSSFragment* GetCSSStyleSheetForPage(int32_t id);
  const CSSFragmentMap& raw_fragments() const { return *raw_fragments_; }
  std::atomic_bool GetStopThread() const { return stop_thread_; }

  void SetThreadStopFlag(bool stop_thread) { stop_thread_ = stop_thread; }

  SharedCSSFragment* GetSharedCSSFragmentById(int32_t id) {
    std::lock_guard<std::mutex> g_lock(fragment_mutex_);
    decoded_fragment_.emplace(id);
    auto fragment_iter = raw_fragments_->find(id);
    return fragment_iter != raw_fragments_->end() ? fragment_iter->second.get()
                                                  : nullptr;
  }

  bool IsSharedCSSFragmentDecoded(int32_t id) {
    std::lock_guard<std::mutex> g_lock(fragment_mutex_);
    if (decoded_fragment_.find(id) != decoded_fragment_.end()) {
      return true;
    }
    return false;
  }

  void AddSharedCSSFragment(std::unique_ptr<SharedCSSFragment> fragment) {
    std::lock_guard<std::mutex> g_lock(fragment_mutex_);
    raw_fragments_->emplace(fragment->id(), std::move(fragment));
  }

  void ReplaceSharedCSSFragment(std::unique_ptr<SharedCSSFragment> fragment) {
    std::lock_guard<std::mutex> g_lock(fragment_mutex_);
    raw_fragments_->insert_or_assign(fragment->id(), std::move(fragment));
  }

  void RemoveSharedCSSFragment(int32_t id) {
    std::lock_guard<std::mutex> g_lock(fragment_mutex_);
    raw_fragments_->erase(id);
  }

  void SetEnableNewImportRule(bool enable) { enable_new_import_rule_ = enable; }

  // Flatten all the css fragments, so that they will be read-only
  void FlattenAllCSSFragment();

  // only copy raw_fragments_
  void CopyFrom(const CSSStyleSheetManager& other);

  const std::shared_ptr<CSSFragmentMap>& GetCSSFragmentMap() const;

  SharedCSSFragment* GetCSSStyleSheet(int32_t id);

  void SetEnableCSSLazyImport(bool enable) { enable_css_lazy_import_ = enable; }

  bool GetEnableCSSLazyImport() { return enable_css_lazy_import_; }

 private:
  friend class TemplateBinaryReader;
  friend class TemplateBinaryReaderSSR;
  friend class LynxBinaryBaseCSSReader;
  friend class LynxBinaryReader;

  void FlatDependentCSS(SharedCSSFragment* fragment);

  CSSRoute route_;
  CSSFragmentMap page_fragments_;
  // shared in pre-decoding
  std::shared_ptr<CSSFragmentMap> raw_fragments_;
  CSSStyleSheetDelegate* delegate_ = nullptr;
  std::unordered_set<int> decoded_fragment_;
  volatile std::atomic_bool stop_thread_ = false;
  std::mutex fragment_mutex_;
  bool enable_new_import_rule_ = false;

  // enableCSSLazyImport default value is false.
  bool enable_css_lazy_import_ = false;
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_CSS_CSS_STYLE_SHEET_MANAGER_H_
