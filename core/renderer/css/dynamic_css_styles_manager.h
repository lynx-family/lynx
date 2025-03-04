// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_CSS_DYNAMIC_CSS_STYLES_MANAGER_H_
#define CORE_RENDERER_CSS_DYNAMIC_CSS_STYLES_MANAGER_H_

#include <array>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <unordered_set>
#include <utility>

#include "core/renderer/css/computed_css_style.h"
#include "core/renderer/css/css_property.h"
#include "core/renderer/css/css_value.h"
#include "core/renderer/css/dynamic_css_configs.h"
#include "core/renderer/css/dynamic_direction_styles_manager.h"
#include "core/renderer/css/measure_context.h"
#include "core/renderer/starlight/style/default_layout_style.h"
#include "core/renderer/starlight/types/layout_configs.h"

namespace lynx {
namespace tasm {

struct PseudoPlaceHolderStyles {
  std::optional<CSSValue> font_size_;
  std::optional<CSSValue> color_;
  std::optional<CSSValue> font_weight_;
  std::optional<CSSValue> font_family_;
};

class Element;
class LayoutNode;

struct PropertiesResolvingStatus {
  // per page status
  struct PageStatus {
    float root_font_size_;
    float font_scale_ = Config::DefaultFontScale();
    starlight::LayoutUnit viewport_width_;
    starlight::LayoutUnit viewport_height_;
    float screen_width_ = 0.f;
  };

  PageStatus page_status_;

  // per element status
  float computed_font_size_;
  starlight::DirectionType direction_type_ =
      starlight::DefaultLayoutStyle::SL_DEFAULT_DIRECTION;

  void ApplyPageStatus(const PropertiesResolvingStatus& status) {
    page_status_ = status.page_status_;
  }
};

class DynamicCSSStylesManager {
 private:
  enum StyleDynamicType : uint32_t {
    kEmType = 0,
    kRemType = 1,
    kScreenMetricsType = 2,
    kDirectionStyleType = 3,
    kFontScaleType = 4,
    kViewportType = 5,
    kDynamicTypeCount = 6
  };

 public:
  DynamicCSSStylesManager(Element* element, const DynamicCSSConfigs& configs,
                          float default_font_size);

  enum StyleUpdateFlag : uint32_t {
    kUpdateEm = 1 << kEmType,
    kUpdateRem = 1 << kRemType,
    kUpdateScreenMetrics = 1 << kScreenMetricsType,
    kUpdateDirectionStyle = 1 << kDirectionStyleType,
    kUpdateFontScale = 1 << kFontScaleType,
    kUpdateViewport = 1 << kViewportType,
  };

  static constexpr uint32_t kAllStyleUpdate =
      kUpdateEm | kUpdateRem | kUpdateScreenMetrics | kUpdateDirectionStyle |
      kUpdateFontScale | kUpdateViewport;

  static constexpr uint32_t kNoUpdate = 0;

  using StyleUpdateFlags = uint32_t;

  static const std::unordered_set<CSSPropertyID>& GetInheritableProps();

  static bool CheckIsDirectionAwareStyle(CSSPropertyID css_id);

  static CSSPropertyID ResolveDirectionAwarePropertyID(
      CSSPropertyID id, starlight::DirectionType direction);

  static std::pair<CSSPropertyID, IsLogic> ResolveLogicPropertyID(
      CSSPropertyID id);
  static CSSPropertyID ResolveDirectionRelatedPropertyID(
      CSSPropertyID id, starlight::DirectionType direction,
      IsLogic is_logic_style);
  static void UpdateDirectionAwareDefaultStyles(
      Element* element, starlight::DirectionType direction);
  static void UpdateDirectionAwareDefaultStyles(
      Element* element, starlight::DirectionType direction,
      const CSSValue& text_align_value);

  static bool IsPropertySimpleInheritable(CSSPropertyID id,
                                          const CSSValue& value);

  static bool IsPropertyComplexInheritable(CSSPropertyID id,
                                           const CSSValue& value);

  static StyleUpdateFlags GetValueFlags(CSSPropertyID id, const CSSValue& value,
                                        bool unify_vw_vh_behavior);

  void SetInitialResolvingStatus(const PropertiesResolvingStatus& status) {
    resolving_data_ = status;
  }
  void AdoptStyle(CSSPropertyID id, const tasm::CSSValue& value);

  void SetPlaceHolderStyle(const PseudoPlaceHolderStyles& styles);

  void UpdateFontSizeStyle(const tasm::CSSValue* value);
  void UpdateDirectionStyle(const tasm::CSSValue& value);

  bool UpdateWithParentStatus(const Element* parent);
  void UpdateWithParentStatusForOnceInheritance(const Element* parent) {
    if (!DynamicCSSConfigs::GetDefaultDynamicCSSConfigs()
             .OnceInheritanceDisabled()) {
      UpdateWithParentStatus(parent);
    }
  }

  void MarkNewlyInserted();

  void ClearChildrenStatus() { status_for_child_.Clear(); }

  // Weird function to keep old buggy behvior;
  void SetViewportSizeWhenInitialize(const LynxEnvConfig& config);
  const starlight::LayoutUnit& vwbase_for_font_size_to_align_with_legacy_bug()
      const {
    return vwbase_for_font_size_to_align_with_legacy_bug_;
  }
  const starlight::LayoutUnit& vhbase_for_font_size_to_align_with_legacy_bug()
      const {
    return vhbase_for_font_size_to_align_with_legacy_bug_;
  }

 private:
  void MarkDirtyRecursively();
  void ClearDirtyFlags() {
    font_size_need_update_ = false;
    direction_need_update_ = false;
    force_reapply_inheritance_ = false;
    dirty_ = false;
  }

  using FlagsMap = std::map<CSSPropertyID, StyleUpdateFlags>;
  using ValueStorage = std::map<CSSPropertyID, CSSValue>;
  struct InheritablePropsState {
    CSSValue value_;
    bool dirty_;
  };
  using InheritMap = std::map<CSSPropertyID, InheritablePropsState>;
  class InheritedProps {
   public:
    InheritedProps() { inherited_props_ = std::make_shared<InheritMap>(); }
    InheritedProps(const InheritedProps& other) = default;
    InheritedProps(InheritedProps&& other) = default;
    InheritedProps& operator=(InheritedProps&&) = default;
    InheritedProps& operator=(const InheritedProps&) = default;

    InheritedProps(std::shared_ptr<InheritMap>&& to_be_inherited)
        : inherited_props_(std::move(to_be_inherited)) {}
    void Inherit(const InheritedProps& to_be_inherited) {
      inherited_props_ = to_be_inherited.inherited_props_;
    }

    const InheritMap& Get() const { return *inherited_props_; }

   private:
    //        std::map<CSSPropertyID, InheritablePropsState> data_;
    std::shared_ptr<InheritMap> inherited_props_ = nullptr;
  };

  struct StatusForChild {
    StatusForChild() = default;
    StatusForChild(const PropertiesResolvingStatus& parent_status)
        : resolving_data(parent_status) {}
    StatusForChild& operator=(const StatusForChild&) = default;
    void Clear() {
      inherit_result.first = false;
      force_apply_inheritance = false;
    }
    PropertiesResolvingStatus resolving_data;
    std::pair<bool, DynamicCSSStylesManager::InheritedProps> inherit_result = {
        false, DynamicCSSStylesManager::InheritedProps()};
    bool force_apply_inheritance = false;
  };

  bool IsInheritable(CSSPropertyID id) const;

  bool UpdateWithResolvingStatus(const StatusForChild& status,
                                 const Element* parent_element);
  std::pair<bool, InheritedProps> ApplyInheritance(const InheritedProps& props,
                                                   bool was_dirty,
                                                   StyleUpdateFlags env_changes,
                                                   bool force_apply_inheritance,
                                                   const Element* parent);

  void ApplyDirection(const PropertiesResolvingStatus& status,
                      StyleUpdateFlags& current_updates,
                      PropertiesResolvingStatus& next_resolving_data);

  void ApplyFontSizeUpdateResolvingData(
      const PropertiesResolvingStatus& status,
      StyleUpdateFlags& current_updates,
      PropertiesResolvingStatus& next_resolving_data, const Element* parent);

  void UpdatePlaceHolderStyle(StyleUpdateFlags current_updates);

  void ForEachFlagDo(
      StyleUpdateFlags flags,
      const base::MoveOnlyClosure<void, std::map<CSSPropertyID, CSSValue>&>&
          func);
  void ResetAllDirectionAwareProperty();
  void SetStyleToElement(CSSPropertyID id, const CSSValue& css_value,
                         bool force_update = false);
  void ResetStyleToElement(CSSPropertyID id);

  // Assuming each of the field will contains only a few styles
  FlagsMap flag_maps_;
  std::map<CSSPropertyID, std::pair<CSSValue, StyleUpdateFlags>> must_updates_;
  std::array<ValueStorage, kDynamicTypeCount> value_storage_;
  std::map<CSSPropertyID, InheritablePropsState> inheritable_props_;
  PropertiesResolvingStatus resolving_data_;
  Element* element_;
  CSSValue font_size_ = CSSValue::Empty();
  StyleUpdateFlags font_size_flags_ = kNoUpdate;
  bool font_size_need_update_ = false;
  bool dirty_ = true;
  const DynamicCSSConfigs& configs_;

  // direction aware style
  bool direction_need_update_ = false;
  CSSValue direction_ = CSSValue::Empty();
  PseudoPlaceHolderStyles placeholder_styles_;
  bool force_reapply_inheritance_ = true;

  // The code is ugly. Make all the buggy behavior we have to keep!!!!!!
  starlight::LayoutUnit vwbase_for_font_size_to_align_with_legacy_bug_;
  starlight::LayoutUnit vhbase_for_font_size_to_align_with_legacy_bug_;

  StatusForChild status_for_child_;
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_CSS_DYNAMIC_CSS_STYLES_MANAGER_H_
