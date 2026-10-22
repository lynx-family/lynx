// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_DOM_FIBER_COMPOSE_ELEMENT_HANDLE_H_
#define CORE_RENDERER_DOM_FIBER_COMPOSE_ELEMENT_HANDLE_H_

#include <cstdint>
#include <memory>
#include <optional>
#include <utility>

#include "base/include/fml/memory/ref_ptr.h"
#include "base/include/value/base_string.h"
#include "base/include/value/ref_counted_class.h"
#include "core/renderer/dom/element.h"

namespace lynx {
namespace tasm {

class CustomLayout;
class ComposeModifierApplicator;
class ElementManager;
class TemplateAssembler;

enum class ComposeElementKind : int32_t {
  // Reserve zero for custom tags; positive ids are dedicated element kinds.
  kCustom = 0,
  kView = 1,
  kText = 2,
  kImage = 3,
  kList = 4,
  kSvg = 5,
  kOverlay = 6,
  kAnimaX = 7,
};

// Stable, opaque identity for one Compose-owned Fiber subtree. The concrete
// Element owns the built-in or custom-tag behavior; mount_root_ describes the
// physical node that should be inserted into its external parent.
class ComposeElementHandle final : public lepus::RefCounted {
 public:
  // tag is used only for kCustom; tasm supplies the runtime for kList.
  ComposeElementHandle(ElementManager* manager, ComposeElementKind kind,
                       const base::String& tag = base::String(),
                       TemplateAssembler* tasm = nullptr);

  ~ComposeElementHandle() override;

  bool SetCustomLayout(std::shared_ptr<CustomLayout> layout);
  void visitor(void* rt, void* func, uint64_t trace_tool) override;
  const std::shared_ptr<CustomLayout>& custom_layout() const {
    return custom_layout_;
  }

  void ReleaseSelf() const override { delete this; }
  lepus::RefType GetRefType() const override {
    return lepus::RefType::kComposeElementHandle;
  }

  const fml::RefPtr<Element>& content_element() const {
    return content_element_;
  }
  const fml::RefPtr<Element>& mount_root() const { return mount_root_; }

  // Engine-owner-thread only; validation failures do not prepare the node.
  std::optional<LayoutResult> MeasureText(ElementManager* manager, double width,
                                          double width_mode, double height,
                                          double height_mode,
                                          const char** error);

 private:
  friend class ComposeModifierApplicator;

  void SetMountRoot(fml::RefPtr<Element> mount_root) {
    mount_root_ = std::move(mount_root);
  }

  void ClearCustomLayout();

  fml::RefPtr<Element> content_element_;
  fml::RefPtr<Element> mount_root_;
  std::shared_ptr<CustomLayout> custom_layout_;
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_DOM_FIBER_COMPOSE_ELEMENT_HANDLE_H_
