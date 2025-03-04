// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_PAGE_CONFIG_H_
#define CORE_RENDERER_PAGE_CONFIG_H_

#include <memory>
#include <ostream>
#include <string>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <utility>

#include "core/renderer/css/dynamic_css_configs.h"
#include "core/renderer/css/parser/css_parser_configs.h"
#include "core/renderer/starlight/types/layout_configs.h"
#include "core/renderer/tasm/config.h"
#include "core/renderer/utils/lynx_env.h"
#include "core/runtime/vm/lepus/lepus_value.h"
#include "core/runtime/vm/lepus/table.h"
#include "core/services/feature_count/feature_counter.h"
#include "core/template_bundle/template_codec/compile_options.h"
#include "core/template_bundle/template_codec/ttml_constant.h"
#include "core/template_bundle/template_codec/version.h"

namespace lynx {
namespace tasm {
enum class TernaryBool { TRUE_VALUE, FALSE_VALUE, UNDEFINE_VALUE };

// Preallocate 64 bit unsigned integer for pipeline scheduler config.
// 0 ~ 7 bit: Reserved for parsing binary bundle into C++ bundle.
// 8 ~ 15 bit: Reserved for MTS Render.
// 16 ~ 23 bit: Reserved for resolve stage in Pixel Pipeline.
// 24 ~ 31 bit: Reserved for layout stage in Pixel Pipeline.
// 32 ~ 39 bit: Reserved for execute UI OP stage in Pixel Pipeline.
// 40 ~ 47 bit: Reserved for paint stage in Pixel Pipeline.
// 48 ~ 63 bit: Flexible bits for extensibility.
// TODO: Need to add a TS definition for PipelineSchedulerConfig.
constexpr static uint64_t kEnableListBatchRenderMask = 1 << 8;
constexpr static uint64_t kEnableParallelElementMask = 1 << 16;
constexpr static uint64_t kEnableListBatchRenderAsyncResolvePropertyMask =
    1 << 17;
constexpr static uint64_t kEnableListBatchRenderAsyncResolveTreeMask = 1 << 18;

// TODO(nihao.royal) unify parameters of different types.
// constexpr static int32_t kTernaryInt32UndefinedValue = 0x7fffffff;
// constexpr static const char* kTernaryStringUndefinedValue = "undefined";

/**
 * EntryConfig provide an independent config for entry.
 * Usually, a lazy bundle / card corresponds to an entry.
 */
class EntryConfig {
 public:
  EntryConfig() = default;
  virtual ~EntryConfig() = default;

  // layout configs
  inline const starlight::LayoutConfigs& GetLayoutConfigs() {
    return layout_configs_;
  }

  // default display linear
  inline void SetDefaultDisplayLinear(bool is_linear) {
    default_display_linear_ = is_linear;
    layout_configs_.default_display_linear_ = is_linear;
  }
  inline bool GetDefaultDisplayLinear() { return default_display_linear_; }

 protected:
  starlight::LayoutConfigs layout_configs_;

 private:
  bool default_display_linear_{false};
};

static constexpr const char kEnableSignalAPI[] = "enableSignalAPI";

/**
 * PageConfig hold overall configs of a page.
 * When adding or modifying some properties, please modify
 * oliver/type-lynx/compile/page-config.d.ts at the same time.
 */
class PageConfig final : public EntryConfig {
 public:
  // Enable attribute flatten if it not defined in index.json
  // Attribute auto-expose is automatically opended
  PageConfig()
      : page_flatten(true),
        dsl_(PackageInstanceDSL::TT),
        enable_auto_show_hide(true),
        bundle_module_mode_(PackageInstanceBundleModuleMode::EVAL_REQUIRE_MODE),
        enable_async_display_(true),
        enable_view_receive_touch_(false),
        enable_lepus_strict_check_(false),
        enable_event_through_(false),
        need_remove_component_element_(false){};

  ~PageConfig() override = default;

  lepus::Value GetConfigToRuntime();

  void DecodePageConfigFromJsonStringWhileUndefined(
      const std::string& config_json_string) {
    rapidjson::Document doc;

    if (!doc.Parse(config_json_string.c_str()).HasParseError()) {
      for (auto it = doc.MemberBegin(); it != doc.MemberEnd(); ++it) {
        const char* const name = it->name.GetString();

        // if is a boolean
        auto bool_pair = GetFuncBoolMap().find(name);
        if (bool_pair != GetFuncBoolMap().end()) {
          if (it->value.IsBool()) {
            const auto [setter, getter] = bool_pair->second;
            if ((this->*(getter))() == TernaryBool::UNDEFINE_VALUE) {
              (this->*(setter))(it->value.GetBool() ? TernaryBool::TRUE_VALUE
                                                    : TernaryBool::FALSE_VALUE);
            }
          }
        }

        // TODO(nihao.royal) unify parameters of different types.
      }
    }
  }

  void ForEachBoolConfig(
      const base::MoveOnlyClosure<TernaryBool, const std::string&> func) {
    for (const auto& [name, pair] : GetFuncBoolMap()) {
      const auto [setter, getter] = pair;
      (this->*(setter))(func(name));
    }
  }

  std::unordered_map<std::string, std::string> GetPageConfigMap() {
    std::unordered_map<std::string, std::string> map;
    map.insert({"page_flatten", page_flatten ? "true" : "false"});
    map.insert({"target_sdk_version", target_sdk_version_});
    map.insert({"radon_mode", radon_mode_});
    map.insert({"enable_lepus_ng", enable_lepus_ng_ ? "true" : "false"});
    map.insert({"react_version", react_version_});
    map.insert({"enable_css_parser", enable_css_parser_ ? "true" : "false"});
    map.insert({"absetting_disable_css_lazy_decode",
                absetting_disable_css_lazy_decode_});
    return map;
  }

  inline void SetVersion(const std::string& version) { page_version = version; }

  inline std::string GetVersion() { return page_version; }

  inline void SetGlobalFlattern(bool flattern) { page_flatten = flattern; }

  inline void SetEnableA11yIDMutationObserver(bool enable) {
    enable_a11y_mutation_observer = enable;
  }

  inline void SetEnableA11y(bool enable) { enable_a11y = enable; }

  inline void SetGlobalImplicit(bool implicit) { page_implicit = implicit; }

  inline bool GetGlobalFlattern() { return page_flatten; }

  inline bool GetEnableA11yIDMutationObserver() {
    return enable_a11y_mutation_observer;
  }

  inline bool GetEnableA11y() { return enable_a11y; }

  inline bool GetGlobalImplicit() { return page_implicit; }

  inline void SetDSL(PackageInstanceDSL dsl) { dsl_ = dsl; }

  inline void SetAutoExpose(bool enable) { enable_auto_show_hide = enable; }

  inline void SetDataStrictMode(bool strict) { data_strict_mode = strict; }

  inline void SetAbsoluteInContentBound(bool enable) {
    layout_configs_.is_absolute_in_content_bound_ = enable;
  }

  inline bool GetAbsoluteInContentBound() {
    return layout_configs_.is_absolute_in_content_bound_;
  }

  inline void SetQuirksMode(bool enable) {
    if (css_align_with_legacy_w3c_ || !enable) {
      layout_configs_.SetQuirksMode(kQuirksModeDisableVersion);
    } else {
      layout_configs_.SetQuirksMode(kQuirksModeEnableVersion);
    }
  }
  inline bool GetQuirksMode() const {
    return layout_configs_.IsFullQuirksMode();
  }

  inline void SetQuirksModeByVersion(const base::Version& version) {
    if (css_align_with_legacy_w3c_) {
      layout_configs_.SetQuirksMode(kQuirksModeDisableVersion);
    } else {
      layout_configs_.SetQuirksMode(version);
    }
  }
  inline base::Version GetQuirksModeVersion() const {
    return layout_configs_.GetQuirksMode();
  }

  inline bool GetAutoExpose() { return enable_auto_show_hide; }

  inline bool GetDataStrictMode() { return data_strict_mode; }

  inline void SetDefaultOverflowVisible(bool is_visible) {
    default_overflow_visible_ = is_visible;
  }

  inline bool GetDefaultOverflowVisible() { return default_overflow_visible_; }

  inline const tasm::DynamicCSSConfigs& GetDynamicCSSConfigs() {
    return css_configs_;
  }

  inline void SetEnableFixedNew(bool enable) {
    layout_configs_.enable_fixed_new_ = enable;
  }
  inline bool GetEnableFixedNew() const {
    return layout_configs_.enable_fixed_new_;
  }

  inline PackageInstanceDSL GetDSL() { return dsl_; }

  inline void SetBundleModuleMode(
      PackageInstanceBundleModuleMode bundle_module_mode) {
    bundle_module_mode_ = bundle_module_mode;
  }

  inline PackageInstanceBundleModuleMode GetBundleModuleMode() {
    return bundle_module_mode_;
  }

  inline void SetEnableAsyncDisplay(bool enable) {
    enable_async_display_ = enable;
  }

  inline bool GetEnableAsyncDisplay() { return enable_async_display_; }

  inline void SetEnableImageDownsampling(bool enable) {
    enable_image_downsampling_ = enable;
  }

  inline bool GetEnableImageDownsampling() {
    return enable_image_downsampling_;
  }

  inline void SetEnableNewImage(bool enable) { enable_New_Image_ = enable; }

  inline bool GetEnableNewImage() { return enable_New_Image_; }

  inline void SetTrailNewImage(TernaryBool enable) {
    trail_New_Image_ = enable;
  }

  inline TernaryBool GetTrailNewImage() const { return trail_New_Image_; }

  inline void SetEnableTextLanguageAlignment(bool enable) {
    enable_text_language_alignment_ = enable;
  }

  inline bool GetEnableTextLanguageAlignment() {
    return enable_text_language_alignment_;
  }
  inline void SetEnableXTextLayoutReused(bool enable) {
    enable_x_text_layout_reused_ = enable;
  }
  inline bool GetEnableXTextLayoutReused() {
    return enable_x_text_layout_reused_;
  }

  inline void SetLogBoxImageSizeWarningThreshold(uint32_t threshold) {
    log_box_image_size_warning_threshold_ = threshold;
  }

  inline uint32_t GetLogBoxImageSizeWarningThreshold() {
    return log_box_image_size_warning_threshold_;
  }

  inline void SetEnableTextNonContiguousLayout(bool enable) {
    enable_text_non_contiguous_layout_ = enable;
  }

  inline bool GetEnableTextNonContiguousLayout() {
    return enable_text_non_contiguous_layout_;
  }

  inline void SetEnableViewReceiveTouch(bool enable) {
    enable_view_receive_touch_ = enable;
  }

  inline bool GetEnableViewReceiveTouch() { return enable_view_receive_touch_; }

  inline void SetEnableLepusStrictCheck(bool enable) {
    enable_lepus_strict_check_ = enable;
  }

  inline void SetLepusQuickjsStackSize(uint32_t stack_size) {
    lepus_quickjs_stack_size_ = stack_size;
  }

  inline void SetEnableLepusNullPropAsUndef(bool enable) {
    enable_lepus_null_prop_as_undef_ = enable;
  }

  inline void SetFontScaleSpOnly(bool font_scale) {
    layout_configs_.font_scale_sp_only_ = font_scale;
  }

  inline bool GetFontScaleSpOnly() {
    return layout_configs_.font_scale_sp_only_;
  }

  bool GetEnableLepusStrictCheck() { return enable_lepus_strict_check_; }

  bool GetLepusQuickjsStackSize() { return lepus_quickjs_stack_size_; }

  bool GetEnableLepusNullPropAsUndef() {
    return enable_lepus_null_prop_as_undef_;
  }

  void SetEnableEventThrough(bool enable) { enable_event_through_ = enable; }

  bool GetEnableEventThrough() { return enable_event_through_; }

  void SetEnableSimultaneousTap(bool enable) {
    enable_simultaneous_tap_ = enable;
  }

  bool GetEnableSimultaneousTap() { return enable_simultaneous_tap_; }

  void SetEnableTouchRefactor(bool enable) {
    enable_touch_refactor_ = enable;
    if (enable == false) {
      report::FeatureCounter::Instance()->Count(
          report::LynxFeature::OBJC_DISABLE_TOUCH_REFACTOR);
    }
  }

  bool GetEnableTouchRefactor() { return enable_touch_refactor_; }

  void SetEnableEndGestureAtLastFingerUp(bool enable) {
    enable_end_gesture_at_last_finger_up_ = enable;
  }

  bool GetEnableEndGestureAtLastFingerUp() {
    return enable_end_gesture_at_last_finger_up_;
  }

  void SetRemoveComponentElement(bool need) {
    need_remove_component_element_ = need;
  }
  bool GetRemoveComponentElement() const {
    return need_remove_component_element_;
  }

  void SetStrictPropType(bool enable) { strict_prop_type_ = enable; }
  bool GetStrictPropType() const { return strict_prop_type_; }

  void SetEnableCSSInheritance(bool enable) {
    css_configs_.enable_css_inheritance_ = enable;
  }

  bool GetEnableCSSInheritance() {
    return css_configs_.enable_css_inheritance_;
  }

  void SetCustomCSSInheritList(std::unordered_set<CSSPropertyID>&& list) {
    css_configs_.custom_inherit_list_ =
        std::forward<std::unordered_set<CSSPropertyID>>(list);
  }

  const std::unordered_set<CSSPropertyID>& GetCustomCSSInheritList() {
    return css_configs_.custom_inherit_list_;
  }

  void SetEnableNewLayoutOnly(bool enable) { enable_new_layout_only_ = enable; }
  bool GetEnableNewLayoutOnly() const { return enable_new_layout_only_; }

  bool GetCSSAlignWithLegacyW3C() const { return css_align_with_legacy_w3c_; }
  void SetCSSAlignWithLegacyW3C(bool val) {
    css_align_with_legacy_w3c_ = val;
    layout_configs_.css_align_with_legacy_w3c_ = val;
    if (val) {
      layout_configs_.SetQuirksMode(kQuirksModeDisableVersion);
    }
  }

  // TODO(liting.src): just a workaround to leave below APIs for ssr
  bool GetEnableLocalAsset() const { return false; }
  void SetEnableLocalAsset(bool val) {}

  bool GetEnableComponentLifecycleAlignWebview() {
    return enable_component_lifecycle_align_webview_;
  }
  void SetEnableComponentLifecycleAlignWebview(bool val) {
    enable_component_lifecycle_align_webview_ = val;
  }

  void SetUseNewImage(TernaryBool enable) { use_new_image = enable; }
  TernaryBool GetUseNewImage() const { return use_new_image; }

  void SetAsyncRedirectUrl(TernaryBool async) { async_redirect_url = async; }
  TernaryBool GetAsyncRedirectUrl() const { return async_redirect_url; }

  void SetSyncImageAttach(bool enable) { sync_image_attach = enable; }
  bool GetSyncImageAttach() const { return sync_image_attach; }

  void SetUseImagePostProcessor(bool enable) {
    use_image_post_processor_ = enable;
  }
  bool GetUseImagePostProcessor() const { return use_image_post_processor_; }

  inline void SetCliVersion(const std::string& cli_version) {
    cli_version_ = cli_version;
  }
  inline std::string GetCliVersion() { return cli_version_; }

  inline void SetCustomData(const std::string& custom_data) {
    custom_data_ = custom_data;
  }
  inline std::string GetCustomData() { return custom_data_; }

  void SetUseNewSwiper(bool enable) { use_new_swiper = enable; }

  bool GetUseNewSwiper() const { return use_new_swiper; }

  void SetEnableAsyncInitTTVideoEngine(bool enable) {
    async_init_tt_video_engine = enable;
  }

  bool GetEnableAsyncInitTTVideoEngine() const {
    return async_init_tt_video_engine;
  }

  void SetEnableCSSStrictMode(bool enable) {
    css_parser_configs_.enable_css_strict_mode = enable;
  }

  bool GetEnableCSSStrictMode() {
    return css_parser_configs_.enable_css_strict_mode;
  }

  inline const CSSParserConfigs& GetCSSParserConfigs() {
    return css_parser_configs_;
  }

  void SetCSSParserConfigs(const CSSParserConfigs& config) {
    css_parser_configs_ = config;
  }

  inline void SetTargetSDKVersion(const std::string& target_sdk_version) {
    target_sdk_version_ = target_sdk_version;
    layout_configs_.SetTargetSDKVersion(target_sdk_version);
    SetIsTargetSdkVerionHigherThan21();
  }
  inline std::string GetTargetSDKVersion() { return target_sdk_version_; }

  inline void SetIsTargetSdkVerionHigherThan21() {
    is_target_sdk_verion_higher_than_2_1_ =
        lynx::base::Version(target_sdk_version_) >
        lynx::base::Version(LYNX_VERSION_2_1);
  }

  inline void SetIsTargetSdkVerionHigherThan21(bool value) {
    is_target_sdk_verion_higher_than_2_1_ = value;
  }

  inline bool GetIsTargetSdkVerionHigherThan21() const {
    return is_target_sdk_verion_higher_than_2_1_;
  }

  inline void SetLepusVersion(const std::string& lepus_version) {
    lepus_version_ = lepus_version;
  }
  inline std::string GetLepusVersion() { return lepus_version_; }

  inline void SetRadonMode(std::string radon_mode) { radon_mode_ = radon_mode; }

  inline std::string GetRadonMode() { return radon_mode_; }

  inline void SetEnableLepusNG(bool enable_lepus_ng) {
    enable_lepus_ng_ = enable_lepus_ng;
  }
  inline bool GetEnableLepusNG() { return enable_lepus_ng_; }

  inline void SetTapSlop(const std::string& tap_slop) { tap_slop_ = tap_slop; }

  inline const std::string& GetTapSlop() { return tap_slop_; }

  void SetEnableCreateViewAsync(bool enable) {
    enable_create_view_async_ = enable;
  }

  void SetEnableVsyncAlignedFlush(bool enable) {
    enable_vsync_aligned_flush = enable;
  }

  bool GetEnableCreateViewAsync() const { return enable_create_view_async_; }

  bool GetEnableVsyncAlignedFlush() const { return enable_vsync_aligned_flush; }

  void SetEnableSavePageData(bool enable) { enable_save_page_data_ = enable; }

  bool GetEnableSavePageData() { return enable_save_page_data_; }
  void SetListNewArchitecture(bool list_new_architecture) {
    list_new_architecture_ = list_new_architecture;
  }

  bool GetListNewArchitecture() { return list_new_architecture_; }

  bool GetEnableNewListContainer() { return enable_new_list_container_; }

  void SetEnableNewListContainer(bool enable_new_list_container) {
    enable_new_list_container_ = enable_new_list_container;
  }

  void SetListRemoveComponent(bool list_remove_component) {
    list_remove_component_ = list_remove_component;
  }
  bool GetListRemoveComponent() { return list_remove_component_; }

  void SetEnableListMoveOperation(bool list_enable_move) {
    list_enable_move_operation_ = list_enable_move;
  }

  bool GetEnableListMoveOperation() { return list_enable_move_operation_; }

  void SetEnableListPlug(bool list_enable_plug) {
    list_enable_plug_ = list_enable_plug;
  }

  bool list_enable_plug() { return list_enable_plug_; }

  void SetEnableAccessibilityElement(bool enable) {
    enable_accessibility_element_ = enable;
  }

  bool GetEnableAccessibilityElement() const {
    return enable_accessibility_element_;
  }

  void SetEnableOverlapForAccessibilityElement(bool enable) {
    enable_overlap_for_accessibility_element_ = enable;
  }

  bool GetEnableOverlapForAccessibilityElement() const {
    return enable_overlap_for_accessibility_element_;
  }

  void SetEnableNewAccessibility(bool enable) {
    enable_new_accessibility_ = enable;
  }

  bool GetEnableNewAccessibility() const { return enable_new_accessibility_; }

  inline void SetReactVersion(const std::string& react_version) {
    react_version_ = react_version;
  }
  inline std::string GetReactVersion() { return react_version_; }
  inline bool GetEnableTextRefactor() { return enable_text_refactor_; }
  void SetEnableTextRefactor(bool enable_text_refactor) {
    enable_text_refactor_ = enable_text_refactor;
  }

  void SetUnifyVWVH(bool unify) { css_configs_.unify_vw_vh_behavior_ = unify; }
  bool GetUnifyVWVH() { return css_configs_.unify_vw_vh_behavior_; }

  inline bool GetEnableZIndex() { return enable_z_index_; }
  inline void SetEnableZIndex(bool enable) { enable_z_index_ = enable; }

  inline bool GetEnableReactOnlyPropsId() const {
    return enable_react_only_props_id_;
  }
  inline void SetEnableReactOnlyPropsId(bool enable) {
    enable_react_only_props_id_ = enable;
  }

  inline bool GetEnableGlobalComponentMap() const {
    return enable_global_component_map_;
  }
  inline void SetEnableGlobalComponentMap(bool enable) {
    enable_global_component_map_ = enable;
  }

  inline bool GetEnableRemoveComponentExtraData() const {
    return enable_remove_component_extra_data_;
  }
  inline void SetEnableRemoveComponentExtraData(bool enable) {
    enable_remove_component_extra_data_ = enable;
  }

  inline void SetGlobalAutoResumeAnimation(bool enable_auto_resume) {
    auto_resume_animation_ = enable_auto_resume;
  }
  inline bool GetGlobalAutoResumeAnimation() { return auto_resume_animation_; }

  inline void SetGlobalEnableNewTransformOrigin(
      bool enable_new_transform_origin) {
    enable_new_transform_origin_ = enable_new_transform_origin;
  }
  inline bool GetGlobalEnableNewTransformOrigin() {
    return enable_new_transform_origin_;
  }
  inline void SetGlobalCircularDataCheck(bool enable_circular_data_check) {
    enable_circular_data_check_ = enable_circular_data_check;
  }
  inline bool GetGlobalCircularDataCheck() {
    return enable_circular_data_check_;
  }

  inline bool GetEnableLynxAir() { return enable_lynx_air_; }
  inline void SetEnableLynxAir(bool enable) { enable_lynx_air_ = enable; }
  inline bool GetEnableFiberArch() { return enable_fiber_arch_; }
  inline void SetEnableFiberArch(bool enable) { enable_fiber_arch_ = enable; }
  inline TernaryBool GetEnableTextLayerRender() {
    return enable_text_layer_render_;
  }

  void SetEnableTextLayerRender(TernaryBool enable_text_layer_render) {
    enable_text_layer_render_ = enable_text_layer_render;
  }

  inline bool GetEnableReduceInitDataCopy() {
    return enable_reduce_init_data_copy_;
  }
  inline void SetEnableReduceInitDataCopy(bool enable) {
    enable_reduce_init_data_copy_ = enable;
  }
  inline bool GetEnableCSSParser() { return enable_css_parser_; }
  inline void SetEnableCSSParser(bool enable) { enable_css_parser_ = enable; }

  inline std::string GetAbSettingDisableCSSLazyDecode() {
    return absetting_disable_css_lazy_decode_;
  }
  inline void SetAbSettingDisableCSSLazyDecode(std::string disable) {
    absetting_disable_css_lazy_decode_ = disable;
  }

  inline void SetKeyboardCallbackUseRelativeHeight(bool enable) {
    keyboard_callback_pass_relative_height_ = enable;
  }

  inline bool GetKeyboardCallbackUseRelativeHeight() const {
    return keyboard_callback_pass_relative_height_;
  }

  inline void SetEnableEventRefactor(bool option) {
    enable_event_refactor_ = option;
    if (option == false) {
      report::FeatureCounter::Instance()->Count(
          report::LynxFeature::CPP_DISABLE_EVENT_REFACTOR);
    }
  }

  bool GetEnableEventRefactor() const { return enable_event_refactor_; }

  inline void SetForceCalcNewStyle(bool option) {
    force_calc_new_style_ = option;
  }

  bool GetForceCalcNewStyle() const { return force_calc_new_style_; }

  inline void SetCompileRender(bool option) { compile_render_ = option; }

  bool GetCompileRender() const { return compile_render_; }

  inline void SetDisableLongpressAfterScroll(bool value) {
    disable_longpress_after_scroll_ = value;
  }

  inline bool GetDisableLongpressAfterScroll() {
    return disable_longpress_after_scroll_;
  }

  inline void SetEnableCheckDataWhenUpdatePage(bool option) {
    enable_check_data_when_update_page_ = option;
  }

  bool GetEnableCheckDataWhenUpdatePage() const {
    return enable_check_data_when_update_page_;
  }

  int32_t GetIncludeFontPadding() const { return include_font_padding_; }

  void SetIncludeFontPadding(bool value) {
    include_font_padding_ = value ? 1 : -1;
  }

  inline void SetEnableNewIntersectionObserver(bool option) {
    enable_new_intersection_observer_ = option;
  }

  inline bool GetEnableNewIntersectionObserver() const {
    return enable_new_intersection_observer_;
  }

  inline void SetObserverFrameRate(int32_t option) {
    observer_frame_rate_ = option;
  }

  inline int32_t GetObserverFrameRate() const { return observer_frame_rate_; }

  inline void SetEnableCheckExposureOptimize(
      bool enable_check_exposure_optimize) {
    enable_check_exposure_optimize_ = enable_check_exposure_optimize;
  }

  inline bool GetEnableCheckExposureOptimize() const {
    return enable_check_exposure_optimize_;
  }

  inline void SetEnableDisexposureWhenLynxHidden(
      bool enable_disexposure_when_lynx_hidden) {
    enable_disexposure_when_lynx_hidden_ = enable_disexposure_when_lynx_hidden;
  }

  inline bool GetEnableDisexposureWhenLynxHidden() const {
    return enable_disexposure_when_lynx_hidden_;
  }

  inline void SetEnableExposureWhenLayout(bool value) {
    enable_exposure_when_layout_ = value;
  }

  inline bool GetEnableExposureWhenLayout() const {
    return enable_exposure_when_layout_;
  }

  inline void SetEnableAirDetectRemovedKeysWhenUpdateData(bool value) {
    enable_air_detect_removed_keys_when_update_data_ = value;
  }
  inline bool GetEnableAirDetectRemovedKeysWhenUpdateData() const {
    return enable_air_detect_removed_keys_when_update_data_;
  }

  inline void SetEnableExposureUIMargin(bool option) {
    enable_exposure_ui_margin_ = option;
  }

  inline bool GetEnableExposureUIMargin() const {
    return enable_exposure_ui_margin_;
  }

  inline void SetEnableNewGesture(bool enable) { enable_new_gesture_ = enable; }

  inline bool GetEnableNewGesture() const { return enable_new_gesture_; }

  inline void SetLongPressDuration(int32_t option) {
    long_press_duration_ = option;
  }

  inline void SetMapContainerType(uint8_t type) { map_container_type_ = type; }

  inline uint8_t GetMapContainerType() { return map_container_type_; }

  inline int32_t GetLongPressDuration() const { return long_press_duration_; }

  inline void SetEnableCheckLocalImage(bool option) {
    enable_check_local_image_ = option;
  }

  inline bool GetEnableCheckLocalImage() const {
    return enable_check_local_image_;
  }

  inline void SetEnableAsyncRequestImage(bool option) {
    enable_async_request_image_ = option;
  }

  inline bool GetEnableAsyncRequestImage() const {
    return enable_async_request_image_;
  }

  inline void SetEnableComponentLayoutOnly(bool enable) {
    enable_component_layout_only_ = enable;
  }

  inline bool GetEnableComponentLayoutOnly() {
    return enable_component_layout_only_;
  }

  inline void SetEnableBackgroundShapeLayer(bool enable) {
    enable_background_shape_layer_ = enable;
  }

  inline bool GetEnableBackgroundShapeLayer() {
    return enable_background_shape_layer_;
  }

  inline void SetLynxAirMode(CompileOptionAirMode air_mode) {
    air_mode_ = air_mode;
  }

  inline CompileOptionAirMode GetLynxAirMode() { return air_mode_; }

  inline void SetEnableLynxResourceServiceProvider(bool option) {
    enable_lynx_resource_service_provider_ = option;
  }

  inline bool GetEnableLynxResourceServiceProvider() {
    return enable_lynx_resource_service_provider_;
  }

  inline bool GetEnableTextOverflow() { return enable_text_overflow_; }
  void SetEnableTextOverflow(bool enable_text_overflow) {
    enable_text_overflow_ = enable_text_overflow;
  }

  inline TernaryBool GetEnableTextBoringLayout() {
    return enable_text_boring_layout_;
  }
  void SetEnableTextBoringLayout(TernaryBool enable_text_boring_layout) {
    enable_text_boring_layout_ = enable_text_boring_layout;
  }

  inline bool GetEnableNewClipMode() { return enable_new_clip_mode_; }
  void SetEnableNewClipMode(bool enable_new_clip_mode) {
    enable_new_clip_mode_ = enable_new_clip_mode;
  }

  inline bool GetEnableCascadePseudo() const { return enable_cascade_pseudo_; }
  inline void SetEnableCascadePseudo(bool value) {
    enable_cascade_pseudo_ = value;
  }

  inline bool GetEnableRasterAnimation() const {
    return enable_raster_animation_;
  }
  inline void SetEnableRasterAnimation(bool value) {
    enable_raster_animation_ = value;
  }

  inline lepus::Value GetExtraInfo() const { return extra_info_; }

  inline void SetExtraInfo(lepus::Value extra_info) {
    extra_info_ = extra_info;
  }

  int64_t GetLepusGCThreshold() { return lepus_gc_threshold_; }
  void SetLepusGCThreshold(int64_t value) { lepus_gc_threshold_ = value; }

  inline bool GetEnableComponentNullProp() const {
    return enable_component_null_prop_;
  }

  inline void SetEnableComponentNullProp(bool enable_component_null_prop) {
    enable_component_null_prop_ = enable_component_null_prop;
  }

  inline bool GetEnableCSSInvalidation() const {
    return enable_css_invalidation_;
  }

  inline void SetEnableCSSInvalidation(bool enable) {
    enable_css_invalidation_ = enable;
  }

  inline bool GetEnableParallelElement() const {
    return enable_parallel_element_;
  }

  inline void SetEnableParallelElement(bool enable) {
    enable_parallel_element_ = enable;
  }

  inline uint64_t GetPipelineSchedulerConfig() const {
    return pipeline_scheduler_config_;
  }

  inline void SetPipelineSchedulerConfig(uint64_t config) {
    pipeline_scheduler_config_ = config;
  }

  bool GetRemoveDescendantSelectorScope() const {
    return remove_descendant_selector_scope_;
  }

  void SetRemoveDescendantSelectorScope(bool enable) {
    remove_descendant_selector_scope_ = enable;
  }

  bool GetEnableStandardCSSSelector() const {
    return enable_standard_css_selector_;
  }

  void SetEnableStandardCSSSelector(bool enable) {
    enable_standard_css_selector_ = enable;
  }

  bool GetEnableDataProcessorOnJs() const {
    return enable_data_processor_on_js_;
  }

  void SetEnableDataProcessorOnJs(bool enable) {
    enable_data_processor_on_js_ = enable;
  }

  inline bool GetEnableNativeList() const { return enable_native_list_; }

  inline void SetEnableNativeList(bool enable) { enable_native_list_ = enable; }

  bool GetEnableMultiTouch() const { return enable_multi_touch_; }

  void SetEnableMultiTouch(bool enable) {
    enable_multi_touch_ = enable;
    if (enable == false) {
      report::FeatureCounter::Instance()->Count(
          report::LynxFeature::CPP_DISABLE_MULTI_TOUCH);
    }
  }

  bool GetEnableComponentAsyncDecode() const {
    switch (enable_component_async_decode_) {
      case TernaryBool::TRUE_VALUE:
        return true;
      case TernaryBool::FALSE_VALUE:
        return false;
      case TernaryBool::UNDEFINE_VALUE:
        static bool enable_from_experiment =
            LynxEnv::GetInstance().EnableComponentAsyncDecode();
        return enable_from_experiment;
    }
  }

  void SetEnableComponentAsyncDecode(bool enable) {
    enable_component_async_decode_ =
        enable ? TernaryBool::TRUE_VALUE : TernaryBool::FALSE_VALUE;
  }

  inline void SetEnableExtendedLayoutOpt(bool enable) {
    extended_layout_only_opt_ = enable;
  }

  inline bool GetEnableExtendedLayoutOpt() { return extended_layout_only_opt_; }

  void SetEnableUseContextPool(bool enable) {
    enable_use_context_pool_ =
        enable ? TernaryBool::TRUE_VALUE : TernaryBool::FALSE_VALUE;
  }

  bool GetEnableUseContextPool() const {
    switch (enable_use_context_pool_) {
      case TernaryBool::TRUE_VALUE:
        return true;
      case TernaryBool::FALSE_VALUE:
        return false;
      case TernaryBool::UNDEFINE_VALUE:
        static bool enable_from_experiment =
            LynxEnv::GetInstance().EnableUseContextPool();
        return enable_from_experiment;
    }
  }

  inline void SetEnableScrollFluencyMonitor(double value) {
    if (value < 0) {
      enable_scroll_fluency_monitor = 0;
    } else if (value > 1) {
      enable_scroll_fluency_monitor = 1;
    } else {
      enable_scroll_fluency_monitor = value;
    }
  }
  inline double GetEnableScrollFluencyMonitor() {
    return enable_scroll_fluency_monitor;
  }

  bool GetEnableJsBindingApiThrowException() const {
    return enable_js_binding_api_throw_exception_;
  }

  void SetEnableJsBindingApiThrowException(bool enable) {
    enable_js_binding_api_throw_exception_ = enable;
  }

  void SetEnableUseMapBuffer(TernaryBool use_map_buffer) {
    enable_use_map_buffer_ = use_map_buffer;
  }

  TernaryBool GetEnableUseMapBuffer() const { return enable_use_map_buffer_; }

  void SetEnableUIOperationOptimize(TernaryBool enable) {
    enable_ui_operation_optimize_ = enable;
  }

  TernaryBool GetEnableUIOperationOptimize() const {
    return enable_ui_operation_optimize_;
  }

  void SetEnableElementAPITypeCheckThrowWarning(bool enable) {
    enable_element_api_type_check_throw_warning_ = enable;
  }

  bool GetEnableElementAPITypeCheckThrowWarning() {
    return enable_element_api_type_check_throw_warning_;
  }

  void SetEnableBindICU(bool enable) { enable_bind_icu_ = enable; }

  bool GetEnableBindICU() { return enable_bind_icu_; }

  void SetEnableQueryComponentSync(bool enable) {
    enable_query_component_sync_ = enable;
  }

  bool GetEnableQueryComponentSync() const {
    return enable_query_component_sync_;
  }

  void SetDisableQuickTracingGC(bool disable) {
    disable_quick_tracing_gc_ = disable;
  }

  bool GetDisableQuickTracingGC() const { return disable_quick_tracing_gc_; }

  void SetEnableReloadLifecycle(bool enable) {
    enable_reload_lifecycle_ = enable;
  }

  bool GetEnableReloadLifecycle() { return enable_reload_lifecycle_; }

  inline void SetEnableFiberElementForRadonDiff(TernaryBool enable) {
    enable_fiber_element_for_radon_diff_ = enable;
  }

  inline TernaryBool GetEnableFiberElementForRadonDiff() const {
    return enable_fiber_element_for_radon_diff_;
  }

  inline void SetPreferredFps(const std::string& preferred_fps) {
    preferred_fps_ = preferred_fps;
  }

  inline std::string GetPreferredFps() { return preferred_fps_; }

  inline void SetEnableCSSLazyImport(TernaryBool enable_css_lazy_import) {
    enable_css_lazy_import_ = enable_css_lazy_import;
  }

  inline bool GetEnableCSSLazyImport() const {
    // pageConfig > Libra > Settings
    switch (enable_css_lazy_import_) {
      case TernaryBool::TRUE_VALUE:
        return true;
      case TernaryBool::FALSE_VALUE:
        return false;
      case TernaryBool::UNDEFINE_VALUE:
        static bool enable_css_lazy_import =
            LynxEnv::GetInstance().EnableCSSLazyImport();
        return enable_css_lazy_import;
    }
  }

  inline void SetEnableNewAnimator(TernaryBool enable_new_animator) {
    enable_new_animator_ = enable_new_animator;
  }

  inline bool GetEnableNewAnimator() const {
    // pageConfig > Libra > Settings
    switch (enable_new_animator_) {
      case TernaryBool::TRUE_VALUE:
        return true;
      case TernaryBool::FALSE_VALUE:
        return false;
      case TernaryBool::UNDEFINE_VALUE:
        static bool enable_new_animator =
            LynxEnv::GetInstance().EnableNewAnimatorFiber();
        return enable_new_animator;
    }
  }

  bool GetEnableMicrotaskPromisePolyfill() const {
    return enable_microtask_promise_polyfill_;
  }

  void SetEnableMicrotaskPromisePolyfill(bool enable) {
    enable_microtask_promise_polyfill_ = enable;
  }

  TernaryBool GetEnableSignalAPI() const { return enable_signal_api_; }

  bool GetEnableSignalAPIBoolValue() {
    if (enable_signal_api_ == TernaryBool::UNDEFINE_VALUE) {
      enable_signal_api_ = LynxEnv::GetInstance().EnableSignalAPI()
                               ? TernaryBool::TRUE_VALUE
                               : TernaryBool::UNDEFINE_VALUE;
    }
    return enable_signal_api_ == TernaryBool::TRUE_VALUE;
  }

  void SetEnableSignalAPI(TernaryBool enable) { enable_signal_api_ = enable; }

  // TODO(songshourui.null): move this function to testing file
  void PrintPageConfig(std::ostream& output) {
#define PAGE_CONFIG_DUMP(key) output << #key << ":" << key << ",";
    PAGE_CONFIG_DUMP(page_version)
    PAGE_CONFIG_DUMP(page_flatten)
    PAGE_CONFIG_DUMP(page_implicit)
    output << "dsl_:" << static_cast<int>(dsl_) << ",";
    PAGE_CONFIG_DUMP(enable_auto_show_hide)
    output << "bundle_module_mode_:" << static_cast<int>(bundle_module_mode_)
           << ",";
    PAGE_CONFIG_DUMP(enable_async_display_)
    PAGE_CONFIG_DUMP(enable_view_receive_touch_)
    PAGE_CONFIG_DUMP(enable_lepus_strict_check_)
    PAGE_CONFIG_DUMP(enable_event_through_)
    PAGE_CONFIG_DUMP(layout_configs_.is_absolute_in_content_bound_)
    output << "layout_configs_.quirks_mode_:"
           << layout_configs_.IsFullQuirksMode() << ",";
    PAGE_CONFIG_DUMP(css_parser_configs_.enable_css_strict_mode)
#undef PAGE_CONFIG_DUMP
  }

  // TODO(songshourui.null): move this function to testing file
  std::string StringifyPageConfig() {
    std::ostringstream output;
    PrintPageConfig(output);
    return output.str();
  }

 private:
  std::string page_version;
  bool page_flatten;
  bool enable_a11y_mutation_observer{false};
  bool enable_a11y{false};
  bool page_implicit{true};
  PackageInstanceDSL dsl_;
  bool enable_auto_show_hide;
  PackageInstanceBundleModuleMode bundle_module_mode_;
  bool enable_async_display_;
  bool enable_image_downsampling_{false};
  bool enable_New_Image_{true};
  bool enable_text_language_alignment_{false};
  bool enable_x_text_layout_reused_{false};
  TernaryBool trail_New_Image_{TernaryBool::UNDEFINE_VALUE};
  bool enable_view_receive_touch_;
  bool enable_lepus_strict_check_;
  uint32_t lepus_quickjs_stack_size_ = 0;
  // default big image warning threshold, adjust it if necessary
  uint32_t log_box_image_size_warning_threshold_ = 1000000;
  bool enable_event_through_;
  bool enable_simultaneous_tap_{false};
  // Default value is false. If this flag is true, the external gesture which's
  // state is possible or began will not cancel the Lynx iOS touch gesture see
  // issue:#7920.
  bool enable_touch_refactor_{true};
  // In the previous commit, when determining whether all fingers had moved off
  // the screen in multiple touch scenarios, touch.view was used for judgment.
  // However, in a scrolling container, touch.view obtained from
  // touchesEnd/touchesMove could be nil, resulting in incorrect judgment of
  // whether all fingers had moved off the screen. _touches could not be
  // cleared, leading to a subsequent failure to trigger tap events.
  // To fix this issue, we added null checks before calling touch.view, and
  // ended the gesture if _touches was empty. This resolved the problem.
  // only for ios, detail can see f-12375631 and its mr.
  bool enable_end_gesture_at_last_finger_up_{false};
  bool enable_lepus_null_prop_as_undef_{false};
  bool enable_text_non_contiguous_layout_{true};
  bool need_remove_component_element_;
  bool strict_prop_type_{false};
  bool enable_new_layout_only_{true};
  bool css_align_with_legacy_w3c_{false};
  bool enable_component_lifecycle_align_webview_{false};
  tasm::DynamicCSSConfigs css_configs_;
  TernaryBool use_new_image{TernaryBool::UNDEFINE_VALUE};
  TernaryBool async_redirect_url{TernaryBool::UNDEFINE_VALUE};
  bool sync_image_attach{true};
  bool use_image_post_processor_{false};
  std::string cli_version_;
  std::string custom_data_;
  bool use_new_swiper{true};
  bool async_init_tt_video_engine{false};
  CSSParserConfigs css_parser_configs_;
  std::string target_sdk_version_;
  std::string lepus_version_;
  std::string radon_mode_;
  bool enable_lepus_ng_{true};
  std::string tap_slop_{};
  bool default_overflow_visible_{false};
  bool enable_create_view_async_{true};
  bool enable_vsync_aligned_flush{false};
  bool enable_save_page_data_{false};
  bool list_new_architecture_{false};
  bool list_remove_component_{false};
  bool enable_new_list_container_{false};
  bool list_enable_move_operation_{false};
  bool list_enable_plug_{false};
  bool enable_accessibility_element_{true};
  bool enable_overlap_for_accessibility_element_{true};
  bool enable_new_accessibility_{false};
  std::string react_version_;
  bool enable_text_refactor_{false};
  bool data_strict_mode{true};
  bool enable_z_index_{false};
  bool enable_react_only_props_id_{false};
  bool enable_global_component_map_{false};
  bool enable_remove_component_extra_data_{false};
  bool enable_lynx_air_{false};
  bool enable_fiber_arch_{false};
  TernaryBool enable_text_layer_render_{TernaryBool::UNDEFINE_VALUE};
  bool auto_resume_animation_{true};
  bool enable_reduce_init_data_copy_{false};
  bool enable_component_layout_only_{false};
  bool enable_cascade_pseudo_{false};
  // Used for lynx config
  bool enable_css_parser_{false};
  std::string absetting_disable_css_lazy_decode_;
  // default include font padding
  // 1 means true
  // -1 means false
  int32_t include_font_padding_{0};

  // page's engine version controller
  bool is_target_sdk_verion_higher_than_2_1_{false};
  bool keyboard_callback_pass_relative_height_{false};
  bool enable_event_refactor_{true};
  bool force_calc_new_style_{true};
  bool enable_check_data_when_update_page_{true};
  bool compile_render_{false};

  // If this flag is true, iOS will not recognize the corresponding long press
  // gesture after triggering scrolling.
  bool disable_longpress_after_scroll_{false};

  bool enable_new_intersection_observer_{false};

  int32_t observer_frame_rate_{20};

  // The switch controlling whether to enable exposure detection optimization.
  bool enable_check_exposure_optimize_{false};

  // The switch controlling whether to enable send disexposure events when
  // lynxview is hidden.
  bool enable_disexposure_when_lynx_hidden_{true};

  // Enable exposure check when LynxView is layoutRequest. In certain scenarios,
  // exposure detection can be inaccurate if it is conducted before the layout
  // is complete. This is because the detection is calculated based on incorrect
  // positioning information. To address this issue, exposure detection is not
  // performed when LynxView isLayoutRequested. However, in some cases, LynxView
  // will call requestsLayout frequently, which prevents exposure detection from
  // being performed, resulting in fewer exposure events. To accommodate both
  // scenarios, a new enableExposureWhenLayout switch has been added to enable
  // businesses to control whether exposure detection is performed during
  // LynxView isLayoutRequested.
  bool enable_exposure_when_layout_{false};

  bool enable_exposure_ui_margin_{false};

  bool enable_new_gesture_{false};

  int32_t long_press_duration_{-1};

  uint8_t map_container_type_{0};

  bool enable_check_local_image_{true};

  bool enable_async_request_image_{false};

  // If this flag is true ,new transform origin algorithm will apply
  bool enable_new_transform_origin_{true};
  // If this flag is true, circular data check will enable when convert js value
  // to other vale.
  bool enable_circular_data_check_{true};

  // Enable iOS background manager to apply shape layer optimization.
  bool enable_background_shape_layer_{true};

  CompileOptionAirMode air_mode_{CompileOptionAirMode::AIR_MODE_OFF};

  // Enable LynxResourceService to fetch external resource.
  bool enable_lynx_resource_service_provider_{false};

  // set text overflow as visible if true
  bool enable_text_overflow_{false};

  // Enable Android text BoringLayout
  TernaryBool enable_text_boring_layout_{TernaryBool::UNDEFINE_VALUE};

  // set new clip mode if true
  bool enable_new_clip_mode_{false};

  // user defined extraInfo.
  lepus::Value extra_info_{};

  // gc threshold of lepusNG. Let default value be 256, and the unit is KB.
  int64_t lepus_gc_threshold_{256};

  // support component can be passed null props.
  // null props is only be supported in LepusNG now.
  // open this switch to support lepus use null prop.
  bool enable_component_null_prop_{false};

  // support CSS invalidation
  bool enable_css_invalidation_{false};

  // If false, descendant selector only works in component scope
  bool remove_descendant_selector_scope_{true};

  // indicate that enable standard css selector
  bool enable_standard_css_selector_{false};

  // indicate that enable data processor on js thread.
  bool enable_data_processor_on_js_{false};

  // enable support multi-finger events
  bool enable_multi_touch_{false};

  // enable air mode to detect removed keys in updating data from native
  bool enable_air_detect_removed_keys_when_update_data_{false};

  // Enable lazy_bundles to be decoded in child threads before they are
  // delivered into tasm in async-loading.
  TernaryBool enable_component_async_decode_{TernaryBool::UNDEFINE_VALUE};

  // A config to force make some special properties can be used to layout only
  // (such as: direction&text-align,etc.)
  bool extended_layout_only_opt_{false};

  // Indicates whether the parallel flush of Element has been enabled. And the
  // default value is false.
  bool enable_parallel_element_{false};

  // enable raster animation
  bool enable_raster_animation_{false};

  // enable use quick_context_pool to construct quick context
  TernaryBool enable_use_context_pool_{TernaryBool::UNDEFINE_VALUE};

  // force report lynx scroll fluency event.
  // When setting pageConfig.enableLynxScrollFluency to a double value in the
  // range [0, 1], we will monitor the fluency metrics for this LynxUI based on
  // this probability. The probability indicates the likelihood of enabling
  // fluency monitoring, and the metrics will be reported unconditionally
  // through the applogService.
  double enable_scroll_fluency_monitor{-1};

  // enable js binding api throw exception rather than report
  bool enable_js_binding_api_throw_exception_{false};

  TernaryBool enable_use_map_buffer_{TernaryBool::UNDEFINE_VALUE};

  // introduced in 2.16, enable the optimization aboult UIOperation batching and
  // CreateViewAsync at Android
  TernaryBool enable_ui_operation_optimize_{TernaryBool::UNDEFINE_VALUE};

  // enable avoid throwing RenderFatal for element api when argument type
  // checking failed
  bool enable_element_api_type_check_throw_warning_{false};

  // enable LynxUI onNodeReload lifecycle;
  bool enable_reload_lifecycle_{false};

  // enable bind primjs-icu
  bool enable_bind_icu_{false};

  TernaryBool enable_fiber_element_for_radon_diff_{TernaryBool::UNDEFINE_VALUE};

  bool enable_query_component_sync_{false};

  // Indicates whether use c++ list.
  bool enable_native_list_{false};

  // peferredFps
  std::string preferred_fps_ = "auto";

  // CSSLazyImport
  TernaryBool enable_css_lazy_import_{TernaryBool::UNDEFINE_VALUE};

  // enableNewAnimator
  TernaryBool enable_new_animator_{TernaryBool::UNDEFINE_VALUE};

  // Composite config representing configs including enableParallelElement,
  // batch-rendering
  uint64_t pipeline_scheduler_config_{0};

  // enable microtask promise polyfill
  bool enable_microtask_promise_polyfill_{false};

  // disable tracing gc mode in quick context
  bool disable_quick_tracing_gc_{false};

  TernaryBool enable_signal_api_{TernaryBool::UNDEFINE_VALUE};

  lepus::Value config_to_runtime_;

  template <typename T>
  using PageConfigSetter = void (PageConfig::*)(T);

  template <typename T>
  using PageConfigGetter = T (PageConfig::*)() const;

  template <typename T>
  using PageConfigPair = std::pair<PageConfigSetter<T>, PageConfigGetter<T>>;

  template <typename T>
  using PageConfigMap = std::unordered_map<std::string, PageConfigPair<T>>;

  static const PageConfigMap<TernaryBool>& GetFuncBoolMap();
};
}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_PAGE_CONFIG_H_
