// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_DOM_FIBER_FRAME_ELEMENT_H_
#define CORE_RENDERER_DOM_FIBER_FRAME_ELEMENT_H_

#include <memory>
#include <string>
#include <utility>

#include "core/renderer/dom/element_manager.h"
#include "core/renderer/dom/fiber/fiber_element.h"
#include "core/resource/lazy_bundle/lazy_bundle_loader.h"
#include "core/template_bundle/lynx_template_bundle.h"

namespace lynx {
namespace tasm {
struct FrameElementData {
  FrameElementData(const std::string& src,
                   std::shared_ptr<LynxTemplateBundle>&& bundle,
                   int32_t error_code, const std::string& error_message)
      : src(src),
        bundle(std::move(bundle)),
        error_code(error_code),
        error_message(error_message) {}
  ~FrameElementData() = default;
  std::string src;
  std::shared_ptr<LynxTemplateBundle> bundle;
  int32_t error_code;
  std::string error_message;
};

class FrameElement : public FiberElement {
 public:
  explicit FrameElement(ElementManager* element_manager);
  ~FrameElement() override;

  void SetAttribute(const base::String& key, const lepus::Value& value,
                    bool need_update_data_model = true) override;

  bool DidBundleLoaded(const std::shared_ptr<FrameElementData>& data);

  void FlushProps() override;

  const std::string& GetSrc() const;

 protected:
  void OnNodeAdded(FiberElement* child) override;

 private:
  // load bundle if src is set
  void OnSetSrc(const base::String& key, const lepus::Value& value);

  // send load event for `bindload` callback
  void SendLoadEvent(const std::shared_ptr<FrameElementData>& data);

  std::shared_ptr<FrameElementData> bundle_data_{nullptr};
  std::string src_{};
};
}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_DOM_FIBER_FRAME_ELEMENT_H_
