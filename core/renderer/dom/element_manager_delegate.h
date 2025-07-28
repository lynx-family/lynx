// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_DOM_ELEMENT_MANAGER_DELEGATE_H_
#define CORE_RENDERER_DOM_ELEMENT_MANAGER_DELEGATE_H_

#include <memory>
#include <string>

#include "core/template_bundle/lynx_template_bundle.h"

namespace lynx {
namespace tasm {

class FrameElement;
class PipelineContext;
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
  virtual void DidFrameBundleLoaded(const std::string &src,
                                    LynxTemplateBundle bundle) = 0;
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

  virtual void OnLayoutAfter(PipelineLayoutData &data) = 0;
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_DOM_ELEMENT_MANAGER_DELEGATE_H_
