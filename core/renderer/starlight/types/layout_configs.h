// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_STARLIGHT_TYPES_LAYOUT_CONFIGS_H_
#define CORE_RENDERER_STARLIGHT_TYPES_LAYOUT_CONFIGS_H_

#include <string>

#include "base/include/version_util.h"

namespace lynx {

constexpr base::Version kQuirksModeEnableVersion(1, 5);
constexpr base::Version kQuirksModeDisableVersion(1, 6);
constexpr base::Version kFlexAlignFixedVersion(2, 9);
constexpr base::Version kFlexWrapFixedVersion(2, 10);
constexpr base::Version kGridPreLayoutFixedVersion(2, 12);
constexpr base::Version kFlexWrapExtraLineFixedVersion(2, 12);
constexpr base::Version kBaselineSupportVerticalDisplayFixedVersion(2, 12);
constexpr base::Version kFlexWrapCrossSizeFixedVersion(2, 13);
constexpr base::Version kFlexIndefinitePercentageFixedVersion(2, 13);
constexpr base::Version kFlexAutoMarginFixedVersion(2, 14);
constexpr base::Version kAbsoluteAndFixedBoxInfoFixedVersion(2, 16);
constexpr base::Version kGridNewVersion(3, 1);

namespace starlight {

struct LayoutConfigs {
  void SetQuirksMode(const base::Version& version) {
    quirks_mode_ = version;
    is_full_quirks_mode_ = !IsVersionHigherOrEqual(kQuirksModeDisableVersion);
    is_flex_align_quirks_mode_ =
        !IsVersionHigherOrEqual(kFlexAlignFixedVersion);
    is_flex_wrap_quirks_mode_ = !IsVersionHigherOrEqual(kFlexWrapFixedVersion);
    is_grid_pre_layout_quirks_mode_ =
        !IsVersionHigherOrEqual(kGridPreLayoutFixedVersion);
    is_flex_wrap_extra_line_quirks_mode_ =
        !IsVersionHigherOrEqual(kFlexWrapExtraLineFixedVersion);
    is_baseline_support_vertical_display_quirks_mode_ =
        !IsVersionHigherOrEqual(kBaselineSupportVerticalDisplayFixedVersion);
    is_flex_wrap_cross_size_quirks_mode_ =
        !IsVersionHigherOrEqual(kFlexWrapCrossSizeFixedVersion);
    is_flex_indefinite_percentage_quirks_mode_ =
        !IsVersionHigherOrEqual(kFlexIndefinitePercentageFixedVersion);
    is_flex_auto_margin_quirks_mode_ =
        !IsVersionHigherOrEqual(kFlexAutoMarginFixedVersion);
    is_absolute_and_fixed_box_info_fixed_quirks_mode_ =
        !IsVersionHigherOrEqual(kAbsoluteAndFixedBoxInfoFixedVersion);
    is_grid_new_quirks_mode_ = !IsVersionHigherOrEqual(kGridNewVersion);
  }
  const base::Version& GetQuirksMode() const { return quirks_mode_; }
  void SetTargetSDKVersion(const std::string& target_sdk_version) {
    target_sdk_version_ = target_sdk_version;
    is_target_sdk_verion_higher_than_2_1_ =
        lynx::base::Version(target_sdk_version_) > lynx::base::Version("2.1");
    is_target_sdk_verion_higher_than_2_13_ =
        lynx::base::Version(target_sdk_version_) > lynx::base::Version("2.13");
  }
  // When engineVersion is higher than 2.1, we add Flex layout styles:
  // justify-content, align-self, align-items in linear layout for front-end
  // friendliness
  bool LinearSupportFlexStyleMode() const {
    return is_target_sdk_verion_higher_than_2_1_;
  }
  bool GetIsTargetSdkVerionHigherThan213() const {
    return is_target_sdk_verion_higher_than_2_13_;
  }
  bool IsFullQuirksMode() const { return is_full_quirks_mode_; }
  bool IsVersionHigherOrEqual(const base::Version& version) const {
    return quirks_mode_ >= version;
  }
  // Flex-align quirks mode. When the size of a flex node depends on dynamic
  // size, it is always aligned to the top. Fix this issue when
  // is_flex_align_quirks_mode_ is false.
  bool IsFlexAlignQuirksMode() const { return is_flex_align_quirks_mode_; }
  // Flex-wrap quirks mode. When using flex-wrap with max-height/width at main
  // side, it is not shrinking to content size. Fix this issue when
  // is_flex_wrap_quirks_mode_ is false.
  bool IsFlexWrapQuirksMode() const { return is_flex_wrap_quirks_mode_; }
  // Quirks mode about Flex-wrap. When the main-axis size of the last flex item
  // is larger than the container's main-axis size, an extra flex line will be
  // added. Fix this issue when is_flex_wrap_extra_line_quirks_mode_ is false.
  bool IsFlexWrapExtraLineQuirksMode() const {
    return is_flex_wrap_extra_line_quirks_mode_;
  }
  bool IsGridPreLayoutQuirksMode() const {
    return is_grid_pre_layout_quirks_mode_;
  }
  // Quirks mode about Baseline. When the relative quirks mode is false,
  // baseline supports flex-direction:column and linear-orientation:vertical
  bool IsBaselineSupportVerticalQuirksMode() const {
    return is_baseline_support_vertical_display_quirks_mode_;
  }
  // Flex-wrap cross-size quirks mode. When cross axis constraint
  // mode is atmost, if preferred cross-size is larger than the max constraint
  // cross size, container cross-size should be preferred cross-size(not
  // clamped by the max constraint cross size). Fix this issue when
  // is_flex_wrap_cross_size_quirks_mode_ is false.
  bool IsFlexWrapCrossSizeQuirksMode() const {
    return is_flex_wrap_cross_size_quirks_mode_;
  }
  // Flex: cross size property with indefinite percentage's quirks mode.
  // Cross size property with indefinite percentage does not compute to 'auto',
  // and thus should not stretch the item. Fix this issue when
  // is_flex_indefinite_percentage_quirks_mode_ is false.
  bool IsFlexIndefinitePercentageQuirksMode() const {
    return is_flex_indefinite_percentage_quirks_mode_;
  }
  // Flex: some auto margin behaviors are not aligned with Web. Fix this issue
  // when is_flex_margin_auto_quirks_mode_ is false.
  bool IsFlexAutoMarginQuirksMode() const {
    return is_flex_auto_margin_quirks_mode_;
  }
  // The height, top, bottom, min-height, max-height properties compute
  // percentage values from the height of the containing block. The width, left,
  // right, padding, margin, min-width, max-width properties compute percentage
  // values from the width of the containing block. What's more, for
  // absolute/fixed, containing block is formed when measure stage is over,
  // hence, we need to resolve boxinfo that contains percentages on align stage.
  // Concerning above case, aligned with Web when
  // is_absolute_and_fixed_box_info_fixed_quirks_mode_ is false.
  bool IsAbsoluteAndFixedBoxInfoQuirksMode() const {
    return is_absolute_and_fixed_box_info_fixed_quirks_mode_;
  }
  // Grid: When is_grid_new_quirks_mode_ is set to false, Lynx supports
  // minmax(), fit-content, and max-content. Furthermore, the Grid Sizing
  // Algorithm behaves more closely to HTML5 standards than previously.
  bool IsGridNewQuirksMode() const { return is_grid_new_quirks_mode_; }

  bool is_absolute_in_content_bound_ = false;
  bool css_align_with_legacy_w3c_ = false;
  std::string target_sdk_version_ = "1.0";
  bool font_scale_sp_only_ = false;
  bool default_display_linear_ = false;
  bool enable_fixed_new_ = false;

 private:
  bool is_target_sdk_verion_higher_than_2_1_ = false;
  bool is_target_sdk_verion_higher_than_2_13_ = false;
  base::Version quirks_mode_ = base::Version(kQuirksModeEnableVersion);
  // compatible with SSR
  bool is_full_quirks_mode_ = true;
  bool is_flex_align_quirks_mode_ = true;
  bool is_flex_wrap_quirks_mode_ = true;
  bool is_grid_pre_layout_quirks_mode_ = true;
  bool is_flex_wrap_extra_line_quirks_mode_ = true;
  bool is_baseline_support_vertical_display_quirks_mode_ = true;
  bool is_flex_wrap_cross_size_quirks_mode_ = true;
  bool is_flex_indefinite_percentage_quirks_mode_ = true;
  bool is_flex_auto_margin_quirks_mode_ = true;
  bool is_absolute_and_fixed_box_info_fixed_quirks_mode_ = true;
  bool is_grid_new_quirks_mode_ = true;
};

}  // namespace starlight
}  // namespace lynx

#endif  // CORE_RENDERER_STARLIGHT_TYPES_LAYOUT_CONFIGS_H_
