// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_OBSERVER_LYNX_ENGINE_OBSERVER_H_
#define CORE_OBSERVER_LYNX_ENGINE_OBSERVER_H_

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "base/include/log/log_context.h"

namespace lynx {

namespace lepus {
class Value;
}

namespace tasm {
class Element;
class TemplateData;
struct EventInfo;
struct UpdatePageOption;
}  // namespace tasm

namespace observer {

class TemplateLifecycleObserver {
 public:
  virtual ~TemplateLifecycleObserver() = default;

  virtual void OnLoadTemplateStart(const base::LogContext& log_context,
                                   const std::string& url,
                                   size_t source_size) = 0;
  virtual void OnLoadTemplateEnd(const base::LogContext& log_context,
                                 const std::string& url,
                                 size_t source_size) = 0;
  virtual void OnReloadTemplateStart(const base::LogContext& log_context,
                                     const std::string& url,
                                     size_t source_size) = 0;
  virtual void OnReloadTemplateEnd(const base::LogContext& log_context,
                                   const std::string& url,
                                   size_t source_size) = 0;
};

class TemplateObserver {
 public:
  virtual ~TemplateObserver() = default;

  virtual void OnLoadTemplate(
      const std::string& url, const std::vector<uint8_t>& source,
      const std::shared_ptr<tasm::TemplateData>& template_data,
      bool is_csr) = 0;
  virtual void OnLoadTemplateBundle(
      const std::string& url, const std::vector<uint8_t>& source,
      const std::shared_ptr<tasm::TemplateData>& template_data) = 0;
  virtual void OnReloadTemplate(
      const std::shared_ptr<tasm::TemplateData>& template_data) = 0;
  virtual void OnGlobalPropsUpdated(const lepus::Value& global_props) = 0;
  virtual void OnConfigUpdated(const lepus::Value& config,
                               bool notice_delegate) = 0;
  virtual void OnFontScaleChanged(float scale,
                                  const std::string& operation) = 0;
  virtual void OnDataUpdated(
      const std::shared_ptr<tasm::TemplateData>& template_data,
      const tasm::UpdatePageOption& update_page_option) = 0;
  virtual void OnLazyBundleLoaded(const std::string& url,
                                  const std::vector<uint8_t>& source, bool sync,
                                  int32_t callback_id) = 0;
  virtual void OnLazyBundleRequired(const std::string& url, bool sync) = 0;
};

class InputObserver {
 public:
  virtual ~InputObserver() = default;

  virtual void OnTouchEvent(const std::string& name, int root_tag,
                            const tasm::EventInfo& info) = 0;
  virtual void OnCustomEvent(const std::string& name, int tag, int root_tag,
                             const lepus::Value& params,
                             const std::string& params_name) = 0;
  virtual void OnBubbleEvent(const std::string& name, int tag, int root_tag,
                             const lepus::Value& params) = 0;
};

class ElementObserver {
 public:
  virtual ~ElementObserver() = default;

  virtual void OnDocumentUpdated() = 0;
  virtual void OnElementNodeAdded(tasm::Element* element) = 0;
  virtual void OnElementNodeRemoved(tasm::Element* element) = 0;
  virtual void OnCharacterDataModified(tasm::Element* element) = 0;
  virtual void OnElementDataModelSet(tasm::Element* element) = 0;
  virtual void OnElementManagerWillDestroy() = 0;
  virtual void OnCSSStyleSheetAdded(tasm::Element* element) = 0;
  virtual void OnCSSMediaQueryResultChanged() = 0;
  virtual void OnSetNativeProps(tasm::Element* element, const std::string& name,
                                const lepus::Value& value, bool is_style) = 0;
};

class LepusConsoleObserver {
 public:
  virtual ~LepusConsoleObserver() = default;

  virtual void OnConsoleEvent(const std::string& level,
                              const std::string& message) = 0;
};

class MTSRuntimeLifecycleObserver {
 public:
  virtual ~MTSRuntimeLifecycleObserver() = default;

  virtual void OnContextDestroyed(const std::string& name) = 0;
};

class LynxEngineObserver {
 public:
  virtual ~LynxEngineObserver() = default;

  virtual TemplateLifecycleObserver* TemplateLifecycle() const = 0;
  virtual TemplateObserver* Template() const = 0;
  virtual InputObserver* Input() const = 0;
  virtual ElementObserver* Element() const = 0;
  virtual LepusConsoleObserver* LepusConsole() const = 0;
  virtual MTSRuntimeLifecycleObserver* MTSRuntimeLifecycle() const = 0;
};

}  // namespace observer
}  // namespace lynx

#endif  // CORE_OBSERVER_LYNX_ENGINE_OBSERVER_H_
