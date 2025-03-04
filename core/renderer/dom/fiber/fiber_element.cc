// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/dom/fiber/fiber_element.h"

#include <algorithm>
#include <memory>
#include <stack>
#include <string>
#include <utility>
#include <vector>

#include "base/include/compiler_specific.h"
#include "base/include/path_utils.h"
#include "base/include/timer/time_utils.h"
#include "base/include/value/base_string.h"
#include "base/trace/native/trace_event.h"
#include "core/base/lynx_trace_categories.h"
#include "core/renderer/css/css_color.h"
#include "core/renderer/css/css_keyframes_token.h"
#include "core/renderer/css/css_property.h"
#include "core/renderer/css/css_utils.h"
#include "core/renderer/css/parser/length_handler.h"
#include "core/renderer/css/unit_handler.h"
#include "core/renderer/dom/element_manager.h"
#include "core/renderer/dom/fiber/component_element.h"
#include "core/renderer/dom/fiber/image_element.h"
#include "core/renderer/dom/fiber/list_element.h"
#include "core/renderer/dom/fiber/none_element.h"
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
#include "core/renderer/starlight/layout/layout_object.h"
#include "core/renderer/starlight/style/default_layout_style.h"
#include "core/renderer/template_assembler.h"
#include "core/renderer/ui_wrapper/layout/layout_node.h"
#include "core/renderer/utils/lynx_env.h"
#include "core/renderer/utils/value_utils.h"
#include "core/runtime/bindings/jsi/java_script_element.h"
#include "core/runtime/vm/lepus/array.h"
#include "core/runtime/vm/lepus/table.h"
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
    : Element(tag, manager),
      dirty_(kDirtyCreated),
      node_manager_(manager ? manager->node_manager() : nullptr),
      css_id_(css_id) {
  css_patching_.SetEnableFiberArch(true);
  InitLayoutBundle();
  SetAttributeHolder(std::make_shared<AttributeHolder>(this));

  if (tag.IsEquals("x-overlay-ng")) {
    can_has_layout_only_children_ = false;
  }

  if (manager == nullptr) {
    return;
  }

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
      path_(element.path_),
      dirty_(element.dirty_ | kDirtyCreated),
      flush_required_(element.flush_required_),
      full_raw_inline_style_(element.full_raw_inline_style_),
      current_raw_inline_styles_(element.current_raw_inline_styles_),
      dynamic_style_flags_(element.dynamic_style_flags_),
      has_extreme_parsed_styles_(element.has_extreme_parsed_styles_),
      only_selector_extreme_parsed_styles_(
          element.only_selector_extreme_parsed_styles_),
      extreme_parsed_styles_(element.extreme_parsed_styles_),
      inherited_styles_(element.inherited_styles_),
      reset_inherited_ids_(element.reset_inherited_ids_),
      can_be_layout_only_(element.can_be_layout_only_),
      parent_component_unique_id_(element.parent_component_unique_id_),
      updated_attr_map_(element.updated_attr_map_),
      reset_attr_vec_(element.reset_attr_vec_),
      css_id_(element.css_id_),
      is_template_(element.is_template_),
      part_id_(element.part_id_),
      builtin_attr_map_(element.builtin_attr_map_) {
  SetAttributeHolder(std::make_shared<AttributeHolder>(*element.data_model()));
  data_model_->set_css_variables_map(element.data_model()->css_variables_map());

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

  if (element.config().IsObject() && element.config().GetLength() > 0) {
    config_ = lepus::Value::ShallowCopy(element.config());
  }

  UpdateInheritedProperty();
  // TODO(wujintian): Clone animation-related objects.
}

void FiberElement::AttachToElementManager(
    ElementManager *manager,
    const std::shared_ptr<CSSStyleSheetManager> &style_manager,
    bool keep_element_id) {
  Element::AttachToElementManager(manager, style_manager, keep_element_id);

  node_manager_ = manager->node_manager();

  const auto &env_config = manager->GetLynxEnvConfig();
  if (platform_css_style_ == nullptr) {
    platform_css_style_ = std::make_unique<starlight::ComputedCSSStyle>(
        *manager->platform_computed_css());
  }

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

  for (auto &layout_style : layout_styles_) {
    UpdateLayoutNodeStyle(layout_style.first, layout_style.second);
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
}

void FiberElement::OnNodeAdded(FiberElement *child) {
  if (is_inline_element() && child != nullptr) {
    child->ConvertToInlineElement();
  }

  UpdateRenderRootElementIfNecessary(child);
}

FiberElement::~FiberElement() {
  if (!will_destroy_ && element_manager()) {
    element_manager_->EraseGlobalBindElementId(global_bind_event_map(),
                                               impl_id());
    element_manager()->NotifyElementDestroy(this);
    DestroyPlatformNode();
    element_manager()->DestroyLayoutNode(impl_id());
    node_manager_->Erase(id_);
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

const FiberElement::InheritedProperty &FiberElement::GetInheritedProperty() {
  return inherited_property_;
}

const FiberElement::InheritedProperty &
FiberElement::GetParentInheritedProperty() {
  // If in a parallel flush process or if the parent is null, return
  // kInheritedProperty indicating that it is not necessary to consider the
  // inheritance logic at this time.
  static base::NoDestructor<InheritedProperty> kInheritedProperty{};
  if (this->is_parallel_flush()) {
    return *kInheritedProperty;
  }

  FiberElement *real_parent = static_cast<FiberElement *>(parent());
  if (real_parent == nullptr) {
    return *kInheritedProperty;
  }

  return real_parent->GetInheritedProperty();
}

bool FiberElement::NeedFastFlushPath(
    const std::pair<CSSPropertyID, tasm::CSSValue> &style) {
  return style.second.IsEmpty() || LayoutNode::IsLayoutOnly(style.first) ||
         LayoutNode::IsLayoutWanted(style.first) ||
         starlight::CSSStyleUtils::IsLayoutRelatedTransform(style) ||
         style.first == kPropertyIDColor || style.first == kPropertyIDFilter;
}

void FiberElement::SetKeyframesByNamesInner(
    std::unique_ptr<PropBundle> bundle) {
  painting_context()->SetKeyframes(std::move(bundle));
}

void FiberElement::ResolveParentComponentElement() const {
  TRACE_EVENT(LYNX_TRACE_CATEGORY,
              "FiberElement::ResolveParentComponentElement");
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
      if (!fragment_ && css_style_sheet_manager_) {
        fragment_ =
            css_style_sheet_manager_->GetCSSStyleSheetForComponent(css_id_);
      }
      style_sheet_ = std::make_shared<CSSFragmentDecorator>(fragment_);
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
  return CSSProperty::GetTotalParsedStyleCountFromMap(
      current_raw_inline_styles_);
}

void FiberElement::MergeInlineStyles(StyleMap &new_styles) {
  // Styles stored by full_raw_inline_style_ had already been parsed to
  // current_raw_inline_styles_. So we only handle current_raw_inline_styles_
  // here.
  auto &configs = element_manager_->GetCSSParserConfigs();
  for (const auto &style : current_raw_inline_styles_) {
    UnitHandler::Process(style.first, style.second, new_styles, configs);
  }
}

void FiberElement::ProcessFullRawInlineStyle() {
  // If self has raw inline styles, parse to current_raw_inline_styles_ but do
  // not process to final style map. Inline styles will be merged finally by
  // MergeInlineStyles.
  if (!full_raw_inline_style_.IsEmpty()) {
    ParseRawInlineStyles(full_raw_inline_style_, nullptr);
    full_raw_inline_style_.SetNil();
  }
}

bool FiberElement::WillResolveStyle(StyleMap &merged_styles) {
  ProcessFullRawInlineStyle();
  return true;
}

void FiberElement::UpdateInheritedProperty() {
  inherited_property_.inherited_styles_ = &inherited_styles_;
  inherited_property_.reset_inherited_ids_ = &reset_inherited_ids_;
}

void FiberElement::AsyncResolveProperty() {
  if ((dirty_ & ~kDirtyTree) != 0) {
    TRACE_EVENT(LYNX_TRACE_CATEGORY, "FiberElement::AsyncResolveProperty");
    UpdateResolveStatus(AsyncResolveStatus::kPrepareRequested);
    if (this->IsAttached()) {
      AsyncPostResolveTaskToThreadPool();
    }
  }
}

void FiberElement::AsyncPostResolveTaskToThreadPool() {
  if ((dirty_ & ~kDirtyTree) != 0) {
    // ResolveParentComponentElement needs to be done on Engine Thread for
    // node_manager may resize when creating element
    ResolveParentComponentElement();
    UpdateResolveStatus(AsyncResolveStatus::kPrepareTriggered);
    element_manager()->GetTasmWorkerTaskRunner()->PostTask([this]() mutable {
      UpdateResolveStatus(AsyncResolveStatus::kPreparing);
      if (parent()) {
        parent()->EnsureTagInfo();
      }
      PostResolveTaskToThreadPool(false, element_manager()->ParallelTasks());
    });
  }
}

void FiberElement::ReplaceElements(
    const std::deque<fml::RefPtr<FiberElement>> &inserted,
    const std::deque<fml::RefPtr<FiberElement>> &removed, FiberElement *ref) {
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
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "FiberElement::InsertNode");
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
    scoped_virtual_children_.push_back(child);
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
                                    nullptr, child->is_fixed_);
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
  EXEC_EXPR_FOR_INSPECTOR(if (element_manager() != nullptr &&
                              element_manager()->IsDomTreeEnabled()) {
    element_manager()->RunDevToolFunction(
        lynx::devtool::DevToolFunction::InitStyleRoot, std::make_tuple(this));
  });
}

void FiberElement::RemovedFrom(FiberElement *insertion_point) {
  // We need to handle the intergenerational node which has zIndex or fixed,
  // they may be inserted to difference parent in UI/layout tree instead of dom
  // parent If the removed node's parent is the insertion_point, no need to do
  // any special action
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
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "FiberElement::SetClass");

  data_model_->SetClass(clazz);
  MarkStyleDirty(NeedForceClassChangeTransmit());
}

void FiberElement::SetClasses(ClassList &&classes) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "FiberElement::SetClasses");
  data_model_->SetClasses(std::move(classes));
  MarkStyleDirty(NeedForceClassChangeTransmit());

  // clear ssr parsed style
  if (has_extreme_parsed_styles_) {
    extreme_parsed_styles_.clear();
    has_extreme_parsed_styles_ = false;
  }
}

void FiberElement::RemoveAllClass() {
  data_model_->RemoveAllClass();
  MarkStyleDirty(NeedForceClassChangeTransmit());
}

void FiberElement::SetStyle(CSSPropertyID id, const lepus::Value &value) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "FiberElement::SetStyle");

  // When the `SetStyle` API is called, the `SetRawInlineStyles` API might
  // already have been invoked. In this case, it is necessary to call
  // `ProcessFullRawInlineStyle` first to ensure that `full_raw_inline_style_`
  // is set into `current_raw_inline_styles_`. Otherwise, `SetRawInlineStyles`
  // might override the `SetStyle` call, leading to unexpected behavior.
  ProcessFullRawInlineStyle();

  if (!value.IsEmpty()) {
    current_raw_inline_styles_.insert_or_assign(id, value);
  } else {
    current_raw_inline_styles_.erase(id);
  }

  MarkDirty(kDirtyStyle);

  if (has_extreme_parsed_styles_ && !only_selector_extreme_parsed_styles_) {
    has_extreme_parsed_styles_ = false;
    extreme_parsed_styles_.clear();
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
  const auto &inherited_property = GetParentInheritedProperty();
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

const lepus::Value &FiberElement::GetRawInlineStyles() {
  return full_raw_inline_style_;
}

const RawLepusStyleMap &FiberElement::GetCurrentRawInlineStyles() const {
  return current_raw_inline_styles_;
}

void FiberElement::SetRawInlineStyles(const lepus::Value &value) {
  full_raw_inline_style_ = value;
  MarkDirty(kDirtyStyle);
}

void FiberElement::RemoveAllInlineStyles() {
  // Only exec the following expr when ENABLE_INSPECTOR, such that devtool can
  // get element's inline style.
  EXEC_EXPR_FOR_INSPECTOR({
    if (element_manager_->IsDomTreeEnabled()) {
      for (const auto &pair : current_raw_inline_styles_) {
        const static base::String kNull;
        data_model()->SetInlineStyle(pair.first, kNull,
                                     element_manager_->GetCSSParserConfigs());
      }
    }
  });

  full_raw_inline_style_.SetNil();
  current_raw_inline_styles_.clear();
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
    default:
      key_is_legal = false;
      break;
  }
  if (key_is_legal) {
    builtin_attr_map_.try_emplace(static_cast<uint32_t>(key), value);
  }
}

void FiberElement::SetAttribute(const base::String &key,
                                const lepus::Value &value,
                                bool need_update_data_model) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "FiberElement::SetAttribute");

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
    reset_attr_vec_.emplace_back(key);
    if (need_update_data_model) {
      data_model_->RemoveAttribute(key);
    }
  }
  MarkDirty(kDirtyAttr);
}

void FiberElement::SetIdSelector(const base::String &idSelector) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "FiberElement::SetIdSelector");
  CheckHasInvalidationForId(data_model_->idSelector().str(), idSelector.str());

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
    pending_updated_direction_related_styles_[css_id] = {
        value, direction_mapping.is_logic_};
  }
}

void FiberElement::HandleKeyframePropsChange(bool need_animation_props) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "FiberElement::HandleKeyframePropsChange",
              [this](lynx::perfetto::EventContext ctx) {
                UpdateTraceDebugInfo(ctx.event());
              });
  if (!enable_new_animator()) {
    ResolveAndFlushKeyframes();
  } else {
    SetDataToNativeKeyframeAnimator();
    if (need_animation_props) {
      ResolveAndFlushKeyframes();
    }
  }
  has_keyframe_props_changed_ = false;
}

void FiberElement::HandleDelayTask(base::MoveOnlyClosure<void> operation) {
  if (this->parallel_flush_) {
    parallel_reduce_tasks_.emplace_back(std::move(operation));
  } else {
    operation();
  }
}

ParallelFlushReturn FiberElement::PrepareForCreateOrUpdate() {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "FiberElement::PrepareForCreateOrUpdate",
              [this](lynx::perfetto::EventContext ctx) {
                UpdateTraceDebugInfo(ctx.event());
              });
  bool need_update = false;

  // Need process attributes first.
  if (dirty_ & kDirtyAttr) {
    TRACE_EVENT(LYNX_TRACE_CATEGORY, "FiberElement::HandleAttr",
                [this](lynx::perfetto::EventContext ctx) {
                  UpdateTraceDebugInfo(ctx.event());
                });
    for (const auto &attr : updated_attr_map_) {
      SetAttributeInternal(attr.first, attr.second);
      need_update = true;
    }
    for (const auto &attr : reset_attr_vec_) {
      ResetAttribute(attr);
      need_update = true;
    }
    if (updated_attr_map_.size() > 0) {
      PropsUpdateFinish();
    }

    updated_attr_map_.clear();
    reset_attr_vec_.clear();
    dirty_ &= ~kDirtyAttr;
  }

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

  if (dirty_ & kDirtyStyle) {
    TRACE_EVENT(LYNX_TRACE_CATEGORY, "FiberElement::HandleStyle",
                [this](lynx::perfetto::EventContext ctx) {
                  UpdateTraceDebugInfo(ctx.event());
                });

    RefreshStyle(parsed_styles, reset_style_ids,
                 force_use_current_parsed_style_map);

    dirty_ &= ~kDirtyStyle;
  } else if (dirty_ & kDirtyRefreshCSSVariables) {
    TRACE_EVENT(LYNX_TRACE_CATEGORY, "FiberElement::HandleStyle",
                [this](lynx::perfetto::EventContext ctx) {
                  UpdateTraceDebugInfo(ctx.event());
                });
    RefreshStyle(parsed_styles, reset_style_ids);

    dirty_ &= ~kDirtyRefreshCSSVariables;
  }

  if (!this->parallel_flush_ && IsCSSInheritanceEnabled()) {
    TRACE_EVENT(LYNX_TRACE_CATEGORY, "FiberElement::HandlePropagateInherited",
                [this](lynx::perfetto::EventContext ctx) {
                  UpdateTraceDebugInfo(ctx.event());
                });

    const auto &inherited_property = GetParentInheritedProperty();
    // process inherit related
    // quick check if any id in reset_style_ids is in parent inherited styles
    if (inherited_property.inherited_styles_) {
      for (auto it = reset_style_ids.begin(); it != reset_style_ids.end();) {
        // do not reset style if it's parent inherited_styles contains it
        const auto &parent_inherited_styles =
            *(inherited_property.inherited_styles_);
        if (parent_inherited_styles.find(*it) !=
            parent_inherited_styles.end()) {
          // we need to mark flag to do self recalculation for inherited styles,
          // if the style is updated instead of reset
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
      if (inherited_property.reset_inherited_ids_) {
        for (const auto reset_id : *(inherited_property.reset_inherited_ids_)) {
          auto it = parsed_styles_map_.find(reset_id);
          if (it == parsed_styles_map_.end()) {
            if (updated_inherited_styles_.find(reset_id) !=
                updated_inherited_styles_.end()) {
              reset_style_ids.push_back(reset_id);
            }
          }
        }
      }

      // #2.parent inherited style changed
      //  merge the inherited styles, but they have lower priority
      if (inherited_property.inherited_styles_) {
        updated_inherited_styles_.clear();
        updated_inherited_styles_.reserve(
            inherited_property.inherited_styles_->size());
        for (auto &pair : *(inherited_property.inherited_styles_)) {
          auto it = parsed_styles_map_.find(pair.first);
          if (it == parsed_styles_map_.end()) {
            updated_inherited_styles_.insert_or_assign(pair.first, pair.second);
            need_update = true;
          }
        }
      }
    }

    // kDirtyPropagateInherited flag is expected to be consumed in above logic
    // Special case: When PrepareForCreateOrUpdate function is executing
    // parallel flush pass, and CSS Inheritance is enabled, CSS Styles inherited
    // from parent element cannot be fully resolved in parallel flush pass, thus
    // only in such scenario, kDirtyPropagateInherited flag need to preserved to
    // force refresh in next pass
    dirty_ &= ~kDirtyPropagateInherited;
  }

  // Process reset before update styles.

  // If the new animator is activated and this element has been created before,
  // we need to reset the transition styles in advance. Additionally, the
  // transition manager should verify each property to decide whether to
  // intercept the reset. Therefore, we break down the operations related to the
  // transition reset process into three steps:
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
  auto update_map =
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
    // need to record some necessary styles which New Animator transition needs,
    // and it needs to be saved before rtl converted logic.
    ResetElementPreviousStyle(id);
    ResetStyleInternal(id);
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
    // #5.3 Update element text_align depending on whether direction is changed
    ResetTextAlign(update_map, direction_changed);
  }

  // process direction: rtl/lynx-rtl firstly
  // FIXME(linxs): maybe can put setFontSize here ?
  if (IsDirectionChangedEnabled()) {
    TRACE_EVENT(LYNX_TRACE_CATEGORY, "FiberElement::HandleDirectionChanged",
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
        auto updated_inherited_map_it =
            updated_inherited_map.find(CSSPropertyID::kPropertyIDDirection);
        if (updated_inherited_map_it != updated_inherited_map.end()) {
          return std::make_pair(
              updated_inherited_map_it->second,
              static_cast<starlight::DirectionType>(
                  updated_inherited_map_it->second.GetValue().Number()));
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
        for (const auto &css_pair : updated_inherited_styles_) {
          ResetDirectionAwareProperty(css_pair.first, css_pair.second);
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
    } while (0);
  }

  bool root_font_size_changed =
      GetCurrentRootFontSize() != GetRecordedRootFontSize();
  if (root_font_size_changed) {
    SetFontSizeForAllElement(GetFontSize(), GetCurrentRootFontSize());
    UpdateLayoutNodeFontSize(GetFontSize(), GetCurrentRootFontSize());
  }

  // set updated Styles to element in the end
  if (!update_map.empty() || !updated_inherited_styles_.empty() ||
      !styles_from_attributes_.empty()) {
    TRACE_EVENT(LYNX_TRACE_CATEGORY, "FiberElement::HandleSetStyle",
                [this](lynx::perfetto::EventContext ctx) {
                  UpdateTraceDebugInfo(ctx.event());
                });
    // if kDirtyPropagateInherited, need to delay to SetStyle in inherit process
    ConsumeStyle(update_map, IsCSSInheritanceEnabled()
                                 ? &updated_inherited_styles_
                                 : nullptr);
    need_update = true;
  }

  // direction change: we always handle direction change after all styles
  // resolved
  if (!pending_updated_direction_related_styles_.empty()) {
    for (const auto &style_pair : pending_updated_direction_related_styles_) {
      TryDoDirectionRelatedCSSChange(style_pair.first, style_pair.second.first,
                                     style_pair.second.second);
    }
    pending_updated_direction_related_styles_.clear();
  }

  // Handle font size change
  if (dirty_ & kDirtyFontSize) {
    TRACE_EVENT(LYNX_TRACE_CATEGORY, "FiberElement::HandleFontSizeChange",
                [this](lynx::perfetto::EventContext ctx) {
                  UpdateTraceDebugInfo(ctx.event());
                });
    do {
      // If `dirty_ & kDirtyCreated`, the `parsed_styles_map_` has already been
      // fully consumed, so there is no possibility of `update_map` being
      // different from `parsed_styles_map_`. Therefore, skip this logic.
      if (dirty_ & kDirtyCreated) {
        break;
      }

      // If `dynamic_style_flags_ & DynamicCSSStylesManager::kUpdateEm == false`
      // and `root_font_size_changed` and `dynamic_style_flags_ &
      // DynamicCSSStylesManager::kUpdateRem == false`, it indicates that the
      // current `parsed_styles_map_` does not contain any font size-sensitive
      // styles, and thus this part of the processing logic can be skipped.
      if (!(dynamic_style_flags_ & DynamicCSSStylesManager::kUpdateEm) &&
          !(root_font_size_changed &&
            dynamic_style_flags_ & DynamicCSSStylesManager::kUpdateRem)) {
        break;
      }

      // We need to reset the styles for the following style pairs because they
      // are possibly font size-sensitive:
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
        SetFontSize();
        need_update = true;
      }

      for (const auto &style : parsed_styles_map_) {
        if (style.first != CSSPropertyID::kPropertyIDFontSize &&
            should_update_em_rem_style(style, root_font_size_changed) &&
            update_map.find(style.first) == update_map.end()) {
          SetStyleInternal(style.first, style.second);
          need_update = true;
        }
      }
    } while (0);
    dirty_ &= ~kDirtyFontSize;
  }

  // Report when enableNewAnimator is the default value.
  if ((has_transition_props_changed_ || has_keyframe_props_changed_) &&
      !enable_new_animator()) {
    report::GlobalFeatureCounter::Count(
        report::LynxFeature::CPP_ENABLE_NEW_ANIMATOR_DEFAULT,
        element_manager()->GetInstanceId());
  }
  // keyframe props
  bool need_animation_props = painting_context()->NeedAnimationProps();
  if (has_keyframe_props_changed_) {
    HandleDelayTask([this, need_animation_props]() {
      HandleKeyframePropsChange(need_animation_props);
    });
    if (!enable_new_animator() ||
        (enable_new_animator() && need_animation_props)) {
      PushToBundle(kPropertyIDAnimation);
    }
    need_update = true;
  }

  if (has_transition_props_changed_) {
    TRACE_EVENT(LYNX_TRACE_CATEGORY, "FiberElement::HandleTransitionProps",
                [this](lynx::perfetto::EventContext ctx) {
                  UpdateTraceDebugInfo(ctx.event());
                });
    if (!enable_new_animator()) {
      PushToBundle(kPropertyIDTransition);
    } else {
      SetDataToNativeTransitionAnimator();
      if (need_animation_props) {
        PushToBundle(kPropertyIDTransition);
      }
    }
    has_transition_props_changed_ = false;
    need_update = true;
  }

  // If above props and styles need to be updated, this patch needs trigger
  // layout.
  if (need_update || dirty_ & kDirtyCreated || dirty_ & kDirtyForceUpdate) {
    RequestLayout();
  }

  // events
  if (dirty_ & kDirtyEvent) {
    TRACE_EVENT(LYNX_TRACE_CATEGORY, "FiberElement::HandleEvents",
                [this](lynx::perfetto::EventContext ctx) {
                  UpdateTraceDebugInfo(ctx.event());
                });
    // OPTME(linxs): pass event diff result later?
    element_manager_->ResolveEvents(data_model_.get(), this);
    dirty_ &= ~kDirtyEvent;
  }

  // gestures
  if (dirty_ & kDirtyGesture) {
    TRACE_EVENT(LYNX_TRACE_CATEGORY, "FiberElement::HandleGestures",
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
    TRACE_EVENT(LYNX_TRACE_CATEGORY, "FiberElement::HandleDataset");
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

  // actions
  if (dirty_ & kDirtyCreated) {
    TRACE_EVENT(LYNX_TRACE_CATEGORY, "FiberElement::HandleCreate",
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
    HandleDelayTask([this]() { element_container()->StyleChanged(); });
  }

  UpdateLayoutNodeByBundle();

  dirty_ &= ~kDirtyForceUpdate;
  ResetPropBundle();

  // Remaining Layout Task should be returned to be executed in threaded flush
  // or sync resolving(i.e. PageElement) scenario
  if (this->parallel_flush_ ||
      this->resolve_status_ == AsyncResolveStatus::kSyncResolving) {
    this->parallel_flush_ = false;
    this->UpdateResolveStatus(AsyncResolveStatus::kResolved);
    return [this]() {
      TRACE_EVENT(LYNX_TRACE_CATEGORY,
                  "FiberElement::HandleParallelReduceTasks");
      for (const auto &task : parallel_reduce_tasks_) {
        task();
      }
      parallel_reduce_tasks_.clear();
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

  VerifyKeyframePropsChangedHandling();
  UpdateInheritedProperty();

  return []() {};
}

void FiberElement::TriggerElementUpdate() {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "FiberElement::TriggerElementUpdate",
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
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "FiberElement::FlushActionsAsRoot",
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
}

void FiberElement::FlushSelf() {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "FiberElement::FlushSelf",
              [this](lynx::perfetto::EventContext ctx) {
                UpdateTraceDebugInfo(ctx.event());
              });

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
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "FiberElement::FlushActions",
              [this](lynx::perfetto::EventContext ctx) {
                UpdateTraceDebugInfo(ctx.event());
              });
  if (!flush_required_) {
    return;
  }

  // Step I: Handle Action for current element: Prepare&HandleFixedChange
  FlushSelf();

  // currently, page's parallel_flush_ is always false, so that there is no
  // chance to mark kDirtyPropagateInherited. We have
  // to force UpdateInheritedProperty for page
  if (is_page() && IsCSSInheritanceEnabled()) {
    UpdateInheritedProperty();
  }

  // Step II: process insert or remove related actions
  PrepareAndGenerateChildrenActions();

  // Throw exception on purpose to catch logic flaw
  DCHECK(dirty_ == 0);

  for (auto *invalidation_set : invalidation_lists_.descendants) {
    InvalidateChildren(invalidation_set);
  }
  invalidation_lists_.descendants.clear();

  // Step III: recursively call FlushActions for each child
  for (const auto &child : scoped_children_) {
    if (inherited_property_.children_propagate_inherited_styles_flag_) {
      child->MarkDirtyLite(kDirtyPropagateInherited);
    }
    child->FlushActions();
  }
  // below flags should be delayed until children flushed
  inherited_property_.children_propagate_inherited_styles_flag_ = false;
  reset_inherited_ids_.clear();

  flush_required_ = false;
}

void FiberElement::OnParallelFlushAsRoot(PerfStatistic &stats) {
  stats.enable_report_stats_ =
      element_manager()->GetEnableReportThreadedElementFlushStatistic();
  stats.total_processing_start_ = base::CurrentTimeMicroseconds();
}

void FiberElement::ParallelFlushAsRoot() {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "FiberElement::ParallelFlushAsRoot");
  if (!element_manager()->GetEnableParallelElement()) {
    return;
  }
  {
    TRACE_EVENT(LYNX_TRACE_CATEGORY, "TasmTaskRunner::WaitForCompletion");
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
    TRACE_EVENT(LYNX_TRACE_CATEGORY, "FiberElement::ConsumeParallelTask");
    if (task_queue.front().get()->GetFuture().wait_for(std::chrono::seconds(
            task_wait_timeout_)) == std::future_status::ready) {
      TRACE_EVENT(LYNX_TRACE_CATEGORY, "FiberElement::ConsumeLeftIter");
      task_queue.front().get()->GetFuture().get()();
      task_queue.pop_front();
    } else if (task_queue.back().get()->Run()) {
      TRACE_EVENT(LYNX_TRACE_CATEGORY, "FiberElement::ConsumeRightIter");
      task_queue.back().get()->GetFuture().get()();
      task_queue.pop_back();
      ++perf_stats.engine_thread_task_count_;
    } else {
      TRACE_EVENT(LYNX_TRACE_CATEGORY, "FiberElement:WaitLeftIter");
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

  std::promise<ParallelFlushReturn> promise;
  std::future<ParallelFlushReturn> future = promise.get_future();

  auto task_info_ptr = fml::MakeRefCounted<base::OnceTask<ParallelFlushReturn>>(
      [target = this, promise = std::move(promise)]() mutable {
        TRACE_EVENT(LYNX_TRACE_CATEGORY,
                    "FiberElement::PrepareForCreateOrUpdateAsync");
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
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "FiberElement::ParallelFlushRecursively");
  if (!flush_required_) {
    return;
  }

  if (!IsAsyncResolveResolving() && ((dirty_ & ~kDirtyTree) != 0)) {
    PostResolveTaskToThreadPool(true, element_manager()->ParallelTasks());
  }

  for (const auto &child : scoped_children_) {
    TRACE_EVENT(LYNX_TRACE_CATEGORY,
                "FiberElement::ChildrenPrepareForCreateOrUpdate",
                [this](lynx::perfetto::EventContext ctx) {
                  UpdateTraceDebugInfo(ctx.event());
                });
    child->ParallelFlushRecursively();
  }
}

void FiberElement::PrepareChildren() {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "FiberElement::PrepareChildren",
              [this](lynx::perfetto::EventContext ctx) {
                UpdateTraceDebugInfo(ctx.event());
              });
  for (const auto &child : scoped_children_) {
    if (inherited_property_.children_propagate_inherited_styles_flag_) {
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
    if (inherited_property_.children_propagate_inherited_styles_flag_) {
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
              "FiberElement::PrepareAndGenerateChildrenActions",
              [this](lynx::perfetto::EventContext ctx) {
                UpdateTraceDebugInfo(ctx.event());
              });
  // When need propagate inherited styles or tree structure is updated, prepare
  // children
  if (dirty_ & kDirtyTree ||
      inherited_property_.children_propagate_inherited_styles_flag_) {
    PrepareChildren();
  }
  // process insert or remove related actions
  if (dirty_ & kDirtyTree) {
    TRACE_EVENT(LYNX_TRACE_CATEGORY, "FiberElement::HandleChildrenAction",
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
            InsertFixedElement(child.get(), nullptr);
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
            InsertFixedElement(param.child_.get(), param.ref_node_);
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

  action_param_list_.clear();

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
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "FiberElement::HandleInsertChildAction",
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
    while (ref_node != nullptr &&
           (ref_node->is_fixed() || ref_node->fixed_changed_)) {
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
    HandleFlushActionsLayoutTask([this, child, ref_node]() mutable {
      InsertLayoutNode(child, ref_node);
    });
  }

  HandleContainerInsertion(parent, child, ref_node);
}

void FiberElement::HandleRemoveChildAction(FiberElement *child) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "FiberElement::HandleRemoveChildAction",
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
    HandleFlushActionsLayoutTask(
        [this, child]() mutable { RemoveLayoutNode(child); });
  }

  child->element_container()->RemoveSelf(false);
}

void FiberElement::HandleContainerInsertion(FiberElement *parent,
                                            FiberElement *child,
                                            FiberElement *ref_node) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "FiberElement::HandleContainerInsertion",
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
  for (size_t i = 0; i < scoped_virtual_children_.size(); ++i) {
    auto *virtual_child =
        static_cast<FiberElement *>(scoped_virtual_children_[i].get());
    if (virtual_child->parent_ == this) {
      virtual_child->parent_ = nullptr;
    }
  }
  // clear element's children only in radon or radon compatible mode.
  scoped_children_.clear();
  scoped_virtual_children_.clear();
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
                                StyleMap *inherit_styles) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "FiberElement::ConsumeStyle",
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
    const StyleMap &styles, StyleMap *inherit_styles,
    std::function<bool(CSSPropertyID, const tasm::CSSValue &)> should_skip) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "FiberElement::ConsumeStyle",
              [this](lynx::perfetto::EventContext ctx) {
                UpdateTraceDebugInfo(ctx.event());
              });
  if (styles.empty() &&
      (inherit_styles == nullptr || inherit_styles->empty())) {
    return;
  }

  // Handle font-size first. Other css may use this to calc rem or em.
  SetFontSize();

  inherited_styles_.reserve(styles.size() + updated_inherited_styles_.size());

  auto consume_func = [this, should_skip = std::move(should_skip)](
                          const StyleMap &styles, bool process_inherit) {
    for (const auto &style : styles) {
      bool is_inherit_style = false;
      if (!is_raw_text() && IsInheritable(style.first)) {
        is_inherit_style = true;
        auto iter = inherited_styles_.find(style.first);
        if (iter == inherited_styles_.end() || iter->second != style.second) {
          // save the css value to inherited styles map
          inherited_styles_.insert_or_assign(style.first, style.second);
          inherited_property_.children_propagate_inherited_styles_flag_ = true;
        }
      }

      if (style.first == CSSPropertyID::kPropertyIDDirection ||
          style.first == CSSPropertyID::kPropertyIDFontSize) {
        // direction has been resolved before
        continue;
      }

      bool complex_inherit_style =
          process_inherit && is_inherit_style &&
          DynamicCSSStylesManager::IsPropertyComplexInheritable(style.first,
                                                                style.second);

      if (auto parent_computed_css = GetParentComputedCSSStyle();
          complex_inherit_style && parent_computed_css != nullptr) {
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

void FiberElement::CacheStyleFromAttributes(CSSPropertyID id,
                                            CSSValue &&value) {
  styles_from_attributes_.insert_or_assign(id, std::move(value));
}

void FiberElement::CacheStyleFromAttributes(CSSPropertyID id,
                                            const lepus::Value &value) {
  UnitHandler::Process(id, value, styles_from_attributes_,
                       element_manager()->GetCSSParserConfigs());
}

void FiberElement::DidConsumeStyle() {
  if (styles_from_attributes_.empty()) {
    return;
  }

  ConsumeStyleInternal(styles_from_attributes_, nullptr,
                       [](auto id, const auto &value) {
                         // Do not skip any style here.
                         return false;
                       });
  styles_from_attributes_.clear();
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
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "FiberElement::AddDataset");

  data_model_->SetDataSet(key, value);
  MarkDirty(kDirtyDataset);
}

void FiberElement::SetDataset(const lepus::Value &data_set) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "FiberElement::SetDataset");

  data_model_->SetDataSet(data_set);
  MarkDirty(kDirtyDataset);
}

void FiberElement::SetJSEventHandler(const base::String &name,
                                     const base::String &type,
                                     const base::String &callback) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "FiberElement::SetJSEventHandler");

  data_model_->SetStaticEvent(type, name, callback);
  MarkDirty(kDirtyEvent);
}

void FiberElement::SetLepusEventHandler(const base::String &name,
                                        const base::String &type,
                                        const lepus::Value &script,
                                        const lepus::Value &callback) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "FiberElement::SetLepusEventHandler");

  data_model_->SetLepusEvent(type, name, script, callback);
  MarkDirty(kDirtyEvent);
}

void FiberElement::SetWorkletEventHandler(const base::String &name,
                                          const base::String &type,
                                          const lepus::Value &worklet_info,
                                          lepus::Context *ctx) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "FiberElement::SetWorkletEventHandler");

  data_model_->SetWorkletEvent(type, name, worklet_info, ctx);
  MarkDirty(kDirtyEvent);
}

void FiberElement::SetNativeProps(const lepus::Value &native_props,
                                  PipelineOptions &pipeline_options) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "FiberElement::SetNativeProps",
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
    element_manager()->OnPatchFinish(pipeline_options, this);
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
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "FiberElement::SetParsedStyles");

  constexpr const static char kOnlySelector[] = "selectorParsedStyles";
  const auto &only_selector_prop =
      config.GetProperty(BASE_STATIC_STRING(kOnlySelector));
  if (only_selector_prop.IsBool()) {
    only_selector_extreme_parsed_styles_ = only_selector_prop.Bool();
  }

  has_extreme_parsed_styles_ = true;
  extreme_parsed_styles_ = parsed_styles.first;
  data_model()->set_css_variables_map(parsed_styles.second);
  MarkDirty(kDirtyStyle);
}

void FiberElement::SetParsedStyles(StyleMap &&parsed_styles,
                                   CSSVariableMap &&css_var) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "FiberElement::SetParsedStyles");
  has_extreme_parsed_styles_ = true;
  only_selector_extreme_parsed_styles_ = false;
  extreme_parsed_styles_ = std::move(parsed_styles);
  data_model()->set_css_variables_map(std::move(css_var));
  MarkDirty(kDirtyStyle);
}

void FiberElement::AddConfig(const base::String &key,
                             const lepus::Value &value) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "FiberElement::AddConfig");
  if (config_.IsTable() && config_.Table()->IsConst()) {
    config_ = lepus::Value::ShallowCopy(config_);
  }
  config_.SetProperty(key, value);
}

void FiberElement::SetConfig(const lepus::Value &config) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "FiberElement::SetConfig");

  // To improve performance, ensure that the isObject check is performed before
  // calling SetConfig, and the check and LOGW in SetConfig are no longer
  // performed.
  config_ = config;
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
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "FiberElement::FlushProps",
              [this](lynx::perfetto::EventContext ctx) {
                UpdateTraceDebugInfo(ctx.event());
              });

  if (is_scroll_view() || is_list()) {
    UpdateLayoutNodeAttribute(starlight::LayoutAttribute::kScroll,
                              lepus::Value(true));
  }

  // Update The root if needed
  if (!has_painting_node_) {
    TRACE_EVENT(LYNX_TRACE_CATEGORY, "Catalyzer::FlushProps::NoPaintingNode",
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
    platform_is_flatten = painting_context()->IsFlatten(std::move(func));
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

void FiberElement::EnsureLayoutBundle() {
  if (layout_bundle_ == nullptr) {
    layout_bundle_ = std::make_unique<LayoutBundle>();
  }
}

void FiberElement::UpdateTagToLayoutBundle() {
  EnsureLayoutBundle();
  layout_bundle_->tag = tag_;
}

void FiberElement::InitLayoutBundle() {
  EnsureLayoutBundle();
  layout_bundle_->tag = tag_;
  layout_bundle_->is_create_bundle = true;
}

void FiberElement::MarkAsLayoutRoot() {
  EnsureLayoutBundle();
  layout_bundle_->is_root = true;
}

void FiberElement::MarkLayoutDirty() {
  EnsureLayoutBundle();
  layout_bundle_->is_dirty = true;
}

void FiberElement::AttachLayoutNode(const std::shared_ptr<PropBundle> &props) {
  EnsureLayoutBundle();
  layout_bundle_->shadownode_prop_bundle = props;
  layout_bundle_->allow_inline = allow_layoutnode_inline_;
}

void FiberElement::UpdateLayoutNodeProps(
    const std::shared_ptr<PropBundle> &props) {
  EnsureLayoutBundle();
  layout_bundle_->update_prop_bundles.emplace_back(props);
}

void FiberElement::UpdateLayoutNodeStyle(CSSPropertyID css_id,
                                         const tasm::CSSValue &value) {
  EnsureLayoutBundle();
  layout_bundle_->styles.emplace_back(css_id, value);
}

void FiberElement::ResetLayoutNodeStyle(tasm::CSSPropertyID css_id) {
  EnsureLayoutBundle();
  layout_bundle_->reset_styles.emplace_back(css_id);
}

void FiberElement::UpdateLayoutNodeFontSize(double cur_node_font_size,
                                            double root_node_font_size) {
  EnsureLayoutBundle();
  layout_bundle_->font_scale = element_manager_->GetLynxEnvConfig().FontScale();
  layout_bundle_->cur_node_font_size = cur_node_font_size;
  layout_bundle_->root_node_font_size = root_node_font_size;
}

void FiberElement::UpdateLayoutNodeAttribute(starlight::LayoutAttribute key,
                                             const lepus::Value &value) {
  EnsureLayoutBundle();
  layout_bundle_->attrs.emplace_back(std::make_pair(key, value));
}

void FiberElement::UpdateLayoutNodeByBundle() {
  if (layout_bundle_ == nullptr) {
    return;
  }
  HandleLayoutTask([element_manager = element_manager(), id = impl_id(),
                    layout_bundle = std::move(layout_bundle_)]() mutable {
    element_manager->UpdateLayoutNodeByBundle(id, std::move(layout_bundle));
  });
  layout_bundle_ = nullptr;
}

void FiberElement::CheckHasInlineContainer(Element *parent) {
  EnsureLayoutBundle();
  allow_layoutnode_inline_ = parent->IsShadowNodeCustom();
}

void FiberElement::HandleLayoutTask(base::MoveOnlyClosure<void> operation) {
  // Layout Task should be stored to be executed in threaded flush or sync
  // resolving(i.e. PageElement) scenario
  if (element_manager()->GetParallelWithSyncLayout() &&
      (this->parallel_flush_ ||
       this->resolve_status_ == AsyncResolveStatus::kSyncResolving)) {
    parallel_reduce_tasks_.emplace_back(std::move(operation));
  } else {
    operation();
  }
}

void FiberElement::RequestLayout() {
  HandleDelayTask(
      [manager = element_manager()]() { manager->SetNeedsLayout(); });
}

void FiberElement::RequestNextFrame() {
  HandleDelayTask([this]() { element_manager()->RequestNextFrame(this); });
}

void FiberElement::UpdateFiberElement() {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "FiberElement::UpdateFiberElement",
              [this](lynx::perfetto::EventContext ctx) {
                UpdateTraceDebugInfo(ctx.event());
              });
  if (!is_layout_only_) {
    TRACE_EVENT(LYNX_TRACE_CATEGORY, "FiberElement::UpdatePaintingNode",
                [this](lynx::perfetto::EventContext ctx) {
                  UpdateTraceDebugInfo(ctx.event());
                });
    painting_context()->UpdatePaintingNode(id_, TendToFlatten(), prop_bundle_);
  } else if (!CanBeLayoutOnly()) {
    TRACE_EVENT(LYNX_TRACE_CATEGORY, "FiberElement::TransitionToNativeView",
                [this](lynx::perfetto::EventContext ctx) {
                  UpdateTraceDebugInfo(ctx.event());
                });
    // Is layout only and can not be layout only
    TransitionToNativeView();
  }
}

bool FiberElement::IsRelatedCSSVariableUpdated(
    AttributeHolder *holder, const lepus::Value changing_css_variables) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "FiberElement::IsRelatedCSSVariableUpdated",
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

void FiberElement::UpdateCSSVariable(const lepus::Value &css_variable_updated) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "FiberElement::UpdateCSSVariable",
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
  PipelineOptions option;
  element_manager()->OnPatchFinish(option, this);
}

void FiberElement::ResolveStyleValue(CSSPropertyID id,
                                     const tasm::CSSValue &value,
                                     bool force_update) {
  if (computed_css_style()->SetValue(id, value)) {
    // The properties of transition and keyframe no need to be pushed to bundle
    // separately here. Those properties will be pushed to bundle together
    // later.
    if (!(CheckTransitionProps(id) || CheckKeyframeProps(id))) {
      PushToBundle(id);
    }
  }
}

void FiberElement::SetFontSize() {
  std::optional<float> result;
  if (auto it = parsed_styles_map_.find(CSSPropertyID::kPropertyIDFontSize);
      it != parsed_styles_map_.end()) {
    CheckDynamicUnit(CSSPropertyID::kPropertyIDFontSize, it->second, false);
    // Take care: GetParentFontSize() here is used to computed em, so it must be
    // parent's fontSize.z
    const auto &env_config = element_manager()->GetLynxEnvConfig();
    auto unify_vw_vh_behavior =
        element_manager()->GetDynamicCSSConfigs().unify_vw_vh_behavior_;
    const auto &vw_base =
        unify_vw_vh_behavior
            ? env_config.ViewportWidth()
            : styles_manager_.vwbase_for_font_size_to_align_with_legacy_bug();
    const auto &vh_base =
        unify_vw_vh_behavior
            ? env_config.ViewportHeight()
            : styles_manager_.vhbase_for_font_size_to_align_with_legacy_bug();
    result = starlight::CSSStyleUtils::ResolveFontSize(
        it->second, env_config, vw_base, vh_base, GetParentFontSize(),
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

    PreparePropBundleIfNeed();
    prop_bundle_->SetProps(
        CSSProperty::GetPropertyName(CSSPropertyID::kPropertyIDFontSize)
            .c_str(),
        *result);
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
  auto root_font_size = is_page() ? font_size : GetFontSize();

  SetFontSizeForAllElement(font_size, root_font_size);
  UpdateLayoutNodeFontSize(font_size, root_font_size);
}

Element *FiberElement::Sibling(int offset) const {
  if (!parent_) return nullptr;
  auto index = static_cast<FiberElement *>(parent_)->IndexOf(this);
  // We know the index can't be -1
  return parent_->GetChildAt(index + offset);
}

void FiberElement::InsertLayoutNode(FiberElement *child, FiberElement *ref) {
  DCHECK(!ref || !ref->is_wrapper());
  if (child->attached_to_layout_parent_) {
    LOGE("FiberElement layout node already inserted !");
    this->LogNodeInfo();
    child->LogNodeInfo();
  }
  element_manager()->InsertLayoutNodeBefore(id_, child->impl_id(),
                                            ref ? ref->impl_id() : -1);
  child->attached_to_layout_parent_ = true;
}

void FiberElement::RemoveLayoutNode(FiberElement *child) {
  element_manager()->RemoveLayoutNode(id_, child->impl_id());
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

void FiberElement::ParseRawInlineStyles(const lepus::Value &input,
                                        StyleMap *parsed_styles) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "FiberElement::ParseRawInlineStyles");
  auto &configs = element_manager_->GetCSSParserConfigs();
  if (input.IsString()) {
    const auto &str = input.StdString();
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
            current_raw_inline_styles_.insert_or_assign(id, std::move(value));
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
}

void FiberElement::DoFullCSSResolving() {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "FiberElement::DoFullStyleResolve");

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
    RecursivelyMarkChildrenCSSVariableDirty(css_var_table);
  }
}

const tasm::CSSValue &FiberElement::ResolveCurrentStyleValue(
    const CSSPropertyID &key, const tasm::CSSValue &default_value) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "FiberElement::ResolveCurrentStyleValue");
  auto iter = parsed_styles_map_.find(key);
  if (iter != parsed_styles_map_.end()) {
    return iter->second;
  }

  const auto &inherited_property = GetParentInheritedProperty();
  if (inherited_property.inherited_styles_ != nullptr) {
    iter = inherited_property.inherited_styles_->find(key);
    if (iter != inherited_property.inherited_styles_->end()) {
      return iter->second;
    }
  }

  return default_value;
}

bool FiberElement::RefreshStyle(StyleMap &parsed_styles,
                                base::Vector<CSSPropertyID> &reset_ids,
                                bool force_use_parsed_styles_map) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "FiberElement::RefreshStyle",
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
    parsed_styles_map_ = extreme_parsed_styles_;
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
        reset_ids.emplace_back(k);
      });
  return ret;
}

void FiberElement::OnClassChanged(const ClassList &old_classes,
                                  const ClassList &new_classes) {
  if (element_manager() && element_manager()->GetEnableStandardCSSSelector()) {
    CheckHasInvalidationForClass(old_classes, new_classes);
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
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "FiberElement::OnPseudoStatusChanged",
              [this](lynx::perfetto::EventContext ctx) {
                UpdateTraceDebugInfo(ctx.event());
              });
  // FIXME: Every element will emit the OnPseudoStatusChanged event
  auto *css_fragment = GetRelatedCSSFragment();
  if (css_fragment && css_fragment->enable_css_selector()) {
    // If disable the invalidation do nothing
    if (!css_fragment->enable_css_invalidation()) {
      return;
    }
    css::InvalidationLists invalidation_lists;
    AttributeHolder::CollectPseudoChangedInvalidation(
        css_fragment, invalidation_lists, prev_status, current_status);
    data_model_->SetPseudoState(current_status);
    for (auto *invalidation_set : invalidation_lists.descendants) {
      if (invalidation_set->InvalidatesSelf()) {
        MarkStyleDirty(false);
      }
      InvalidateChildren(invalidation_set);
      PipelineOptions pipeline_options;
      element_manager_->OnPatchFinish(pipeline_options, this);
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
  PipelineOptions pipeline_options;
  element_manager_->OnPatchFinish(pipeline_options, this);
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
  auto it = inherited_styles_.find(css_id);
  if (it != inherited_styles_.end()) {
    inherited_styles_.erase(it);
    reset_inherited_ids_.emplace_back(css_id);
    inherited_property_.children_propagate_inherited_styles_flag_ = true;
  }
}

void FiberElement::ConvertToInlineElement() {
  MarkAsInline();
  for (auto &child : scoped_children_) {
    child->ConvertToInlineElement();
  }
}

void FiberElement::HandleSelfFixedChange() {
  if (!fixed_changed_ || !render_parent_ || GetEnableFixedNew()) {
    return;
  }
  if (is_fixed_) {
    // non-fixed to fixed
    auto *parent = render_parent_;
    parent->HandleRemoveChildAction(this);
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
  if (!element_manager() ||
      !element_manager()->GetEnableStandardCSSSelector()) {
    return false;
  }
  auto *css_fragment = GetRelatedCSSFragment();
  // resolve styles from css fragment
  if (!css_fragment || !css_fragment->enable_css_invalidation()) {
    return false;
  }
  auto old_size = invalidation_lists_.descendants.size();
  AttributeHolder::CollectIdChangedInvalidation(
      css_fragment, invalidation_lists_, old_id, new_id);
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
  AttributeHolder::CollectClassChangedInvalidation(
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

void FiberElement::OnPatchFinish(PipelineOptions &option) {
  element_manager_->OnPatchFinish(option, this);
}

void FiberElement::FlushAnimatedStyleInternal(tasm::CSSPropertyID id,
                                              const tasm::CSSValue &value) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "FiberElement::FlushAnimatedStyleInternal");
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
  if (iter == parsed_styles_map_.end()) {
    iter = updated_inherited_styles_.find(css_id);
    if (iter == updated_inherited_styles_.end()) {
      return std::optional<CSSValue>();
    }
  }
  return iter->second;
}

void FiberElement::UpdateDynamicElementStyle(uint32_t style,
                                             bool force_update) {
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
      const auto &property = GetParentInheritedProperty();

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

      if (!inherited_styles_.empty()) {
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
    child->UpdateDynamicElementStyle(style, inner_force_update);
    child = child->next_render_sibling_;
  }
}

void FiberElement::SetCSSID(int32_t id) {
  if (css_id_ != id) {
    style_sheet_ = nullptr;
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
  style_sheet_ = nullptr;
  for (const auto &child : children()) {
    child->ResetSheetRecursively(manager);
  }
}

void FiberElement::PrepareOrUpdatePseudoElement(PseudoState state,
                                                StyleMap &style_map) {
  if (style_map.empty() &&
      pseudo_elements_.find(state) == pseudo_elements_.end()) {
    return;
  }

  PseudoElement *pseudo_element = CreatePseudoElementIfNeed(state);
  pseudo_element->UpdateStyleMap(style_map);
}

PseudoElement *FiberElement::CreatePseudoElementIfNeed(PseudoState state) {
  auto it = pseudo_elements_.find(state);
  if (it == pseudo_elements_.end()) {
    auto new_pseudo_element = std::make_unique<PseudoElement>(state, this);
    auto result = new_pseudo_element.get();
    pseudo_elements_[state] = std::move(new_pseudo_element);
    return result;
  } else {
    return it->second.get();
  }
}

void FiberElement::RecursivelyMarkRenderRootElement(FiberElement *render_root) {
  render_root_element_ = render_root;
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

void FiberElement::HandleFlushActionsLayoutTask(
    base::MoveOnlyClosure<void> operation) {
  // Dispatch operation according to batch rendering state
  auto *parent = this;
  if (parent->render_root_element_ != nullptr &&
      parent->render_root_element_->scheduler_adapter_.get() &&
      parent->render_root_element_->scheduler_adapter_->IsBatchRendering()) {
    parent->render_root_element_->scheduler_adapter_
        ->resolve_element_tree_queue()
        .emplace_back(std::move(operation));
  } else {
    operation();
  }
}

void FiberElement::SetFontSizeForAllElement(double cur_node_font_size,
                                            double root_node_font_size) {
  computed_css_style()->SetFontSize(cur_node_font_size, root_node_font_size);

  for (const auto &[key, pseudo_element] : pseudo_elements_) {
    pseudo_element->SetFontSize(cur_node_font_size, root_node_font_size);
  }
}

void FiberElement::UpdateLengthContextValueForAllElement(
    const LynxEnvConfig &env_config) {
  computed_css_style()->SetFontScale(env_config.FontScale());
  computed_css_style()->SetViewportWidth(env_config.ViewportWidth());
  computed_css_style()->SetViewportHeight(env_config.ViewportHeight());
  computed_css_style()->SetScreenWidth(env_config.ScreenWidth());

  for (const auto &[key, pseudo_element] : pseudo_elements_) {
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

// TODO: Move this method out of fiber_element when a more general render root
// is introduced.
void FiberElement::AsyncResolveSubtreeProperty() {
  if (element_manager()->GetEnableParallelElement() &&
      ((dirty_ & ~kDirtyTree) != 0) && scheduler_adapter_.get()) {
    element_manager()->GetTasmWorkerTaskRunner()->PostTask([this]() mutable {
      scheduler_adapter_->ResolveSubtreeProperty();

      std::promise<ParallelFlushReturn> promise;
      std::future<ParallelFlushReturn> future = promise.get_future();
      auto task_info_ptr =
          fml::MakeRefCounted<base::OnceTask<ParallelFlushReturn>>(
              [promise = std::move(promise),
               scheduler = scheduler_adapter_]() mutable {
                promise.set_value(
                    scheduler->GenerateReduceTaskForResolveProperty());
              },
              std::move(future));
      element_manager()->ParallelTasks().emplace_back(std::move(task_info_ptr));
    });
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

}  // namespace tasm
}  // namespace lynx
