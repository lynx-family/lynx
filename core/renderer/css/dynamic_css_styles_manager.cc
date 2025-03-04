// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/css/dynamic_css_styles_manager.h"

#include <memory>
#include <string>
#include <unordered_set>

#include "base/include/no_destructor.h"
#include "core/build/gen/lynx_sub_error_code.h"
#include "core/renderer/css/css_style_utils.h"
#include "core/renderer/css/measure_context.h"
#include "core/renderer/dom/element.h"
#include "core/renderer/dom/element_manager.h"
#include "core/renderer/starlight/style/default_layout_style.h"
#include "core/renderer/ui_wrapper/layout/layout_node.h"

namespace lynx {
namespace tasm {
using starlight::DirectionType;
using starlight::TextAlignType;

namespace {

// Not that kPropertyIDFontSize, kPropertyIDLineSpacing,
// kPropertyIDLetterSpacing, kPropertyIDLineHeight are not simple inheritable
// props, should inherit computed style value for them.
const std::unordered_set<CSSPropertyID>& GetDefaultInheritableProps() {
  const static base::NoDestructor<std::unordered_set<CSSPropertyID>>
      kDefaultInheritableProps(std::unordered_set<CSSPropertyID>{
          kPropertyIDFontSize, kPropertyIDFontFamily, kPropertyIDTextAlign,
          kPropertyIDLineSpacing, kPropertyIDLetterSpacing,
          kPropertyIDLineHeight, kPropertyIDFontStyle, kPropertyIDFontWeight,
          kPropertyIDColor, kPropertyIDTextDecoration, kPropertyIDTextShadow,
          kPropertyIDDirection, kPropertyIDCursor});
  return *kDefaultInheritableProps;
}

const std::unordered_set<CSSPropertyID>& GetSimpleInheritableProps() {
  const static base::NoDestructor<std::unordered_set<CSSPropertyID>>
      kSimpleInheritableProps(std::unordered_set<CSSPropertyID>{
          kPropertyIDFontFamily, kPropertyIDTextAlign, kPropertyIDFontStyle,
          kPropertyIDFontWeight, kPropertyIDColor, kPropertyIDTextDecoration,
          kPropertyIDTextShadow, kPropertyIDDirection, kPropertyIDCursor});
  return *kSimpleInheritableProps;
}

const std::unordered_set<CSSPropertyID>& GetComplexDynamicProps() {
  const static base::NoDestructor<std::unordered_set<CSSPropertyID>>
      kComplexDynamicProps(std::unordered_set<CSSPropertyID>{
          kPropertyIDTransformOrigin,
          kPropertyIDBackgroundSize,
          kPropertyIDBackgroundPosition,
          kPropertyIDBorderRadius,
          kPropertyIDBorderTopLeftRadius,
          kPropertyIDBorderTopRightRadius,
          kPropertyIDBorderBottomLeftRadius,
          kPropertyIDBorderBottomRightRadius,
          kPropertyIDBorderStartStartRadius,
          kPropertyIDBorderStartEndRadius,
          kPropertyIDBorderEndEndRadius,
          kPropertyIDBorderEndStartRadius,
          kPropertyIDTransform,
          kPropertyIDBoxShadow,
          kPropertyIDTextShadow,
          kPropertyIDGridAutoRows,
          kPropertyIDGridAutoColumns,
          kPropertyIDGridTemplateRows,
          kPropertyIDGridTemplateColumns,
      });
  return *kComplexDynamicProps;
}

// FIXME(zhixuan): temproray optimize for proeprties that will disable flatten.
// Will be removed later。
const std::unordered_set<CSSPropertyID>& GetFlattenRelatedProps() {
#if defined(OS_ANDROID)
  const static base::NoDestructor<std::unordered_set<CSSPropertyID>>
      kFlattenRelatedProps(std::unordered_set<CSSPropertyID>{
          kPropertyIDTransformOrigin,
          kPropertyIDTransform,
          kPropertyIDBoxShadow,
      });
#else
  const static base::NoDestructor<std::unordered_set<CSSPropertyID>>
      kFlattenRelatedProps(std::unordered_set<CSSPropertyID>{});
#endif
  return *kFlattenRelatedProps;
}

inline DynamicCSSStylesManager::StyleUpdateFlags GetPercentDependency(
    CSSPropertyID id) {
  // Currently, only line-height and font size are supported to have behavior of
  // percentage unit
  if (id == kPropertyIDLineHeight) {
    return DynamicCSSStylesManager::kUpdateEm;
  }
  return DynamicCSSStylesManager::kNoUpdate;
}

inline DynamicCSSStylesManager::StyleUpdateFlags CheckFontScaleRelevance(
    CSSPropertyID id) {
  // Currently, only line-height and font size are supported to have behavior of
  // percentage unit
  if (id == kPropertyIDFontSize || id == kPropertyIDLineHeight ||
      id == kPropertyIDLetterSpacing) {
    return DynamicCSSStylesManager::kUpdateFontScale;
  }
  return DynamicCSSStylesManager::kNoUpdate;
}

inline DynamicCSSStylesManager::StyleUpdateFlags
GetPercentDependencyOfInheritedProp(CSSPropertyID id, const CSSValue& value) {
  // Suprisingly percentage line-height will be inherited by final computed
  // value. But number line-height will be inherited as raw value in W3C CSS.
  if (id == kPropertyIDLineHeight &&
      value.GetPattern() == CSSValuePattern::NUMBER) {
    return DynamicCSSStylesManager::kUpdateEm;
  }
  return DynamicCSSStylesManager::kNoUpdate;
}

DynamicCSSStylesManager::StyleUpdateFlags CleanInheritedFlags(
    DynamicCSSStylesManager::StyleUpdateFlags flags) {
  static constexpr DynamicCSSStylesManager::StyleUpdateFlags kInherited =
      DynamicCSSStylesManager::kUpdateEm;

  return flags & (~kInherited);
}

DynamicCSSStylesManager::StyleUpdateFlags GetStatusChanges(
    const PropertiesResolvingStatus& old_status,
    const PropertiesResolvingStatus& new_status) {
  DynamicCSSStylesManager::StyleUpdateFlags ret =
      DynamicCSSStylesManager::kNoUpdate;

  if (base::FloatsNotEqual(old_status.computed_font_size_,
                           new_status.computed_font_size_)) {
    ret |= DynamicCSSStylesManager::kUpdateEm;
  }
  if (base::FloatsNotEqual(old_status.page_status_.root_font_size_,
                           new_status.page_status_.root_font_size_)) {
    ret |= DynamicCSSStylesManager::kUpdateRem;
  }
  if (base::FloatsNotEqual(old_status.page_status_.screen_width_,
                           new_status.page_status_.screen_width_)) {
    ret |= DynamicCSSStylesManager::kUpdateScreenMetrics;
  }
  if (old_status.direction_type_ != new_status.direction_type_) {
    ret |= DynamicCSSStylesManager::kUpdateDirectionStyle;
  }
  // Font scale has high precision
  if (old_status.page_status_.font_scale_ !=
      new_status.page_status_.font_scale_) {
    ret |= DynamicCSSStylesManager::kUpdateFontScale;
  }
  // viewport size has high precision
  if (old_status.page_status_.viewport_width_ !=
          new_status.page_status_.viewport_width_ ||
      old_status.page_status_.viewport_height_ !=
          new_status.page_status_.viewport_height_) {
    ret |= DynamicCSSStylesManager::kUpdateViewport;
  }
  return ret;
}

}  // namespace

DynamicCSSStylesManager::DynamicCSSStylesManager(
    Element* element, const DynamicCSSConfigs& configs, float default_font_size)
    : element_(element), configs_(configs) {
  // To keep the default value behaviour after removing the global values used
  // in PropertiesResolvingStatus.
  resolving_data_.page_status_.root_font_size_ = default_font_size;
  resolving_data_.computed_font_size_ = default_font_size;
  status_for_child_.resolving_data.page_status_.root_font_size_ =
      default_font_size;
  status_for_child_.resolving_data.computed_font_size_ = default_font_size;
}

void DynamicCSSStylesManager::ForEachFlagDo(
    StyleUpdateFlags flags,
    const base::MoveOnlyClosure<void, std::map<CSSPropertyID, CSSValue>&>&
        func) {
  for (uint32_t i = 0; i < kDynamicTypeCount; i++) {
    if ((1 << i) & flags) {
      func(value_storage_[i]);
    }
  }
}

const std::unordered_set<CSSPropertyID>&
DynamicCSSStylesManager::GetInheritableProps() {
  return GetDefaultInheritableProps();
}

void DynamicCSSStylesManager::UpdateFontSizeStyle(const tasm::CSSValue* value) {
  if (value && *value != font_size_) {
    font_size_ = *value;
    font_size_flags_ = GetValueFlags(kPropertyIDFontSize, font_size_,
                                     configs_.unify_vw_vh_behavior_);
    font_size_need_update_ = true;
    MarkDirtyRecursively();
    if (element_->element_manager()) {
      element_->element_manager()->SetNeedsLayout();
    }
  }
}

void DynamicCSSStylesManager::UpdateDirectionStyle(
    const tasm::CSSValue& value) {
  if (direction_ != value) {
    direction_ = value;
    direction_need_update_ = true;
    MarkDirtyRecursively();
  }
}

void DynamicCSSStylesManager::AdoptStyle(CSSPropertyID css_id,
                                         const tasm::CSSValue& value) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "DynamicCSSStylesManager::AdoptStyle");
  if (element_->element_manager() && (LayoutNode::IsLayoutOnly(css_id) ||
                                      LayoutNode::IsLayoutWanted(css_id))) {
    element_->element_manager()->SetNeedsLayout();
  }

  if (css_id == kPropertyIDFontSize) {
    LOGE("Font-size was passed to adopt style!");
  }

  auto entry = flag_maps_.find(css_id);
  if (entry != flag_maps_.end()) {
    ForEachFlagDo(entry->second,
                  [&css_id](std::map<CSSPropertyID, CSSValue>& storage) {
                    storage.erase(css_id);
                  });
    must_updates_.erase(css_id);
    MarkDirtyRecursively();
  }
  if (value.IsEmpty()) {
    ResetStyleToElement(css_id);
    if (configs_.enable_css_inheritance_ && IsInheritable(css_id)) {
      auto entry = inheritable_props_.find(css_id);
      if (entry != inheritable_props_.end()) {
        entry->second = InheritablePropsState{value, true};
        MarkDirtyRecursively();
      }
    }
    return;
  }
  StyleUpdateFlags new_flags =
      GetValueFlags(css_id, value, configs_.unify_vw_vh_behavior_);

  if (new_flags != kNoUpdate) {
    // The computation and of config related properties will be postponed util
    // diff finish to avoid unnecessary processing
    ForEachFlagDo(new_flags, [&css_id, &value](ValueStorage& storage) {
      storage.emplace(css_id, value);
    });
    flag_maps_.emplace(css_id, new_flags);

    // TODO(wangzhixuan.0821): Remove the following test for
    // GetFlattenRelatedProps when once inheritance is proved to be stable.
    if (!IsLogicalDirectionStyle(css_id) &&
        (!configs_.enable_css_inheritance_ ||
         GetFlattenRelatedProps().count(css_id))) {
      element_->SetStyleInternal(css_id, value);
    } else {
      must_updates_[css_id] =
          std::pair<CSSValue, StyleUpdateFlags>(value, new_flags);
      MarkDirtyRecursively();
    }

  } else {
    // Update the style now if the property is resolving status irrelevant
    element_->SetStyleInternal(css_id, value);
  }
  if (configs_.enable_css_inheritance_ && IsInheritable(css_id)) {
    inheritable_props_[css_id] = InheritablePropsState{value, true};
    MarkDirtyRecursively();
  }
}

void DynamicCSSStylesManager::MarkNewlyInserted() {
  force_reapply_inheritance_ = true;
  dirty_ = true;
  if (element_->parent() && DynamicCSSConfigs::GetDefaultDynamicCSSConfigs()
                                .OnceInheritanceDisabled()) {
    element_->parent()->StylesManager().MarkDirtyRecursively();
  }
}

void DynamicCSSStylesManager::MarkDirtyRecursively() {
  // TODO(wangzhixuan): remove this function when the new dynamic css process is
  // validated.
  if (!dirty_) {
    dirty_ = true;
    if (element_->parent()) {
      element_->parent()->StylesManager().MarkDirtyRecursively();
    }
  }
}

void DynamicCSSStylesManager::ApplyDirection(
    const PropertiesResolvingStatus& status, StyleUpdateFlags& current_updates,
    PropertiesResolvingStatus& next_resolving_data) {
  DirectionType new_direction_type = status.direction_type_;
  if (!direction_.IsEmpty()) {
    new_direction_type = direction_.GetEnum<DirectionType>();
  }

  DirectionType old_direction_type = element_->Direction();
  // clear flag
  current_updates &= ~kUpdateDirectionStyle;
  if (old_direction_type != new_direction_type) {
    current_updates |= kUpdateDirectionStyle;
    // call before SetDirectionInternal.
    ResetAllDirectionAwareProperty();
  }

  if (direction_need_update_ || (current_updates & kUpdateDirectionStyle)) {
    UpdateDirectionAwareDefaultStyles(element_, new_direction_type);
    element_->SetDirectionInternal(
        CSSValue(lepus_value(static_cast<int>(new_direction_type)),
                 CSSValuePattern::ENUM));
  }

  if (configs_.enable_css_inheritance_ && IsInheritable(kPropertyIDDirection)) {
    next_resolving_data.direction_type_ = new_direction_type;
  }
}

void DynamicCSSStylesManager::ApplyFontSizeUpdateResolvingData(
    const PropertiesResolvingStatus& status, StyleUpdateFlags& current_updates,
    PropertiesResolvingStatus& next_resolving_data, const Element* parent) {
  // high piorities
  float new_font_size = element_->GetFontSize();
  const auto& lynx_env = element_->element_manager()->GetLynxEnvConfig();
  if (!font_size_.IsEmpty()) {
    if (font_size_need_update_ || (current_updates & font_size_flags_)) {
      std::optional<float> resolved_font_size;
      auto& configs = element_->element_manager()->GetCSSParserConfigs();
      const auto& vw_base =
          configs_.unify_vw_vh_behavior_
              ? lynx_env.ViewportWidth()
              : vwbase_for_font_size_to_align_with_legacy_bug_;
      const auto& vh_base =
          configs_.unify_vw_vh_behavior_
              ? lynx_env.ViewportHeight()
              : vhbase_for_font_size_to_align_with_legacy_bug_;

      resolved_font_size = starlight::CSSStyleUtils::ResolveFontSize(
          font_size_, lynx_env, vw_base, vh_base, status.computed_font_size_,
          status.page_status_.root_font_size_, configs);

      if (resolved_font_size.has_value()) {
        new_font_size = *resolved_font_size;
      }
    }
  } else {
    new_font_size = status.computed_font_size_;
  }

  current_updates = CleanInheritedFlags(current_updates);
  if (base::FloatsNotEqual(new_font_size, element_->GetFontSize())) {
    current_updates |= kUpdateEm;
  }
  next_resolving_data.computed_font_size_ = new_font_size;

  if (!parent) {
    next_resolving_data.page_status_.root_font_size_ = new_font_size;
    if (base::FloatsNotEqual(new_font_size,
                             element_->GetRecordedRootFontSize())) {
      current_updates |= kUpdateRem;
    }
  }

  if (font_size_need_update_ || (current_updates & kUpdateEm) ||
      (current_updates & kUpdateRem) || (current_updates & kUpdateFontScale)) {
    // CSSValue is not relevant to what is actually set for font size.
    element_->SetComputedFontSize(
        CSSValue(lepus_value(next_resolving_data.computed_font_size_),
                 CSSValuePattern::NUMBER),
        next_resolving_data.computed_font_size_,
        next_resolving_data.page_status_.root_font_size_);
  } else {
    next_resolving_data.computed_font_size_ = element_->GetFontSize();
    next_resolving_data.page_status_.root_font_size_ =
        element_->GetRecordedRootFontSize();
  }
  if (!configs_.enable_css_inheritance_ ||
      !IsInheritable(kPropertyIDFontSize)) {
    next_resolving_data.computed_font_size_ = lynx_env.PageDefaultFontSize();
  }
}

bool DynamicCSSStylesManager::UpdateWithResolvingStatus(
    const StatusForChild& status, const Element* parent) {
  // When a node is forcely applied inheritance, the entire subtree of the node
  // should be forcely applied inheritance as well.
  TRACE_EVENT(LYNX_TRACE_CATEGORY,
              "DynamicCSSStylesManager::UpdateWithResolvingStatus");
  LynxFatal(element_, error::E_ELEMENT_UPDATE_NODE_IS_NULL,
            "Element is null when updating resolving status");
  auto current_updates =
      GetStatusChanges(status.resolving_data, resolving_data_);
  if (current_updates == kNoUpdate && !dirty_ && !status.inherit_result.first) {
    status_for_child_.Clear();
    return false;
  }
  if (current_updates & kUpdateFontScale) {
    element_->computed_css_style()->SetFontScale(
        status.resolving_data.page_status_.font_scale_);
  }
  if (current_updates & kUpdateScreenMetrics) {
    element_->computed_css_style()->SetScreenWidth(
        status.resolving_data.page_status_.screen_width_);
  }
  if (current_updates & kUpdateViewport) {
    element_->computed_css_style()->SetViewportWidth(
        status.resolving_data.page_status_.viewport_width_);
    element_->computed_css_style()->SetViewportHeight(
        status.resolving_data.page_status_.viewport_height_);
  }

  status_for_child_ = status;
  status_for_child_.force_apply_inheritance =
      (status.force_apply_inheritance || force_reapply_inheritance_);

  resolving_data_ = status.resolving_data;
  status_for_child_.resolving_data.ApplyPageStatus(status.resolving_data);

  ApplyFontSizeUpdateResolvingData(status.resolving_data, current_updates,
                                   status_for_child_.resolving_data, parent);
  ApplyDirection(status.resolving_data, current_updates,
                 status_for_child_.resolving_data);
  status_for_child_.inherit_result = ApplyInheritance(
      status.inherit_result.second, status.inherit_result.first,
      current_updates, status_for_child_.force_apply_inheritance, parent);

  for (const auto& i : must_updates_) {
    if (!(i.second.second & current_updates)) {
      SetStyleToElement(i.first, i.second.first);
    }
  }
  ForEachFlagDo(current_updates,
                [this](std::map<CSSPropertyID, CSSValue>& storage) {
                  for (auto style : storage) {
                    SetStyleToElement(style.first, style.second);
                  }
                });
  UpdatePlaceHolderStyle(current_updates);

  must_updates_.clear();

  ClearDirtyFlags();
  return true;
}

std::pair<bool, DynamicCSSStylesManager::InheritedProps>
DynamicCSSStylesManager::ApplyInheritance(
    const DynamicCSSStylesManager::InheritedProps& props, bool was_dirty,
    StyleUpdateFlags env_changes, bool force_apply_inheritance,
    const Element* parent) {
  std::pair<bool, InheritedProps> result;
  const auto& ApplyProps =
      [this, env_changes, force_apply_inheritance,
       parent](const std::map<CSSPropertyID, InheritablePropsState>& props) {
        bool has_dirty = false;
        for (const auto& prop : props) {
          const bool is_inherited_prop_affected_by_env =
              (GetPercentDependencyOfInheritedProp(prop.first,
                                                   prop.second.value_) &
               env_changes);
          const bool is_inherited_prop_affected_by_direction =
              (IsDirectionAwareStyle(prop.first) &&
               (env_changes & kUpdateDirectionStyle));

          // When node is newly inserted to inheritance tree. The css
          // inheritance needs to be forcely applied to this node.
          if (prop.second.dirty_ || is_inherited_prop_affected_by_env ||
              is_inherited_prop_affected_by_direction ||
              force_apply_inheritance) {
            has_dirty = prop.second.dirty_ || has_dirty;

            // if inherited property changed, or property is inherited by
            // percent and percent base changed
            if (prop.second.value_.IsEmpty()) {
              // if Inherited property got removed
              DCHECK(prop.second.dirty_);
              ResetStyleToElement(prop.first);
            } else {
              // if property needs update
              const bool is_style_newly_added =
                  inheritable_props_.find(prop.first) !=
                  inheritable_props_.end();
              if (is_style_newly_added ||
                  IsPropertySimpleInheritable(prop.first, prop.second.value_)) {
                // if property is newly added o simple inheritable or
                SetStyleToElement(prop.first, prop.second.value_);
              } else if (parent) {
                // If the property is inherited by computed value.
                const bool value_changed =
                    element_->computed_css_style()->InheritValue(
                        prop.first, *(parent->computed_css_style()));
                if (value_changed || is_inherited_prop_affected_by_env) {
                  element_->PreparePropBundleIfNeed();
                  element_->PushToBundle(prop.first);
                }
              }
            }
          }
        }
        return has_dirty;
      };

  if (inheritable_props_.empty()) {
    result.first = was_dirty || force_apply_inheritance;
    result.second.Inherit(props);
    if (was_dirty || env_changes || force_apply_inheritance) {
      ApplyProps(props.Get());
    }

    return result;
  }

  auto new_inherit_root = std::make_shared<InheritMap>(props.Get());
  for (auto it = inheritable_props_.begin(); it != inheritable_props_.end();) {
    auto next = it;
    ++next;
    auto entry = new_inherit_root->find(it->first);
    if (entry == new_inherit_root->end()) {
      new_inherit_root->insert(*it);
    } else {
      if (it->second.value_.IsEmpty()) {
        entry->second.dirty_ = true;
      } else {
        entry->second = it->second;
      }
    }
    if (it->second.value_.IsEmpty()) {
      inheritable_props_.erase(it);
    } else {
      it->second.dirty_ = false;
    }
    it = next;
  }
  result.first = ApplyProps(*new_inherit_root) || force_apply_inheritance;
  result.second = InheritedProps(std::move(new_inherit_root));
  return result;
}

bool DynamicCSSStylesManager::IsInheritable(CSSPropertyID id) const {
  if (!configs_.custom_inherit_list_.empty()) {
    return configs_.custom_inherit_list_.count(id);
  }
  return GetDefaultInheritableProps().count(id);
}

void DynamicCSSStylesManager::ResetAllDirectionAwareProperty() {
  for (const auto& entry : value_storage_[kDirectionStyleType]) {
    ResetStyleToElement(entry.first);
  }
}

void DynamicCSSStylesManager::SetStyleToElement(CSSPropertyID id,
                                                const CSSValue& css_value,
                                                bool force_update) {
  CSSPropertyID trans_id = id;

  // special case.
  if (trans_id == kPropertyIDTextAlign) {
    CSSStyleValue style_type =
        ResolveTextAlign(trans_id, css_value, element_->Direction());
    element_->SetStyleInternal(style_type.first, style_type.second);
    return;
  }

  element_->RecordElementPreviousStyle(id, css_value);
  // 1.start ---> left/right
  // 2.rtl:left/right ---> right/left
  trans_id = ResolveDirectionAwareProperty(id, element_->Direction());
  element_->SetStyleInternal(trans_id, css_value, force_update);
}

void DynamicCSSStylesManager::ResetStyleToElement(CSSPropertyID id) {
  element_->ResetElementPreviousStyle(id);
  CSSPropertyID trans_id = id;
  trans_id = ResolveDirectionAwareProperty(id, element_->Direction());
  element_->ResetStyleInternal(trans_id);
}

// static
bool DynamicCSSStylesManager::CheckIsDirectionAwareStyle(CSSPropertyID css_id) {
  return IsDirectionAwareStyle(css_id);
}

// static
CSSPropertyID DynamicCSSStylesManager::ResolveDirectionAwarePropertyID(
    CSSPropertyID id, starlight::DirectionType direction) {
  return ResolveDirectionAwareProperty(id, direction);
}

// static
std::pair<CSSPropertyID, IsLogic>
DynamicCSSStylesManager::ResolveLogicPropertyID(CSSPropertyID id) {
  return ResolveLogicStyleID(id);
}

// static
CSSPropertyID DynamicCSSStylesManager::ResolveDirectionRelatedPropertyID(
    CSSPropertyID id, starlight::DirectionType direction,
    IsLogic is_logic_style) {
  return ResolveDirectionRelatedStyleID(id, direction, is_logic_style);
}

// static
void DynamicCSSStylesManager::UpdateDirectionAwareDefaultStyles(
    Element* element, starlight::DirectionType direction) {
  // Currently, only text-align has direction aware default property.
  const auto default_align = ResolveTextAlign(
      kPropertyIDTextAlign,
      CSSValue(lepus::Value(static_cast<int32_t>(TextAlignType::kStart)),
               CSSValuePattern::ENUM),
      direction);
  element->SetStyleInternal(default_align.first, default_align.second);
}

// static
// TODO(zhouzhitao): unify logic with radon element, remove this overwritten
// version of UpdateDirectionAwareDefaultStyles
void DynamicCSSStylesManager::UpdateDirectionAwareDefaultStyles(
    Element* element, starlight::DirectionType direction,
    const CSSValue& text_align_value) {
  // Currently, only text-align has direction aware default property.
  auto default_align_value = text_align_value;

  if (default_align_value.IsEmpty()) {
    default_align_value =
        CSSValue(lepus::Value(static_cast<int32_t>(TextAlignType::kStart)),
                 CSSValuePattern::ENUM);
  }
  const auto default_align =
      ResolveTextAlign(kPropertyIDTextAlign, default_align_value, direction);
  element->SetStyleInternal(kPropertyIDTextAlign, default_align.second);
}

// static
bool DynamicCSSStylesManager::IsPropertySimpleInheritable(
    CSSPropertyID id, const CSSValue& value) {
  if (value.IsEnum() || value.IsBoolean()) {
    return true;
  }
  return GetSimpleInheritableProps().count(id);
}

bool DynamicCSSStylesManager::IsPropertyComplexInheritable(
    CSSPropertyID id, const CSSValue& value) {
  return GetComplexDynamicProps().count(id);
}

DynamicCSSStylesManager::StyleUpdateFlags
DynamicCSSStylesManager::GetValueFlags(CSSPropertyID id, const CSSValue& value,
                                       bool unify_vw_vh_behavior) {
  DynamicCSSStylesManager::StyleUpdateFlags flags =
      DynamicCSSStylesManager::kNoUpdate;

  switch (value.GetPattern()) {
    case CSSValuePattern::EMPTY:
    case CSSValuePattern::ENUM:
      break;
    case CSSValuePattern::RPX:
      flags = DynamicCSSStylesManager::kUpdateScreenMetrics;
    case CSSValuePattern::PX:
    case CSSValuePattern::PPX:
      flags |= CheckFontScaleRelevance(id);
      break;
    case CSSValuePattern::PERCENT:
    case CSSValuePattern::NUMBER:
      flags = GetPercentDependency(id);
      flags |= CheckFontScaleRelevance(id);
      break;
    case CSSValuePattern::REM:
      flags = DynamicCSSStylesManager::kUpdateRem;
      break;
    case CSSValuePattern::EM:
      flags = DynamicCSSStylesManager::kUpdateEm;
      break;
    case CSSValuePattern::VW:
    case CSSValuePattern::VH:
      flags = DynamicCSSStylesManager::kUpdateViewport;
      break;
    case CSSValuePattern::CALC: {
      const auto& calc_str = value.GetValue().StdString();
      if (calc_str.find("rpx") != std::string::npos) {
        flags |= DynamicCSSStylesManager::kUpdateScreenMetrics;
        flags |= CheckFontScaleRelevance(id);
      }
      if (calc_str.find("em") != std::string::npos) {
        flags |= DynamicCSSStylesManager::kUpdateEm;
      }
      if (calc_str.find("rem") != std::string::npos) {
        flags |= DynamicCSSStylesManager::kUpdateRem;
      }
      if (calc_str.find("%") != std::string::npos) {
        flags |= GetPercentDependency(id);
      }
      if (calc_str.find("px") != std::string::npos ||
          calc_str.find("ppx") != std::string::npos) {
        flags |= CheckFontScaleRelevance(id);
      }
      if (calc_str.find("vw") != std::string::npos ||
          calc_str.find("vh") != std::string::npos ||
          calc_str.find("view_width") != std::string::npos ||
          calc_str.find("view_height") != std::string::npos) {
        flags |= DynamicCSSStylesManager::kUpdateViewport;
      }
      if (calc_str.find("sp") != std::string::npos) {
        flags |= DynamicCSSStylesManager::kUpdateFontScale;
      }
      break;
    }
    case CSSValuePattern::ENV:
      // TODO:
      break;
    case CSSValuePattern::SP:
      flags |= DynamicCSSStylesManager::kUpdateFontScale;
    default:
      // TODO: Currently always recompute complex properties, we can
      // struturelize the properties before passing it to computed style in
      // future.
      if (GetComplexDynamicProps().count(id)) {
        flags = DynamicCSSStylesManager::kUpdateScreenMetrics |
                DynamicCSSStylesManager::kUpdateEm |
                DynamicCSSStylesManager::kUpdateRem |
                DynamicCSSStylesManager::kUpdateFontScale;
        if (unify_vw_vh_behavior) {
          flags = (flags | DynamicCSSStylesManager::kUpdateViewport);
        }
      }
      break;
  }
  if (IsDirectionAwareStyle(id)) {
    flags |= DynamicCSSStylesManager::kUpdateDirectionStyle;
  }

  return flags;
}

void DynamicCSSStylesManager::SetPlaceHolderStyle(
    const PseudoPlaceHolderStyles& styles) {
  element_->SetPlaceHolderStylesInternal(styles);
  placeholder_styles_ = styles;
}

void DynamicCSSStylesManager::UpdatePlaceHolderStyle(
    StyleUpdateFlags current_updates) {
  if (placeholder_styles_.font_size_.has_value()) {
    const auto flags =
        GetValueFlags(kPropertyIDFontSize, *placeholder_styles_.font_size_,
                      configs_.unify_vw_vh_behavior_);
    if (flags & current_updates) {
      element_->SetPlaceHolderStylesInternal(placeholder_styles_);
    }
  }
}

void DynamicCSSStylesManager::SetViewportSizeWhenInitialize(
    const LynxEnvConfig& config) {
  vwbase_for_font_size_to_align_with_legacy_bug_ =
      config.ViewportWidth().IsDefinite()
          ? config.ViewportWidth()
          : starlight::LayoutUnit(config.ScreenWidth());
  vhbase_for_font_size_to_align_with_legacy_bug_ =
      config.ViewportHeight().IsDefinite()
          ? config.ViewportHeight()
          : starlight::LayoutUnit(config.ScreenHeight());
}

bool DynamicCSSStylesManager::UpdateWithParentStatus(const Element* parent) {
  // ATTENSION: The element_->parent() does not necessarily return the actual
  // parent of element because UpdateWithParentStatus Maybe called while the
  // element tree is being constructed. Always use the passed in parent to get
  // the parent of current element.
  if (parent) {
    auto& parent_manager = parent->StylesManager();
    return UpdateWithResolvingStatus(parent_manager.status_for_child_, parent);
  } else {
    // At root node
    const auto& env_config = element_->element_manager()->GetLynxEnvConfig();
    element_->computed_css_style()->SetScreenWidth(env_config.ScreenWidth());
    element_->computed_css_style()->SetFontScale(env_config.FontScale());

    element_->computed_css_style()->SetViewportWidth(
        env_config.ViewportWidth());
    element_->computed_css_style()->SetViewportHeight(
        env_config.ViewportHeight());

    StatusForChild status(element_->GenerateRootPropertyStatus());
    return UpdateWithResolvingStatus(status, nullptr);
  }
}

}  // namespace tasm
}  // namespace lynx
