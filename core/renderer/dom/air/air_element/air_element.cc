// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/dom/air/air_element/air_element.h"

#include <utility>

#include "base/include/string/string_number_convert.h"
#include "base/trace/native/trace_event.h"
#include "core/base/lynx_trace_categories.h"
#include "core/renderer/css/css_keyframes_token.h"
#include "core/renderer/dom/element_manager.h"
#include "core/renderer/page_proxy.h"
#include "core/renderer/starlight/layout/layout_object.h"
#include "core/renderer/utils/base/tasm_constants.h"
#include "core/renderer/utils/lynx_env.h"
#include "core/renderer/utils/value_utils.h"
#include "core/runtime/vm/lepus/array.h"
#include "core/runtime/vm/lepus/table.h"
#include "core/value_wrapper/value_impl_lepus.h"

namespace lynx {
namespace tasm {

namespace {
// compute array:[value , pattern , value , pattern], need to compute px.
// eg:border-radius , padding , margin
//      "property_id": {
//        "value": [
//          width_value,
//          width_value_pattern,
//          height_value,
//          height_value_pattern
//        ],
//        "pattern": 14
//      }
constexpr static int32_t kInvalidIndex = -1;
// constexpr static float kRpxRatio = 750.0f;
}  // namespace

AirElement::AirElement(AirElementType type, ElementManager *manager,
                       const base::String &tag, uint32_t lepus_id, int32_t id)
    : element_type_(type),
      tag_(tag),
      catalyzer_(manager->catalyzer()),
      dirty_(kDirtyCreated),
      lepus_id_(lepus_id),
      font_size_(manager->GetLynxEnvConfig().DefaultFontSize()),
      root_font_size_(manager->GetLynxEnvConfig().DefaultFontSize()),
      air_element_manager_(manager) {
  id_ = id > 0 ? id : manager->GenerateElementID();
  // page config switch for enabling layout-only ability
  config_enable_layout_only_ = manager->GetEnableLayoutOnly();
  config_flatten_ = manager->GetPageFlatten();
  element_manager()->CreateLayoutNode(id_, tag_);
}

void AirElement::MergeHigherPriorityCSSStyle(StyleMap &primary,
                                             const StyleMap &higher) {
  for (const auto &it : higher) {
    primary[it.first] = it.second;
  }
}

bool AirElement::ResolveKeyframesMap(CSSPropertyID id,
                                     const std::string &keyframes_name) {
  StyleMap keyframes;
  GetKeyframesMap(keyframes_name, keyframes);
  if (!keyframes.empty()) {
    return ResolveKeyframesMap(kPropertyIDAnimationName,
                               keyframes[kPropertyIDAnimationName].GetValue());
  }
  return false;
}

bool AirElement::ResolveKeyframesMap(CSSPropertyID id,
                                     const lepus::Value &keyframes_map) {
  if (CSSProperty::IsKeyframeProps(id)) {
    if (id == kPropertyIDAnimation) {
      // If animation is declared in inline style ,we should resolve it by using
      // keyframes saved by parse_style_
      auto animation_name =
          keyframes_map.GetProperty(std::to_string(kPropertyIDAnimationName));
      if (animation_name.IsString()) {
        return ResolveKeyframesMap(kPropertyIDAnimationName,
                                   animation_name.StdString());
      }
    } else if (id == kPropertyIDAnimationName) {
      if (keyframes_map.IsString()) {
        return ResolveKeyframesMap(id, keyframes_map.StdString());
      } else if (keyframes_map.IsTable()) {
        // decode keyframe map, for example
        // {"translateX-ani":{
        //    "0"(string to float) : {"transform"(string) :
        //    "translateX(0)"(string)}, "1" : {"transform"(string) :
        //    "translateX(50px)"(string)},
        // }}
        ForEachLepusValue(
            keyframes_map, [this](const lepus::Value &keyframe_name,
                                  const lepus::Value &keyframe_dic) {
              starlight::CSSStyleUtils::UpdateCSSKeyframes(
                  keyframes_map_, keyframe_name.StdString(), keyframe_dic,
                  air_element_manager_->GetCSSParserConfigs());
            });
      }
    }
    return true;
  }
  return false;
}

void AirElement::PushKeyframesToPlatform() {
  if (!keyframes_map_.empty()) {
    auto lepus_keyframes = starlight::CSSStyleUtils::ResolveCSSKeyframes(
        keyframes_map_, computed_css_style()->GetMeasureContext(),
        air_element_manager_->GetCSSParserConfigs());
    if (!lepus_keyframes.IsTable()) {
      return;
    }
    auto bundle = element_manager()->GetPropBundleCreator()->CreatePropBundle();
    bundle->SetProps("keyframes", pub::ValueImplLepus(lepus_keyframes));
    PushToPropsBundle(BASE_STATIC_STRING(kPropertyNameAnimation),
                      computed_css_style()->GetValue(kPropertyIDAnimation));
    painting_context()->SetKeyframes(std::move(bundle));
    keyframes_map_.clear();
  }
}

AirElement *AirElement::GetParentComponent() const {
  AirElement *parent_node = air_parent_;
  while (parent_node != nullptr) {
    if (parent_node->is_component() || parent_node->is_page()) {
      return parent_node;
    }
    parent_node = parent_node->air_parent_;
  }
  return nullptr;
}

/// used by touch_event_handle mainly
bool AirElement::InComponent() const {
  auto parent = GetParentComponent();
  if (parent) {
    return !(parent->is_page());
  }
  return false;
}

void AirElement::InsertNode(AirElement *child, bool from_virtual_child) {
  if (child != this) {
    if (!from_virtual_child) {
      InsertAirNode(child);
    }
    if (child->is_virtual_node()) {
      child->set_parent(this);
      return;
    }
    size_t index = FindInsertIndex(children_, child);
    AddChildAt(child, index);
    dirty_ |= kDirtyTree;
    child->dirty_ |= kDirtyTree;
  }
}

void AirElement::InsertNodeIndex(AirElement *child, size_t index) {
  element_manager()->InsertLayoutNode(impl_id(), child->impl_id(),
                                      static_cast<int>(index));
  child->layout_node_inserted_ = true;
  element_container()->AttachChildToTargetContainer(GetChildAt(index));
}

void AirElement::InsertNodeBefore(AirElement *child,
                                  const AirElement *reference_child) {
  if (child->is_virtual_node()) {
    // for virtual_node, only save the parent's ptr
    child->set_parent(this);
    return;
  }
  auto index = IndexOf(reference_child);
  if (index >= static_cast<int>(children_.size())) {
    return;
  }
  AddChildAt(child, index);
  dirty_ |= kDirtyTree;
  child->dirty_ |= kDirtyTree;
}

void AirElement::InsertNodeAfterIndex(AirElement *child, int &index) {
  dirty_ |= kDirtyTree;
  child->dirty_ |= kDirtyTree;
  if (!child->is_virtual_node()) {
    ++index;
    AddChildAt(child, index);
  } else {
    child->set_parent(this);
    for (auto air_child : child->air_children_) {
      InsertNodeAfterIndex(air_child.get(), index);
    }
  }
}

void AirElement::InsertNodeAtBottom(AirElement *child) {
  int index = kInvalidIndex;
  if (!children_.empty()) {
    index = static_cast<int>(children_.size() - 1);
  }
  InsertNodeAfterIndex(child, index);
}

void AirElement::InsertAirNode(AirElement *child) {
  if (child != this) {
    size_t index = FindInsertIndex(air_children_, child);
    AddAirChildAt(child, index);
  }
}

AirElement *AirElement::LastNonVirtualNode() {
  AirElement *result = nullptr;
  if (!is_virtual_node()) {
    return this;
  }
  for (auto it = air_children_.rbegin(); it != air_children_.rend(); ++it) {
    if (!(*it)->is_virtual_node()) {
      result = (*it).get();
    } else {
      result = (*it)->LastNonVirtualNode();
    }
    if (result != nullptr) {
      return result;
    }
  }
  return nullptr;
}

void AirElement::RemoveNode(AirElement *child, bool destroy) {
  child->has_been_removed_ = true;
  if (child->is_virtual_node()) {
    child->RemoveAllNodes(destroy);
    if (destroy) {
      element_manager()->DestroyLayoutNode(child->impl_id());
      element_manager()->air_node_manager()->Erase(child->impl_id());
      element_manager()->air_node_manager()->EraseLepusId(child->GetLepusId(),
                                                          child);
    }
    RemoveAirNode(child);
    dirty_ |= kDirtyTree;
    return;
  }
  if (child->is_component()) {
    child->OnElementRemoved();
  }
  auto index = IndexOf(child);
  if (index != kInvalidIndex) {
    child->RemoveAllNodes();
    RemoveNode(child, index, destroy);
    RemoveAirNode(child);
    dirty_ |= kDirtyTree;
  }
}

void AirElement::RemoveAllNodes(bool destroy) {
  for (auto const &child : SharedAirElementVector(air_children_)) {
    RemoveNode(child.get(), destroy);
  }

  if (destroy) {
    air_children_.erase(air_children_.begin(), air_children_.end());
  }
}

void AirElement::RemoveAirNode(AirElement *child) {
  auto index = IndexOfAirChild(child);
  RemoveAirNode(child, index);
}

void AirElement::SetAttribute(const base::String &key,
                              const lepus::Value &value, bool resolve) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "AirElement::SetAttribute");
  // check flatten prop and update config_flatten_
  if (resolve) {
    CheckFlattenProp(key, value);
    PushToPropsBundle(key, value);
#if OS_ANDROID
    CheckHasNonFlattenAttr(key, value);
#endif
  } else {
    raw_attributes_[key] = value.ToLepusValue();
  }
}

void AirElement::ComputeCSSStyle(CSSPropertyID id, tasm::CSSValue &css_value) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "AirElement::ComputeCSSStyle");
  computed_css_style()->SetValue(id, css_value);
  css_value.SetValue(computed_css_style()->GetValue(id));
}

void AirElement::Destroy() {
  // Layout only destroy recursively
  if (!IsLayoutOnly()) {
    painting_context()->DestroyPaintingNode(
        parent() ? parent()->impl_id() : kInvalidIndex, impl_id(), 0);
  } else {
    for (int i = static_cast<int>(GetChildCount()) - 1; i >= 0; --i) {
      GetChildAt(i)->Destroy();
    }
  }
  if (parent()) {
    parent()->RemoveNode(this);
  }
}

void AirElement::MarkPlatformNodeDestroyedRecursively() {
  has_painting_node_ = false;
  // All descent UI will be deleted recursively in platform side, should mark it
  // recursively
  for (size_t i = 0; i < GetChildCount(); ++i) {
    auto child = GetChildAt(i);
    child->MarkPlatformNodeDestroyedRecursively();
    child->Destroy();
    if (child->parent() == this) {
      child->parent_ = nullptr;
    }
  }
  children_.clear();
}

void AirElement::RemoveNode(AirElement *child, unsigned int index,
                            bool destroy) {
  if (index >= children_.size()) {
    return;
  }
  bool destroy_platform_node = destroy && child->HasPaintingNode();
  element_manager()->RemoveLayoutNode(impl_id(), child->impl_id());
  if (child->HasPaintingNode()) {
    child->element_container()->RemoveSelf(destroy_platform_node);
  }
  if (destroy_platform_node) {
    child->MarkPlatformNodeDestroyedRecursively();
  }
  RemoveChildAt(index);
  if (destroy) {
    element_manager()->DestroyLayoutNode(child->impl_id());
    element_manager()->air_node_manager()->Erase(child->impl_id());
    element_manager()->air_node_manager()->EraseLepusId(child->GetLepusId(),
                                                        child);
  }
}

void AirElement::RemoveAirNode(AirElement *child, unsigned int index,
                               bool destroy) {
  if (index >= air_children_.size()) {
    return;
  }
  RemoveAirChildAt(index);
}

void AirElement::UpdateLayout(float left, float top, float width, float height,
                              const std::array<float, 4> &paddings,
                              const std::array<float, 4> &margins,
                              const std::array<float, 4> &borders,
                              const std::array<float, 4> *sticky_positions,
                              float max_height) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "AirElement::UpdateLayout");
  frame_changed_ = true;
  top_ = top;
  left_ = left;
  width_ = width;
  height_ = height;
  paddings_ = paddings;
  margins_ = margins;
  borders_ = borders;
}

/// for radon mode
void AirElement::PushDynamicNode(AirElement *node) {
  dynamic_nodes_.push_back(node);
}

AirElement *AirElement::GetDynamicNode(uint32_t index,
                                       uint32_t lepus_id) const {
  if (index >= dynamic_nodes_.size()) {
    LOGF("GetDynamicNode overflow. node_index "
         << lepus_id << " index: " << index
         << " dynamic_nodes_.size(): " << dynamic_nodes_.size());
  }
  auto *node = dynamic_nodes_[index];
  if (node->GetLepusId() != lepus_id) {
    LOGF("GetDynamicNode indices not equal. target node index "
         << lepus_id << " but got: " << node->GetLepusId());
  }
  return node;
}

void AirElement::UpdateUIChildrenCountInParent(int delta) {
  if (IsLayoutOnly()) {
    AirElement *parent = parent_;
    while (parent) {
      parent->ui_children_count += delta;
      if (!parent->IsLayoutOnly()) {
        break;
      }
      parent = static_cast<AirElement *>(parent->parent());
    }
  }
}

// element tree
void AirElement::AddChildAt(AirElement *child, size_t index) {
  children_.insert(
      children_.begin() + index,
      air_element_manager_->air_node_manager()->Get(child->impl_id()));
  child->parent_ = this;
}

AirElement *AirElement::RemoveChildAt(size_t index) {
  if (index < children_.size()) {
    AirElement *removed = children_[index].get();
    children_.erase(children_.begin() + index);
    removed->parent_ = nullptr;
    return removed;
  }
  return nullptr;
}

int AirElement::IndexOf(const AirElement *child) {
  for (size_t index = 0; index != children_.size(); ++index) {
    if (children_[index].get() == child) {
      return static_cast<int>(index);
    }
  }
  return kInvalidIndex;
}

void AirElement::AddAirChildAt(AirElement *child, size_t index) {
  air_children_.insert(
      air_children_.begin() + index,
      air_element_manager_->air_node_manager()->Get(child->impl_id()));
  child->air_parent_ = this;
}

AirElement *AirElement::RemoveAirChildAt(size_t index) {
  if (index < air_children_.size()) {
    auto removed = air_children_[index];
    air_children_.erase(air_children_.begin() + index);
    removed->air_parent_ = nullptr;
    return removed.get();
  }
  return nullptr;
}

int AirElement::IndexOfAirChild(AirElement *child) {
  if (child == nullptr) {
    return kInvalidIndex;
  }
  for (size_t index = 0; index < air_children_.size(); ++index) {
    if (air_children_[index].get() == child) {
      return static_cast<int>(index);
    }
  }
  return kInvalidIndex;
}

AirElement *AirElement::GetChildAt(size_t index) const {
  if (index >= children_.size()) {
    return nullptr;
  }
  return children_[index].get();
}
// TODO(liukeang): after transformation of air_element extending element,
// relevant codes from element.cc can be reused.
void AirElement::CheckHasNonFlattenCSSProps(CSSPropertyID id) {
  if (has_non_flatten_attrs_) {
    // never change has_non_flatten_attrs_ to false again
    return;
  }
  // check is transition animation
  if (CSSProperty::IsTransitionProps(id)) {
    has_non_flatten_attrs_ = true;
    return;
  }

  // check opacity props
  if (UNLIKELY(id == CSSPropertyID::kPropertyIDOpacity) &&
      !tag_.IsEquals("text") && !tag_.IsEquals("image")) {
    has_non_flatten_attrs_ = true;
    return;
  }

  // check specific CSS Property
  if (id == CSSPropertyID::kPropertyIDFilter || id == kPropertyIDVisibility ||
      id == kPropertyIDClipPath || id == CSSPropertyID::kPropertyIDBoxShadow ||
      id == CSSPropertyID::kPropertyIDTransform ||
      id == CSSPropertyID::kPropertyIDTransformOrigin ||
      (id >= CSSPropertyID::kPropertyIDOutline &&
       id <= CSSPropertyID::kPropertyIDOutlineWidth) ||
      (id >= CSSPropertyID::kPropertyIDLayoutAnimationCreateDuration &&
       id <= CSSPropertyID::kPropertyIDLayoutAnimationUpdateDelay)) {
    has_non_flatten_attrs_ = true;
  }
}

void AirElement::CheckHasNonFlattenAttr(const base::String &key,
                                        const lepus::Value &value) {
  if (has_non_flatten_attrs_) {
    return;
  }
  // TODO(songshourui.null): When set exposure prop, the ui must not be
  // flatten, since the flattenUI exposure rect may be not correct. Will
  // remove the exposure prop when fix flattenUI exposure rect's bug.
  static const base::NoDestructor<std::unordered_set<std::string>>
      non_flatten_attr_set{{"name", "clip-radius", "overlap",
                            "enter-transition-name", "exit-transition-name",
                            "pause-transition-name", "resume-transition-name",
                            "exposure-scene", "exposure-id"}};

  if (non_flatten_attr_set->count(key.str())) {
    has_non_flatten_attrs_ = true;
  }
}

void AirElement::FlushFontSize() {
  computed_css_style()->SetFontSize(font_size_, root_font_size_);
  if (!EnableAsyncCalc()) {
    element_manager()->UpdateLayoutNodeFontSize(id_, font_size_,
                                                root_font_size_);
  }
  prop_bundle_->SetProps(
      CSSProperty::GetPropertyName(CSSPropertyID::kPropertyIDFontSize).c_str(),
      font_size_);
}

void AirElement::SetStyle(CSSPropertyID id, tasm::CSSValue &value) {
  // After CSS Diff&Merge is completed, the entry method for processing css
  // properties. Including four types:
  // 1. animation
  // 2. layoutOnly: set to starlight
  // 3. layoutWanted: set to starlight & set to platform
  // 4. other:set to platform
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "AirElement::SetStyle");
  // Handle animation properties
  if (UNLIKELY(ResolveKeyframesMap(id, value.GetValue()))) {
    has_animate_props_ = true;
    ComputeCSSStyle(id, value);
    return;
  }
  if (UNLIKELY(id == kPropertyIDFontSize)) {
    auto result = starlight::CSSStyleUtils::ResolveFontSize(
        value, element_manager()->GetLynxEnvConfig(),
        element_manager()->GetLynxEnvConfig().ViewportWidth(),
        element_manager()->GetLynxEnvConfig().ViewportHeight(), font_size_,
        root_font_size_, element_manager()->GetCSSParserConfigs());
    if (result.has_value() && *result != font_size_) {
      font_size_ = *result;
      FlushFontSize();
      has_font_size_ = true;
    }
    return;
  }
  // The LayoutOnly css property only needs to be set to starlight
  if (LayoutNode::IsLayoutOnly(id)) {
    ConsumeStyle(id, value);
  } else {
    // check if css props passing to platform are flatten-related
    if (!has_animate_props_) {
#if OS_ANDROID
      // check flatten flag for Android platform if needed
      // Normally, it's better to move below checks to Android platform side,
      // but checking in C++ size has a better performance
      CheckHasNonFlattenCSSProps(id);
#endif
    }
    if (!has_transition_attrs_ && CSSProperty::IsTransitionProps(id)) {
      has_transition_attrs_ = true;
    }

    // The LayoutWanted css property needs to be set to starlight, and also
    // needs to be set to the platform.
    if (LayoutNode::IsLayoutWanted(id)) {
      ConsumeStyle(id, value);
      ComputeCSSStyle(id, value);
      // set to the platform
      PushToPropsBundle(CSSProperty::GetPropertyName(id), value.GetValue());
    } else {
      // Other css properties only need to be set to the platform layer
      ComputeCSSStyle(id, value);
      if (!CSSProperty::IsTransitionProps(id)) {
        PushToPropsBundle(CSSProperty::GetPropertyName(id), value.GetValue());
      }
    }
  }
}

void AirElement::ResetStyle(CSSPropertyID id) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "AirElement::ResetStyle");
  bool is_layout_only = LayoutNode::IsLayoutOnly(id);
  if (is_layout_only || LayoutNode::IsLayoutWanted(id)) {
    if (EnableAsyncCalc()) {
      async_reset_styles_.insert(id);
    } else {
      air_element_manager_->ResetLayoutNodeStyle(impl_id(), id);
    }
    dirty_ |= kDirtyStyle;
  }
  if (is_layout_only) {
    return;
  }
  has_layout_only_props_ = false;
  prop_bundle_->SetNullProps(CSSProperty::GetPropertyName(id).c_str());
  dirty_ |= kDirtyAttr;
}

starlight::ComputedCSSStyle *AirElement::computed_css_style() {
  if (!platform_css_style_ && air_element_manager_) {
    platform_css_style_ = std::make_unique<starlight::ComputedCSSStyle>(
        *air_element_manager_->platform_computed_css());
    const auto &env_config = air_element_manager_->GetLynxEnvConfig();
    platform_css_style_->SetScreenWidth(env_config.ScreenWidth());
    platform_css_style_->SetViewportHeight(env_config.ViewportHeight());
    platform_css_style_->SetViewportWidth(env_config.ViewportWidth());
    platform_css_style_->SetCssAlignLegacyWithW3c(
        air_element_manager_->GetLayoutConfigs().css_align_with_legacy_w3c_);
    platform_css_style_->SetFontScaleOnlyEffectiveOnSp(
        air_element_manager_->GetLynxEnvConfig().FontScaleSpOnly());
    platform_css_style_->SetFontScale(
        air_element_manager_->GetLynxEnvConfig().FontScale());
  }
  return platform_css_style_.get();
}

void AirElement::CheckHasAnimateProps(CSSPropertyID id) {
  if (!has_animate_props_) {
    has_animate_props_ =
        (id >= CSSPropertyID::kPropertyIDTransform &&
         id <= CSSPropertyID::kPropertyIDAnimationPlayState) ||
        (id >= CSSPropertyID::kPropertyIDLayoutAnimationCreateDuration &&
         id <= CSSPropertyID::kPropertyIDLayoutAnimationUpdateDelay) ||
        (id >= CSSPropertyID::kPropertyIDTransition &&
         id <= CSSPropertyID::kPropertyIDTransitionDuration) ||
        (id == CSSPropertyID::kPropertyIDTransformOrigin);
  }
}

// event handler
void AirElement::SetEventHandler(const base::String &name,
                                 EventHandler *handler) {
  PreparePropBundleIfNeed();
  prop_bundle_->SetEventHandler(handler->ToPubLepusValue());
  has_layout_only_props_ = false;
}

void AirElement::ResetEventHandlers() {
  if (prop_bundle_ != nullptr) {
    prop_bundle_->ResetEventHandler();
  }
}

size_t AirElement::GetUIChildrenCount() const {
  size_t ret = 0;
  for (auto current : children_) {
    if (current->IsLayoutOnly()) {
      ret += current->GetUIChildrenCount();
    } else {
      ret++;
    }
  }
  return ret;
}

size_t AirElement::GetUIIndexForChild(AirElement *child) const {
  int index = 0;
  bool found = false;
  for (auto it : children_) {
    auto current = it;
    if (child == current.get()) {
      found = true;
      break;
    }
    index += (current->IsLayoutOnly() ? current->GetUIChildrenCount() : 1);
  }
  if (!found) {
    LOGE("air_element can not found:" + tag_.str());
    // Child id was not a child of parent id
    // TODO(renpengcheng):logbox error
    DCHECK(false);
  }
  return index;
}

void AirElement::CreateElementContainer(bool platform_is_flatten) {
  element_container_ = std::make_unique<AirElementContainer>(this);
  if (IsLayoutOnly()) {
    return;
  }
  painting_context()->CreatePaintingNode(id_, tag_.str(), prop_bundle_,
                                         platform_is_flatten, false, lepus_id_);
}

void AirElement::ConsumeStyle(CSSPropertyID id, const tasm::CSSValue &value) {
  if (EnableAsyncCalc()) {
    async_resolved_styles_.emplace_back(std::make_pair(id, value));
  } else {
    element_manager()->UpdateLayoutNodeStyle(impl_id(), id, value);
  }
  dirty_ |= kDirtyStyle;
}

void AirElement::FlushProps() { FlushPropsResolveStyles(true); }

void AirElement::FlushPropsResolveStyles(bool resolve_styles) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "AirElement::FlushPropsResolveStyles");
  if (!resolve_styles && state_ & ElementState::kPropsUpdated) {
    return;
  };
  if (resolve_styles && !is_page() && !is_virtual_node()) {
    RefreshStyles();
  }
  PushKeyframesToPlatform();
  // Only view and component can be optimized as layout only node
  if (has_layout_only_props_ &&
      !(tag_.IsEquals("view") || tag_.IsEquals("component"))) {
    has_layout_only_props_ = false;
  }

  if (has_transition_attrs_) {
    PushToPropsBundle(CSSProperty::GetPropertyName(kPropertyIDTransition),
                      computed_css_style()->GetValue(kPropertyIDTransition));
    has_transition_attrs_ = false;
  }

  // Update The root if needed
  if (!has_painting_node_) {
    TRACE_EVENT(LYNX_TRACE_CATEGORY, "AirElement::FlushProps::NoPaintingNode");
    const auto &env_config = element_manager()->GetLynxEnvConfig();
    if (EnableAsyncCalc()) {
      // send to layoutcontext in main thread
      if (has_font_size_) {
        element_manager()->UpdateLayoutNodeFontSize(id_, font_size_,
                                                    root_font_size_);
      }
      for (const auto &[css_id, css_value] : async_resolved_styles_) {
        element_manager()->UpdateLayoutNodeStyle(impl_id(), css_id, css_value);
      }
      for (const auto &css_id : async_reset_styles_) {
        element_manager()->ResetLayoutNodeStyle(impl_id(), css_id);
      }
      async_resolved_styles_.clear();
      async_reset_styles_.clear();
    }

    if (Config::DefaultFontScale() != env_config.FontScale() &&
        !has_font_size_ && (tag_.IsEquals("text") || tag_.IsEquals("x-text"))) {
      font_size_ = root_font_size_ = env_config.PageDefaultFontSize();
      FlushFontSize();
    }
    PreparePropBundleIfNeed();
    element_manager()->AttachLayoutNodeType(impl_id(), tag_, false,
                                            prop_bundle_);
    is_virtual_ = air_element_manager_->IsShadowNodeVirtual(tag_);
    set_is_layout_only(CanBeLayoutOnly() || is_virtual_);
    CreateElementContainer(TendToFlatten());
    has_painting_node_ = true;
    enable_async_calc_ = false;
  } else {
    if (!prop_bundle_) {
      prop_bundle_ =
          element_manager()->GetPropBundleCreator()->CreatePropBundle();
    }
    element_manager()->UpdateLayoutNodeProps(impl_id(), prop_bundle_);
    if (!is_virtual_) {
      if (!IsLayoutOnly()) {
        painting_context()->UpdatePaintingNode(impl_id(), TendToFlatten(),
                                               prop_bundle_);
      }
      element_container_->SetPropsChanged(true);
    }
  }
  prop_bundle_ = nullptr;
  dirty_ = 0;
  state_ |= ElementState::kPropsUpdated;
}

bool AirElement::CalcStyle(bool waiting) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "AirElement::CalcStyle",
              [&](lynx::perfetto::EventContext ctx) {
                auto *debug = ctx.event()->add_debug_annotations();
                debug->set_name("lepus_id");
                debug->set_string_value(std::to_string(lepus_id_));
                auto *debug1 = ctx.event()->add_debug_annotations();
                debug1->set_name("impl_id");
                debug1->set_string_value(std::to_string(id_));
              });
  if (!waiting && state_ & kStyleCalculating) {
    return false;
  }
  std::lock_guard<std::mutex> guard(cal_mutex_);
  if (state_ & kPropsUpdated || state_ & kStyleCalculated) {
    return true;
  }
  state_ |= kStyleCalculating;
  PreparePropBundleIfNeed();
  if (!dynamic_inline_style_.empty()) {
    SetInlineStyle(dynamic_inline_style_);
    dynamic_inline_style_.clear();
  }
  // resolve static style
  if (!static_inline_style_.empty()) {
    for (const auto &[css_id, lepus_value] : static_inline_style_) {
      SetInlineStyle(css_id, lepus_value);
    }
    static_inline_style_.clear();
  }
  if (!is_page() && !is_virtual_node()) {
    RefreshStyles();
  }
  // resolve attributes
  if (!raw_attributes_.empty()) {
    for (const auto &[key, value] : raw_attributes_) {
#if OS_ANDROID
      CheckHasNonFlattenAttr(key, value);
#endif
      prop_bundle_->SetProps(key.c_str(), pub::ValueImplLepus(value));
    }
    raw_attributes_.clear();
  }
  state_ |= ElementState::kStyleCalculated;
  return true;
}

void AirElement::FlushRecursively() {
  if (is_virtual_node() && parent_) {
    parent_->FlushRecursively();
    return;
  }
  if (dirty_ & ~kDirtyTree || style_dirty_ > 0 || !has_painting_node_) {
    FlushProps();
  }
  for (size_t i = 0; i < GetChildCount(); ++i) {
    auto *child = GetChildAt(i);
    bool has_flush_recursive = false;
    bool child_has_painting_node = child->has_painting_node_;
    if (child->dirty_ & ~kDirtyTree || child->style_dirty_ > 0 ||
        !child_has_painting_node) {
      child->FlushProps();
    }
    if (child->IsLayoutOnly()) {
      child->FlushRecursively();
      has_flush_recursive = true;
    }
    if (!child_has_painting_node || child->has_been_removed_ ||
        !child->layout_node_inserted_) {
      InsertNodeIndex(child, i);
      child->has_been_removed_ = false;
    }
    //    if (!(child->layout_node_->parent())) {
    //      // In case the order of multiple flushes is not as expected, insert
    //      again
    //      // to ensure that the nodes are correct.
    //      InsertNodeIndex(child, i);
    //    }
    if (!has_flush_recursive) {
      child->FlushRecursively();
    }
  }
}

void AirElement::PreparePropBundleIfNeed() {
  if (!prop_bundle_) {
    prop_bundle_ =
        element_manager()->GetPropBundleCreator()->CreatePropBundle();
  }
}

bool AirElement::TendToFlatten() const {
  return config_flatten_ && !has_animate_props_ && !has_non_flatten_attrs_;
}

PaintingContext *AirElement::painting_context() {
  return catalyzer_->painting_context();
}

lepus::Value AirElement::GetData() {
  AirElement *component = GetParentComponent();
  if (component) {
    return component->GetData();
  }
  return lepus::Value();
}

lepus::Value AirElement::GetProperties() {
  AirElement *component = GetParentComponent();
  if (component->is_component()) {
    return component->GetProperties();
  }
  return lepus::Value();
}

bool AirElement::CheckFlattenProp(const base::String &key,
                                  const lepus::Value &value) {
  if (key.IsEquals("flatten")) {
    if ((value.IsString() && value.StdString() == "false") ||
        (value.IsBool() && !value.Bool())) {
      config_flatten_ = false;
      return true;
    }
    config_flatten_ = true;
    return true;
  }
  return false;
}

void AirElement::SetClasses(const lepus::Value &class_names) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "AirElement::SetClasses");
  if (class_names.IsEmpty()) {
    classes_.clear();
    style_dirty_ |= Selector::kCLASS;
    return;
  }

  std::vector<std::string> class_name_vec;
  base::SplitString(class_names.StringView(), ' ', class_name_vec);

  if (classes_.size() != class_name_vec.size()) {
    style_dirty_ |= Selector::kCLASS;
  } else {
    for (auto &class_name : class_name_vec) {
      bool need_mark_style_dirty =
          !(style_dirty_ & Selector::kCLASS) && !class_name.empty() &&
          std::find(classes_.begin(), classes_.end(), class_name) ==
              classes_.end();
      if (need_mark_style_dirty) {
        style_dirty_ |= Selector::kCLASS;
        break;
      }
    }
  }
  if (style_dirty_ & Selector::kCLASS) {
    classes_.swap(class_name_vec);
  }
}

void AirElement::SetIdSelector(const lepus::Value &id_selector) {
  const auto &input_str = id_selector.StdString();
  if (input_str != id_selector_) {
    id_selector_ = input_str;
    PushToPropsBundle(BASE_STATIC_STRING(kElementIdSelector), id_selector);
    style_dirty_ |= Selector::kID;
  }
}

size_t AirElement::FindInsertIndex(const SharedAirElementVector &target,
                                   AirElement *child) {
  // The lepus id of elements in vector are arranged from small to large. Find
  // the first element whose lepus id is greater than the element to be inserted
  // and insert the element in front of it. (In most cases, elements are
  // inserted in order from small to large, so search from the end.)
  size_t index = target.size();
  for (auto iter = target.rbegin(); iter != target.rend(); ++iter) {
    if (child->lepus_id_ < (*iter)->lepus_id_) {
      index--;
    } else {
      break;
    }
  }
  return index;
}

void AirElement::SetInlineStyle(CSSPropertyID id, const CSSValue &value) {
  inline_style_map_[id] = value;
  style_dirty_ |= Selector::kINLINE;
}

void AirElement::SetInlineStyle(CSSPropertyID id, tasm::CSSValue &&value) {
  inline_style_map_[id] = std::move(value);
  style_dirty_ |= Selector::kINLINE;
}

void AirElement::SetInlineStyle(CSSPropertyID id, const lepus::Value &value,
                                bool resolve) {
  if (resolve) {
    tasm::StyleMap ret;
    tasm::UnitHandler::Process(id, value, ret,
                               air_element_manager_->GetCSSParserConfigs());
    // key for aggregation css may change after UnitHandler::Process,
    // e.g.: kPropertyIDBorderBottom-> kPropertyIDBorderBottomStyle
    for (auto &[css_id, css_value] : ret) {
      SetInlineStyle(css_id, std::move(css_value));
    }
  } else {
    static_inline_style_[id] = value.ToLepusValue();
  }
}

void AirElement::SetInlineStyle(const std::string &inline_style, bool resolve) {
  if (resolve) {
    auto splits = base::SplitStringByCharsOrderly<':', ';'>(inline_style);
    // inline_style is empty means reset all inline styles
    if (splits.size() < 2) {
      style_dirty_ |= Selector::kINLINE;
      inline_style_map_.clear();
      return;
    }
    for (size_t i = 0; i + 1 < splits.size(); i += 2) {
      std::string key = base::TrimString(splits[i]);
      CSSPropertyID id = CSSProperty::GetPropertyID(key);
      if (CSSProperty::IsPropertyValid(id)) {
        std::string value = base::TrimString(splits[i + 1]);
        auto css_values =
            UnitHandler::Process(id, lepus::Value(std::move(value)),
                                 air_element_manager_->GetCSSParserConfigs());
        for (auto &[css_id, css_value] : css_values) {
          SetInlineStyle(css_id, std::move(css_value));
        }
      }
    }
  } else {
    dynamic_inline_style_ = inline_style;
  }
}

void AirElement::DiffStyles(StyleMap &old_map, const StyleMap &new_map,
                            StylePatch &style_patch, bool is_final,
                            bool is_dirty) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "AirElement::DiffStyles");
  // Selector has not changed
  if (new_map.empty()) {
    for (auto &old_map_item : old_map) {
      // Delete those css ids that need to be updated in the low priority but
      // need to be reserved in the high priority from the update_map of the
      // patch
      auto exist = style_patch.update_styles_map_.find(old_map_item.first);
      if (exist != style_patch.update_styles_map_.end()) {
        style_patch.update_styles_map_.erase(exist);
      }

      if (is_dirty) {
        // When new_map is empty but style is dirty, all ids in old_map need to
        // be reset
        style_patch.reset_id_set_.insert(old_map_item.first);
      } else {
        // When new_map is empty and style is not dirty, all ids in old_map need
        // to be reserved
        style_patch.reserve_styles_map_.insert_or_assign(old_map_item.first,
                                                         old_map_item.second);
      }
    }
  } else {
    for (auto &new_map_item : new_map) {
      style_patch.reset_id_set_.erase(new_map_item.first);
      auto old_map_iterator = old_map.find(new_map_item.first);
      // Exists in new_map but not in old_map
      if (old_map_iterator == old_map.end() ||
          new_map_item.second != old_map_iterator->second) {
        // Delete those css ids that need to be reserved in low priority but
        // need to be update in high priority from the reserve_set of patch
        auto exist = style_patch.reserve_styles_map_.find(new_map_item.first);
        if (exist != style_patch.reserve_styles_map_.end()) {
          style_patch.reserve_styles_map_.erase(exist);
        }
        style_patch.update_styles_map_.insert_or_assign(new_map_item.first,
                                                        new_map_item.second);
        // Delete the css ids that have been processed, and determine which ones
        // need to be reset from the remaining ids
        old_map.erase(old_map_iterator);
      } else {
        // Delete those css ids that need to be updated in the low priority but
        // need to be reserved in the high priority from the update_map of the
        // patch
        auto exist =
            style_patch.update_styles_map_.find(old_map_iterator->first);
        if (exist != style_patch.update_styles_map_.end()) {
          style_patch.update_styles_map_.erase(exist);
        }
        // When new_map is empty, all ids in old_map need to be kept
        style_patch.reserve_styles_map_.insert_or_assign(
            old_map_iterator->first, old_map_iterator->second);
        // Delete the css ids that have been processed, and determine which ones
        // need to be reset from the remaining ids
        old_map.erase(old_map_iterator);
      }
    }
    // Determine the id that needs to be reset in this diff
    for (auto &old_map_item : old_map) {
      style_patch.reset_id_set_.insert(old_map_item.first);
    }
  }

  if (is_final) {
    // Handle the id appearing in reserve_map or update_map in reset_set.'reset'
    // has the lowest priority and needs to be removed from reset_set
    std::unordered_set<CSSPropertyID>::iterator iterator =
        style_patch.reset_id_set_.begin();
    while (iterator != style_patch.reset_id_set_.end()) {
      if (style_patch.update_styles_map_.find(*iterator) !=
          style_patch.update_styles_map_.end()) {
        style_patch.reset_id_set_.erase(iterator++);
      } else {
        auto reserve_iterator = style_patch.reserve_styles_map_.find(*iterator);
        if (reserve_iterator != style_patch.reserve_styles_map_.end()) {
          style_patch.update_styles_map_.insert_or_assign(
              *iterator, reserve_iterator->second);
          style_patch.reset_id_set_.erase(iterator++);
          style_patch.reserve_styles_map_.erase(reserve_iterator);
        } else {
          ++iterator;
        }
      }
    }
  }
}

void AirElement::RefreshStyles() {
  // Diff&merge the css StyleMap corresponding to global, tag, class, id and
  // inline in order to get stylePatch
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "AirElement::RefreshStyles");
  StylePatch style_patch;
  UpdateStylePatch(Selector::kSTABLE, style_patch);
  UpdateStylePatch(Selector::kCLASS, style_patch);
  UpdateStylePatch(Selector::kID, style_patch);
  UpdateStylePatch(Selector::kINLINE, style_patch);

  // Reset the css props in reset_id_set_
  PreparePropBundleIfNeed();
  if (!style_patch.reset_id_set_.empty()) {
    for (auto css_id : style_patch.reset_id_set_) {
      ResetStyle(css_id);
    }
  }
  // Update the css props in update_styles_map_
  if (!style_patch.update_styles_map_.empty()) {
    for (auto &style : style_patch.update_styles_map_) {
      SetStyle(style.first, style.second);
    }
  }

  inline_style_map_.clear();
  style_dirty_ = 0;
}

void AirElement::UpdateStylePatch(Selector selector,
                                  AirElement::StylePatch &style_patch) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "AirElement::UpdateStylePatch");
  // first screen & not style_dirty_
  if (!has_painting_node_ && !(style_dirty_ & selector)) {
    return;
  }
  StyleMap selector_styles;
  auto old_selector_styles_iterator = cur_css_styles_.find(selector);
  if (old_selector_styles_iterator == cur_css_styles_.end()) {
    // first screen
    GetStyleMap(selector, selector_styles);
    style_patch.update_styles_map_.insert(selector_styles.begin(),
                                          selector_styles.end());
  } else {
    // Decide whether to get a new styleMap through style_dirty_.
    //'SetClass', 'SetId' and 'SetInlineStyle' will change style_dirty_value
    if (style_dirty_ & selector) {
      GetStyleMap(selector, selector_styles);
    }
    // Diff old styleMap and new styleMap(may be empty), and update stylePatch
    DiffStyles(old_selector_styles_iterator->second, selector_styles,
               style_patch, selector == Selector::kINLINE,
               style_dirty_ & selector);
  }

  if (style_dirty_ & selector) {
    if (selector_styles.empty()) {
      cur_css_styles_.erase(selector);
    } else {
      cur_css_styles_.insert_or_assign(selector, selector_styles);
    }
  }
}
void AirElement::GetStyleMap(Selector selector, StyleMap &result) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "AirElement::GetStyleMap");
  switch (selector) {
    case Selector::kSTABLE:
      GetStableStyleMap(tag_.str(), result);
      break;
    case Selector::kCLASS:
      GetClassStyleMap(classes_, result);
      break;
    case Selector::kID:
      GetIdStyleMap(id_selector_, result);
      break;
    case Selector::kINLINE:
      result = inline_style_map_;
      break;
    default:
      LOGE("invalid css selector");
  }
}

void AirElement::GetStableStyleMap(const std::string &tag_name,
                                   StyleMap &result) {
  if (is_page() || is_component()) {
    static constexpr const char *kGlobal = "*";
    auto iterator = parsed_styles_.find(kGlobal);
    if (iterator != parsed_styles_.end()) {
      result = *(iterator->second);
    }
    if (!tag_name.empty()) {
      StyleMap tag_style_map;
      iterator = parsed_styles_.find(tag_name);
      if (iterator != parsed_styles_.end()) {
        tag_style_map = *(iterator->second);
      }
      MergeHigherPriorityCSSStyle(result, tag_style_map);
    }
  } else {
    AirElement *parent_component = GetParentComponent();
    if (parent_component) {
      parent_component->GetStableStyleMap(tag_name, result);
    }
  }
}

void AirElement::GetClassStyleMap(const ClassVector &class_list,
                                  StyleMap &result) {
  if (is_page() || is_component()) {
    for (const auto &class_name : class_list) {
      auto iterator = parsed_styles_.find("." + class_name);
      if (iterator != parsed_styles_.end()) {
        const auto &class_css_styles = *(iterator->second);
        MergeHigherPriorityCSSStyle(result, class_css_styles);
      }
    }
  } else {
    if (class_list.empty()) {
      return;
    }
    AirElement *parent_component = GetParentComponent();
    if (parent_component) {
      parent_component->GetClassStyleMap(class_list, result);
    }
  }
}

void AirElement::GetIdStyleMap(const std::string &id_name, StyleMap &result) {
  if (is_page() || is_component()) {
    auto iterator = parsed_styles_.find("#" + id_name);
    if (iterator != parsed_styles_.end()) {
      result = *(iterator->second);
    }
  } else {
    if (id_name.empty()) {
      return;
    }
    AirElement *parent_component = GetParentComponent();
    if (parent_component) {
      parent_component->GetIdStyleMap(id_name, result);
    }
  }
}

void AirElement::GetKeyframesMap(const std::string &keyframes_name,
                                 StyleMap &result) {
  if (keyframes_name.empty()) {
    return;
  }
  if (is_page() || is_component()) {
    auto iterator = parsed_styles_.find("$keyframes" + keyframes_name);
    if (iterator != parsed_styles_.end()) {
      result = *(iterator->second);
    }
  } else {
    AirElement *parent_component = GetParentComponent();
    if (parent_component) {
      parent_component->GetKeyframesMap(keyframes_name, result);
    }
  }
}

void AirElement::PushToPropsBundle(const base::String &key,
                                   const lepus::Value &value) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "AirElement::PushToPropsBundle");
  PreparePropBundleIfNeed();
  has_layout_only_props_ = false;
  prop_bundle_->SetProps(key.c_str(), pub::ValueImplLepus(value));
  dirty_ |= kDirtyAttr;
}

bool AirElement::AirComputedCSSStyle::Process(CSSPropertyID css_property_id,
                                              CSSValuePattern pattern,
                                              lepus::Value &value) {
  return ProcessWithPattern(pattern, value);
}

// this function is not used currently.
bool AirElement::AirComputedCSSStyle::ProcessWithPattern(
    CSSValuePattern attr_pattern, lepus::Value &attr_value) {
  // compute with density when pattern is px. eg:fontSize
  if (attr_pattern == CSSValuePattern::PX) {
    //    attr_value.SetNumber(attr_value.Number() * Config::Density());
    return true;
  } else if (attr_pattern == CSSValuePattern::RPX) {
    //    attr_value.SetNumber(attr_value.Number() * Config::Width() /
    //    kRpxRatio);
    return true;
  }
  return false;
}

}  // namespace tasm
}  // namespace lynx
