// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_DOM_FIBER_FIBER_ELEMENT_H_
#define CORE_RENDERER_DOM_FIBER_FIBER_ELEMENT_H_

#include <list>
#include <memory>
#include <string>
#include <tuple>
#include <utility>

#include "base/include/auto_create_optional.h"
#include "base/include/fml/memory/ref_counted.h"
#include "base/include/vector.h"
#include "base/trace/native/trace_event.h"
#include "core/base/thread/once_task.h"
#include "core/event/event_listener.h"
#include "core/renderer/css/css_fragment_decorator.h"
#include "core/renderer/css/css_property.h"
#include "core/renderer/css/css_property_bitset.h"
#include "core/renderer/css/css_style_sheet_manager.h"
#include "core/renderer/css/css_value.h"
#include "core/renderer/dom/attribute_holder.h"
#include "core/renderer/dom/element.h"
#include "core/renderer/dom/fiber/list_item_scheduler_adapter.h"
#include "core/renderer/dom/layout_bundle.h"
#include "core/renderer/simple_styling/style_object.h"
#include "core/renderer/utils/base/element_template_info.h"

namespace lynx {
namespace tasm {
class NodeManager;
class PlatformLayoutFunctionWrapper;

enum NodeInfoBits : int32_t {
  // Mask for layout node type, using lower 16 bits.
  kLayoutNodeTypeMask = 0x0000FFFF,
  // Mask for async creation flag.
  kCreateAsyncMask = 0x00010000,
};

constexpr const int32_t kCommonBuiltInNodeInfo =
    (static_cast<int32_t>(LayoutNodeType::COMMON) &
     NodeInfoBits::kLayoutNodeTypeMask) |
    NodeInfoBits::kCreateAsyncMask;
constexpr const int32_t kVirtualBuiltInNodeInfo =
    (static_cast<int32_t>(LayoutNodeType::VIRTUAL) &
     NodeInfoBits::kLayoutNodeTypeMask);
constexpr const int32_t kCustomBuiltInNodeInfo =
    (static_cast<int32_t>(LayoutNodeType::CUSTOM) &
     NodeInfoBits::kLayoutNodeTypeMask) |
    NodeInfoBits::kCreateAsyncMask;

class FiberElement : public Element {
 public:
  using Action = Element::Action;
  using ActionParam = Element::ActionParam;
  using AsyncResolveStatus = Element::AsyncResolveStatus;

  FiberElement(ElementManager* manager, const base::String& tag);
  FiberElement(ElementManager* manager, const base::String& tag,
               int32_t css_id);

  // This function will clone an incomplete fiber element that is not attached
  // to the element manager. Before using this fiber element, it needs to be
  // attached to the element manager first.
  fml::RefPtr<Element> CloneElement(bool clone_resolved_props) const override {
    // Because the performance of the copy constructor is better than the
    // combination of default construction and assignment operation, we choose
    // to use the copy constructor to copy the element here. To minimize the
    // impact caused by exposing the copy constructor, we have made it protected
    // and encapsulated it in CloneElement.
    return fml::AdoptRef<Element>(
        new FiberElement(*this, clone_resolved_props));
  }

  void SetupFragmentBehavior(Fragment* fragment) override;

  ~FiberElement() override = default;

  void TraversalInsertFixedElementOfTree() override;

  void ConsumeStyle(const StyleMap& styles,
                    const StyleMap* inherit_styles) override;

  const EventMap& event_map() const override {
    if (data_model_) {
      return data_model_->static_events();
    }
    return AttributeHolder::EventBundle::DefaultEmptyEventMap();
  }
  const EventMap& lepus_event_map() override {
    if (data_model_) {
      return data_model_->lepus_events();
    }
    return AttributeHolder::EventBundle::DefaultEmptyEventMap();
  }

  virtual void CheckHasInlineContainer(Element* parent) override;

  void OnPseudoStatusChanged(PseudoState prev_status,
                             PseudoState current_status) override;

  // The text element can call this function to convert child fiber elements
  // into inline elements. Currently, only view, text, image and wrapper
  // elements may be converted into inline elements.

  bool IsEventPathCatch(event::EventTarget* target,
                        event::Event* event) override;

 protected:
  FiberElement(const FiberElement& element, bool clone_resolved_props);

  void ConsumeStyleInternal(
      const StyleMap& styles, const StyleMap* inherit_styles,
      std::function<bool(CSSPropertyID, const tasm::CSSValue&)> should_skip)
      override;

  void SetStyleInternal(CSSPropertyID id, const tasm::CSSValue& value) override;
  bool ResetCSSValue(CSSPropertyID id) override;

 private:
  friend class WrapperElement;
  friend class ComponentElement;
  friend class BlockElement;

  void PrepareComponentExternalStyles(AttributeHolder* holder);
  void PrepareRootCSSVariables(AttributeHolder* holder);
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_DOM_FIBER_FIBER_ELEMENT_H_
