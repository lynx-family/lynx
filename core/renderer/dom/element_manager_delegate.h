// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_DOM_ELEMENT_MANAGER_DELEGATE_H_
#define CORE_RENDERER_DOM_ELEMENT_MANAGER_DELEGATE_H_

#include <cstdint>
#include <memory>
#include <string>

#include "base/include/fml/memory/ref_ptr.h"
#include "core/event/event_dispatch_result.h"
#include "core/resource/lazy_bundle/lazy_bundle_loader.h"
#include "core/template_bundle/lynx_template_bundle.h"

namespace lynx {
namespace runtime {
class MessageEvent;
class MTSRuntime;
}  // namespace runtime

namespace worklet {
class LepusApiHandler;
}  // namespace worklet

namespace tasm {

enum class EventResult : int;
class FrameElement;
struct PipelineOptions;
class PipelineContext;
class PageConfig;
struct PipelineLayoutData;

/**
 * ElementManagerDelegate provides APIs which ElementManager needs to call but
 * not implemented in ElementManager.
 */
class ElementManagerDelegate {
 public:
  ElementManagerDelegate() = default;
  virtual ~ElementManagerDelegate() = default;

  /**
   * APIs to load bundle for frame and manage frame element.
   */
  // load bundle for frame element
  virtual void LoadFrameBundle(const std::string &src,
                               FrameElement *element) = 0;
  // callback for frame bundle loaded
  virtual void DidFrameBundleLoaded(
      const LazyBundleLoader::CallBackInfo &callback_info) = 0;
  // Call for frame removed.
  virtual void OnFrameRemoved(FrameElement *element) = 0;

  // Call for the current pipeline context.
  virtual PipelineContext *GetCurrentPipelineContext() = 0;

  // Call for create and update the pipeline context.
  virtual PipelineContext *CreateAndUpdateCurrentPipelineContext(
      const std::shared_ptr<PipelineOptions> &pipeline_options,
      bool is_major_updated = false) = 0;

  // Call for sending global event.
  virtual void SendGlobalEvent(const std::string &event,
                               const lepus::Value &info) = 0;

  virtual std::shared_ptr<PageConfig> GetPageConfigForEntry(
      const std::string &entry_name) const {
    return nullptr;
  }

  // Call for sending Lepus global event.
  virtual void TriggerLepusGlobalEvent(const std::string &event,
                                       const lepus::Value &info) = 0;

  // Call for dispatching message event.
  virtual event::DispatchEventResult DispatchMessageEvent(
      fml::RefPtr<runtime::MessageEvent> event) = 0;

  virtual bool EnableEventHandleRefactor() const = 0;

  virtual bool SupportComponentJS() const = 0;

  virtual runtime::MTSRuntime *GetDefaultEntryRuntime() const = 0;

  virtual runtime::MTSRuntime *GetEntryRuntime(
      const std::string &entry_name) const = 0;

  virtual std::string GetDefaultEntryLogicalName() const = 0;

  virtual EventResult FireElementWorkletAndRequestResolve(
      const std::string &component_id, const std::string &entry_name,
      const lepus::Value &callback, const lepus::Value &event_detail,
      const std::shared_ptr<worklet::LepusApiHandler> &task_handler,
      int32_t element_id,
      std::shared_ptr<PipelineOptions> &pipeline_options) = 0;

  virtual void OnLayoutAfter(PipelineLayoutData &data) = 0;
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_DOM_ELEMENT_MANAGER_DELEGATE_H_
