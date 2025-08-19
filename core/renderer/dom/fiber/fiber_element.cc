// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/dom/fiber/fiber_element.h"

#include <algorithm>
#include <deque>
#include <memory>
#include <stack>
#include <string>
#include <utility>
#include <vector>

#include "base/include/compiler_specific.h"
#include "base/include/path_utils.h"
#include "base/include/timer/time_utils.h"
#include "base/include/value/array.h"
#include "base/include/value/base_string.h"
#include "base/include/value/table.h"
#include "base/trace/native/trace_event.h"
#include "core/renderer/css/css_color.h"
#include "core/renderer/css/css_keyframes_token.h"
#include "core/renderer/css/css_property.h"
#include "core/renderer/css/css_utils.h"
#include "core/renderer/css/layout_property.h"
#include "core/renderer/css/parser/length_handler.h"
#include "core/renderer/css/unit_handler.h"
#include "core/renderer/dom/element_manager.h"
#include "core/renderer/dom/fiber/component_element.h"
#include "core/renderer/dom/fiber/image_element.h"
#include "core/renderer/dom/fiber/list_element.h"
#include "core/renderer/dom/fiber/none_element.h"
#include "core/renderer/dom/fiber/platform_layout_function_wrapper.h"
#include "core/renderer/dom/fiber/raw_text_element.h"
#include "core/renderer/dom/fiber/scroll_element.h"
#include "core/renderer/dom/fiber/text_element.h"
#include "core/renderer/dom/fiber/tree_resolver.h"
#include "core/renderer/dom/fiber/view_element.h"
#include "core/renderer/dom/fiber/wrapper_element.h"
#include "core/renderer/dom/list_component_info.h"
#include "core/renderer/dom/vdom/radon/node_select_options.h"
#include "core/renderer/dom/vdom/radon/node_selector.h"
#include "core/renderer/page_proxy.h"
#include "core/renderer/simple_styling/style_object.h"
#include "core/renderer/starlight/layout/layout_object.h"
#include "core/renderer/starlight/style/default_layout_style.h"
#include "core/renderer/template_assembler.h"
#include "core/renderer/trace/renderer_trace_event_def.h"
#include "core/renderer/utils/lynx_env.h"
#include "core/renderer/utils/value_utils.h"
#include "core/runtime/bindings/jsi/java_script_element.h"
#include "core/services/event_report/event_tracker.h"
#include "core/services/feature_count/feature_counter.h"
#include "core/services/feature_count/global_feature_counter.h"
#include "core/value_wrapper/value_impl_lepus.h"

namespace lynx {
namespace tasm {

FiberElement::FiberElement(ElementManager *manager, const base::String &tag)
    : FiberElement(manager, tag, kInvalidCssId) {}

FiberElement::FiberElement(ElementManager *manager, const base::String &tag,
                           int32_t css_id)
    : Element(tag, manager), dirty_(kDirtyCreated), css_id_(css_id) {
  InitLayoutBundle();
  SetAttributeHolder(fml::MakeRefCounted<AttributeHolder>(this));

  if (tag.IsEquals("x-overlay-ng")) {
    can_has_layout_only_children_ = false;
  }

  if (manager == nullptr) {
    return;
  }

  element_context_delegate_ = manager;

  // Set font scale and font size if needed.
  const auto &env_config = manager->GetLynxEnvConfig();

  computed_css_style()->SetFontScale(env_config.FontScale());
  if (Config::DefaultFontScale() != env_config.FontScale()) {
    SetComputedFontSize(tasm::CSSValue(), env_config.PageDefaultFontSize(),
                        env_config.PageDefaultFontSize(), true);
  }

  if (element_manager_->GetEnableStandardCSSSelector()) {
    // in new selector, mark style dirty while Created.
    MarkDirty(kDirtyStyle);
  }
}

FiberElement::FiberElement(const FiberElement &element,
                           bool clone_resolved_props)
    : Element(element, clone_resolved_props),
      invalidation_lists_(element.invalidation_lists_),
      parent_component_unique_id_(element.parent_component_unique_id_),
      dirty_(element.dirty_ | kDirtyCreated | kDirtyCloned),
      css_id_(element.css_id_),
      dynamic_style_flags_(element.dynamic_style_flags_),
      has_extreme_parsed_styles_(element.has_extreme_parsed_styles_),
      only_selector_extreme_parsed_styles_(
          element.only_selector_extreme_parsed_styles_),
      can_be_layout_only_(element.can_be_layout_only_),
      is_template_(element.is_template_),
      flush_required_(element.flush_required_),
      full_raw_inline_style_(element.full_raw_inline_style_),
      current_raw_inline_styles_(element.current_raw_inline_styles_),
      extreme_parsed_styles_(element.extreme_parsed_styles_),
      inherited_styles_(element.inherited_styles_),
      reset_inherited_ids_(element.reset_inherited_ids_),
      updated_attr_map_(element.updated_attr_map_),
      builtin_attr_map_(element.builtin_attr_map_),
      reset_attr_vec_(element.reset_attr_vec_),
      part_id_(element.part_id_) {
  SetAttributeHolder(
      fml::MakeRefCounted<AttributeHolder>(*element.data_model()));
  data_model_->SetCSSVariableBundle(*element.data_model());

  if (clone_resolved_props) {
    parsed_styles_map_ = element.parsed_styles_map_;
    updated_inherited_styles_ = element.updated_inherited_styles_;
    layout_styles_ = element.layout_styles_;

    // FIXME(wujintian): The prop bundle stores the style of incremental
    // updates. If the element flush props has been executed multiple times
    // before cloning the element, then this prop bundle cannot represent all
    // the stock styles since the element was created.
    if (element.pre_prop_bundle_) {
      prop_bundle_ = element.pre_prop_bundle_->ShallowCopy();
    } else if (element.prop_bundle_) {
      prop_bundle_ = element.prop_bundle_->ShallowCopy();
    }
  }

  if (element.config().IsTable() && element.config().GetLength() > 0) {
    config_ = lepus::Value::ShallowCopy(element.config()).Table();
  }

  element_context_delegate_ = element.element_context_delegate_;
  // TODO(wujintian): Clone animation-related objects.
}

void FiberElement::AttachToElementManager(
    ElementManager *manager,
    const std::shared_ptr<CSSStyleSheetManager> &style_manager,
    bool keep_element_id) {
  Element::AttachToElementManager(manager, style_manager, keep_element_id);

  const auto &env_config = manager->GetLynxEnvConfig();
  if (platform_css_style_ == nullptr) {
    platform_css_style_ = std::make_unique<starlight::ComputedCSSStyle>(
        *manager->platform_computed_css());
  }
  record_parent_font_size_ = env_config.DefaultFontSize();

  // ComputedCSSStyle
  platform_css_style_->SetScreenWidth(env_config.ScreenWidth());
  platform_css_style_->SetViewportHeight(env_config.ViewportHeight());
  platform_css_style_->SetViewportWidth(env_config.ViewportWidth());
  platform_css_style_->SetCssAlignLegacyWithW3c(
      manager->GetLayoutConfigs().css_align_with_legacy_w3c_);
  platform_css_style_->SetFontScaleOnlyEffectiveOnSp(
      manager->GetLynxEnvConfig().FontScaleSpOnly());

  // Create layout node and update layout styles
  InitLayoutBundle();
  UpdateLayoutNodeFontSize(GetFontSize(), GetRecordedRootFontSize());

  if (layout_styles_.has_value()) {
    for (auto &layout_style : *layout_styles_) {
      UpdateLayoutNodeStyle(layout_style.first, layout_style.second);
    }
  }

  SetFontSizeForAllElement(GetFontSize(), GetRecordedRootFontSize());

  if (Config::DefaultFontScale() != env_config.FontScale()) {
    computed_css_style()->SetFontScale(env_config.FontScale());
  }

  if (Config::DefaultFontScale() != env_config.FontScale()) {
    SetComputedFontSize(tasm::CSSValue(), env_config.PageDefaultFontSize(),
                        env_config.PageDefaultFontSize(), true);
  }

  if (element_manager_->GetEnableStandardCSSSelector()) {
    // in new selector, mark style dirty while Created.
    MarkDirty(kDirtyStyle);
  }

  element_context_delegate_ = manager;
}

void FiberElement::OnNodeAdded(FiberElement *child) {
  if (is_inline_element() && child != nullptr) {
    child->ConvertToInlineElement();
  }

  UpdateRenderRootElementIfNecessary(child);
}

bool FiberElement::ShouldDestroy() const {
  return !will_destroy_ && element_manager();
}

FiberElement::~FiberElement() {
  if (ShouldDestroy()) {
    element_manager_->EraseGlobalBindElementId(global_bind_event_map(),
                                               impl_id());
    element_manager()->NotifyElementDestroy(this);
    DestroyPlatformNode();
    element_manager()->DestroyLayoutNode(impl_id());
    element_manager()->node_manager()->Erase(id_);
    if (customized_layout_node_) {
      customized_layout_node_->Destroy();
    }
    // If FiberElement to be destroyed is the root of its ElementContext, need
    // to remove corresponding ElementContext from tree
    if (element_context_delegate_ &&
        element_context_delegate_->GetElementContextRoot() == this) {
      element_context_delegate_->RemoveSelf();
    }
  }
}

void FiberElement::SetDefaultOverflow(bool visible) {
  computed_css_style()->SetOverflowDefaultVisible(visible);
  overflow_ = visible ? OVERFLOW_XY : OVERFLOW_HIDDEN;
}

void FiberElement::RequireFlush() {
  if (flush_required_) {
    return;
  }
  MarkRequireFlush();
  auto *parent = static_cast<FiberElement *>(parent_);
  if (parent && !parent->flush_required_) {
    parent->RequireFlush();
  }
}

const FiberElement::InheritedProperty FiberElement::GetInheritedProperty() {
  return {children_propagate_inherited_styles_flag_, inherited_styles_.get(),
          reset_inherited_ids_.get()};
}

const FiberElement::InheritedProperty
FiberElement::GetParentInheritedProperty() {
  // If in a parallel flush process or if the parent is null, return
  // empty InheritedProperty indicating that it is not necessary to consider the
  // inheritance logic at this time.
  if (this->is_parallel_flush()) {
    return InheritedProperty();
  }

  FiberElement *real_parent = static_cast<FiberElement *>(parent());
  if (real_parent == nullptr) {
    return InheritedProperty();
  }

  return real_parent->GetInheritedProperty();
}

bool FiberElement::NeedFastFlushPath(
    const std::pair<CSSPropertyID, tasm::CSSValue> &style) {
  return style.second.IsEmpty() || LayoutProperty::IsLayoutOnly(style.first) ||
         LayoutProperty::IsLayoutWanted(style.first) ||
         starlight::CSSStyleUtils::IsLayoutRelatedTransform(style) ||
         style.first == kPropertyIDColor || style.first == kPropertyIDFilter;
}

void FiberElement::SetKeyframesByNamesInner(fml::RefPtr<PropBundle> bundle) {
  painting_context()->SetKeyframes(std::move(bundle));
}

void FiberElement::ResolveParentComponentElement() const {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, FIBER_ELEMENT_RESOLVE_PARENT_COMPONENT);
  // parent_component_unique_id_ less than page element element id is invalid.
  if (!parent_component_element_ &&
      parent_component_unique_id_ >= kInitialImplId) {
    if (element_manager()->GetPageElement() != nullptr &&
        parent_component_unique_id_ ==
            element_manager()->GetPageElement()->impl_id()) {
      // fast path: if parent_component_unique_id is page element's id, set
      // parent_component_element to page_element
      parent_component_element_ = element_manager()->GetPageElement();
    } else {
      ResolveParentComponentElementImpl();
    }
  }
}

void FiberElement::ResolveParentComponentElementImpl() const {
  if (this->parent() == nullptr) {
    return;
  }

  FiberElement *anchor = static_cast<FiberElement *>(this->parent());
  while (anchor != nullptr) {
    if (anchor->parent_component_unique_id_ == parent_component_unique_id_ &&
        anchor->parent_component_element_ != nullptr) {
      // anchor element has identical parent_component_element with current
      // element, reuse anchor element's parent component element
      parent_component_element_ = anchor->parent_component_element_;
      return;
    }

    if (anchor->impl_id() == parent_component_unique_id_) {
      // anchor element is current element's parent component element
      parent_component_element_ = anchor;
      return;
    }

    anchor = static_cast<FiberElement *>(anchor->parent());
  }
}

Element *FiberElement::GetParentComponentElement() const {
  if (IsDetached()) {
    // if the Element is not attached, it is meaningless to return parent
    // component, and more important, the parent component may be destroyed!
    return nullptr;
  }
  ResolveParentComponentElement();
  return parent_component_element_;
}

CSSFragment *FiberElement::GetRelatedCSSFragment() {
  if (css_id_ != kInvalidCssId) {
    if (!style_sheet_) {
      if (!css_style_sheet_manager_ && GetParentComponentElement()) {
        css_style_sheet_manager_ =
            static_cast<ComponentElement *>(GetParentComponentElement())
                ->style_sheet_manager();
      }
      CSSFragment *fragment =
          css_style_sheet_manager_
              ? css_style_sheet_manager_->GetCSSStyleSheetForComponent(css_id_)
              : nullptr;
      style_sheet_ = std::make_unique<CSSFragmentDecorator>(fragment);
    }
    return style_sheet_.get();
  } else {
    auto *parent_component = GetParentComponentElement();
    if (parent_component) {
      return static_cast<ComponentElement *>(parent_component)
          ->GetCSSFragment();
    } else {
      return nullptr;
    }
  }
}

int32_t FiberElement::GetCSSID() const {
  if (css_id_ != kInvalidCssId) {
    return css_id_;
  } else {
    auto *parent_component = GetParentComponentElement();
    if (parent_component) {
      return static_cast<ComponentElement *>(parent_component)
          ->GetComponentCSSID();
    } else {
      return kInvalidCssId;
    }
  }
}

size_t FiberElement::CountInlineStyles() {
  return current_raw_inline_styles_.has_value()
             ? CSSProperty::GetTotalParsedStyleCountFromMap(
                   *current_raw_inline_styles_)
             : 0;
}

void FiberElement::MergeInlineStyles(StyleMap &new_styles) {
  // Styles stored by full_raw_inline_style_ had already been parsed to
  // current_raw_inline_styles_. So we only handle current_raw_inline_styles_
  // here.
  if (current_raw_inline_styles_.has_value()) {
    auto &configs = element_manager_->GetCSSParserConfigs();
    for (const auto &style : *current_raw_inline_styles_) {
      UnitHandler::Process(style.first, style.second, new_styles, configs);
    }
  }
}

void FiberElement::ProcessFullRawInlineStyle() {
  // If self has raw inline styles, parse to current_raw_inline_styles_ but do
  // not process to final style map. Inline styles will be merged finally by
  // MergeInlineStyles.
  if (!full_raw_inline_style_.empty()) {
    ParseRawInlineStyles(nullptr);
    full_raw_inline_style_ = base::String();
  }
}

bool FiberElement::WillResolveStyle(StyleMap &merged_styles) {
  ProcessFullRawInlineStyle();
  return true;
}

void FiberElement::DispatchAsyncResolveProperty() {
  if ((dirty_ & ~kDirtyTree) != 0 && IsAttached()) {
    UpdateResolveStatus(AsyncResolveStatus::kPreparing);
    ResolveParentComponentElement();
    if (parent()) {
      parent()->EnsureTagInfo();
    }
    PostResolveTaskToThreadPool(false, element_manager()->ParallelTasks());
  }
}

#pragma region simple styling

void FiberElement::SetStyleObjects(
    std::unique_ptr<style::StyleObject *, style::StyleObjectArrayDeleter>
        style_objects) {
  last_style_objects_ = std::move(style_objects_);

  style_objects_ = std::move(style_objects);

  MarkDirty(kDirtyForceUpdate | kDirtyStyleObjects);
}

void FiberElement::UpdateSimpleStyles(const tasm::StyleMap &style_map) {
  std::for_each(
      style_map.begin(), style_map.end(), [this](const auto &pair) -> void {
        EXEC_EXPR_FOR_INSPECTOR(
            if (element_manager_ && element_manager_->IsDomTreeEnabled()) {
              if (pair.second.IsEmpty()) {
                data_model()->ResetInlineStyle(pair.first);
              } else {
                data_model()->SetInlineStyle(pair.first, pair.second);
              }
            });
        if (pair.second.IsEmpty()) {
          ResetSimpleStyle(pair.first);
        } else {
          if (pair.first == kPropertyIDFontSize) {
            SetFontSize(pair.second);
            // FIXME(linxs): to be determined if we need to align with the
            // process of kDirtyFontSize
            dirty_ &= ~kDirtyFontSize;
          } else {
            this->SetStyleInternal(pair.first, pair.second);
          }
        }
      });
  EXEC_EXPR_FOR_INSPECTOR(
      element_manager()->OnElementNodeSetForInspector(this););
  MarkDirty(kDirtyForceUpdate);
}

void FiberElement::ResetSimpleStyle(const tasm::CSSPropertyID id) {
  if (id == kPropertyIDFontSize) {
    ResetFontSize();
  }
  ResetStyleInternal(id);
  EXEC_EXPR_FOR_INSPECTOR({
    if (element_manager_ && element_manager_->IsDomTreeEnabled()) {
      data_model()->ResetInlineStyle(id);
    }
  });
}

#pragma endregion  // simple styling

void FiberElement::AsyncResolveProperty() {
  if ((dirty_ & ~kDirtyTree) != 0) {
    TRACE_EVENT(LYNX_TRACE_CATEGORY, FIBER_ELEMENT_ASYNC_RESOLVE_PROPERTY);
    UpdateResolveStatus(AsyncResolveStatus::kPrepareRequested);
    if (this->IsAttached()) {
      AsyncPostResolveTaskToThreadPool();
    }
  }
}

void FiberElement::AsyncPostResolveTaskToThreadPool() {
  if ((dirty_ & ~kDirtyTree) != 0) {
    UpdateResolveStatus(AsyncResolveStatus::kPrepareTriggered);
    element_manager()->GetTasmWorkerTaskRunner()->PostTask([this]() mutable {
      UpdateResolveStatus(AsyncResolveStatus::kPreparing);
      ResolveParentComponentElement();
      if (parent()) {
        parent()->EnsureTagInfo();
      }
      PostResolveTaskToThreadPool(false, element_manager()->ParallelTasks());
    });
  }
}

void FiberElement::ReplaceElements(
    const base::Vector<fml::RefPtr<FiberElement>> &inserted,
    const base::Vector<fml::RefPtr<FiberElement>> &removed, FiberElement *ref) {
  if (removed.empty()) {
    for (const auto &child : inserted) {
      InsertNodeBeforeInternal(child, ref);
    }
    return;
  }

  // 1. Make sure remove first.
  // 2. And exec InsertNodeBeforeInternal(child, ref).

  for (const auto &child : removed) {
    RemoveNode(child);
  }
  if (!inserted.empty()) {
    for (const auto &child : inserted) {
      InsertNodeBeforeInternal(child, ref);
    }
  }
}

void FiberElement::InsertNode(const fml::RefPtr<Element> &raw_child) {
  InsertNode(raw_child, static_cast<int32_t>(scoped_children_.size()));
}

void FiberElement::InsertNode(const fml::RefPtr<Element> &raw_child,
                              int32_t index) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, FIBER_ELEMENT_INSERT_NODE);
  auto child = fml::static_ref_ptr_cast<FiberElement>(raw_child);

  if (index < 0 || index > static_cast<int>(scoped_children_.size())) {
    LOGE("[FiberElement] InsertNode index is out of bounds, index:"
         << index << ",size:" << scoped_children_.size());
    return;
  }
  // reserve parent node for block element in AirModeFiber
  if (element_manager() && element_manager()->IsAirModeFiberEnabled() &&
      child->is_block()) {
    child->set_parent(this);
    scoped_virtual_children_->push_back(child);
    return;
  }
  // ref_node: nullptr: means to append this node to the end
  FiberElement *ref = (index < static_cast<int>(scoped_children_.size()))
                          ? scoped_children_[index].get()
                          : nullptr;
  InsertNodeBeforeInternal(child, ref);
}

void FiberElement::InsertNodeBeforeInternal(
    const fml::RefPtr<FiberElement> &child, FiberElement *ref_node) {
  int index = -1;
  if (ref_node) {
    index = IndexOf(ref_node);
    if (index >= static_cast<int>(scoped_children_.size()) || index < 0) {
      LOGE("[Fiber] can not find the ref node:" << ref_node);
      return;
    }
  }
  if (child->parent_ != nullptr) {
    LOGE(
        "FiberElement re-insert node, try to do remove node from old parent "
        "first");
    this->LogNodeInfo();
    child->LogNodeInfo();
    static_cast<FiberElement *>(child->parent_)->LogNodeInfo();
    static_cast<FiberElement *>(child->parent_)->RemoveNode(child);
  }
  // FIXME(linxs): use linked element to reduce the Element index calculation
  AddChildAt(child, index);

  // the insert Action should be inserted to Child, should make sure the child
  // has been flushed
  if (has_to_store_insert_remove_actions_) {
    action_param_list_.emplace_back(Action::kInsertChildAct, this, child, index,
                                    ref_node, child->is_fixed_);
  }

  if (IsCSSInheritanceEnabled()) {
    // new inserted child should be marked to do inheritance from parent
    child->MarkDirty(kDirtyPropagateInherited);
  }
  MarkDirty(kDirtyTree);
}

void FiberElement::InsertNodeBefore(
    const fml::RefPtr<FiberElement> &child,
    const fml::RefPtr<FiberElement> &reference_child) {
  InsertNodeBeforeInternal(child, reference_child.get());
}

void FiberElement::RemoveNode(const fml::RefPtr<Element> &raw_child,
                              bool destroy) {
  auto child = fml::static_ref_ptr_cast<FiberElement>(raw_child);

  // FIXME(linxs): to use linked node to avoid the index calculation asap!
  int index = IndexOf(child.get());
  if (index >= static_cast<int>(scoped_children_.size()) || index < 0) {
    LOGE("FiberElement RemoveNode got wrong child index!!");
    return;
  }

  // the Remove Action should be inserted to Parent, due to child has been
  // removed from element tree here
  if (has_to_store_insert_remove_actions_) {
    action_param_list_.emplace_back(Action::kRemoveChildAct, this, child, index,
                                    nullptr, child->is_fixed_,
                                    child->ZIndex() != 0);
  }

  // take care: NotifyNodeRemoved after removeAction inserted!
  OnNodeRemoved(child.get());
  TreeResolver::NotifyNodeRemoved(this, child.get());

  FiberElement *removed = scoped_children_[index].get();
  scoped_children_.erase(scoped_children_.begin() + index);
  removed->set_parent(nullptr);

  MarkDirty(kDirtyTree);
}

void FiberElement::InsertedInto(FiberElement *insertion_point) {
  MarkAttached();
  if (resolve_status_ == AsyncResolveStatus::kPrepareRequested) {
    AsyncPostResolveTaskToThreadPool();
  }
  EXEC_EXPR_FOR_INSPECTOR(if (element_manager() != nullptr) {
    element_manager()->RunDevToolFunction(
        lynx::devtool::DevToolFunction::InitStyleRoot, std::make_tuple(this));
  });
}

void FiberElement::RemovedFrom(FiberElement *insertion_point) {
  // We need to handle the intergenerational node which has zIndex or fixed,
  // they may be inserted to difference parent in UI/layout tree instead of dom
  // parent If the removed node's parent is the insertion_point, no need to do
  // any special action

  // Todo(kechenglong): Remove IsRadonArch.
  if (LynxEnv::GetInstance().GetBoolEnv(
          LynxEnv::Key::FIX_FIBER_REMOVE_TWICE_BUG, false) &&
      IsRadonArch()) {
    if (IsDetached()) {
      return;
    }

    if (!action_param_list_.empty()) {
      auto iter = action_param_list_.begin();
      while (iter != action_param_list_.end()) {
        if (iter->type_ == Action::kRemoveIntergenerationAct ||
            (iter->type_ == Action::kRemoveChildAct &&
             (iter->is_fixed_ || iter->has_z_index_))) {
          iter->type_ = Action::kRemoveIntergenerationAct;
          insertion_point->action_param_list_.emplace_back(std::move(*iter));
          iter = action_param_list_.erase(iter);
        } else {
          ++iter;
        }
      }
    }
  }

  if ((parent() != insertion_point) && (ZIndex() != 0 || is_fixed_)) {
    insertion_point->action_param_list_.emplace_back(
        Action::kRemoveIntergenerationAct, insertion_point,
        fml::RefPtr<FiberElement>(this), 0, nullptr, is_fixed_);
    MarkDirty(kDirtyReAttachContainer);
  }

  MarkDetached();
}

void FiberElement::DestroyPlatformNode() {
  if (element_container() && has_painting_node_) {
    element_container()->Destroy();
  }
  has_painting_node_ = false;
  MarkPlatformNodeDestroyed();
}

// TODO(wujintian) : Perhaps we can provide an rvalue version of the API to
// achieve better performance. However, this would result in the need to
// maintain two versions of the code: one for lvalues and one for rvalues.
void FiberElement::SetClass(const base::String &clazz) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, FIBER_ELEMENT_SET_CLASS);

  data_model_->SetClass(clazz);
  MarkStyleDirty(NeedForceClassChangeTransmit());
}

void FiberElement::SetClasses(ClassList &&classes) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, FIBER_ELEMENT_SET_CLASSES);
  data_model_->SetClasses(std::move(classes));
  MarkStyleDirty(NeedForceClassChangeTransmit());

  // clear ssr parsed style
  if (has_extreme_parsed_styles_) {
    extreme_parsed_styles_.reset();
    has_extreme_parsed_styles_ = false;
  }
}

void FiberElement::RemoveAllClass() {
  data_model_->RemoveAllClass();
  MarkStyleDirty(NeedForceClassChangeTransmit());
}

void FiberElement::SetStyle(CSSPropertyID id, const lepus::Value &value) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, FIBER_ELEMENT_SET_STYLE);

  // When the `SetStyle` API is called, the `SetRawInlineStyles` API might
  // already have been invoked. In this case, it is necessary to call
  // `ProcessFullRawInlineStyle` first to ensure that `full_raw_inline_style_`
  // is set into `current_raw_inline_styles_`. Otherwise, `SetRawInlineStyles`
  // might override the `SetStyle` call, leading to unexpected behavior.
  ProcessFullRawInlineStyle();

  if (!value.IsEmpty()) {
    current_raw_inline_styles_->insert_or_assign(id, value);
  } else if (current_raw_inline_styles_.has_value()) {
    current_raw_inline_styles_->erase(id);
  }

  MarkDirty(kDirtyStyle);

  if (has_extreme_parsed_styles_ && !only_selector_extreme_parsed_styles_) {
    has_extreme_parsed_styles_ = false;
    extreme_parsed_styles_.reset();
  }

  // Only exec the following expr when ENABLE_INSPECTOR, such that devtool can
  // get element's inline style.
  EXEC_EXPR_FOR_INSPECTOR({
    if (element_manager_ && element_manager_->IsDomTreeEnabled()) {
      if (value.IsEmpty()) {
        data_model()->ResetInlineStyle(id);
      } else {
        data_model()->SetInlineStyle(id, value.ToString(),
                                     element_manager_->GetCSSParserConfigs());
      }
    }
  });
}

StyleMap FiberElement::GetStylesForWorklet() {
  if (!IsCSSInheritanceEnabled()) {
    return parsed_styles_map_;
  }

  StyleMap result;
  const auto inherited_property = GetParentInheritedProperty();
  if (inherited_property.inherited_styles_ != nullptr) {
    result = *inherited_property.inherited_styles_;
  }
  for (const auto &pair : parsed_styles_map_) {
    result.emplace_or_assign(pair.first, pair.second);
  }
  return result;
}

const AttrMap &FiberElement::GetAttributesForWorklet() {
  if (data_model() == nullptr) {
    static base::NoDestructor<AttrMap> kEmptyMap =
        base::NoDestructor<AttrMap>{};
    return *kEmptyMap;
  }
  return data_model()->attributes();
}

const base::String &FiberElement::GetRawInlineStyles() {
  return full_raw_inline_style_;
}

void FiberElement::SetRawInlineStyles(base::String value) {
  full_raw_inline_style_ = std::move(value);
  MarkDirty(kDirtyStyle);
}

void FiberElement::RemoveAllInlineStyles() {
  // Only exec the following expr when ENABLE_INSPECTOR, such that devtool can
  // get element's inline style.
  EXEC_EXPR_FOR_INSPECTOR({
    if (element_manager_->IsDomTreeEnabled() &&
        current_raw_inline_styles_.has_value()) {
      for (const auto &pair : *current_raw_inline_styles_) {
        const static base::String kNull;
        data_model()->SetInlineStyle(pair.first, kNull,
                                     element_manager_->GetCSSParserConfigs());
      }
    }
  });

  full_raw_inline_style_ = base::String();
  current_raw_inline_styles_.reset();
  MarkDirty(kDirtyStyle);
}

void FiberElement::SetBuiltinAttribute(ElementBuiltInAttributeEnum key,
                                       const lepus::Value &value) {
  bool key_is_legal = true;
  switch (key) {
    case ElementBuiltInAttributeEnum::NODE_INDEX:
      node_index_ = static_cast<uint32_t>(value.Number());
      break;
    case ElementBuiltInAttributeEnum::CSS_ID:
      css_id_ = static_cast<int32_t>(value.Number());
      break;
    case ElementBuiltInAttributeEnum::DIRTY_ID:
      MarkPartElement(value.String());
      break;
    case ElementBuiltInAttributeEnum::CONFIG:
      if (value.IsTable()) {
        config_ = value.Table();
      } else if (value.IsJSTable()) {
        config_ = value.ToLepusValue().Table();
      } else {
        DCHECK(false);
      }
      break;
    case ElementBuiltInAttributeEnum::IS_TEMPLATE_PART:
      if (value.Bool()) {
        MarkTemplateElement();
      }
      break;
    default:
      key_is_legal = false;
      break;
  }
  if (key_is_legal) {
    builtin_attr_map_->try_emplace(static_cast<uint32_t>(key), value);
  }
}

void FiberElement::ReserveForAttribute(size_t count) {
  updated_attr_map_.reserve(count);
}

void FiberElement::SetAttribute(const base::String &key,
                                const lepus::Value &value,
                                bool need_update_data_model) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, FIBER_ELEMENT_SET_ATTRIBUTE);

  CheckClassChangeTransmitAttribute(key, value);

  if (!value.IsEmpty()) {
    updated_attr_map_[key] = value;
    // In the RadonNode-driven Fiber architecture, the attribute
    // used for diffing is already stored in the data_model,
    //  so there is no need to update this attribute in the data_model again.
    if (need_update_data_model) {
      data_model_->SetStaticAttribute(key, value);
    }
  } else {
    reset_attr_vec_->emplace_back(key);
    if (need_update_data_model) {
      data_model_->RemoveAttribute(key);
    }
  }
  MarkDirty(kDirtyAttr);
}

void FiberElement::SetIdSelector(const base::String &idSelector) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, FIBER_ELEMENT_SET_ID_SELECTOR);
  if (element_manager() && element_manager()->GetEnableStandardCSSSelector()) {
    if (element_manager()->CSSFragmentParsingOnTASMWorkerMTSRender()) {
      element_manager()->GetTasmWorkerTaskRunner()->PostTask(
          [this, old_id = data_model_->idSelector().str(),
           new_id = idSelector.str()]() {
            CheckHasInvalidationForId(old_id, new_id);
          });
    } else {
      CheckHasInvalidationForId(data_model_->idSelector().str(),
                                idSelector.str());
    }
  }

  updated_attr_map_[BASE_STATIC_STRING(AttributeHolder::kIdSelectorAttrName)]
      .SetString(idSelector);
  data_model_->SetIdSelector(idSelector);
  MarkDirty(kDirtyStyle | kDirtyAttr);
}

bool FiberElement::CheckHasIdMapInCSSFragment() {
  auto *css_fragment = GetRelatedCSSFragment();
  // resolve styles from css fragment
  if (css_fragment && css_fragment->HasIdSelector()) {
    return true;
  }

  return false;
}

static bool DiffStyleImpl(StyleMap &old_map, StyleMap &new_map,
                          StyleMap &update_styles) {
  if (new_map.empty()) {
    return false;
  }
  // When the first screen is rendered, old_map must be empty, so there is no
  // need to perform the following for loop.
  if (old_map.empty()) {
    update_styles = new_map;
    return true;
  }
  update_styles.reserve(old_map.size() + new_map.size());
  bool need_update = false;
  // iterate all styles in new_map
  for (const auto &[key, value] : new_map) {
    // try to find the corresponding style in old_map
    auto it_old_map = old_map.find(key);
    // if r does not exist in lhs, r is a new style to add
    // if r exist in lhs but with different value, update it
    if (it_old_map == old_map.end() || value != it_old_map->second) {
      need_update = true;
      update_styles.insert_or_assign(key, value);
    }
    // erase old property which is already in new_map, then the remaining
    // properties in old_map need to be removed
    if (it_old_map != old_map.end()) {
      old_map.erase(it_old_map);
    }
  }
  return need_update;
}

// TODO: Place logic in FiberElement only for now. If other module need to apply
// same logic, move it to css_property
FiberElement::DirectionMapping FiberElement::CheckDirectionMapping(
    CSSPropertyID css_id) {
  static const base::NoDestructor<
      std::array<FiberElement::DirectionMapping, kPropertyEnd>>
      kDirectionMappingProperty([]() {
        std::array<FiberElement::DirectionMapping, kPropertyEnd>
            property_mapping_array;
        std::fill(property_mapping_array.begin(), property_mapping_array.end(),
                  FiberElement::DirectionMapping());
#define DECLARE_DIRECTION_MAPPING(name, is_logic, ltr_value, rtl_value) \
  property_mapping_array[kPropertyID##name] =                           \
      FiberElement::DirectionMapping(is_logic, ltr_value, rtl_value);
        FOREACH_DIRECTION_MAPPING_PROPERTY(DECLARE_DIRECTION_MAPPING)
#undef DECLARE_DIRECTION_MAPPING
        return property_mapping_array;
      }());

  return (*kDirectionMappingProperty)[css_id];
}

void FiberElement::ResetDirectionAwareProperty(const CSSPropertyID &id,
                                               const CSSValue &value) {
  auto css_id = id;
  auto direction_mapping = CheckDirectionMapping(css_id);
  auto is_direction_aware_property =
      direction_mapping.rtl_property_ != kPropertyStart ||
      direction_mapping.ltr_property_ != kPropertyStart;
  if (is_direction_aware_property) {
    auto tran_css_id = (IsRTL(direction_) && direction_mapping.is_logic_) ||
                               IsLynxRTL(direction_)
                           ? direction_mapping.rtl_property_
                           : direction_mapping.ltr_property_;
    ResetCSSValue(tran_css_id);
    (*pending_updated_direction_related_styles_)[css_id] = {
        value, direction_mapping.is_logic_};
  }
}

void FiberElement::HandleKeyframePropsChange() {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, FIBER_ELEMENT_HANDLE_KEYFRAME_PROPS_CHANGE,
              [this](lynx::perfetto::EventContext ctx) {
                UpdateTraceDebugInfo(ctx.event());
              });
  if (!enable_new_animator()) {
    ResolveAndFlushKeyframes();
  } else {
    SetDataToNativeKeyframeAnimator();
  }
  has_keyframe_props_changed_ = false;
}

void FiberElement::HandleDelayTask(base::MoveOnlyClosure<void> operation) {
  if (this->parallel_flush_) {
    parallel_reduce_tasks_->emplace_back(std::move(operation));
  } else {
    operation();
  }
}

void FiberElement::HandleBeforeFlushActionsTask(
    base::MoveOnlyClosure<void> operation) {
  if (this->parallel_flush_) {
    parallel_before_flush_action_tasks_->emplace_back(std::move(operation));
  } else {
    operation();
  }
}

void FiberElement::ResolveCSSStyles(
    StyleMap &parsed_styles,
    base::InlineVector<CSSPropertyID, 16> &reset_style_ids, bool &need_update,
    bool &force_use_current_parsed_style_map) {
  if (dirty_ & kDirtyStyle) {
    TRACE_EVENT(LYNX_TRACE_CATEGORY, FIBER_ELEMENT_HANDLE_STYLE,
                [this](lynx::perfetto::EventContext ctx) {
                  UpdateTraceDebugInfo(ctx.event());
                });

    RefreshStyle(parsed_styles, reset_style_ids,
                 force_use_current_parsed_style_map);

    dirty_ &= ~kDirtyStyle;
  } else if (dirty_ & kDirtyRefreshCSSVariables) {
    TRACE_EVENT(LYNX_TRACE_CATEGORY, FIBER_ELEMENT_HANDLE_STYLE,
                [this](lynx::perfetto::EventContext ctx) {
                  UpdateTraceDebugInfo(ctx.event());
                });
    RefreshStyle(parsed_styles, reset_style_ids);

    dirty_ &= ~kDirtyRefreshCSSVariables;
  }

  if (!this->parallel_flush_ && IsCSSInheritanceEnabled()) {
    TRACE_EVENT(LYNX_TRACE_CATEGORY, FIBER_ELEMENT_HANDLE_PROPAGATE_INHERITED,
                [this](lynx::perfetto::EventContext ctx) {
                  UpdateTraceDebugInfo(ctx.event());
                });

    const auto inherited_property = GetParentInheritedProperty();
    // process inherit related
    // quick check if any id in reset_style_ids is in parent inherited styles
    if (inherited_property.inherited_styles_) {
      for (auto it = reset_style_ids.begin(); it != reset_style_ids.end();) {
        // do not reset style if it's parent inherited_styles contains it
        const auto &parent_inherited_styles =
            *(inherited_property.inherited_styles_);
        if (parent_inherited_styles.find(*it) !=
            parent_inherited_styles.end()) {
          // we need to mark flag to do self recalculation for inherited
          // styles, if the style is updated instead of reset
          MarkDirtyLite(kDirtyPropagateInherited);
          it = reset_style_ids.erase(it);
        } else {
          ++it;
        }
      }
    }

    if (dirty_ & kDirtyPropagateInherited) {
      // comes here means parent propagates this change
      // there are two status:
      // 1. parent inherited style deleted; 2.parent inherited style changed;
      // #1 parent inherited style deleted
      if (inherited_property.reset_inherited_ids_ &&
          updated_inherited_styles_.has_value()) {
        for (const auto reset_id : *(inherited_property.reset_inherited_ids_)) {
          auto it = parsed_styles_map_.find(reset_id);
          if (it == parsed_styles_map_.end()) {
            if (updated_inherited_styles_->find(reset_id) !=
                updated_inherited_styles_->end()) {
              reset_style_ids.push_back(reset_id);
            }
          }
        }
      }

      // #2.parent inherited style changed
      //  merge the inherited styles, but they have lower priority
      if (inherited_property.inherited_styles_) {
        updated_inherited_styles_->clear();
        updated_inherited_styles_->reserve(
            inherited_property.inherited_styles_->size());
        for (auto &pair : *(inherited_property.inherited_styles_)) {
          auto it = parsed_styles_map_.find(pair.first);
          if (it == parsed_styles_map_.end()) {
            updated_inherited_styles_->insert_or_assign(pair.first,
                                                        pair.second);
            need_update = true;
          }
        }
      }
    }

    // kDirtyPropagateInherited flag is expected to be consumed in above logic
    // Special case: When PrepareForCreateOrUpdate function is executing
    // parallel flush pass, and CSS Inheritance is enabled, CSS Styles
    // inherited from parent element cannot be fully resolved in parallel
    // flush pass, thus only in such scenario, kDirtyPropagateInherited flag
    // need to preserved to force refresh in next pass
    dirty_ &= ~kDirtyPropagateInherited;
  }

  // Process update_map for cloned elements.
  if (dirty_ & kDirtyCloned) {
    // Because cloned elements typically do not undergo style changes,
    // animation-related styles must be reapplied to initiate keyframe or
    // transition animations.
    for (const auto &pair : parsed_styles_map_) {
      if (CSSProperty::IsTransitionProps(pair.first) ||
          CSSProperty::IsKeyframeProps(pair.first)) {
        parsed_styles.insert_or_assign(pair.first, pair.second);
      }
    }
    dirty_ &= ~kDirtyCloned;
  }

  // Process reset before update styles.

  // If the new animator is activated and this element has been created
  // before, we need to reset the transition styles in advance. Additionally,
  // the transition manager should verify each property to decide whether to
  // intercept the reset. Therefore, we break down the operations related to
  // the transition reset process into three steps:
  // 1. We check whether we need to reset transition styles in advance.
  // 2. If these styles have been reset beforehand, we can skip the transition
  // styles in the later steps.
  // 3. We review each property to determine whether the reset should be
  // intercepted.
  bool should_consume_trans_styles_in_advance =
      ShouldConsumeTransitionStylesInAdvance();
  // #1. Consume all transition styles in advance, either update_map or
  // reset_map.
  if (should_consume_trans_styles_in_advance) {
    has_transition_props_ |= ResetTransitionStylesInAdvance(reset_style_ids);
  }
  auto &update_map =
      force_use_current_parsed_style_map ? parsed_styles_map_ : parsed_styles;
  if (should_consume_trans_styles_in_advance) {
    has_transition_props_ |= ConsumeTransitionStylesInAdvance(update_map);
  }

  // #2. Check whether direction need reset
  bool direction_reset = false;
  bool text_align_reset = false;
  for (const auto &id : reset_style_ids) {
    // #2. If these transition styles have been reset beforehand, skip them
    // here.
    if (should_consume_trans_styles_in_advance &&
        CSSProperty::IsTransitionProps(id)) {
      continue;
    }
    // #3. Review each property to determine whether the reset should be
    // intercepted.
    if (css_transition_manager_ &&
        css_transition_manager_->ConsumeCSSProperty(id, CSSValue::Empty())) {
      continue;
    }

    if (id == kPropertyIDDirection) {
      direction_reset = true;
    }

    // #4. If it is text-align property, delay reset to next step.
    if (id == kPropertyIDTextAlign) {
      text_align_reset = true;
      continue;
    }

    // Since the previous element styles cannot be accessed in element, we
    // need to record some necessary styles which New Animator transition
    // needs, and it needs to be saved before rtl converted logic.
    ResetElementPreviousStyle(id);
    auto direction_aware_pair = ConvertRtlCSSPropertyID(id);
    ResetStyleInternal(direction_aware_pair.second);
    need_update = true;
  }

  // #5. Reset text_align property depending on whether direction is changed
  if (text_align_reset) {
    // #5.1 Remove id from inherited_styles_
    CSSPropertyID text_align_id = CSSPropertyID::kPropertyIDTextAlign;
    WillResetCSSValue(text_align_id);
    // #5.2 Check whether direction property is changed
    auto direction_updated =
        update_map.find(CSSPropertyID::kPropertyIDDirection) !=
        update_map.end();
    auto direction_changed = direction_reset || direction_updated;
    // #5.3 Update element text_align depending on whether direction is
    // changed
    ResetTextAlign(update_map, direction_changed);
  }

  // process direction: rtl/lynx-rtl firstly
  if (IsDirectionChangedEnabled()) {
    TRACE_EVENT(LYNX_TRACE_CATEGORY, FIBER_ELEMENT_HANDLE_DIRECTION_CHANGED,
                [this](lynx::perfetto::EventContext ctx) {
                  UpdateTraceDebugInfo(ctx.event());
                });
    do {
      // case 1: direction changed, trigger to re calculate all direction
      // related styles case 2: only direction related style updated, just do
      // rtl for this style
      auto get_direction = [](const auto &update_map,
                              const auto &updated_inherited_map,
                              const auto &pre_direction) {
        auto update_map_it =
            update_map.find(CSSPropertyID::kPropertyIDDirection);
        if (update_map_it != update_map.end()) {
          return std::make_pair(update_map_it->second,
                                static_cast<starlight::DirectionType>(
                                    update_map_it->second.GetValue().Number()));
        }
        if (updated_inherited_map.has_value()) {
          auto updated_inherited_map_it =
              updated_inherited_map->find(CSSPropertyID::kPropertyIDDirection);
          if (updated_inherited_map_it != updated_inherited_map->end()) {
            return std::make_pair(
                updated_inherited_map_it->second,
                static_cast<starlight::DirectionType>(
                    updated_inherited_map_it->second.GetValue().Number()));
          }
        }
        return std::make_pair(CSSValue::Empty(), pre_direction);
      };

      auto new_direction =
          get_direction(update_map, updated_inherited_styles_, direction_);
      if (new_direction.second == direction_) {
        break;
      }

      // Reset all direction related styles when not switching between normal
      // and ltr
      if (IsAnyRTL(new_direction.second) || IsAnyRTL(direction_)) {
        if (updated_inherited_styles_.has_value()) {
          for (const auto &css_pair : *updated_inherited_styles_) {
            ResetDirectionAwareProperty(css_pair.first, css_pair.second);
          }
        }
        for (const auto &css_pair : parsed_styles_map_) {
          ResetDirectionAwareProperty(css_pair.first, css_pair.second);
        }
      }
      if (is_text()) {
        auto current_text_align = CSSValue(
            lepus::Value(
                static_cast<int32_t>(starlight::TextAlignType::kStart)),
            CSSValuePattern::ENUM);
        current_text_align =
            ResolveCurrentStyleValue(kPropertyIDTextAlign, current_text_align);
        DynamicCSSStylesManager::UpdateDirectionAwareDefaultStyles(
            this, new_direction.second, current_text_align);
      }
      direction_ = new_direction.second;
      SetStyleInternal(kPropertyIDDirection, new_direction.first);
    } while (false);
  }

  bool root_font_size_changed =
      GetCurrentRootFontSize() != GetRecordedRootFontSize();
  if (root_font_size_changed) {
    SetFontSizeForAllElement(GetFontSize(), GetCurrentRootFontSize());
    UpdateLayoutNodeFontSize(GetFontSize(), GetCurrentRootFontSize());
  }

  // TODO: A refactor of the animation-related style handling is needed later,
  // once the correct dependencies between animation and other special CSS
  // property changes are identified. set updated Styles to element in the end
  if (!update_map.empty() ||
      (updated_inherited_styles_.has_value() &&
       !updated_inherited_styles_->empty()) ||
      (styles_from_attributes_.has_value() &&
       !styles_from_attributes_->empty())) {
    TRACE_EVENT(LYNX_TRACE_CATEGORY, FIBER_ELEMENT_HANDLE_SET_STYLE,
                [this](lynx::perfetto::EventContext ctx) {
                  UpdateTraceDebugInfo(ctx.event());
                });
    // if kDirtyPropagateInherited, need to delay to SetStyle in inherit
    // process
    ConsumeStyle(update_map, IsCSSInheritanceEnabled()
                                 ? updated_inherited_styles_.get()
                                 : nullptr);
    need_update = true;
  }

  // direction change: we always handle direction change after all styles
  // resolved
  if (pending_updated_direction_related_styles_.has_value()) {
    for (const auto &style_pair : *pending_updated_direction_related_styles_) {
      TryDoDirectionRelatedCSSChange(style_pair.first, style_pair.second.first,
                                     style_pair.second.second);
    }
    if (!element_manager_->FixFontSizeOverrideDirectionChangeBug()) {
      pending_updated_direction_related_styles_.reset();
    }
  }

  // Handle font size change
  // TODO: A refactor of the font-size-related style handling is needed later,
  // once the correct dependencies between font-size and other special CSS
  // property(text-align, direction) changes are identified.
  if (dirty_ & kDirtyFontSize) {
    TRACE_EVENT(LYNX_TRACE_CATEGORY, FIBER_ELEMENT_HANDLE_FONT_SIZE_CHANGE,
                [this](lynx::perfetto::EventContext ctx) {
                  UpdateTraceDebugInfo(ctx.event());
                });
    do {
      // If `dirty_ & kDirtyCreated`, the `parsed_styles_map_` has already
      // been fully consumed, so there is no possibility of `update_map` being
      // different from `parsed_styles_map_`. Therefore, skip this logic.
      if (dirty_ & kDirtyCreated) {
        break;
      }

      // If `dynamic_style_flags_ & DynamicCSSStylesManager::kUpdateEm ==
      // false` and `root_font_size_changed` and `dynamic_style_flags_ &
      // DynamicCSSStylesManager::kUpdateRem == false`, it indicates that the
      // current `parsed_styles_map_` does not contain any font size-sensitive
      // styles, and thus this part of the processing logic can be skipped.
      if (!(dynamic_style_flags_ & DynamicCSSStylesManager::kUpdateEm) &&
          !(root_font_size_changed &&
            dynamic_style_flags_ & DynamicCSSStylesManager::kUpdateRem)) {
        break;
      }

      // We need to reset the styles for the following style pairs because
      // they are possibly font size-sensitive:
      // 1. If the unit of the style property value is EM, CALC, MAP or ARRAY
      // 2. If the unit of the style property value is REM and
      // `root_font_size_changed`
      // 3. If the style property ID is `kPropertyIDTransform` or
      // `kPropertyIDLineHeight`
      auto should_update_em_rem_style = [](const auto &style_pair,
                                           bool root_font_size_changed) {
        return style_pair.second.GetPattern() == CSSValuePattern::EM ||
               style_pair.second.GetPattern() == CSSValuePattern::CALC ||
               style_pair.second.GetPattern() == CSSValuePattern::MAP ||
               style_pair.second.GetPattern() == CSSValuePattern::ARRAY ||
               (style_pair.second.GetPattern() == CSSValuePattern::REM &&
                root_font_size_changed) ||
               style_pair.first == CSSPropertyID::kPropertyIDTransform ||
               style_pair.first == CSSPropertyID::kPropertyIDLineHeight;
      };

      // If the style pair is font size-sensitive and the current `update_map`
      // does not include this style pair, then force the reset of this style
      // pair. And process kPropertyIDFontSize first.
      auto iter = parsed_styles_map_.find(CSSPropertyID::kPropertyIDFontSize);
      if (iter != parsed_styles_map_.end() &&
          should_update_em_rem_style(*iter, root_font_size_changed) &&
          update_map.find(CSSPropertyID::kPropertyIDFontSize) ==
              update_map.end()) {
        SetFontSize(iter->second);
        need_update = true;
      }

      for (const auto &style : parsed_styles_map_) {
        bool need_handle_pending_updated_direction_related_style =
            element_manager_->FixFontSizeOverrideDirectionChangeBug() &&
            pending_updated_direction_related_styles_.has_value() &&
            pending_updated_direction_related_styles_->find(style.first) !=
                pending_updated_direction_related_styles_->end();
        if (style.first != CSSPropertyID::kPropertyIDFontSize &&
            should_update_em_rem_style(style, root_font_size_changed) &&
            update_map.find(style.first) == update_map.end()) {
          if (need_handle_pending_updated_direction_related_style) {
            auto style_pair =
                *pending_updated_direction_related_styles_->find(style.first);
            TryDoDirectionRelatedCSSChange(style.first, style_pair.second.first,
                                           style_pair.second.second);
          } else {
            SetStyleInternal(style.first, style.second);
          }
          need_update = true;
        }
      }
    } while (false);
    dirty_ &= ~kDirtyFontSize;
  }

  if (element_manager_->FixFontSizeOverrideDirectionChangeBug() &&
      pending_updated_direction_related_styles_.has_value()) {
    // reset cached style map impacted by direction
    pending_updated_direction_related_styles_.reset();
  }

  // Report when enableNewAnimator is the default value.
  if ((has_transition_props_changed_ || has_keyframe_props_changed_) &&
      !enable_new_animator()) {
    report::GlobalFeatureCounter::Count(
        report::LynxFeature::CPP_ENABLE_NEW_ANIMATOR_DEFAULT,
        element_manager()->GetInstanceId());
  }
  // keyframe props
  if (has_keyframe_props_changed_) {
    HandleDelayTask([this]() { HandleKeyframePropsChange(); });
    if (!enable_new_animator()) {
      PushToBundle(kPropertyIDAnimation);
    }
    need_update = true;
  }

  if (has_transition_props_changed_) {
    TRACE_EVENT(LYNX_TRACE_CATEGORY, FIBER_ELEMENT_HANDLE_TRANSITION_PROPS,
                [this](lynx::perfetto::EventContext ctx) {
                  UpdateTraceDebugInfo(ctx.event());
                });
    if (!enable_new_animator()) {
      PushToBundle(kPropertyIDTransition);
    } else {
      SetDataToNativeTransitionAnimator();
    }
    has_transition_props_changed_ = false;
    need_update = true;
  }
}

ParallelFlushReturn FiberElement::PrepareForCreateOrUpdate() {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, FIBER_ELEMENT_PREPARE_FOR_CRATE_OR_UPDATE,
              [this](lynx::perfetto::EventContext ctx) {
                UpdateTraceDebugInfo(ctx.event());
              });
  bool need_update = false;

  // Need process attributes first.
  need_update = ConsumeAllAttributes();

  // If it's the first flush of the element and parsed_styles_map_ is empty, we
  // can take the fast path, directly using parsed_styles_map_ as the updated
  // style. If the element is cloned, its parsed_styles_map_ may not be empty
  // and be in the kDirtyCreated state at the same time.
  bool force_use_current_parsed_style_map =
      (dirty_ & kDirtyCreated) && parsed_styles_map_.empty();
  StyleMap parsed_styles;
  base::InlineVector<CSSPropertyID, 16> reset_style_ids;

  if (this->parallel_flush_ && IsCSSInheritanceEnabled()) {
    MarkDirtyLite(kDirtyPropagateInherited);
  }

  if (dirty_ & kDirtyStyleObjects) {
    TRACE_EVENT(LYNX_TRACE_CATEGORY, "FiberElement::HandleStyleObjects");
    StyleResolver::ResolveStyleObjects(
        last_style_objects_ ? last_style_objects_.get() : nullptr,
        style_objects_ ? style_objects_.get() : nullptr, this);
    // Animation and Direction should be handled here
    dirty_ &= ~kDirtyStyleObjects;
  } else {
    ResolveCSSStyles(parsed_styles, reset_style_ids, need_update,
                     force_use_current_parsed_style_map);
  }

  // If above props and styles need to be updated, this patch needs trigger
  // layout.
  if (need_update || dirty_ & kDirtyCreated || dirty_ & kDirtyForceUpdate) {
    RequestLayout();
  }

  // events
  if (dirty_ & kDirtyEvent) {
    TRACE_EVENT(LYNX_TRACE_CATEGORY, FIBER_ELEMENT_HANDLE_EVENTS,
                [this](lynx::perfetto::EventContext ctx) {
                  UpdateTraceDebugInfo(ctx.event());
                });
    // OPTME(linxs): pass event diff result later?
    element_manager_->ResolveEvents(data_model_.get(), this);
    dirty_ &= ~kDirtyEvent;
  }

  // gestures
  if (dirty_ & kDirtyGesture) {
    TRACE_EVENT(LYNX_TRACE_CATEGORY, FIBER_ELEMENT_HANDLE_GESTURES,
                [this](lynx::perfetto::EventContext ctx) {
                  UpdateTraceDebugInfo(ctx.event());
                });
    PreparePropBundleIfNeed();
    element_manager_->ResolveGestures(data_model_.get(), this);
    dirty_ &= ~kDirtyGesture;
    need_update = true;
  }

  // dataset
  if (dirty_ & kDirtyDataset) {
    // Pass the element's dataset as an attribute, with the key 'dataset', into
    // the propbundle for LynxUI.
    TRACE_EVENT(LYNX_TRACE_CATEGORY, FIBER_ELEMENT_HANDLE_DATASET);
    PreparePropBundleIfNeed();
    lepus::Value dataset_val(lepus::Dictionary::Create());
    for (const auto &pair : data_model()->dataset()) {
      dataset_val.SetProperty(pair.first, pair.second);
    }
    prop_bundle_->SetProps("dataset", pub::ValueImplLepus(dataset_val));
    dirty_ &= ~kDirtyDataset;
    need_update = true;
  }

  {
    // FIXME(linxs): [workaround]!!!!!to be removed later, current layout has an
    // issue: inline node can not mark parent dirty when only layout property
    // updated!
    if (need_update && !prop_bundle_ && is_inline_element()) {
      PreparePropBundleIfNeed();
    }
  }

  // Commit Create or Update UI Ops
  PerformElementContainerCreateOrUpdate(need_update);

  // update to layout node
  UpdateLayoutNodeByBundle();

  ResetPropBundle();

  if (ShouldProcessParallelTasks()) {
    return CreateParallelTaskHandler();
  }

  VerifyKeyframePropsChangedHandling();

  return []() {};
}

void FiberElement::TriggerElementUpdate() {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, FIBER_ELEMENT_TRIGGER_ELEMENT_UPDATE,
              [this](lynx::perfetto::EventContext ctx) {
                UpdateTraceDebugInfo(ctx.event());
              });
  UpdateLayoutNodeProps(prop_bundle_);

  if (!is_virtual()) {
    UpdateFiberElement();
  }
}

void FiberElement::VerifyKeyframePropsChangedHandling() {
  if (has_keyframe_props_changed_) {
    // Throw exception on purpose in debug mode or UT to indicate that
    // keyframe_props is not handled properly in this flow
    DCHECK(!has_keyframe_props_changed_);
    has_keyframe_props_changed_ = false;
  }
}

void FiberElement::FlushActionsAsRoot() {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, FIBER_ELEMENT_FLUSH_ACTIONS_AS_ROOT,
              [this](lynx::perfetto::EventContext ctx) {
                UpdateTraceDebugInfo(ctx.event());
              });
  if (parent() == nullptr) {
    LOGE("FiberElement::FlushActionsAsRoot failed since parent is nullptr");
    return;
  }

  // find the first non wrapper && non dirty parent to get the flush option
  auto *flush_parent = static_cast<FiberElement *>(parent());

  // find the first non dirty parent to do flush,if flush from subtree
  if (flush_parent->dirty_) {
    LOGW("FiberElement::FlushActionsAsRoot maybe from a wrong parent, this tag:"
         << tag_.str() << ",component:" << ParentComponentEntryName());
    return flush_parent->FlushActionsAsRoot();
  }

  // find the first non block parent to get the flush option for AirModeFiber
  if (element_manager()->IsAirModeFiberEnabled() && is_block() &&
      flush_parent) {
    return flush_parent->FlushActionsAsRoot();
  }

  // find the first non wrapper parent to get the flush option
  while (flush_parent && flush_parent->is_wrapper()) {
    flush_parent = static_cast<FiberElement *>(flush_parent->parent());
  }

  if (!flush_parent) {
    LOGE(
        "FiberElement::FlushActionsAsRoot failed since can not find a clean "
        "flush parent!");
    return;
  }

  if (IsDetached()) {
    LOGE(
        "FiberElement::FlushActionsAsRoot failed since current node is "
        "detached!");
    return;
  }

  ParallelFlushAsRoot();
  FlushActions();
  if (element_manager()->GetEnableBatchLayoutTaskWithSyncLayout()) {
    element_context_delegate_->FlushEnqueuedTasks();
  }
}

void FiberElement::FlushSelf() {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, FIBER_ELEMENT_FLUSH_SELF,
              [this](lynx::perfetto::EventContext ctx) {
                UpdateTraceDebugInfo(ctx.event());
              });
  if (parallel_before_flush_action_tasks_.has_value()) {
    for (const auto &task : *parallel_before_flush_action_tasks_) {
      task();
    }
    parallel_before_flush_action_tasks_.reset();
  }

  if ((dirty_ & ~kDirtyTree) != 0) {
    // create or update Platform Op
    PrepareForCreateOrUpdate();
  }

  // handle fixed style changed if needed
  if (fixed_changed_) {
    HandleSelfFixedChange();
    fixed_changed_ = false;
  }
}

// need parent's option
void FiberElement::FlushActions() {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, FIBER_ELEMENT_FLUSH_ACTIONS,
              [this](lynx::perfetto::EventContext ctx) {
                UpdateTraceDebugInfo(ctx.event());
              });
  if (!flush_required_) {
    return;
  }

  // Step I: Handle Action for current element: Prepare&HandleFixedChange
  FlushSelf();

  // Step II: process insert or remove related actions
  PrepareAndGenerateChildrenActions();

  // Throw exception on purpose to catch logic flaw
  DCHECK(dirty_ == 0);

  for (auto *invalidation_set : invalidation_lists_.descendants) {
    InvalidateChildren(invalidation_set);
  }
  invalidation_lists_.descendants.clear_and_shrink();

  // Step III: recursively call FlushActions for each child
  for (const auto &child : scoped_children_) {
    if (children_propagate_inherited_styles_flag_) {
      child->MarkDirtyLite(kDirtyPropagateInherited);
    }
    child->FlushActions();
  }
  // below flags should be delayed until children flushed
  children_propagate_inherited_styles_flag_ = false;
  reset_inherited_ids_.reset();

  flush_required_ = false;
  is_async_flush_root_ = false;
}

void FiberElement::OnParallelFlushAsRoot(PerfStatistic &stats) {
  stats.enable_report_stats_ =
      element_manager()->GetEnableReportThreadedElementFlushStatistic();
  stats.total_processing_start_ = base::CurrentTimeMicroseconds();
}

void FiberElement::ParallelFlushAsRoot() {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, FIBER_ELEMENT_PARALLEL_FLUSH_AS_ROOT);
  if (!element_manager()->GetEnableParallelElement()) {
    return;
  }
  {
    TRACE_EVENT(LYNX_TRACE_CATEGORY, TASM_TASK_RUNNER_WAIT_FOR_COMPLETION);
    element_manager()->GetTasmWorkerTaskRunner()->WaitForCompletion();
  }
  ParallelFlushRecursively();

  auto &task_queue = element_manager()->ParallelTasks();
  if (task_queue.empty()) {
    return;
  }

  uint32_t total_task_count = static_cast<uint32_t>(task_queue.size());

  PerfStatistic perf_stats(total_task_count);
  OnParallelFlushAsRoot(perf_stats);

  while (!task_queue.empty()) {
    TRACE_EVENT(LYNX_TRACE_CATEGORY, FIBER_ELEMENT_CONSUME_PARALLEL_TASK);
    if (task_queue.front().get()->GetFuture().wait_for(
            std::chrono::seconds(element_manager()->GetTaskWaitTimeout())) ==
        std::future_status::ready) {
      TRACE_EVENT(LYNX_TRACE_CATEGORY, FIBER_ELEMENT_CONSUME_LEFT_ITER);
      task_queue.front().get()->GetFuture().get()();
      task_queue.pop_front();
    } else if (task_queue.back().get()->Run()) {
      TRACE_EVENT(LYNX_TRACE_CATEGORY, FIBER_ELEMENT_CONSUME_RIGHT_ITER);
      task_queue.back().get()->GetFuture().get()();
      task_queue.pop_back();
      ++perf_stats.engine_thread_task_count_;
    } else {
      TRACE_EVENT(LYNX_TRACE_CATEGORY, FIBER_ELEMENT_WAIT_LEFT_ITER);
      ParallelFlushReturn task;
      if (perf_stats.enable_report_stats_) {
        uint64_t wait_start = base::CurrentTimeMicroseconds();
        task = task_queue.front().get()->GetFuture().get();
        perf_stats.total_waiting_time_ +=
            (base::CurrentTimeMicroseconds() - wait_start);
      } else {
        task = task_queue.front().get()->GetFuture().get();
      }

      task();
      task_queue.pop_front();
    }
  }

  DidParallelFlushAsRoot(perf_stats);
}

void FiberElement::DidParallelFlushAsRoot(PerfStatistic &stats) {
  if (stats.enable_report_stats_) {
    uint64_t total_processing_end = base::CurrentTimeMicroseconds();
    report::EventTracker::OnEvent(
        [perf_stats = std::move(stats),
         total_processing_end](report::MoveOnlyEvent &event) {
          auto thread_pool_task_count = perf_stats.total_task_count_ -
                                        perf_stats.engine_thread_task_count_;
          event.SetName("lynxsdk_threaded_element_flush_statistic");
          event.SetProps("total_task_count", perf_stats.total_task_count_);
          event.SetProps("thread_pool_task_count", thread_pool_task_count);
          event.SetProps("mode", kFiberParallelPrepareMode);
          event.SetProps("tasm_thread_processing_duration",
                         static_cast<int>(total_processing_end -
                                          perf_stats.total_processing_start_));
          event.SetProps("tasm_thread_waiting_duration",
                         static_cast<int>(perf_stats.total_waiting_time_));
        });
  }
}

void FiberElement::PostResolveTaskToThreadPool(
    bool is_engine_thread, ParallelReduceTaskQueue &task_queue) {
  // Get Tag Info
  EnsureTagInfo();
  // Decode first
  GetRelatedCSSFragment();
  if (is_component()) {
    static_cast<ComponentElement *>(this)->GetCSSFragment();
  }

  std::promise<ParallelFlushReturn> promise;
  std::future<ParallelFlushReturn> future = promise.get_future();

  auto task_info_ptr = fml::MakeRefCounted<base::OnceTask<ParallelFlushReturn>>(
      [target = this, promise = std::move(promise)]() mutable {
        TRACE_EVENT(
            LYNX_TRACE_CATEGORY,
            FIBER_ELEMENT_PREPARE_FOR_CRATE_OR_UPDATE_ASYNC,
            [target](lynx::perfetto::EventContext ctx) {
              if (target->element_manager()) {
                ctx.event()->add_debug_annotations(
                    INSTANCE_ID,
                    std::to_string(target->element_manager()->GetInstanceId()));
              }
            });

        target->UpdateResolveStatus(AsyncResolveStatus::kResolving);
        target->parallel_flush_ = true;
        promise.set_value(target->PrepareForCreateOrUpdate());
      },
      std::move(future));

  base::TaskRunnerManufactor::PostTaskToConcurrentLoop(
      [task_info_ptr]() { task_info_ptr->Run(); },
      base::ConcurrentTaskType::HIGH_PRIORITY);
  task_queue.emplace_back(std::move(task_info_ptr));
}

void FiberElement::ParallelFlushRecursively() {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, FIBER_ELEMENT_PARALLEL_FLUSH_RECURSIVELY);
  if (!flush_required_) {
    return;
  }

  if (!IsAsyncResolveResolving() && ((dirty_ & ~kDirtyTree) != 0)) {
    PostResolveTaskToThreadPool(true, element_manager()->ParallelTasks());
  }

  for (const auto &child : scoped_children_) {
    child->ParallelFlushRecursively();
  }
}

void FiberElement::PrepareChildren() {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, FIBER_ELEMENT_PREPARE_CHILDREN,
              [this](lynx::perfetto::EventContext ctx) {
                UpdateTraceDebugInfo(ctx.event());
              });
  for (const auto &child : scoped_children_) {
    if (children_propagate_inherited_styles_flag_) {
      // mark propagateInherited when necessary
      child->MarkDirtyLite(kDirtyPropagateInherited);
    }

    if ((child->dirty_ & ~kDirtyTree) != 0) {
      child->PrepareForCreateOrUpdate();
    }

    if (child->is_layout_only_ && !child->is_raw_text()) {
      child->PrepareChildren();
    }
  }
}

void FiberElement::PrepareChildForInsertion(FiberElement *child) {
  if (child->dirty() & FiberElement::kDirtyCreated) {
    // make sure the child has been created,before insert op
    if (children_propagate_inherited_styles_flag_) {
      child->MarkDirtyLite(FiberElement::kDirtyPropagateInherited);
    }
    child->PrepareForCreateOrUpdate();
  }
  if (child->IsLayoutOnly() && !child->is_raw_text()) {
    for (const auto &grand : child->children()) {
      child->PrepareChildForInsertion(grand.get());
    }
  }
}

void FiberElement::PrepareAndGenerateChildrenActions() {
  TRACE_EVENT(LYNX_TRACE_CATEGORY,
              FIBER_ELEMENT_PREPARE_AND_GENERATE_CHILDREN_ACTIONS,
              [this](lynx::perfetto::EventContext ctx) {
                UpdateTraceDebugInfo(ctx.event());
              });
  // When need propagate inherited styles or tree structure is updated, prepare
  // children
  if (dirty_ & kDirtyTree || children_propagate_inherited_styles_flag_) {
    PrepareChildren();
  }
  // process insert or remove related actions
  if (dirty_ & kDirtyTree) {
    TRACE_EVENT(LYNX_TRACE_CATEGORY, FIBER_ELEMENT_HANDLE_CHILDREN_ACTION,
                [this](lynx::perfetto::EventContext ctx) {
                  UpdateTraceDebugInfo(ctx.event());
                });
    if (!has_to_store_insert_remove_actions_) {
      for (const auto &child : scoped_children_) {
        if (!child->render_parent_) {
          // if no pending tree actions, we just do insertion here
          if (!child->is_fixed_ || GetEnableFixedNew()) {
            this->HandleInsertChildAction(child.get(), -1, nullptr);
          } else {
            if (IsFiberArch()) {
              InsertFixedElement(child.get(), nullptr);
            } else {
              child->need_handle_fixed_ = true;
            }
          }
        }
      }
    }

    for (const auto &param : action_param_list_) {
      switch (param.type_) {
        case Action::kInsertChildAct: {
          PrepareChildForInsertion(param.child_.get());
          if (!param.is_fixed_ || GetEnableFixedNew()) {
            HandleInsertChildAction(param.child_.get(),
                                    static_cast<int>(param.index_),
                                    param.ref_node_);
          } else {
            if (IsFiberArch()) {
              InsertFixedElement(param.child_.get(), param.ref_node_);
            } else {
              param.child_.get()->need_handle_fixed_ = true;
            }
          }
        } break;

        case Action::kRemoveChildAct: {
          if (!param.is_fixed_ || GetEnableFixedNew()) {
            HandleRemoveChildAction(param.child_.get());
          } else {
            RemoveFixedElement(param.child_.get());
          }
        } break;

        case Action::kRemoveIntergenerationAct: {
          if (param.child_->parent_ == this) {
            break;
          }
          if (param.is_fixed_ && !GetEnableFixedNew()) {
            RemoveFixedElement(param.child_.get());
          } else if (param.child_->ZIndex() != 0 || param.is_fixed_) {
            if (param.is_fixed_) {
              // new fixed, remove fixed node and its layout node from its
              // parent.
              param.child_.get()->render_parent_->HandleRemoveChildAction(
                  param.child_.get());
            } else {
              // node with z-index only needs remove its element container.
              param.child_->element_container()->RemoveSelf(false);
            }
          }
        } break;

        default:
          break;
      }
    }
    dirty_ &= ~kDirtyTree;
    RequestLayout();

    // if has any child, mark the flag true, otherwise set it false
    has_to_store_insert_remove_actions_ = (scoped_children_.size() > 0);
  }

  action_param_list_.clear_and_shrink();

  if (dirty_ & kDirtyReAttachContainer) {
    if (is_fixed_ && !GetEnableFixedNew()) {
      InsertFixedElement(this, nullptr);
    } else if (ZIndex() != 0 || is_fixed_) {
      // new fixed.
      if (is_fixed_) {
        // When new fixed is enabled, layout node should be re-inserted to its
        // render_parent, with an full insertion call.
        ((FiberElement *)render_parent_)
            ->HandleInsertChildAction(this, 0, next_render_sibling_);
      } else {
        // z-index only has to insert its element container again.
        HandleContainerInsertion(render_parent_, this, next_render_sibling_);
      }
    }
    dirty_ &= ~kDirtyReAttachContainer;
  }
}

void FiberElement::HandleInsertChildAction(FiberElement *child, int to_index,
                                           FiberElement *ref_node) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, FIBER_ELEMENT_HANDLE_INSERT_CHILD_ACTION,
              [this](lynx::perfetto::EventContext ctx) {
                UpdateTraceDebugInfo(ctx.event());
              });

  auto *parent = this;

  if (child->render_parent_ != nullptr) {
    LOGE("FiberElement do re-insert child action");
    this->LogNodeInfo();
    child->LogNodeInfo();
  }

  if (!GetEnableFixedNew()) {
    while (
        ref_node != nullptr &&
        (ref_node->is_fixed() || ref_node->fixed_changed_ ||
         (element_manager() && element_manager()->FixInsertBeforeFixedBug() &&
          ref_node->render_parent() == nullptr))) {
      // Two cases:
      // 1. `ref_node` is a fixed node, find its `next_sibling`.
      // 2. `ref_node` changed from fixed to non-fixed; since
      // `ref_node->HandleSelfFixedChange` was not executed, also find its
      // `next_sibling`.
      ref_node = static_cast<FiberElement *>(ref_node->next_sibling());
    }
  }

  StoreLayoutNode(child, ref_node);

  if (child->is_wrapper()) {
    // try to mark for wrapper element related.
    FindEnclosingNoneWrapper(parent, child);
  }

  if (UNLIKELY(parent->is_wrapper() || (parent->wrapper_element_count_ > 0) ||
               child->is_wrapper())) {
    TreeResolver::AttachChildToTargetParentForWrapper(parent, child, ref_node);
  } else {
    InsertLayoutNode(child, ref_node);
  }

  HandleContainerInsertion(parent, child, ref_node);
}

void FiberElement::HandleRemoveChildAction(FiberElement *child) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, FIBER_ELEMENT_HANDLE_REMOVE_CHILD_ACTION,
              [this](lynx::perfetto::EventContext ctx) {
                UpdateTraceDebugInfo(ctx.event());
              });
  auto *parent = this;

  if (child->render_parent_ != this) {
    LOGE("FiberElement remove wrong child node !");
    parent->LogNodeInfo();
    child->LogNodeInfo();
    return;
  }

  RestoreLayoutNode(child);
  if (!child->is_wrapper() && !child->attached_to_layout_parent_ &&
      !child->IsNewFixed()) {
    // parent is detached, child is removed from parent, and then the parent is
    // inserted to view tree,but the action is still stored in its parent

    // 1.if the child is not wrapper and not attached to layout tree, just
    // return
    // 2. if the child is wrapper, remove the wrapper's children recursively in
    // RemoveFromParentForWrapperChild
    // 3. if the parent is wrapper, just handle in
    // RemoveFromParentForWrapperChild
    return;
  }

  if (UNLIKELY(parent->is_wrapper() || parent->wrapper_element_count_ > 0) ||
      child->is_wrapper()) {
    if (child->enclosing_none_wrapper_) {
      child->enclosing_none_wrapper_->wrapper_element_count_--;
    }
    TreeResolver::RemoveFromParentForWrapperChild(parent, child);
  } else {
    RemoveLayoutNode(child);
  }

  child->element_container()->RemoveSelf(false);
}

void FiberElement::HandleContainerInsertion(FiberElement *parent,
                                            FiberElement *child,
                                            FiberElement *ref_node) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, FIBER_ELEMENT_HANDLE_CONTAINER_INSERTION,
              [this](lynx::perfetto::EventContext ctx) {
                UpdateTraceDebugInfo(ctx.event());
              });
  // for element container tree
  // a quick check for determine if need to append the container to the
  // end(check ref is null) ref is null, find the first none-wrapper ancestor's
  // next sibling as ref! ref_node: null means to append to the real parent!!
  // FIXME(linxs): for wrapper, we can merge the below logic
  auto *temp_parent = parent;
  while (!ref_node && temp_parent && temp_parent->is_layout_only_) {
    ref_node = temp_parent->next_render_sibling_;
    temp_parent = temp_parent->render_parent_;
  }

  if (!child->element_container()->parent()) {
    // the child has been inserted to parent in
    // AttachChildToTargetContainerRecursive, just ignore it
    parent->element_container()->AttachChildToTargetContainer(child, ref_node);
  }
}

FiberElement *FiberElement::FindEnclosingNoneWrapper(FiberElement *parent,
                                                     FiberElement *node) {
  while (parent) {
    if (!parent->is_wrapper()) {
      node->enclosing_none_wrapper_ = parent;
      parent->wrapper_element_count_++;
      break;
    }
    parent = static_cast<FiberElement *>(parent->parent_);
  }
  return parent;
}

void FiberElement::MarkPlatformNodeDestroyed() {
  for (size_t i = 0; i < GetChildCount(); ++i) {
    auto *child = static_cast<FiberElement *>(GetChildAt(i));
    // FiberElement may be referenced by JS engine. Just clear the parent-child
    // relationship.
    if (child->parent_ == this) {
      child->parent_ = nullptr;
    }
    if (child->render_parent_ == this) {
      child->render_parent_ = nullptr;
    }
  }
  if (scoped_virtual_children_.has_value()) {
    for (size_t i = 0; i < scoped_virtual_children_->size(); ++i) {
      auto *virtual_child =
          static_cast<FiberElement *>((*scoped_virtual_children_)[i].get());
      if (virtual_child->parent_ == this) {
        virtual_child->parent_ = nullptr;
      }
    }
  }
  // clear element's children only in radon or radon compatible mode.
  scoped_children_.clear();
  scoped_virtual_children_.reset();
}

bool FiberElement::InComponent() const {
  auto p = static_cast<FiberElement *>(GetParentComponentElement());
  if (p) {
    return !(p->is_page());
  }
  return false;
}

std::string FiberElement::ParentComponentIdString() const {
  auto *p = static_cast<FiberElement *>(GetParentComponentElement());
  if (p) {
    return static_cast<ComponentElement *>(p)->component_id().str();
  }
  return "";
}

const std::string &FiberElement::ParentComponentEntryName() const {
  auto *p = static_cast<FiberElement *>(GetParentComponentElement());
  if (p) {
    return static_cast<ComponentElement *>(p)->GetEntryName();
  }
  static std::string kDefaultEntryName(tasm::DEFAULT_ENTRY_NAME);
  return kDefaultEntryName;
}

void FiberElement::AddChildAt(fml::RefPtr<FiberElement> child, int index) {
  if (index == -1) {
    scoped_children_.push_back(child);
  } else {
    scoped_children_.insert(scoped_children_.begin() + index, child);
  }
  OnNodeAdded(child.get());
  TreeResolver::NotifyNodeInserted(this, child.get());
  child->set_parent(this);
}

int32_t FiberElement::IndexOf(const Element *child) const {
  const auto fiber_child = static_cast<const FiberElement *>(child);
  for (auto it = scoped_children_.begin(); it != scoped_children_.end(); ++it) {
    if (it->get() == fiber_child) {
      return static_cast<int>(std::distance(scoped_children_.begin(), it));
    }
  }
  return -1;
}

Element *FiberElement::GetChildAt(size_t index) {
  if (index >= scoped_children_.size()) {
    return nullptr;
  }
  return scoped_children_[index].get();
}

ElementChildrenArray FiberElement::GetChildren() {
  ElementChildrenArray ret;
  ret.reserve(scoped_children_.size());
  for (const auto &child : scoped_children_) {
    ret.push_back(child.get());
  }
  return ret;
}

// If new animator is enabled and this element has been created before, we
// should consume transition styles in advance. Also transition manager needs to
// verify every property to determine whether to intercept this update.
// Therefore, the operations related to Transition in the SetStyle process are
// divided into three steps:
// 1. Check whether to consume all transition styles in advance if needed.
// 2. Skip all transition styles in the later process if they have been consume
// in advance.
// 3. Check every property to determine whether to intercept this update.
void FiberElement::ConsumeStyle(const StyleMap &styles,
                                const StyleMap *inherit_styles) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, FIBER_ELEMENT_CONSUME_STYLE,
              [this](lynx::perfetto::EventContext ctx) {
                UpdateTraceDebugInfo(ctx.event());
              });
  bool should_consume_trans_styles_in_advance =
      ShouldConsumeTransitionStylesInAdvance();

  ConsumeStyleInternal(
      styles, inherit_styles,
      [this, should_consume_trans_styles_in_advance](auto id,
                                                     const auto &value) {
        // #2. Skip all transition styles in the later process if they have been
        // consume in advance.
        if (should_consume_trans_styles_in_advance &&
            CSSProperty::IsTransitionProps(id)) {
          return true;
        }
        // #3. Check every property to determine whether to intercept this
        // update.
        if (css_transition_manager_ &&
            css_transition_manager_->ConsumeCSSProperty(id, value)) {
          return true;
        }

        return false;
      });

  DidConsumeStyle();
}

void FiberElement::ConsumeStyleInternal(
    const StyleMap &styles, const StyleMap *inherit_styles,
    std::function<bool(CSSPropertyID, const tasm::CSSValue &)> should_skip) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, FIBER_ELEMENT_CONSUME_STYLE,
              [this](lynx::perfetto::EventContext ctx) {
                UpdateTraceDebugInfo(ctx.event());
              });
  if (styles.empty() &&
      (inherit_styles == nullptr || inherit_styles->empty())) {
    return;
  }

  // Handle font-size first. Other css may use this to calc rem or em.
  const auto it = parsed_styles_map_.find(CSSPropertyID::kPropertyIDFontSize);
  CSSValue font_value =
      (it != parsed_styles_map_.end()) ? it->second : CSSValue::Empty();
  SetFontSize(font_value);

  auto consume_func = [this, should_skip = std::move(should_skip)](
                          const StyleMap &styles, bool process_inherit) {
    for (const auto &style : styles) {
      bool is_inherit_style = false;
      if (!is_raw_text() && IsInheritable(style.first)) {
        is_inherit_style = true;
        auto iter = inherited_styles_->find(style.first);
        if (iter == inherited_styles_->end() || iter->second != style.second) {
          // save the css value to inherited styles map
          inherited_styles_->insert_or_assign(style.first, style.second);
          children_propagate_inherited_styles_flag_ = true;
        }
      }

      if (style.first == CSSPropertyID::kPropertyIDDirection ||
          style.first == CSSPropertyID::kPropertyIDFontSize) {
        // direction has been resolved before
        continue;
      }

      const bool is_platform_inheritable_property =
          process_inherit && is_inherit_style &&
          starlight::ComputedCSSStyle::IsPlatformInheritableProperty(
              style.first);

      if (const auto *parent_computed_css = GetParentComputedCSSStyle();
          is_platform_inheritable_property && parent_computed_css != nullptr) {
        if (parsed_styles_map_.find(style.first) != parsed_styles_map_.end()) {
          // Inline style or matched selectors has same style property
          continue;
        }
        const bool value_changed = computed_css_style()->InheritValue(
            style.first, *parent_computed_css);
        if (value_changed) {
          PreparePropBundleIfNeed();
          PushToBundle(style.first);
        }
        continue;
      }

      if (LIKELY(!TryResolveLogicStyleAndSaveDirectionRelatedStyle(
              style.first, style.second))) {
        if (should_skip(style.first, style.second)) {
          continue;
        }

        // Since the previous element styles cannot be accessed in element, we
        // need to record some necessary styles which New Animator transition
        // needs, and it needs to be saved before rtl converted logic.
        RecordElementPreviousStyle(style.first, style.second);
        SetStyleInternal(style.first, style.second);
      }
    }
  };

  if (inherit_styles != nullptr) {
    consume_func(*inherit_styles, true);
  }

  consume_func(styles, false);
}

bool FiberElement::ConsumeAllAttributes() {
  bool need_update = false;
  if (dirty_ & kDirtyAttr) {
    TRACE_EVENT(LYNX_TRACE_CATEGORY, FIBER_ELEMENT_HANDLE_ATTR,
                [this](lynx::perfetto::EventContext ctx) {
                  UpdateTraceDebugInfo(ctx.event());
                });
    for (const auto &attr : updated_attr_map_) {
      SetAttributeInternal(attr.first, attr.second);
      need_update = true;
    }
    if (reset_attr_vec_.has_value()) {
      for (const auto &attr : *reset_attr_vec_) {
        ResetAttribute(attr);
        need_update = true;
      }
      reset_attr_vec_.reset();
    }
    if (updated_attr_map_.size() > 0) {
      PropsUpdateFinish();
      updated_attr_map_.clear();
    }

    dirty_ &= ~kDirtyAttr;
  }
  return need_update;
}

void FiberElement::PerformElementContainerCreateOrUpdate(bool need_update) {
  if (dirty_ & kDirtyCreated) {
    TRACE_EVENT(LYNX_TRACE_CATEGORY, FIBER_ELEMENT_HANDLE_CRATE,
                [this](lynx::perfetto::EventContext ctx) {
                  UpdateTraceDebugInfo(ctx.event());
                });
    // FIXME(linxs): FlushProps can be optimized, for example can we just
    // create viewElement,imageElement,textElement.. directly?
    FlushProps();
    dirty_ &= ~kDirtyCreated;
  } else if (need_update || dirty_ & kDirtyForceUpdate) {
    if (prop_bundle_) {
      TriggerElementUpdate();
    }

    if (element_manager() && element_manager()->FixZIndexCrash()) {
      HandleBeforeFlushActionsTask(
          [this]() { element_container()->StyleChanged(); });
    } else {
      HandleDelayTask([this]() { element_container()->StyleChanged(); });
    }
  }
  dirty_ &= ~kDirtyForceUpdate;
}

ParallelFlushReturn FiberElement::CreateParallelTaskHandler() {
  // Remaining Layout Task should be returned to be executed in threaded flush
  // or sync resolving(i.e. PageElement) scenario
  this->parallel_flush_ = false;
  this->UpdateResolveStatus(AsyncResolveStatus::kResolved);
  return [this]() {
    TRACE_EVENT(LYNX_TRACE_CATEGORY,
                FIBER_ELEMENT_HANDLE_PARALLEL_REDUCE_TASKS);
    if (parallel_reduce_tasks_.has_value()) {
      for (const auto &task : *parallel_reduce_tasks_) {
        task();
      }
      parallel_reduce_tasks_.reset();
    }
    // Executing task in parallel_reduce_tasks_ may produce prop_bundle_,
    // need to consume newly created prop_bundle_
    if (prop_bundle_) {
      TriggerElementUpdate();
      UpdateLayoutNodeByBundle();
      ResetPropBundle();
    }
    this->UpdateResolveStatus(AsyncResolveStatus::kUpdated);
    VerifyKeyframePropsChangedHandling();
  };
}

void FiberElement::CacheStyleFromAttributes(CSSPropertyID id,
                                            CSSValue &&value) {
  styles_from_attributes_->insert_or_assign(id, std::move(value));
}

void FiberElement::CacheStyleFromAttributes(CSSPropertyID id,
                                            const lepus::Value &value) {
  UnitHandler::Process(id, value, *styles_from_attributes_,
                       element_manager()->GetCSSParserConfigs());
}

void FiberElement::DidConsumeStyle() {
  if (!styles_from_attributes_.has_value()) {
    return;
  }
  if (styles_from_attributes_->empty()) {
    return;
  }

  ConsumeStyleInternal(*styles_from_attributes_, nullptr,
                       [](auto id, const auto &value) {
                         // Do not skip any style here.
                         return false;
                       });
  styles_from_attributes_.reset();
}

void FiberElement::MarkHasLayoutOnlyPropsIfNecessary(
    const base::String &attribute_key) {
  // Any attribute will cause has_layout_only_props_ = false
  has_layout_only_props_ = false;
}

void FiberElement::SetAttributeInternal(const base::String &key,
                                        const lepus::Value &value) {
  WillConsumeAttribute(key, value);

  PreparePropBundleIfNeed();

  MarkHasLayoutOnlyPropsIfNecessary(key);

  prop_bundle_->SetProps(key.c_str(), pub::ValueImplLepus(value));

  // If the current node is a list child node, it is necessary to convert
  // kFullSpan's value to ListComponentInfo::Type and synchronize it to
  // LayoutNode.
  constexpr const static char kFullSpan[] = "full-span";
  if (parent() != nullptr && static_cast<FiberElement *>(parent())->is_list()) {
    if (key.IsEquals(kFullSpan)) {
      ListComponentInfo::Type type = ListComponentInfo::Type::DEFAULT;
      if (value.IsBool() && value.Bool()) {
        type = ListComponentInfo::Type::LIST_ROW;
      }
      UpdateLayoutNodeAttribute(starlight::LayoutAttribute::kListCompType,
                                lepus::Value(static_cast<int32_t>(type)));
      // If the key is already equal to ListComponentInfo::Type, just sync it to
      // LayoutNode.
    } else if (key.IsEqual(ListComponentInfo::kListCompType)) {
      UpdateLayoutNodeAttribute(starlight::LayoutAttribute::kListCompType,
                                value);
    }
  }

#if 0  // TODO(linxs): to process it in CLI compile period
    StyleMap attr_styles;
    if (key.IsEquals("scroll-x") && value.String().IsEqual("true")) {
        attr_styles.insert_or_assign(
                                     kPropertyIDLinearOrientation,
                                     CSSValue::MakeEnum((int) starlight::LinearOrientationType::kHorizontal));
        element_manager()->UpdateLayoutNodeAttribute(
                                                     layout_node_, starlight::LayoutAttribute::kScroll, lepus::Value(true));
    } else if (key.IsEquals("scroll-y") && value.String().IsEqual("true")) {
        attr_styles.insert_or_assign(
                                     kPropertyIDLinearOrientation,
                                     CSSValue::MakeEnum((int) starlight::LinearOrientationType::kVertical));
        element_manager()->UpdateLayoutNodeAttribute(
                                                     layout_node_, starlight::LayoutAttribute::kScroll, lepus::Value(true));
    } else if (key.IsEquals("scroll-x-reverse") &&
               value.String().IsEqual("true")) {
        attr_styles.insert_or_assign(
                                     kPropertyIDLinearOrientation,
                                     CSSValue::MakeEnum(
                                                        (int) starlight::LinearOrientationType::kHorizontalReverse));
        element_manager()->UpdateLayoutNodeAttribute(
                                                     layout_node_, starlight::LayoutAttribute::kScroll, lepus::Value(true));
    } else if (key.IsEquals("scroll-y-reverse") &&
               value.String().IsEqual("true")) {
        attr_styles.insert_or_assign(
                                     kPropertyIDLinearOrientation,
                                     CSSValue::MakeEnum(
                                                        (int) starlight::LinearOrientationType::kVerticalReverse));
        element_manager()->UpdateLayoutNodeAttribute(
                                                     layout_node_, starlight::LayoutAttribute::kScroll, lepus::Value(true));
    } else if (key.IsEqual("column-count")) {
        element_manager()->UpdateLayoutNodeAttribute(
                                                     layout_node_, starlight::LayoutAttribute::kColumnCount, value);
    } else if (key.IsEqual(ListComponentInfo::kListCompType)) {
        element_manager()->UpdateLayoutNodeAttribute(
                                                     layout_node_, starlight::LayoutAttribute::kListCompType, value);
    }
    SetStyle(attr_styles);
#endif
}

void FiberElement::AddDataset(const base::String &key,
                              const lepus::Value &value) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, FIBER_ELEMENT_ADD_DATA_SET);

  data_model_->SetDataSet(key, value);
  MarkDirty(kDirtyDataset);
}

void FiberElement::SetDataset(const lepus::Value &data_set) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, FIBER_ELEMENT_SET_DATA_SET);

  data_model_->SetDataSet(data_set);
  MarkDirty(kDirtyDataset);
}

void FiberElement::SetJSEventHandler(const base::String &name,
                                     const base::String &type,
                                     const base::String &callback) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, FIBER_ELEMENT_SET_JS_EVENT_HANDLER);

  data_model_->SetStaticEvent(type, name, callback);
  MarkDirty(kDirtyEvent);
}

void FiberElement::SetLepusEventHandler(const base::String &name,
                                        const base::String &type,
                                        const lepus::Value &script,
                                        const lepus::Value &callback) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, FIBER_ELEMENT_SET_LEPUS_EVENT_HANDLER);

  data_model_->SetLepusEvent(type, name, script, callback);
  MarkDirty(kDirtyEvent);
}

void FiberElement::SetWorkletEventHandler(const base::String &name,
                                          const base::String &type,
                                          const lepus::Value &worklet_info,
                                          lepus::Context *ctx) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, FIBER_ELEMENT_SET_WORKLET_EVENT_HANDLER);

  data_model_->SetWorkletEvent(type, name, worklet_info, ctx);
  MarkDirty(kDirtyEvent);
}

void FiberElement::SetNativeProps(
    const lepus::Value &native_props,
    std::shared_ptr<PipelineOptions> &pipeline_options) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, FIBER_ELEMENT_SET_NATIVE_PROPS,
              [this](lynx::perfetto::EventContext ctx) {
                UpdateTraceDebugInfo(ctx.event());
              });
  if (!native_props.IsTable()) {
    LOGE("SetNativeProps's param must be a Table!");
    return;
  }

  if (native_props.Table()->size() == 0) {
    LOGE("SetNativeProps's param must not be empty!");
    return;
  }

  ForEachLepusValue(
      native_props, [this](const lepus::Value &key, const lepus::Value &value) {
        auto key_str = key.String();
        auto id = CSSProperty::GetPropertyID(key_str);
        if (id != kPropertyEnd) {
          SetStyle(id, value);
          EXEC_EXPR_FOR_INSPECTOR(element_manager()->OnSetNativeProps(
              this, key.ToString(), value, true));
        } else {
          SetAttribute(key_str, value);
          EXEC_EXPR_FOR_INSPECTOR(element_manager()->OnSetNativeProps(
              this, key.ToString(), value, false));
        }
      });
  if (IsAttached()) {
    // TODO(nihao.royal): use `enable_unified_pixel_pipeline` to switch multi
    // behaviours. After `RunPixelPipeline` is unified, we may remove the
    // redundant logic here.
    if (pipeline_options->enable_unified_pixel_pipeline) {
      pipeline_options->resolve_requested = true;
      pipeline_options->target_node = this;
    } else {
      element_manager()->OnPatchFinish(pipeline_options, this);
    }
  } else {
    LOGE("FiberElement::SetNativeProps to an detached element!");
  }
}

void FiberElement::RemoveEvent(const base::String &name,
                               const base::String &type) {
  data_model_->RemoveEvent(name, type);
  MarkDirty(kDirtyEvent);
}

void FiberElement::SetGestureDetector(const uint32_t gesture_id,
                                      GestureDetector gesture_detector) {
  data_model_->SetGestureDetector(gesture_id, gesture_detector);
  MarkDirty(kDirtyGesture);
}

void FiberElement::RemoveGestureDetector(const uint32_t gesture_id) {
  data_model_->RemoveGestureDetector(gesture_id);
  MarkDirty(kDirtyGesture);
}

void FiberElement::RemoveAllEvents() {
  data_model_->RemoveAllEvents();
  MarkDirty(kDirtyEvent);
}

void FiberElement::SetParsedStyles(const ParsedStyles &parsed_styles,
                                   const lepus::Value &config) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, FIBER_ELEMENT_SET_PARSED_STYLES);

  constexpr const static char kOnlySelector[] = "selectorParsedStyles";
  const auto &only_selector_prop =
      config.GetProperty(BASE_STATIC_STRING(kOnlySelector));
  if (only_selector_prop.IsBool()) {
    only_selector_extreme_parsed_styles_ = only_selector_prop.Bool();
  }

  has_extreme_parsed_styles_ = true;
  *extreme_parsed_styles_ = parsed_styles.first;
  data_model()->set_css_variables_map(parsed_styles.second);
  MarkDirty(kDirtyStyle);
}

void FiberElement::SetParsedStyles(StyleMap &&parsed_styles,
                                   CSSVariableMap &&css_var) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, FIBER_ELEMENT_SET_PARSED_STYLES);
  has_extreme_parsed_styles_ = true;
  only_selector_extreme_parsed_styles_ = false;
  *extreme_parsed_styles_ = std::move(parsed_styles);
  data_model()->set_css_variables_map(std::move(css_var));
  MarkDirty(kDirtyStyle);
}

void FiberElement::AddConfig(const base::String &key,
                             const lepus::Value &value) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, FIBER_ELEMENT_ADD_CONFIG);
  if (config_ == nullptr) {
    config_ = lepus::Dictionary::Create();
  } else if (config_->IsConst()) {
    config_ = lepus::Value::ShallowCopy(lepus::Value(config_)).Table();
  }
  config_->SetValue(key, value);
}

void FiberElement::SetConfig(const lepus::Value &config) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, FIBER_ELEMENT_SET_CONFIG);

  // To improve performance, ensure that the isObject check is performed before
  // calling SetConfig, and the check and LOGW in SetConfig are no longer
  // performed.
  if (config.IsTable()) {
    config_ = config.Table();
  } else if (config.IsJSTable()) {
    config_ = config.ToLepusValue().Table();
  } else {
    DCHECK(false);
  }
}

void FiberElement::MarkStyleDirty(bool recursive) {
  MarkDirty(kDirtyStyle);
  if (recursive) {
    for (const auto &child : scoped_children_) {
      child->MarkStyleDirty(recursive);
    }
  }
}

void FiberElement::MarkFontSizeInvalidateRecursively() {
  MarkDirty(kDirtyFontSize);
  auto *child = first_render_child_;
  while (child) {
    child->MarkFontSizeInvalidateRecursively();
    child = child->next_render_sibling_;
  }
}

void FiberElement::FlushProps() {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, FIBER_ELEMENT_FLUSH_PROPS,
              [this](lynx::perfetto::EventContext ctx) {
                UpdateTraceDebugInfo(ctx.event());
              });

  if (is_scroll_view() || is_list()) {
    UpdateLayoutNodeAttribute(starlight::LayoutAttribute::kScroll,
                              lepus::Value(true));
  }

  // Update The root if needed
  if (!has_painting_node_) {
    TRACE_EVENT(LYNX_TRACE_CATEGORY, CATALYZER_NO_PAINTING_NODE,
                [this](lynx::perfetto::EventContext ctx) {
                  UpdateTraceDebugInfo(ctx.event());
                });

    PreparePropBundleIfNeed();

    // check is in inlineContainer before attachLayoutNode
    auto *real_parent = static_cast<FiberElement *>(
        (!is_fixed_ || GetEnableFixedNew()) ? parent_
                                            : element_manager_->root());
    while (real_parent && real_parent->is_wrapper()) {
      real_parent = static_cast<FiberElement *>(real_parent->parent());
    }
    if (real_parent) {
      CheckHasInlineContainer(real_parent);
    }
    AttachLayoutNode(prop_bundle_);
    EnsureSLNode();

    // FIXME(linxs): any other case has platform layout nodes??
    is_virtual_ = IsShadowNodeVirtual();
    bool platform_is_flatten = true;
    base::MoveOnlyClosure<bool, bool> func =
        [radon_element = this, has_z_props = has_z_props_,
         is_fixed = is_fixed_](bool judge_by_props) {
          if (judge_by_props) {
            return !(has_z_props || is_fixed);
          } else {
            return radon_element->TendToFlatten();
          }
        };
    if (!is_virtual_) {
      platform_is_flatten = painting_context()->IsFlatten(std::move(func));
    }
    bool is_layout_only = CanBeLayoutOnly() || is_virtual_;
    is_layout_only_ = is_layout_only;
    // native layer don't flatten.
    CreateElementContainer(platform_is_flatten);
    has_painting_node_ = true;
  }
  has_transition_props_changed_ = false;
}

// if child's related css variable is updated, invalidate child's style.
void FiberElement::RecursivelyMarkChildrenCSSVariableDirty(
    const lepus::Value &css_variable_updated) {
  for (const auto &child : scoped_children_) {
    if (child->is_raw_text()) {
      continue;
    }
    lepus::Value css_variable_updated_merged = css_variable_updated;
    // first, merge changing_css_variables with element's css_variable,
    // element's css_variable is with high priority.
    child->data_model()->MergeWithCSSVariables(css_variable_updated_merged);
    if (IsRelatedCSSVariableUpdated(child->data_model(),
                                    css_variable_updated_merged)) {
      child->MarkStyleDirty(false);
    }
    child->RecursivelyMarkChildrenCSSVariableDirty(css_variable_updated_merged);
  }
}

void FiberElement::EnsureSLNode() {
  if (EnableLayoutInElementMode() && sl_node_ == nullptr) {
    sl_node_ = std::make_unique<SLNode>(
        element_manager()->GetLayoutConfigs(),
        computed_css_style()->GetLayoutComputedStyle());
    if (is_page()) {
      MarkAsLayoutRoot();
    }
    OnLayoutObjectCreated();
  }
}

void FiberElement::OnLayoutObjectCreated() {}

void FiberElement::EnsureLayoutBundle() {
  if (EnableLayoutInElementMode()) {
    return;
  }

  if (layout_bundle_ == nullptr) {
    layout_bundle_ = std::make_unique<LayoutBundle>();
  }
}

void FiberElement::SetMeasureFunc(std::unique_ptr<MeasureFunc> measure_func) {
  if (customized_layout_node_ != nullptr) {
    customized_layout_node_->SetMeasureFunc(std::move(measure_func));
  }
}

void FiberElement::UpdateTagToLayoutBundle() {
  if (EnableLayoutInElementMode()) {
    return;
  }

  EnsureLayoutBundle();
  layout_bundle_->tag = tag_;
}

void FiberElement::InitLayoutBundle() {
  if (EnableLayoutInElementMode()) {
    return;
  }

  EnsureLayoutBundle();
  layout_bundle_->tag = tag_;
  layout_bundle_->is_create_bundle = true;
}

void FiberElement::MarkAsLayoutRoot() {
  if (EnableLayoutInElementMode()) {
    EnsureSLNode();
    // The default flex direction is column for root
    sl_node_->GetCSSMutableStyle()->SetFlexDirection(
        starlight::FlexDirectionType::kColumn);
    sl_node_->SetContext(element_manager());
    sl_node_->MarkDirty();
    return;
  }

  EnsureLayoutBundle();
  layout_bundle_->is_root = true;
}

void FiberElement::MarkLayoutDirty() {
  if (EnableLayoutInElementMode()) {
    MarkLayoutDirtyLite();
    return;
  }

  EnsureLayoutBundle();
  layout_bundle_->is_dirty = true;
}

void FiberElement::AttachLayoutNode(const fml::RefPtr<PropBundle> &props) {
  if (EnableLayoutInElementMode()) {
    if (IsShadowNodeCustom()) {
      customized_layout_node_ =
          std::make_unique<PlatformLayoutFunctionWrapper>(*this, props);
      element_manager()->layout_context()->CreateLayoutNode(id_, tag_.str(),
                                                            props.get(), false);
    }
    return;
  }

  EnsureLayoutBundle();
  layout_bundle_->shadownode_prop_bundle = props;
  layout_bundle_->allow_inline = allow_layoutnode_inline_;
}

void FiberElement::UpdateLayoutNodeProps(const fml::RefPtr<PropBundle> &props) {
  if (EnableLayoutInElementMode()) {
    if (customized_layout_node_) {
      customized_layout_node_->UpdateLayoutNodeProps(props);
    }
    return;
  }

  EnsureLayoutBundle();
  layout_bundle_->update_prop_bundles.emplace_back(props);
}

void FiberElement::UpdateLayoutNodeStyle(CSSPropertyID css_id,
                                         const tasm::CSSValue &value) {
  if (EnableLayoutInElementMode()) {
    return;
  }

  EnsureLayoutBundle();
  layout_bundle_->styles.emplace_back(css_id, value);
}

void FiberElement::ResetLayoutNodeStyle(tasm::CSSPropertyID css_id) {
  if (EnableLayoutInElementMode()) {
    return;
  }

  EnsureLayoutBundle();
  layout_bundle_->reset_styles.emplace_back(css_id);
}

void FiberElement::UpdateLayoutNodeFontSize(double cur_node_font_size,
                                            double root_node_font_size) {
  if (EnableLayoutInElementMode()) {
    return;
  }

  EnsureLayoutBundle();
  layout_bundle_->font_scale = element_manager_->GetLynxEnvConfig().FontScale();
  layout_bundle_->cur_node_font_size = cur_node_font_size;
  layout_bundle_->root_node_font_size = root_node_font_size;
}

void FiberElement::UpdateLayoutNodeAttribute(starlight::LayoutAttribute key,
                                             const lepus::Value &value) {
  if (EnableLayoutInElementMode()) {
    return;
  }

  EnsureLayoutBundle();
  layout_bundle_->attrs.emplace_back(std::make_pair(key, value));
}

void FiberElement::UpdateLayoutNodeByBundle() {
  if (EnableLayoutInElementMode()) {
    EnsureSLNode();
    return;
  }

  if (layout_bundle_ == nullptr) {
    return;
  }
  EnqueueLayoutTask([element_manager = element_manager(), id = impl_id(),
                     layout_bundle = std::move(layout_bundle_)]() mutable {
    element_manager->UpdateLayoutNodeByBundle(id, std::move(layout_bundle));
  });
  layout_bundle_ = nullptr;
}

void FiberElement::CheckHasInlineContainer(Element *parent) {
  EnsureLayoutBundle();
  allow_layoutnode_inline_ = parent->IsShadowNodeCustom();
}

void FiberElement::EnqueueLayoutTask(base::MoveOnlyClosure<void> operation) {
  if (element_manager()->GetEnableBatchLayoutTaskWithSyncLayout()) {
    element_context_delegate_->EnqueueTask(std::move(operation));
  } else {
    element_manager()->LegacyHandleLayoutTask(this, std::move(operation));
  }
}

void FiberElement::RequestLayout() {
  if (EnableLayoutInElementMode()) {
    HandleBeforeFlushActionsTask([manager = element_manager(), this]() {
      MarkLayoutDirty();
      manager->SetNeedsLayout();
    });
    return;
  }

  HandleDelayTask(
      [manager = element_manager()]() { manager->SetNeedsLayout(); });
}

void FiberElement::RequestNextFrame() {
  HandleDelayTask([this]() { element_manager()->RequestNextFrame(this); });
}

void FiberElement::UpdateFiberElement() {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, FIBER_ELEMENT_UPDATE_FIBER_ELEMENT,
              [this](lynx::perfetto::EventContext ctx) {
                UpdateTraceDebugInfo(ctx.event());
              });
  if (!is_layout_only_) {
    TRACE_EVENT(LYNX_TRACE_CATEGORY, FIBER_ELEMENT_UPDATE_PAINTING_NODE,
                [this](lynx::perfetto::EventContext ctx) {
                  UpdateTraceDebugInfo(ctx.event());
                });
    painting_context()->UpdatePaintingNode(id_, TendToFlatten(), prop_bundle_);
  } else if (!CanBeLayoutOnly()) {
    TRACE_EVENT(LYNX_TRACE_CATEGORY, FIBER_ELEMENT_TRANSITION_TO_NATIVE_VIEW,
                [this](lynx::perfetto::EventContext ctx) {
                  UpdateTraceDebugInfo(ctx.event());
                });
    // Is layout only and can not be layout only
    TransitionToNativeView();
  }
}

bool FiberElement::IsRelatedCSSVariableUpdated(
    AttributeHolder *holder, const lepus::Value changing_css_variables) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, FIBER_ELEMENT_IS_RELATED_CSS_UPDATED,
              [this](lynx::perfetto::EventContext ctx) {
                UpdateTraceDebugInfo(ctx.event());
              });

  bool changed = false;
  ForEachLepusValue(
      changing_css_variables,
      [holder, &changed](const lepus::Value &key, const lepus::Value &value) {
        if (!changed) {
          auto it = holder->css_variable_related().find(key.String());
          if (it != holder->css_variable_related().end() &&
              !it->second.IsEqual(value.String())) {
            changed = true;
          }
        }
      });
  return changed;
}

void FiberElement::UpdateCSSVariable(
    const lepus::Value &css_variable_updated,
    std::shared_ptr<PipelineOptions> &pipeline_option) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, FIBER_ELEMENT_UPDATE_CSS_VARIABLE,
              [this](lynx::perfetto::EventContext ctx) {
                UpdateTraceDebugInfo(ctx.event());
              });
  ForEachLepusValue(
      css_variable_updated,
      [self = this](const lepus::Value &key, const lepus::Value &value) {
        self->data_model()->UpdateCSSVariableFromSetProperty(key.String(),
                                                             value.String());
      });
  // merge updated css_variable to merged_,
  // since it may not related with updated variables.
  if (IsRelatedCSSVariableUpdated(data_model(), css_variable_updated)) {
    // invalidate self.
    MarkStyleDirty(false);
  }
  RecursivelyMarkChildrenCSSVariableDirty(css_variable_updated);

  // TODO(nihao.royal): use `enable_unified_pixel_pipeline` to switch multi
  // behaviours. After `RunPixelPipeline` is unified, we may remove the
  // redundant logic here.
  if (pipeline_option->enable_unified_pixel_pipeline) {
    pipeline_option->resolve_requested = true;
    pipeline_option->target_node = this;
  } else {
    element_manager()->OnPatchFinish(pipeline_option, this);
  }
}

bool FiberElement::ResolveStyleValue(CSSPropertyID id,
                                     const tasm::CSSValue &value,
                                     bool force_update) {
  bool resolve_success = false;
  if (computed_css_style()->SetValue(id, value)) {
    // The properties of transition and keyframe no need to be pushed to bundle
    // separately here. Those properties will be pushed to bundle together
    // later.
    if (!(CheckTransitionProps(id) || CheckKeyframeProps(id))) {
      PushToBundle(id);
    }

    resolve_success = true;
  }

  if (EnableLayoutInElementMode()) {
    if (LayoutProperty::IsLayoutWanted(id)) {
      MarkLayoutDirtyLite();
    }
  }

  return resolve_success;
}

void FiberElement::SetFontSize(const tasm::CSSValue &value) {
  base::flex_optional<float> result;
  if (!value.IsEmpty()) {
    CheckDynamicUnit(CSSPropertyID::kPropertyIDFontSize, value, false);
    // Take care: GetParentFontSize() here is used to computed em, so it must be
    // parent's fontSize.z
    const auto &env_config = element_manager()->GetLynxEnvConfig();
    auto unify_vw_vh_behavior =
        element_manager()->GetDynamicCSSConfigs().unify_vw_vh_behavior_;
    const auto &vw_base =
        unify_vw_vh_behavior
            ? env_config.ViewportWidth()
            : env_config.vwbase_for_font_size_to_align_with_legacy_bug();
    const auto &vh_base =
        unify_vw_vh_behavior
            ? env_config.ViewportHeight()
            : env_config.vhbase_for_font_size_to_align_with_legacy_bug();
    result = starlight::CSSStyleUtils::ResolveFontSize(
        value, env_config, vw_base, vh_base, GetParentFontSize(),
        GetRecordedRootFontSize(), element_manager()->GetCSSParserConfigs());
  } else {
    result = GetParentFontSize();
  }

  if (result.has_value() && *result != GetFontSize()) {
    NotifyUnitValuesUpdatedToAnimation(DynamicCSSStylesManager::kUpdateEm);

    if (is_page()) {
      SetFontSizeForAllElement(*result, *result);
      UpdateLayoutNodeFontSize(*result, *result);
    } else {
      SetFontSizeForAllElement(*result, GetRecordedRootFontSize());
      UpdateLayoutNodeFontSize(*result, GetRecordedRootFontSize());
    }

    if (!EnableLayoutInElementMode() || IsShadowNodeCustom()) {
      PreparePropBundleIfNeed();

      prop_bundle_->SetProps(
          CSSProperty::GetPropertyName(CSSPropertyID::kPropertyIDFontSize)
              .c_str(),
          *result);
    }
    if (is_page() && !parallel_flush_) {
      // TODO(zhouzhitao): to find a better way to make this work
      MarkFontSizeInvalidateRecursively();
    } else {
      // FIXME(linxs): if parent's font-size changed, we need to invalidate all
      // descendant‘ style, otherwise the descendant em pattern values will not
      // be updated dynamically. But it may cause perf issue, we can support it
      // later.
      MarkDirty(kDirtyFontSize);
    }
  }
}

void FiberElement::ResetFontSize() {
  CheckDynamicUnit(CSSPropertyID::kPropertyIDFontSize, CSSValue::Empty(), true);
  // root_font_size_&font_size_ here are used to computed rem&rem
  auto font_size = element_manager()->GetLynxEnvConfig().PageDefaultFontSize();
  auto root_font_size = is_page() ? font_size : GetCurrentRootFontSize();

  if (font_size != GetFontSize()) {
    SetFontSizeForAllElement(font_size, root_font_size);
    if (!EnableLayoutInElementMode() || IsShadowNodeCustom()) {
      PreparePropBundleIfNeed();
      prop_bundle_->SetProps(
          CSSProperty::GetPropertyName(CSSPropertyID::kPropertyIDFontSize)
              .c_str(),
          font_size);
    }
    UpdateLayoutNodeFontSize(font_size, root_font_size);
  }
}

Element *FiberElement::Sibling(int offset) const {
  if (!parent_) return nullptr;
  auto index = static_cast<FiberElement *>(parent_)->IndexOf(this);
  // We know the index can't be -1
  return parent_->GetChildAt(index + offset);
}

void FiberElement::InsertLayoutNode(FiberElement *child, FiberElement *ref) {
  DCHECK(!ref || !ref->is_wrapper());
  if (EnableLayoutInElementMode()) {
    EnsureSLNode();
    if (!is_virtual_ && !child->is_virtual_) {
      child->EnsureSLNode();
      sl_node_->InsertChildBefore(child->sl_node_.get(),
                                  ref ? ref->sl_node_.get() : nullptr);
    }
    child->attached_to_layout_parent_ = true;
    return;
  }

  if (child->attached_to_layout_parent_) {
    LOGE("FiberElement layout node already inserted !");
    this->LogNodeInfo();
    child->LogNodeInfo();
  }
  EnqueueLayoutTask([element_manager = element_manager(), id = id_,
                     child_id = child->impl_id(),
                     ref_id = ref ? ref->impl_id() : -1]() {
    element_manager->InsertLayoutNodeBefore(id, child_id, ref_id);
  });
  child->attached_to_layout_parent_ = true;
}

void FiberElement::RemoveLayoutNode(FiberElement *child) {
  if (EnableLayoutInElementMode()) {
    sl_node_->RemoveChild(child->sl_node_.get());
    return;
  }

  EnqueueLayoutTask([element_manager = element_manager(), id = id_,
                     child_id = child->impl_id()]() {
    element_manager->RemoveLayoutNode(id, child_id);
  });
  child->attached_to_layout_parent_ = false;
}

void FiberElement::StoreLayoutNode(FiberElement *child, FiberElement *ref) {
  child->render_parent_ = this;
  FiberElement *next_layout_sibling = ref;
  FiberElement *previous_layout_sibling =
      next_layout_sibling ? next_layout_sibling->previous_render_sibling_
                          : last_render_child_;
  if (previous_layout_sibling) {
    previous_layout_sibling->next_render_sibling_ = child;
  } else {
    first_render_child_ = child;
  }
  child->previous_render_sibling_ = previous_layout_sibling;

  if (next_layout_sibling) {
    next_layout_sibling->previous_render_sibling_ = child;
  } else {
    last_render_child_ = child;
  }
  child->next_render_sibling_ = next_layout_sibling;
}

void FiberElement::RestoreLayoutNode(FiberElement *node) {
  if (node->previous_render_sibling_) {
    node->previous_render_sibling_->next_render_sibling_ =
        node->next_render_sibling_;
  } else {
    first_render_child_ = node->next_render_sibling_;
  }
  if (node->next_render_sibling_) {
    node->next_render_sibling_->previous_render_sibling_ =
        node->previous_render_sibling_;
  } else {
    last_render_child_ = node->previous_render_sibling_;
  }
  node->render_parent_ = nullptr;
  node->previous_render_sibling_ = nullptr;
  node->next_render_sibling_ = nullptr;
}

void FiberElement::ParseRawInlineStyles(StyleMap *parsed_styles) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, FIBER_ELEMENT_PARSE_RAW_INLINE_STYLES);
  auto &configs = element_manager_->GetCSSParserConfigs();
  const auto &str = full_raw_inline_style_.str();
  ParseStyleDeclarationList(
      str.c_str(), static_cast<uint32_t>(str.size()),
      [this, parsed_styles, &configs](
          const char *key_start, uint32_t key_length, const char *value_start,
          uint32_t value_length) {
        auto id = CSSProperty::GetPropertyID(
            base::static_string::GenericCacheKey(key_start, key_length));
        if (CSSProperty::IsPropertyValid(id)) {
          auto value = lepus::Value(base::String(value_start, value_length));
          if (parsed_styles != nullptr) {
            UnitHandler::Process(id, value, *parsed_styles, configs);
          }
          current_raw_inline_styles_->insert_or_assign(id, std::move(value));
        }

        // DevTool needs to get InlineStyle information from DataModel's
        // InlineStyle, so when DevTool is enabled, it needs to set the
        // corresponding InlineStyle for DataModel.
        EXEC_EXPR_FOR_INSPECTOR(if (element_manager()->IsDomTreeEnabled()) {
          if (data_model() == nullptr) {
            return;
          }
          data_model()->SetInlineStyle(
              id, base::String(value_start, value_length), configs);
        });
      });

  EXEC_EXPR_FOR_INSPECTOR(if (element_manager()->IsDomTreeEnabled()) {
    element_manager()->OnElementNodeSetForInspector(this);
  });
}

void FiberElement::DoFullCSSResolving() {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, FIBER_ELEMENT_DO_FULL_STYLE_RESOLVE);

  CSSVariableMap changed_css_vars;
  ResolveStyle(parsed_styles_map_, &changed_css_vars);
  HandlePseudoElement();

  if (!(dirty_ & kDirtyCreated) && !changed_css_vars.empty()) {
    auto table = lepus::Dictionary::Create();

    for (const auto &iter : changed_css_vars) {
      table.get()->SetValue(iter.first, iter.second);
    }
    auto css_var_table = lepus::Value(std::move(table));

    if (IsRelatedCSSVariableUpdated(data_model(), css_var_table)) {
      // invalidate self.
      MarkStyleDirty(false);
    }
    HandleBeforeFlushActionsTask(
        [this, css_var_table_clone = lepus::Value::Clone(css_var_table)]() {
          RecursivelyMarkChildrenCSSVariableDirty(css_var_table_clone);
        });
  }
}

const tasm::CSSValue &FiberElement::ResolveCurrentStyleValue(
    const CSSPropertyID &key, const tasm::CSSValue &default_value) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, FIBER_ELEMENT_RESOLVE_CURRENT_STYLE);
  auto iter = parsed_styles_map_.find(key);
  if (iter != parsed_styles_map_.end()) {
    return iter->second;
  }

  const auto inherited_property = GetParentInheritedProperty();
  if (inherited_property.inherited_styles_ != nullptr) {
    auto iter = inherited_property.inherited_styles_->find(key);
    if (iter != inherited_property.inherited_styles_->end()) {
      return iter->second;
    }
  }

  return default_value;
}

bool FiberElement::RefreshStyle(StyleMap &parsed_styles,
                                base::Vector<CSSPropertyID> &reset_ids,
                                bool force_use_parsed_styles_map) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, FIBER_ELEMENT_REFRESH_STYLE,
              [this](lynx::perfetto::EventContext ctx) {
                UpdateTraceDebugInfo(ctx.event());
              });
  StyleMap pre_parsed_styles_map;
  if (!parsed_styles_map_.empty()) {
    pre_parsed_styles_map = std::move(parsed_styles_map_);
  }
  if (!has_extreme_parsed_styles_) {
    DoFullCSSResolving();
  } else {
    // if extreme_parsed_styles_ has set, we should ignore any class&inline
    // styles
    parsed_styles_map_ = *extreme_parsed_styles_;
    if (only_selector_extreme_parsed_styles_) {
      ProcessFullRawInlineStyle();
      MergeInlineStyles(parsed_styles_map_);
    }
    // Handle CSS varibale
    HandleCSSVariables(parsed_styles_map_);
  }
  if (force_use_parsed_styles_map) {
    // first flush does not need to do diff, just use parsed_styles_map_
    // directly
    return true;
  }

  // diff styles if needed
  bool ret =
      DiffStyleImpl(pre_parsed_styles_map, parsed_styles_map_, parsed_styles);
  // styles left in old_map need to be removed
  pre_parsed_styles_map.foreach (
      [&](const CSSPropertyID &k, const CSSValue &v) {
        // Filter shorthand property that need to be expanded
        if (!CSSProperty::IsShorthandProperty(k)) {
          reset_ids.emplace_back(k);
        }
      });
  return ret;
}

void FiberElement::OnClassChanged(const ClassList &old_classes,
                                  const ClassList &new_classes) {
  if (element_manager() && element_manager()->GetEnableStandardCSSSelector()) {
    if (element_manager()->CSSFragmentParsingOnTASMWorkerMTSRender()) {
      element_manager()->GetTasmWorkerTaskRunner()->PostTask(
          [this, old_classes_ = old_classes, new_classes_ = new_classes]() {
            CheckHasInvalidationForClass(old_classes_, new_classes_);
          });
    } else {
      CheckHasInvalidationForClass(old_classes, new_classes);
    }
  }
}

// For snapshot test
void FiberElement::DumpStyle(StyleMap &computed_styles) {
  StyleMap styles;
  base::InlineVector<CSSPropertyID, 16> reset_style_ids;
  this->RefreshStyle(styles, reset_style_ids);
  computed_styles = parsed_styles_map_;
}

void FiberElement::OnPseudoStatusChanged(PseudoState prev_status,
                                         PseudoState current_status) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, FIBER_ELEMENT_PSEUDO_CHANGED,
              [this](lynx::perfetto::EventContext ctx) {
                UpdateTraceDebugInfo(ctx.event());
              });
  auto current_context =
      element_manager_->element_manager_delegate()->GetCurrentPipelineContext();
  std::shared_ptr<PipelineOptions> pipeline_options;
  if (current_context) {
    pipeline_options = current_context->GetOptions();
  } else {
    pipeline_options = std::make_shared<PipelineOptions>();
  }
  // FIXME: Every element will emit the OnPseudoStatusChanged event
  auto *css_fragment = GetRelatedCSSFragment();
  if (css_fragment && css_fragment->enable_css_selector()) {
    // If disable the invalidation do nothing
    if (!css_fragment->enable_css_invalidation()) {
      return;
    }
    css::InvalidationLists invalidation_lists;
    CSSFragment::CollectPseudoChangedInvalidation(
        css_fragment, invalidation_lists, prev_status, current_status);
    data_model_->SetPseudoState(current_status);
    for (auto *invalidation_set : invalidation_lists.descendants) {
      if (invalidation_set->InvalidatesSelf()) {
        MarkStyleDirty(false);
      }
      InvalidateChildren(invalidation_set);
      element_manager_->RequestResolve(pipeline_options);
    }
    return;
  }

  if (!css_fragment || css_fragment->pseudo_map().empty()) {
    // no need do any pseudo changing logic if no any touch pseudo token
    return;
  }

  bool cascade_pseudo_enabled = element_manager_->GetEnableCascadePseudo();
  MarkStyleDirty(cascade_pseudo_enabled);

  has_extreme_parsed_styles_ = false;

  data_model_->SetPseudoState(current_status);
  element_manager_->RequestResolve(pipeline_options);
}

bool FiberElement::IsInheritable(CSSPropertyID id) const {
  if (!IsCSSInheritanceEnabled()) {
    return false;
  }

  if (!element_manager_->GetDynamicCSSConfigs().custom_inherit_list_.empty()) {
    return element_manager_->GetDynamicCSSConfigs().custom_inherit_list_.count(
        id);
  }

  return DynamicCSSStylesManager::GetInheritableProps().count(id);
}

bool FiberElement::IsDirectionChangedEnabled() const {
  // FIXME(linxs): we just use enable_css_inheritance_ to indicate is enable
  // direction temporarily
  // DirectionChange is enabled by default in RadonArch mode.
  // TODO(kechenglong): Avoid using IsRadonArch() & IsFiberArch() in Dom layer.
  return IsRadonArch() ||
         element_manager_->GetDynamicCSSConfigs().enable_css_inheritance_;
}

// return ture means the style has already been processed
bool FiberElement::TryResolveLogicStyleAndSaveDirectionRelatedStyle(
    CSSPropertyID id, const CSSValue &value) {
  if (!IsDirectionChangedEnabled()) {
    return false;
  }
  // special case.
  if (id == kPropertyIDTextAlign) {
    CSSStyleValue style_type = ResolveTextAlign(id, value, direction_);
    SetStyleInternal(style_type.first, style_type.second);
    return true;
  }
  auto res = ConvertRtlCSSPropertyID(id);
  if (res.first) {
    // Consume and record transition style first before RTL mode.
    if (css_transition_manager_ &&
        css_transition_manager_->ConsumeCSSProperty(id, value)) {
      return true;
    }
    RecordElementPreviousStyle(id, value);
    SetStyleInternal(res.second, value);
    return true;
  }
  return false;
}

std::pair<bool, CSSPropertyID> FiberElement::ConvertRtlCSSPropertyID(
    CSSPropertyID id) {
  auto direction_mapping = CheckDirectionMapping(id);
  bool is_logic_property = direction_mapping.is_logic_;

  // default ltr_property/rtl_property for CSSProperty is kPropertyStart.
  bool is_direction_aware_property =
      direction_mapping.ltr_property_ != kPropertyStart ||
      direction_mapping.rtl_property_ != kPropertyStart;
  if (is_direction_aware_property) {
    // When in LynxRTL mode or RTL mode with current property is a logic
    // property, use RTL CSSPropertyID, other wise use LTR CSSPropertyID
    bool use_rtl_value =
        (IsRTL(direction_) && is_logic_property) || IsLynxRTL(direction_);
    return std::make_pair(true, use_rtl_value
                                    ? direction_mapping.rtl_property_
                                    : direction_mapping.ltr_property_);
  }
  return std::make_pair(false, id);
}

// try to Resolve Direction css
void FiberElement::TryDoDirectionRelatedCSSChange(CSSPropertyID id,
                                                  const CSSValue &value,
                                                  IsLogic is_logic_style) {
  CSSPropertyID trans_id = id;
  if ((IsRTL(direction_) && is_logic_style) || IsLynxRTL(direction_)) {
    auto direction_mapping = CheckDirectionMapping(id);
    trans_id = direction_mapping.rtl_property_;
  } else if (is_logic_style) {
    // Logical property need to be converted to non-logical property that can be
    // recognized by layout engine,i.e. start->left/right
    auto direction_mapping = CheckDirectionMapping(id);
    trans_id = direction_mapping.ltr_property_;
  }

  SetStyleInternal(trans_id, value);
}

void FiberElement::ResetTextAlign(StyleMap &update_map,
                                  bool direction_changed) {
  // If direction has been changed in current render loop, text_align will be
  // reset when handling direction change. Thus when reset text align,
  // set kPropertyIDTextAlign to kStart only when direction is not changed.
  if (!direction_changed) {
    update_map[CSSPropertyID::kPropertyIDTextAlign] = CSSValue(
        lepus::Value(static_cast<int32_t>(starlight::TextAlignType::kStart)),
        CSSValuePattern::ENUM);
  }
}

void FiberElement::WillResetCSSValue(CSSPropertyID &css_id) {
  if (css_id == CSSPropertyID::kPropertyIDFontSize) {
    ResetFontSize();
  }

  // remove self inherit properties if needed
  if (inherited_styles_.has_value()) {
    auto it = inherited_styles_->find(css_id);
    if (it != inherited_styles_->end()) {
      inherited_styles_->erase(it);
      reset_inherited_ids_->emplace_back(css_id);
      children_propagate_inherited_styles_flag_ = true;
    }
  }
}

void FiberElement::ConvertToInlineElement() {
  MarkAsInline();
  for (auto &child : scoped_children_) {
    child->ConvertToInlineElement();
  }
}

void FiberElement::TraversalInsertFixedElementOfTree() {
  if (!is_page() && need_handle_fixed_) {
    HandleSelfFixedChange();
    need_handle_fixed_ = false;
  }
  for (auto child : scoped_children_) {
    child->TraversalInsertFixedElementOfTree();
  }
}

void FiberElement::HandleSelfFixedChange() {
  // 1. If enableFixedNew is `true`, return directly.
  if (GetEnableFixedNew()) {
    return;
  }
  // 2. When Using NoDiff, if the element's fixed status is not changed or the
  // element don't have its render_parnet_, return directly.
  // 3. When Using RadonDiff, if the element is not fixed and its fixed status
  // is not changed, return directly.
  bool early_return_condition = false;
  if (IsFiberArch()) {
    early_return_condition = !fixed_changed_ || !render_parent_;
  } else if (IsRadonArch()) {
    early_return_condition = !is_fixed_ && !fixed_changed_;
  }
  if (early_return_condition) {
    return;
  }

  if (is_fixed_) {
    // non-fixed to fixed
    auto *parent = render_parent_;
    if (!IsFiberArch() && !parent) {
      parent = element_manager()->GetPageElement();
    } else if (parent) {
      parent->HandleRemoveChildAction(this);
    }
    parent->InsertFixedElement(this, next_render_sibling_);
  } else {
    // fixed to non-fixed
    RemoveFixedElement(this);
    auto *parent = static_cast<FiberElement *>(this->parent_);
    auto index = parent->IndexOf(this);
    auto *ref_node = static_cast<FiberElement *>(parent->GetChildAt(index + 1));
    parent->HandleInsertChildAction(this, -1, ref_node);
  }
}

void FiberElement::InsertFixedElement(FiberElement *child,
                                      FiberElement *ref_node) {
  DCHECK(child->is_fixed_);
  // FIXME(linxs): insert fixed child, to be refined later, currently always
  // insert to the end
  auto *parent = static_cast<FiberElement *>(element_manager_->root());
  parent->HandleInsertChildAction(child, 0, nullptr);
  child->fixed_changed_ = false;
}

void FiberElement::RemoveFixedElement(FiberElement *child) {
  // FIXME(linxs): remove fixed child, to be refined later
  if (child->render_parent_ != element_manager_->root()) {
    LOGE("FiberElement::RemoveFixedElement got error for wrong render parent");
    return;
  }

  auto *parent = static_cast<FiberElement *>(element_manager_->root());
  parent->HandleRemoveChildAction(child);
  child->fixed_changed_ = false;
}

// FIXME(baiqiang): CALC info should be parsed before, and uniftied with
// DynamicCSSStylesManager::GetValueFlags
bool CheckCALCValueHasViewPortUnit(const CSSValue &value) {
  const auto &str = value.GetValue().StdString();
  if (str.find("vw") != std::string::npos ||
      str.find("vh") != std::string::npos ||
      str.find("view_width") != std::string::npos ||
      str.find("view_height") != std::string::npos) {
    return true;
  }
  return false;
}

void FiberElement::CheckDynamicUnit(CSSPropertyID id, const CSSValue &value,
                                    bool reset) {
  if (reset && parsed_styles_map_.empty()) {
    // TODO(linxs): try to clear dynamic_style_flags_ here
    dynamic_style_flags_ = 0;
    return;
  }

  dynamic_style_flags_ |= DynamicCSSStylesManager::GetValueFlags(
      id, value,
      element_manager()->GetDynamicCSSConfigs().unify_vw_vh_behavior_);
}

bool FiberElement::CheckHasInvalidationForId(const std::string &old_id,
                                             const std::string &new_id) {
  auto *css_fragment = GetRelatedCSSFragment();
  // resolve styles from css fragment
  if (!css_fragment || !css_fragment->enable_css_invalidation()) {
    return false;
  }
  auto old_size = invalidation_lists_.descendants.size();
  CSSFragment::CollectIdChangedInvalidation(css_fragment, invalidation_lists_,
                                            old_id, new_id);
  return invalidation_lists_.descendants.size() != old_size;
}

bool FiberElement::CheckHasInvalidationForClass(const ClassList &old_classes,
                                                const ClassList &new_classes) {
  auto *css_fragment = GetRelatedCSSFragment();
  // resolve styles from css fragment
  if (!css_fragment || !css_fragment->enable_css_invalidation()) {
    return false;
  }
  auto old_size = invalidation_lists_.descendants.size();
  CSSFragment::CollectClassChangedInvalidation(
      css_fragment, invalidation_lists_, old_classes, new_classes);
  return invalidation_lists_.descendants.size() != old_size;
}

void FiberElement::InvalidateChildren(css::InvalidationSet *invalidation_set) {
  if (invalidation_set->WholeSubtreeInvalid() || !invalidation_set->IsEmpty()) {
    VisitChildren([invalidation_set](FiberElement *child) {
      if (!child->StyleDirty() && !child->is_raw_text() &&
          invalidation_set->InvalidatesElement(*child->data_model())) {
        child->MarkStyleDirty(false);
      }
    });
  }
}

void FiberElement::VisitChildren(
    const base::MoveOnlyClosure<void, FiberElement *> &visitor) {
  for (auto &child : scoped_children_) {
    // In fiber mode, we skip the children in component
    if (!child->is_component()) {
      visitor(child.get());
      child->VisitChildren(visitor);
    }
  }
}

void FiberElement::LogNodeInfo() {
  LOGE("FiberElement node ,this:"
       << this << ", tag:" << tag_.str() << ",id:" << id_
       << (!data_model_->idSelector().empty() ? data_model_->idSelector().str()
                                              : "")
       << ", first class:"
       << (data_model_->classes().size() > 0 ? data_model_->classes()[0].str()
                                             : ""));
}

void FiberElement::ConsumeTransitionStylesInAdvanceInternal(
    CSSPropertyID css_id, const tasm::CSSValue &value) {
  SetStyleInternal(css_id, value);
}

void FiberElement::ResetTransitionStylesInAdvanceInternal(
    CSSPropertyID css_id) {
  ResetStyleInternal(css_id);
}

void FiberElement::OnPatchFinish(std::shared_ptr<PipelineOptions> &option) {
  element_manager_->OnPatchFinish(option, this);
}

void FiberElement::FlushAnimatedStyleInternal(tasm::CSSPropertyID id,
                                              const tasm::CSSValue &value) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, FIBER_ELEMENT_FLUSH_ANIMATED_STYLE);
  auto trans_id = ConvertRtlCSSPropertyID(id).second;
  if (value != CSSValue::Empty()) {
    SetStyleInternal(trans_id, value);
  } else {
    ResetStyleInternal(trans_id);
  }
}

std::optional<CSSValue> FiberElement::GetElementStyle(
    tasm::CSSPropertyID css_id) {
  auto iter = parsed_styles_map_.find(css_id);
  if (iter != parsed_styles_map_.end()) {
    return iter->second;
  }
  if (updated_inherited_styles_.has_value()) {
    iter = updated_inherited_styles_->find(css_id);
    if (iter != updated_inherited_styles_->end()) {
      return iter->second;
    }
  }
  return {};
}

void FiberElement::UpdateDynamicElementStyleRecursively(uint32_t style,
                                                        bool force_update) {
  if (is_raw_text()) {
    return;
  }
  bool inner_force_update = false || force_update;

  if ((dynamic_style_flags_ > 0 || inner_force_update) && !is_wrapper()) {
    // Style could never be "all" here.
    NotifyUnitValuesUpdatedToAnimation(style);
    const auto &env_config = element_manager()->GetLynxEnvConfig();
    const auto &css_config = element_manager()->GetDynamicCSSConfigs();

    bool font_scale_changed =
        (dynamic_style_flags_ & DynamicCSSStylesManager::kUpdateFontScale) &&
        (style & DynamicCSSStylesManager::kUpdateFontScale) &&
        (computed_css_style()->GetMeasureContext().font_scale_ !=
         env_config.FontScale());
    bool viewport_changed =
        (dynamic_style_flags_ & DynamicCSSStylesManager::kUpdateViewport) &&
        (style & DynamicCSSStylesManager::kUpdateViewport) &&
        !(env_config.ViewportWidth() ==
              computed_css_style()->GetMeasureContext().viewport_width_ &&
          env_config.ViewportHeight() ==
              computed_css_style()->GetMeasureContext().viewport_height_);
    bool screen_matrix_changed =
        (dynamic_style_flags_ &
         DynamicCSSStylesManager::kUpdateScreenMetrics) &&
        (style & DynamicCSSStylesManager::kUpdateScreenMetrics) &&
        (env_config.ScreenWidth() !=
         computed_css_style()->GetMeasureContext().screen_width_);
    bool rem_changed =
        (dynamic_style_flags_ & DynamicCSSStylesManager::kUpdateRem) &&
        (style & DynamicCSSStylesManager::kUpdateRem);

    if (GetCurrentRootFontSize() != GetRecordedRootFontSize()) {
      computed_css_style()->SetFontSize(GetFontSize(),
                                        GetCurrentRootFontSize());
      UpdateLayoutNodeFontSize(GetFontSize(), GetCurrentRootFontSize());
    }

    if (inner_force_update || font_scale_changed || viewport_changed ||
        screen_matrix_changed || rem_changed) {
      UpdateLengthContextValueForAllElement(env_config);
      const auto property = GetParentInheritedProperty();

      ConsumeStyleInternal(
          parsed_styles_map_, property.inherited_styles_,
          [this, style, &css_config](auto id, const auto &value) {
            if (CSSProperty::IsTransitionProps(id) ||
                CSSProperty::IsKeyframeProps(id)) {
              return true;
            }

            if (css_transition_manager_ &&
                css_transition_manager_->NeedsTransition(id)) {
              return true;
            }

            auto new_flags = DynamicCSSStylesManager::GetValueFlags(
                id, value, css_config.unify_vw_vh_behavior_);

            if ((new_flags & (style | ((dirty_ & kDirtyFontSize)
                                           ? DynamicCSSStylesManager::kUpdateEm
                                           : 0))) == 0) {
              return true;
            }

            return false;
          });

      if (inherited_styles_.has_value() && !inherited_styles_->empty()) {
        inner_force_update |= true;
      }

      if (prop_bundle_) {
        UpdateLayoutNodeProps(prop_bundle_);
        if (!is_virtual()) {
          UpdateFiberElement();
        }
      }

      UpdateLayoutNodeByBundle();
    }
  }

  if (dirty_ & kDirtyFontSize) {
    if (is_page()) {
      style |= DynamicCSSStylesManager::kUpdateRem;
    }
    dirty_ &= ~kDirtyFontSize;
  }

  auto *child = first_render_child_;
  while (child) {
    child->UpdateDynamicElementStyleRecursively(style, inner_force_update);
    child = child->next_render_sibling_;
  }
}

void FiberElement::UpdateDynamicElementStyle(uint32_t style,
                                             bool force_update) {
  UpdateDynamicElementStyleRecursively(style, force_update);
  if (element_manager()->GetEnableBatchLayoutTaskWithSyncLayout()) {
    element_context_delegate_->FlushEnqueuedTasks();
  }
}

void FiberElement::SetCSSID(int32_t id) {
  if (css_id_ != id) {
    ResetStyleSheet();
    css_id_ = id;
  }
}

FiberElement *FiberElement::root_virtual_parent() {
  FiberElement *root_virtual = virtual_parent_;
  while (root_virtual && root_virtual->virtual_parent() != nullptr) {
    root_virtual = root_virtual->virtual_parent();
  }
  return root_virtual;
}

void FiberElement::ResetSheetRecursively(
    const std::shared_ptr<CSSStyleSheetManager> &manager) {
  if (is_page() || is_component() || css_id_ != kInvalidCssId) {
    set_style_sheet_manager(manager);
  }

  // reset style sheet.
  ResetStyleSheet();
  for (const auto &child : children()) {
    child->ResetSheetRecursively(manager);
  }
}

void FiberElement::PrepareOrUpdatePseudoElement(PseudoState state,
                                                StyleMap &style_map) {
  if (style_map.empty() &&
      (!pseudo_elements_.has_value() ||
       pseudo_elements_->find(state) == pseudo_elements_->end())) {
    return;
  }

  PseudoElement *pseudo_element = CreatePseudoElementIfNeed(state);
  pseudo_element->UpdateStyleMap(style_map);
}

PseudoElement *FiberElement::CreatePseudoElementIfNeed(PseudoState state) {
  if (pseudo_elements_.has_value()) {
    auto it = pseudo_elements_->find(state);
    if (it != pseudo_elements_->end()) {
      return it->second.get();
    }
  }

  auto new_pseudo_element = std::make_unique<PseudoElement>(state, this);
  auto result = new_pseudo_element.get();
  (*pseudo_elements_)[state] = std::move(new_pseudo_element);
  return result;
}

void FiberElement::RecursivelyMarkRenderRootElement(FiberElement *render_root) {
  render_root_element_ = render_root;
  if (render_root) {
    element_context_delegate_ = render_root->element_context_delegate_;
  }
  for (auto child : scoped_children_) {
    if (!child->is_list_item()) {
      child->RecursivelyMarkRenderRootElement(render_root);
    }
  }
}

void FiberElement::UpdateRenderRootElementIfNecessary(FiberElement *child) {
  if (child->render_root_element_ == this->render_root_element_) {
    // 1. Child has same render root as parent, indicating tree mutation inside
    // same render root, no need to propagate change
    return;
  }
  if (child->render_root_element_ == nullptr) {
    // 2. child doesn't hava valid render_root_element, propagate parent's
    // render_root_element to child subtree
    child->RecursivelyMarkRenderRootElement(this->render_root_element_);
    return;
  }
  if (this->render_root_element_ == nullptr) {
    // 3. parent doesn't have valid render_root_element, reset chlld subtree
    // render root
    child->RecursivelyMarkRenderRootElement(nullptr);
    return;
  }
  // 4.child and parent have different valid render_root_elements, throw warning
  LOGW(
      "FiberElement move element to a different render root, inefficient "
      "operation");
  // Update child subtree render root with parent render root
  child->RecursivelyMarkRenderRootElement(this->render_root_element_);
}

void FiberElement::SetFontSizeForAllElement(double cur_node_font_size,
                                            double root_node_font_size) {
  computed_css_style()->SetFontSize(cur_node_font_size, root_node_font_size);

  if (pseudo_elements_.has_value()) {
    for (const auto &[key, pseudo_element] : *pseudo_elements_) {
      pseudo_element->SetFontSize(cur_node_font_size, root_node_font_size);
    }
  }
}

void FiberElement::UpdateLengthContextValueForAllElement(
    const LynxEnvConfig &env_config) {
  computed_css_style()->SetFontScale(env_config.FontScale());
  computed_css_style()->SetViewportWidth(env_config.ViewportWidth());
  computed_css_style()->SetViewportHeight(env_config.ViewportHeight());
  computed_css_style()->SetScreenWidth(env_config.ScreenWidth());

  if (pseudo_elements_.has_value()) {
    for (const auto &[key, pseudo_element] : *pseudo_elements_) {
      pseudo_element.get()->ComputedCSSStyle()->SetFontScale(
          env_config.FontScale());
      pseudo_element.get()->ComputedCSSStyle()->SetViewportWidth(
          env_config.ViewportWidth());
      pseudo_element.get()->ComputedCSSStyle()->SetViewportHeight(
          env_config.ViewportHeight());
      pseudo_element.get()->ComputedCSSStyle()->SetScreenWidth(
          env_config.ScreenWidth());
    }
  }
}

// TODO: Move this method out of fiber_element when a more general render root
// is introduced.
void FiberElement::AsyncResolveSubtreeProperty() {
  if (element_manager()->GetEnableBatchLayoutTaskWithSyncLayout()) {
    if (element_manager()->GetEnableParallelElement() &&
        ((dirty_ & ~kDirtyTree) != 0) && element_context_delegate_ &&
        element_context_delegate_->IsListItemElementContext()) {
      element_manager()->GetTasmWorkerTaskRunner()->PostTask([this]() mutable {
        auto list_item_context_ptr =
            static_cast<ListItemSchedulerAdapter *>(element_context_delegate_);
        list_item_context_ptr->ResolveSubtreeProperty();

        std::promise<ParallelFlushReturn> promise;
        std::future<ParallelFlushReturn> future = promise.get_future();
        auto task_info_ptr =
            fml::MakeRefCounted<base::OnceTask<ParallelFlushReturn>>(
                [promise = std::move(promise),
                 context_ptr = list_item_context_ptr]() mutable {
                  promise.set_value(
                      context_ptr->GenerateReduceTaskForResolveProperty());
                },
                std::move(future));
        element_manager()->ParallelTasks().emplace_back(
            std::move(task_info_ptr));
      });
    }
  } else {
    // TODO(ZHOUZHITAO): remove this branch once
    // ENABLE_BATCH_LAYOUT_TASK_WITH_SYNC_LAYOUT is fully rolled out
    if (element_manager()->GetEnableParallelElement() &&
        ((dirty_ & ~kDirtyTree) != 0) && scheduler_adapter_.get()) {
      element_manager()->GetTasmWorkerTaskRunner()->PostTask([this]() mutable {
        scheduler_adapter_->ResolveSubtreeProperty();

        std::promise<ParallelFlushReturn> promise;
        std::future<ParallelFlushReturn> future = promise.get_future();
        auto task_info_ptr =
            fml::MakeRefCounted<base::OnceTask<ParallelFlushReturn>>(
                [promise = std::move(promise),
                 scheduler = scheduler_adapter_.get()]() mutable {
                  promise.set_value(
                      scheduler->GenerateReduceTaskForResolveProperty());
                },
                std::move(future));
        element_manager()->ParallelTasks().emplace_back(
            std::move(task_info_ptr));
      });
    }
  }
}

void FiberElement::CreateListItemScheduler(
    list::BatchRenderStrategy batch_render_strategy,
    ElementContextDelegate *parent_context, bool continuous_resolve_tree) {
  if (element_manager()->GetEnableBatchLayoutTaskWithSyncLayout()) {
    std::shared_ptr<ElementContextDelegate> element_context_delegate_ptr =
        std::make_shared<ListItemSchedulerAdapter>(this, batch_render_strategy,
                                                   parent_context,
                                                   continuous_resolve_tree);
    element_context_delegate_ = element_context_delegate_ptr.get();
    parent_context->OnChildElementContextAdded(element_context_delegate_ptr);
  } else {
    scheduler_adapter_ = std::make_unique<ListItemSchedulerAdapter>(
        this, batch_render_strategy, parent_context, continuous_resolve_tree);
  }
}

void FiberElement::DispatchAsyncResolveSubtreeProperty() {
  if (element_manager()->GetEnableParallelElement() &&
      ((dirty_ & ~kDirtyTree) != 0) && this->IsAttached()) {
    UpdateResolveStatus(AsyncResolveStatus::kPrepareTriggered);
    element_manager()->GetTasmWorkerTaskRunner()->PostTask([this]() mutable {
      std::deque<FiberElement *> queue;
      auto root = this;
      queue.emplace_back(root);
      while (!queue.empty()) {
        auto current = queue.front();
        if ((current != root && current->IsAsyncFlushRoot()) ||
            current->IsAsyncResolveResolving()) {
          // skip async flush root element
          queue.pop_front();
          continue;
        }
        {
          current->UpdateResolveStatus(AsyncResolveStatus::kPreparing);
          current->ResolveParentComponentElement();
          if (current->parent()) {
            current->parent()->EnsureTagInfo();
          }
          current->PostResolveTaskToThreadPool(
              false, element_manager()->ParallelTasks());
        }
        for (const auto &child : current->children()) {
          queue.emplace_back(child.get());
        }
        queue.pop_front();
      }
    });
  }
}

bool FiberElement::CanBeLayoutOnly() const {
  return can_be_layout_only_ && element_manager()->GetEnableLayoutOnly() &&
         has_layout_only_props_ && overflow_ == OVERFLOW_XY;
}

void FiberElement::MarkLayoutDirtyLite() {
  if (!is_virtual_) {
    EnsureSLNode();
    sl_node_->MarkDirty();
  } else {
    auto *parent = render_parent_;
    while (parent) {
      if (!parent->is_virtual_) {
        parent->MarkLayoutDirtyLite();
        break;
      }
      parent = parent->render_parent_;
    }
  }
}

/**
 * Reference {@link LayoutContext#IfNeedsUpdateLayoutInfo }
 */
bool FiberElement::IfNeedsUpdateLayoutInfo() {
  if (sl_node_ == nullptr) {
    return false;
  }

  return sl_node_->GetHasNewLayout();
}

/**
 * Reference {@link LayoutContext#LayoutRecursively }
 */
void FiberElement::UpdateLayoutInfoRecursively() {
  if (!is_wrapper()) {
    if (sl_node_ == nullptr || !(sl_node_->IsDirty())) {
      return;
    }

    if (IfNeedsUpdateLayoutInfo()) {
      UpdateLayoutInfo();
    }

    sl_node_->MarkUpdated();
  }

  for (auto &child : scoped_children_) {
    child->UpdateLayoutInfoRecursively();
  }
}

/**
 * Reference {@link LayoutContext#UpdateLayoutInfo }
 */
void FiberElement::UpdateLayoutInfo() {
  const auto &layout_result = sl_node_->GetLayoutResult();
  width_ = layout_result.size_.width_;
  height_ = layout_result.size_.height_;
  top_ = layout_result.offset_.Y();
  left_ = layout_result.offset_.X();
  // paddings
  paddings_[0] = layout_result.padding_[starlight::kLeft];
  paddings_[1] = layout_result.padding_[starlight::kTop];
  paddings_[2] = layout_result.padding_[starlight::kRight];
  paddings_[3] = layout_result.padding_[starlight::kBottom];
  // margins
  margins_[0] = layout_result.margin_[starlight::kLeft];
  margins_[1] = layout_result.margin_[starlight::kTop];
  margins_[2] = layout_result.margin_[starlight::kRight];
  margins_[3] = layout_result.margin_[starlight::kBottom];
  // borders
  borders_[0] = layout_result.border_[starlight::kLeft];
  borders_[1] = layout_result.border_[starlight::kTop];
  borders_[2] = layout_result.border_[starlight::kRight];
  borders_[3] = layout_result.border_[starlight::kBottom];

  if (IsShadowNodeCustom()) {
    customized_layout_node_->OnLayoutAfter();
  }
  frame_changed_ = true;
}

void FiberElement::SetMeasureFunc(void *context,
                                  starlight::SLMeasureFunc measure_func) {
  sl_node_->SetContext(context);
  sl_node_->SetSLMeasureFunc(std::move(measure_func));
}

void FiberElement::SetAlignmentFunc(void *context,
                                    starlight::SLAlignmentFunc alignment_func) {
  sl_node_->SetSLAlignmentFunc(std::move(alignment_func));
}

/**
 * Reference {@link LayoutContext#DispatchLayoutBeforeRecursively }
 */
void FiberElement::DispatchLayoutBeforeRecursively() {
  if (!is_wrapper()) {
    if (sl_node_ == nullptr || !(sl_node_->IsDirty())) {
      return;
    }

    if (sl_node_->GetSLMeasureFunc()) {
      DispatchLayoutBefore();
    }
  }

  for (auto &child : scoped_children_) {
    child->DispatchLayoutBeforeRecursively();
  }
}

void FiberElement::DispatchLayoutBefore() {
  if (customized_layout_node_) {
    customized_layout_node_->OnLayoutBefore();
  }
}

#if ENABLE_TRACE_PERFETTO
void FiberElement::UpdateTraceDebugInfo(TraceEvent *event) {
  auto *tagInfo = event->add_debug_annotations();
  tagInfo->set_name("tagName");
  tagInfo->set_string_value(tag_.str());

  if (!data_model_) {
    return;
  }

  if (!data_model_->idSelector().empty()) {
    auto *idInfo = event->add_debug_annotations();
    idInfo->set_name("idSelector");
    idInfo->set_string_value(data_model_->idSelector().str());
  }
  if (!data_model_->classes().empty()) {
    std::string class_str = "";
    for (auto &aClass : data_model_->classes()) {
      class_str = class_str + " " + aClass.str();
    }
    if (!class_str.empty()) {
      auto *classInfo = event->add_debug_annotations();
      classInfo->set_name("class");
      classInfo->set_string_value(class_str);
    }
  }
}
#endif

bool FiberElement::IsEventPathCatch() {
  if (IsDetached()) {
    LOGE("FiberElement::IsEventPathCatch error: the target is detached.");
    return true;
  }
  // Compatible with the previous logic that position:fixed will modify
  // the structure of the element tree.
  bool enable_fiber_element_for_radon_diff =
      element_manager()->GetEnableFiberElementForRadonDiff();
  if (enable_fiber_element_for_radon_diff && IsRadonArch() && is_fixed()) {
    auto root = element_manager()->root();
    if (this != root) {
      LOGI("FiberElement::IsEventPathCatch fixed target.");
      return true;
    }
  }
  return false;
}

lepus::Value FiberElement::GetEventTargetInfo(bool is_core_event) {
  auto dict = lepus::Dictionary::Create();
  if (data_model_ != nullptr) {
    BASE_STATIC_STRING_DECL(kId, "id");
    BASE_STATIC_STRING_DECL(kDataset, "dataset");
    BASE_STATIC_STRING_DECL(kUid, "uid");

    dict.get()->SetValue(kId, data_model_->idSelector());
    auto dataset = lepus::Dictionary::Create();
    for (const auto &[key, value] : data_model_->dataset()) {
      dataset.get()->SetValue(key, value);
    }
    dict.get()->SetValue(kDataset, std::move(dataset));
    dict.get()->SetValue(kUid, id_);
  }

  // element ref needed in fiber element worklet
  if (is_core_event) {
    BASE_STATIC_STRING_DECL(kElementRefptr, "elementRefptr");
    dict.get()->SetValue(kElementRefptr, fml::RefPtr<tasm::FiberElement>(this));
  }

  return lepus::Value(std::move(dict));
}

lepus::Value FiberElement::GetEventControlInfo(const std::string &event_type,
                                               bool is_global) {
  auto array = lepus::CArray::Create();
  if (InComponent()) {
    array->emplace_back(false);
    array->emplace_back("");
  } else {
    array->emplace_back(true);
    array->emplace_back(ParentComponentId());
  }
  if (is_global) {
    const auto &bind_event_map = global_bind_event_map();
    if (auto it = bind_event_map.find(event_type); it != bind_event_map.end()) {
      auto event_handler = it->second.get();
      array->emplace_back(event_handler->function().str());
    }
  } else {
    const auto &bind_event_map = event_map();
    if (auto it = bind_event_map.find(event_type); it != bind_event_map.end()) {
      auto event_handler = it->second.get();
      array->emplace_back(event_handler->function().str());
    }
  }

  return lepus::Value(std::move(array));
}

}  // namespace tasm
}  // namespace lynx
