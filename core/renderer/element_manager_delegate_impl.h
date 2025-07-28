// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_ELEMENT_MANAGER_DELEGATE_IMPL_H_
#define CORE_RENDERER_ELEMENT_MANAGER_DELEGATE_IMPL_H_

#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include "core/renderer/dom/element_manager_delegate.h"
#include "core/resource/lazy_bundle/lazy_bundle_loader.h"

namespace lynx {
namespace tasm {

class PipelineContext;

/**
 * Member of TemplateAssembler, provided to ElementManager.
 */
class ElementManagerDelegateImpl : public ElementManagerDelegate {
 public:
  explicit ElementManagerDelegateImpl(TemplateAssembler *tasm) : tasm_(tasm) {}
  ~ElementManagerDelegateImpl() override = default;

  void LoadFrameBundle(const std::string &src, FrameElement *element) override;

  void DidFrameBundleLoaded(const std::string &src,
                            LynxTemplateBundle bundle) override;

  void OnFrameRemoved(FrameElement *element) override;

  void SetBundleLoader(const std::shared_ptr<LazyBundleLoader> &loader) {
    bundle_loader_ = loader;
  }

  PipelineContext *GetCurrentPipelineContext() override;

  PipelineContext *CreateAndUpdateCurrentPipelineContext(
      const std::shared_ptr<PipelineOptions> &pipeline_options,
      bool is_major_updated = false) override;

  void SendGlobalEvent(const std::string &event,
                       const lepus::Value &info) override;

  void OnLayoutAfter(PipelineLayoutData &data) override;

 private:
  std::unordered_set<FrameElement *> frame_element_set_;
  std::unordered_map<std::string, std::shared_ptr<LynxTemplateBundle>>
      frame_bundles_{};
  std::shared_ptr<LazyBundleLoader> bundle_loader_{nullptr};
  TemplateAssembler *tasm_{nullptr};
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_ELEMENT_MANAGER_DELEGATE_IMPL_H_
