// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_TEMPLATE_BUNDLE_TEMPLATE_CODEC_BINARY_DECODER_PAGE_CONFIG_H_
#define CORE_TEMPLATE_BUNDLE_TEMPLATE_CODEC_BINARY_DECODER_PAGE_CONFIG_H_

#include <memory>
#include <ostream>
#include <string>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <utility>

#include "base/include/value/base_value.h"
#include "base/include/value/table.h"
#include "core/renderer/css/dynamic_css_configs.h"
#include "core/renderer/css/parser/css_parser_configs.h"
#include "core/renderer/starlight/types/layout_configs.h"
#include "core/renderer/tasm/config.h"
#include "core/renderer/utils/lynx_env.h"
#include "core/template_bundle/template_codec/compile_options.h"
#include "core/template_bundle/template_codec/ttml_constant.h"
#include "core/template_bundle/template_codec/version.h"

namespace lynx {
namespace tasm {
enum class TernaryBool : uint8_t { TRUE_VALUE, FALSE_VALUE, UNDEFINE_VALUE };

// Preallocate 64 bit unsigned integer for pipeline scheduler config.
// 0 ~ 7 bit: Reserved for parsing binary bundle into C++ bundle.
// 8 ~ 15 bit: Reserved for MTS Render.
// 16 ~ 23 bit: Reserved for resolve stage in Pixel Pipeline.
// 24 ~ 31 bit: Reserved for layout stage in Pixel Pipeline.
// 32 ~ 39 bit: Reserved for execute UI OP stage in Pixel Pipeline.
// 40 ~ 47 bit: Reserved for paint stage in Pixel Pipeline.
// 48 ~ 63 bit: Flexible bits for extensibility.
// Use 2-bits for each feature flag to represent three states:
// true/false/undefine 00: represents undefine 01: represents enable feature
// flag 10: represents disable feature flag
// TODO: Need to add a TS definition for PipelineSchedulerConfig.
constexpr static uint64_t kEnableParallelParseElementTemplate = 1;
constexpr static uint64_t kEnableListBatchRenderMask = 1 << 8;
constexpr static uint64_t kEnableParallelElementMask = 1 << 16;
constexpr static uint64_t kDisableParallelElementMask = 1 << 17;
constexpr static uint64_t kEnableListBatchRenderAsyncResolvePropertyMask =
    1 << 18;
constexpr static uint64_t kEnableListBatchRenderAsyncResolveTreeMask = 1 << 20;

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

/**
 * PageConfig hold overall configs of a page.
 * When adding or modifying some properties, please modify
 * oliver/type-lynx/compile/page-config.d.ts at the same time.
 */
class PageConfig final : public EntryConfig {
 public:
  // Enable attribute flatten if it not defined in index.json
  // Attribute auto-expose is automatically opended
  PageConfig(){};

  ~PageConfig() override = default;

  // BEGIN CONFIG GET ADN SET FUNC GEN
  inline void SetExtraInfo(lepus::Value extra_info) {
    extra_info_ = extra_info;
  }
  inline lepus::Value GetExtraInfo() const { return extra_info_; }

  inline void SetVersion(const std::string& version) { version_ = version; }
  inline std::string GetVersion() const { return version_; }

  inline void SetCli(const std::string& cli) { cli_ = cli; }
  inline std::string GetCli() const { return cli_; }

  inline void SetReactVersion(const std::string& react_version) {
    react_version_ = react_version;
  }
  inline std::string GetReactVersion() const { return react_version_; }

  inline void SetCustomData(const std::string& custom_data) {
    custom_data_ = custom_data;
  }
  inline std::string GetCustomData() const { return custom_data_; }

  inline void SetTapSlop(const std::string& tap_slop) { tap_slop_ = tap_slop; }
  inline std::string GetTapSlop() const { return tap_slop_; }

  inline void SetPreferredFps(const std::string& preferred_fps) {
    preferred_fps_ = preferred_fps;
  }
  inline std::string GetPreferredFps() const { return preferred_fps_; }

  inline void SetLepusGCThreshold(int64_t lepus_gc_threshold) {
    lepus_gc_threshold_ = lepus_gc_threshold;
  }
  inline int64_t GetLepusGCThreshold() const { return lepus_gc_threshold_; }

  inline void SetObserverFrameRate(int32_t observer_frame_rate) {
    observer_frame_rate_ = observer_frame_rate;
  }
  inline int32_t GetObserverFrameRate() const { return observer_frame_rate_; }

  inline void SetLongPressDuration(int32_t long_press_duration) {
    long_press_duration_ = long_press_duration;
  }
  inline int32_t GetLongPressDuration() const { return long_press_duration_; }

  inline void SetLepusQuickjsStacksize(uint32_t lepus_quickjs_stacksize) {
    lepus_quickjs_stacksize_ = lepus_quickjs_stacksize;
  }
  inline uint32_t GetLepusQuickjsStacksize() const {
    return lepus_quickjs_stacksize_;
  }

  inline void SetLogBoxImageSizeWarningThreshold(
      uint32_t log_box_image_size_warning_threshold) {
    log_box_image_size_warning_threshold_ =
        log_box_image_size_warning_threshold;
  }
  inline uint32_t GetLogBoxImageSizeWarningThreshold() const {
    return log_box_image_size_warning_threshold_;
  }

  inline void SetBundleModuleMode(
      PackageInstanceBundleModuleMode bundle_module_mode) {
    bundle_module_mode_ = bundle_module_mode;
  }
  inline PackageInstanceBundleModuleMode GetBundleModuleMode() const {
    return bundle_module_mode_;
  }

  inline void SetMapContainerType(uint8_t map_container_type) {
    map_container_type_ = map_container_type;
  }
  inline uint8_t GetMapContainerType() const { return map_container_type_; }

  inline void SetDSL(PackageInstanceDSL dsl) { dsl_ = dsl; }
  inline PackageInstanceDSL GetDSL() const { return dsl_; }

  inline void SetUseNewImage(TernaryBool use_new_image) {
    use_new_image_ = use_new_image;
  }
  inline TernaryBool GetUseNewImage() const { return use_new_image_; }

  inline void SetEnableTextBoringLayout(TernaryBool enable_text_boring_layout) {
    enable_text_boring_layout_ = enable_text_boring_layout;
  }
  inline TernaryBool GetEnableTextBoringLayout() const {
    return enable_text_boring_layout_;
  }

  inline void SetEnableTextLayerRender(TernaryBool enable_text_layer_render) {
    enable_text_layer_render_ = enable_text_layer_render;
  }
  inline TernaryBool GetEnableTextLayerRender() const {
    return enable_text_layer_render_;
  }

  inline void SetEnableTextLayoutCache(TernaryBool enable_text_layout_cache) {
    enable_text_layout_cache_ = enable_text_layout_cache;
  }
  inline TernaryBool GetEnableTextLayoutCache() const {
    return enable_text_layout_cache_;
  }

  inline void SetEnableAsyncResolveSubtree(
      TernaryBool enable_async_resolve_subtree) {
    enable_async_resolve_subtree_ = enable_async_resolve_subtree;
  }
  inline TernaryBool GetEnableAsyncResolveSubtree() const {
    return enable_async_resolve_subtree_;
  }

  inline void SetEnableSignalAPI(TernaryBool enable_signal_api) {
    enable_signal_api_ = enable_signal_api;
  }
  inline TernaryBool GetEnableSignalAPI() const { return enable_signal_api_; }

  inline void SetTrailNewImage(TernaryBool trail_new_image) {
    trail_new_image_ = trail_new_image;
  }
  inline TernaryBool GetTrailNewImage() const { return trail_new_image_; }

  inline void SetAsyncRedirect(TernaryBool async_redirect) {
    async_redirect_ = async_redirect;
  }
  inline TernaryBool GetAsyncRedirect() const { return async_redirect_; }

  inline void SetEnableUseMapBuffer(TernaryBool enable_use_map_buffer) {
    enable_use_map_buffer_ = enable_use_map_buffer;
  }
  inline TernaryBool GetEnableUseMapBuffer() const {
    return enable_use_map_buffer_;
  }

  inline void SetEnableUIOperationOptimize(
      TernaryBool enable_ui_operation_optimize) {
    enable_ui_operation_optimize_ = enable_ui_operation_optimize;
  }
  inline TernaryBool GetEnableUIOperationOptimize() const {
    return enable_ui_operation_optimize_;
  }

  inline void SetEnableNativeList(TernaryBool enable_native_list) {
    enable_native_list_ = enable_native_list;
  }
  inline TernaryBool GetEnableNativeList() const { return enable_native_list_; }

  inline void SetEnableFiberElementForRadonDiff(
      TernaryBool enable_fiber_element_for_radon_diff) {
    enable_fiber_element_for_radon_diff_ = enable_fiber_element_for_radon_diff;
  }
  inline TernaryBool GetEnableFiberElementForRadonDiff() const {
    return enable_fiber_element_for_radon_diff_;
  }

  inline void SetEnableMicrotaskPromisePolyfill(
      TernaryBool enable_microtask_promise_polyfill) {
    enable_microtask_promise_polyfill_ = enable_microtask_promise_polyfill;
  }
  inline TernaryBool GetEnableMicrotaskPromisePolyfill() const {
    return enable_microtask_promise_polyfill_;
  }

  inline void SetEnableOptPushStyleToBundle(
      TernaryBool enable_opt_push_style_to_bundle) {
    enable_opt_push_style_to_bundle_ = enable_opt_push_style_to_bundle;
  }
  inline TernaryBool GetEnableOptPushStyleToBundle() const {
    return enable_opt_push_style_to_bundle_;
  }

  inline void SetEnableNativeScheduleCreateViewAsync(
      TernaryBool enable_native_schedule_create_view_async) {
    enable_native_schedule_create_view_async_ =
        enable_native_schedule_create_view_async;
  }
  inline TernaryBool GetEnableNativeScheduleCreateViewAsync() const {
    return enable_native_schedule_create_view_async_;
  }

  inline void SetEnableUnifiedPipeline(TernaryBool enable_unified_pipeline) {
    enable_unified_pipeline_ = enable_unified_pipeline;
  }
  inline TernaryBool GetEnableUnifiedPipeline() const {
    return enable_unified_pipeline_;
  }

  inline void SetFlatten(bool flatten) { flatten_ = flatten; }
  inline bool GetFlatten() const { return flatten_; }

  inline void SetImplicit(bool implicit) { implicit_ = implicit; }
  inline bool GetImplicit() const { return implicit_; }

  inline void SetLepusStrict(bool lepus_strict) {
    lepus_strict_ = lepus_strict;
  }
  inline bool GetLepusStrict() const { return lepus_strict_; }

  inline void SetLepusNullPropAsUndef(bool lepus_null_prop_as_undef) {
    lepus_null_prop_as_undef_ = lepus_null_prop_as_undef;
  }
  inline bool GetLepusNullPropAsUndef() const {
    return lepus_null_prop_as_undef_;
  }

  inline void SetDataStrictMode(bool data_strict_mode) {
    data_strict_mode_ = data_strict_mode;
  }
  inline bool GetDataStrictMode() const { return data_strict_mode_; }

  inline void SetEnableAsyncDisplay(bool enable_async_display) {
    enable_async_display_ = enable_async_display;
  }
  inline bool GetEnableAsyncDisplay() const { return enable_async_display_; }

  inline void SetEnableImageDownsampling(bool enable_image_downsampling) {
    enable_image_downsampling_ = enable_image_downsampling;
  }
  inline bool GetEnableImageDownsampling() const {
    return enable_image_downsampling_;
  }

  inline void SetEnableNewImage(bool enable_new_image) {
    enable_new_image_ = enable_new_image;
  }
  inline bool GetEnableNewImage() const { return enable_new_image_; }

  inline void SetEnableTextNonContiguousLayout(
      bool enable_text_non_contiguous_layout) {
    enable_text_non_contiguous_layout_ = enable_text_non_contiguous_layout;
  }
  inline bool GetEnableTextNonContiguousLayout() const {
    return enable_text_non_contiguous_layout_;
  }

  inline void SetEnableViewReceiveTouch(bool enable_view_receive_touch) {
    enable_view_receive_touch_ = enable_view_receive_touch;
  }
  inline bool GetEnableViewReceiveTouch() const {
    return enable_view_receive_touch_;
  }

  inline void SetEnableEventThrough(bool enable_event_through) {
    enable_event_through_ = enable_event_through;
  }
  inline bool GetEnableEventThrough() const { return enable_event_through_; }

  inline void SetRemoveComponentElement(bool remove_component_element) {
    remove_component_element_ = remove_component_element;
  }
  inline bool GetRemoveComponentElement() const {
    return remove_component_element_;
  }

  inline void SetStrictPropType(bool strict_prop_type) {
    strict_prop_type_ = strict_prop_type;
  }
  inline bool GetStrictPropType() const { return strict_prop_type_; }

  inline void SetSyncImageAttach(bool sync_image_attach) {
    sync_image_attach_ = sync_image_attach;
  }
  inline bool GetSyncImageAttach() const { return sync_image_attach_; }

  inline void SetUseImagePostProcessor(bool use_image_post_processor) {
    use_image_post_processor_ = use_image_post_processor;
  }
  inline bool GetUseImagePostProcessor() const {
    return use_image_post_processor_;
  }

  inline void SetUseNewSwiper(bool use_new_swiper) {
    use_new_swiper_ = use_new_swiper;
  }
  inline bool GetUseNewSwiper() const { return use_new_swiper_; }

  inline void SetEnableAsyncInitVideoEngine(
      bool enable_async_init_video_engine) {
    enable_async_init_video_engine_ = enable_async_init_video_engine;
  }
  inline bool GetEnableAsyncInitVideoEngine() const {
    return enable_async_init_video_engine_;
  }

  inline void SetEnableComponentLifecycleAlignWebview(
      bool enable_component_lifecycle_align_webview) {
    enable_component_lifecycle_align_webview_ =
        enable_component_lifecycle_align_webview;
  }
  inline bool GetEnableComponentLifecycleAlignWebview() const {
    return enable_component_lifecycle_align_webview_;
  }

  inline void SetEnableListNewArchitecture(bool enable_list_new_architecture) {
    enable_list_new_architecture_ = enable_list_new_architecture;
  }
  inline bool GetEnableListNewArchitecture() const {
    return enable_list_new_architecture_;
  }

  inline void SetEnableNewListContainer(bool enable_new_list_container) {
    enable_new_list_container_ = enable_new_list_container;
  }
  inline bool GetEnableNewListContainer() const {
    return enable_new_list_container_;
  }

  inline void SetEnableListPlug(bool enable_list_plug) {
    enable_list_plug_ = enable_list_plug;
  }
  inline bool GetEnableListPlug() const { return enable_list_plug_; }

  inline void SetEnableListMoveOperation(bool enable_list_move_operation) {
    enable_list_move_operation_ = enable_list_move_operation;
  }
  inline bool GetEnableListMoveOperation() const {
    return enable_list_move_operation_;
  }

  inline void SetEnableCreateViewAsync(bool enable_create_view_async) {
    enable_create_view_async_ = enable_create_view_async;
  }
  inline bool GetEnableCreateViewAsync() const {
    return enable_create_view_async_;
  }

  inline void SetEnableVsyncAlignedFlush(bool enable_vsync_aligned_flush) {
    enable_vsync_aligned_flush_ = enable_vsync_aligned_flush;
  }
  inline bool GetEnableVsyncAlignedFlush() const {
    return enable_vsync_aligned_flush_;
  }

  inline void SetEnableAccessibilityElement(bool enable_accessibility_element) {
    enable_accessibility_element_ = enable_accessibility_element;
  }
  inline bool GetEnableAccessibilityElement() const {
    return enable_accessibility_element_;
  }

  inline void SetEnableOverlapForAccessibilityElement(
      bool enable_overlap_for_accessibility_element) {
    enable_overlap_for_accessibility_element_ =
        enable_overlap_for_accessibility_element;
  }
  inline bool GetEnableOverlapForAccessibilityElement() const {
    return enable_overlap_for_accessibility_element_;
  }

  inline void SetEnableNewAccessibility(bool enable_new_accessibility) {
    enable_new_accessibility_ = enable_new_accessibility;
  }
  inline bool GetEnableNewAccessibility() const {
    return enable_new_accessibility_;
  }

  inline void SetEnableNewLayoutOnly(bool enable_new_layout_only) {
    enable_new_layout_only_ = enable_new_layout_only;
  }
  inline bool GetEnableNewLayoutOnly() const { return enable_new_layout_only_; }

  inline void SetEnableReactOnlyPropsId(bool enable_react_only_props_id) {
    enable_react_only_props_id_ = enable_react_only_props_id;
  }
  inline bool GetEnableReactOnlyPropsId() const {
    return enable_react_only_props_id_;
  }

  inline void SetEnableGlobalComponentMap(bool enable_global_component_map) {
    enable_global_component_map_ = enable_global_component_map;
  }
  inline bool GetEnableGlobalComponentMap() const {
    return enable_global_component_map_;
  }

  inline void SetEnableTextRefactor(bool enable_text_refactor) {
    enable_text_refactor_ = enable_text_refactor;
  }
  inline bool GetEnableTextRefactor() const { return enable_text_refactor_; }

  inline void SetEnableTextOverflow(bool enable_text_overflow) {
    enable_text_overflow_ = enable_text_overflow;
  }
  inline bool GetEnableTextOverflow() const { return enable_text_overflow_; }

  inline void SetEnableNewClipMode(bool enable_new_clip_mode) {
    enable_new_clip_mode_ = enable_new_clip_mode;
  }
  inline bool GetEnableNewClipMode() const { return enable_new_clip_mode_; }

  inline void SetAutoResumeAnimation(bool auto_resume_animation) {
    auto_resume_animation_ = auto_resume_animation;
  }
  inline bool GetAutoResumeAnimation() const { return auto_resume_animation_; }

  inline void SetEnableNewTransformOrigin(bool enable_new_transform_origin) {
    enable_new_transform_origin_ = enable_new_transform_origin;
  }
  inline bool GetEnableNewTransformOrigin() const {
    return enable_new_transform_origin_;
  }

  inline void SetEnableCircularDataCheck(bool enable_circular_data_check) {
    enable_circular_data_check_ = enable_circular_data_check;
  }
  inline bool GetEnableCircularDataCheck() const {
    return enable_circular_data_check_;
  }

  inline void SetEnableReduceInitDataCopy(bool enable_reduce_init_data_copy) {
    enable_reduce_init_data_copy_ = enable_reduce_init_data_copy;
  }
  inline bool GetEnableReduceInitDataCopy() const {
    return enable_reduce_init_data_copy_;
  }

  inline void SetEnableSimultaneousTap(bool enable_simultaneous_tap) {
    enable_simultaneous_tap_ = enable_simultaneous_tap;
  }
  inline bool GetEnableSimultaneousTap() const {
    return enable_simultaneous_tap_;
  }

  inline void SetEnableComponentLayoutOnly(bool enable_component_layout_only) {
    enable_component_layout_only_ = enable_component_layout_only;
  }
  inline bool GetEnableComponentLayoutOnly() const {
    return enable_component_layout_only_;
  }

  inline void SetExtendedLayoutOnlyOpt(bool extended_layout_only_opt) {
    extended_layout_only_opt_ = extended_layout_only_opt;
  }
  inline bool GetExtendedLayoutOnlyOpt() const {
    return extended_layout_only_opt_;
  }

  inline void SetEnableTouchRefactor(bool enable_touch_refactor) {
    enable_touch_refactor_ = enable_touch_refactor;
  }
  inline bool GetEnableTouchRefactor() const { return enable_touch_refactor_; }

  inline void SetEnableEndGestureAtLastFingerUp(
      bool enable_end_gesture_at_last_finger_up) {
    enable_end_gesture_at_last_finger_up_ =
        enable_end_gesture_at_last_finger_up;
  }
  inline bool GetEnableEndGestureAtLastFingerUp() const {
    return enable_end_gesture_at_last_finger_up_;
  }

  inline void SetDisableLongpressAfterScroll(
      bool disable_longpress_after_scroll) {
    disable_longpress_after_scroll_ = disable_longpress_after_scroll;
  }
  inline bool GetDisableLongpressAfterScroll() const {
    return disable_longpress_after_scroll_;
  }

  inline void SetKeyboardCallbackPassRelativeHeight(
      bool keyboard_callback_pass_relative_height) {
    keyboard_callback_pass_relative_height_ =
        keyboard_callback_pass_relative_height;
  }
  inline bool GetKeyboardCallbackPassRelativeHeight() const {
    return keyboard_callback_pass_relative_height_;
  }

  inline void SetEnableNewIntersectionObserver(
      bool enable_new_intersection_observer) {
    enable_new_intersection_observer_ = enable_new_intersection_observer;
  }
  inline bool GetEnableNewIntersectionObserver() const {
    return enable_new_intersection_observer_;
  }

  inline void SetEnableCheckDataWhenUpdatePage(
      bool enable_check_data_when_update_page) {
    enable_check_data_when_update_page_ = enable_check_data_when_update_page;
  }
  inline bool GetEnableCheckDataWhenUpdatePage() const {
    return enable_check_data_when_update_page_;
  }

  inline void SetForceCalcNewStyle(bool force_calc_new_style) {
    force_calc_new_style_ = force_calc_new_style;
  }
  inline bool GetForceCalcNewStyle() const { return force_calc_new_style_; }

  inline void SetEnableBackgroundShapeLayer(
      bool enable_background_shape_layer) {
    enable_background_shape_layer_ = enable_background_shape_layer;
  }
  inline bool GetEnableBackgroundShapeLayer() const {
    return enable_background_shape_layer_;
  }

  inline void SetCompileRender(bool compile_render) {
    compile_render_ = compile_render;
  }
  inline bool GetCompileRender() const { return compile_render_; }

  inline void SetEnableTextLanguageAlignment(
      bool enable_text_language_alignment) {
    enable_text_language_alignment_ = enable_text_language_alignment;
  }
  inline bool GetEnableTextLanguageAlignment() const {
    return enable_text_language_alignment_;
  }

  inline void SetEnableXTextLayoutReused(bool enable_x_text_layout_reused) {
    enable_x_text_layout_reused_ = enable_x_text_layout_reused;
  }
  inline bool GetEnableXTextLayoutReused() const {
    return enable_x_text_layout_reused_;
  }

  inline void SetEnableRemoveComponentExtraData(
      bool enable_remove_component_extra_data) {
    enable_remove_component_extra_data_ = enable_remove_component_extra_data;
  }
  inline bool GetEnableRemoveComponentExtraData() const {
    return enable_remove_component_extra_data_;
  }

  inline void SetEnableExposureUIMargin(bool enable_exposure_ui_margin) {
    enable_exposure_ui_margin_ = enable_exposure_ui_margin;
  }
  inline bool GetEnableExposureUIMargin() const {
    return enable_exposure_ui_margin_;
  }

  inline void SetEnableNewGesture(bool enable_new_gesture) {
    enable_new_gesture_ = enable_new_gesture;
  }
  inline bool GetEnableNewGesture() const { return enable_new_gesture_; }

  inline void SetEnableCheckLocalImage(bool enable_check_local_image) {
    enable_check_local_image_ = enable_check_local_image;
  }
  inline bool GetEnableCheckLocalImage() const {
    return enable_check_local_image_;
  }

  inline void SetEnableAsyncRequestImage(bool enable_async_request_image) {
    enable_async_request_image_ = enable_async_request_image;
  }
  inline bool GetEnableAsyncRequestImage() const {
    return enable_async_request_image_;
  }

  inline void SetEnableComponentNullProp(bool enable_component_null_prop) {
    enable_component_null_prop_ = enable_component_null_prop;
  }
  inline bool GetEnableComponentNullProp() const {
    return enable_component_null_prop_;
  }

  inline void SetEnableCascadePseudo(bool enable_cascade_pseudo) {
    enable_cascade_pseudo_ = enable_cascade_pseudo;
  }
  inline bool GetEnableCascadePseudo() const { return enable_cascade_pseudo_; }

  inline void SetRemoveDescendantSelectorScope(
      bool remove_descendant_selector_scope) {
    remove_descendant_selector_scope_ = remove_descendant_selector_scope;
  }
  inline bool GetRemoveDescendantSelectorScope() const {
    return remove_descendant_selector_scope_;
  }

  inline void SetAutoExpose(bool auto_expose) { auto_expose_ = auto_expose; }
  inline bool GetAutoExpose() const { return auto_expose_; }

  inline void SetDisableQuickTracingGC(bool disable_quick_tracing_gc) {
    disable_quick_tracing_gc_ = disable_quick_tracing_gc;
  }
  inline bool GetDisableQuickTracingGC() const {
    return disable_quick_tracing_gc_;
  }

  inline void SetFixCSSImportRuleOrder(bool fix_css_import_rule_order) {
    fix_css_import_rule_order_ = fix_css_import_rule_order;
  }
  inline bool GetFixCSSImportRuleOrder() const {
    return fix_css_import_rule_order_;
  }

  inline void SetEnableReloadLifecycle(bool enable_reload_lifecycle) {
    enable_reload_lifecycle_ = enable_reload_lifecycle;
  }
  inline bool GetEnableReloadLifecycle() const {
    return enable_reload_lifecycle_;
  }

  inline void SetEnableA11y(bool enable_a11y) { enable_a11y_ = enable_a11y; }
  inline bool GetEnableA11y() const { return enable_a11y_; }

  inline void SetEnableA11yIDMutationObserver(
      bool enable_a11y_id_mutation_observer) {
    enable_a11y_id_mutation_observer_ = enable_a11y_id_mutation_observer;
  }
  inline bool GetEnableA11yIDMutationObserver() const {
    return enable_a11y_id_mutation_observer_;
  }

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

  inline void SetEnableExposureWhenLayout(bool enable_exposure_when_layout) {
    enable_exposure_when_layout_ = enable_exposure_when_layout;
  }
  inline bool GetEnableExposureWhenLayout() const {
    return enable_exposure_when_layout_;
  }

  inline void SetEnableAirDetectRemovedKeysWhenUpdateData(
      bool enable_air_detect_removed_keys_when_update_data) {
    enable_air_detect_removed_keys_when_update_data_ =
        enable_air_detect_removed_keys_when_update_data;
  }
  inline bool GetEnableAirDetectRemovedKeysWhenUpdateData() const {
    return enable_air_detect_removed_keys_when_update_data_;
  }

  inline void SetEnableJSDataProcessor(bool enable_js_data_processor) {
    enable_js_data_processor_ = enable_js_data_processor;
  }
  inline bool GetEnableJSDataProcessor() const {
    return enable_js_data_processor_;
  }

  inline void SetEnableMultiTouch(bool enable_multi_touch) {
    enable_multi_touch_ = enable_multi_touch;
  }
  inline bool GetEnableMultiTouch() const { return enable_multi_touch_; }

  inline void SetEnableMultiTouchParamsCompatible(
      bool enable_multi_touch_params_compatible) {
    enable_multi_touch_params_compatible_ =
        enable_multi_touch_params_compatible;
  }
  inline bool GetEnableMultiTouchParamsCompatible() const {
    return enable_multi_touch_params_compatible_;
  }

  inline void SetEnableJsBindingApiThrowException(
      bool enable_js_binding_api_throw_exception) {
    enable_js_binding_api_throw_exception_ =
        enable_js_binding_api_throw_exception;
  }
  inline bool GetEnableJsBindingApiThrowException() const {
    return enable_js_binding_api_throw_exception_;
  }

  inline void SetEnableICU(bool enable_icu) { enable_icu_ = enable_icu; }
  inline bool GetEnableICU() const { return enable_icu_; }

  inline void SetEnableQueryComponentSync(bool enable_query_component_sync) {
    enable_query_component_sync_ = enable_query_component_sync;
  }
  inline bool GetEnableQueryComponentSync() const {
    return enable_query_component_sync_;
  }

  // END CONFIG GET ADN SET FUNC GEN

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
              need_post_to_platform_ = true;
            }
          }
        }

        // if is a uint64
        auto uint64_pair = GetFuncUint64Map().find(name);
        if (uint64_pair != GetFuncUint64Map().end()) {
          if (it->value.IsUint64()) {
            const auto [setter, getter] = uint64_pair->second;
            if ((this->*(getter))() == 0) {
              (this->*(setter))(it->value.GetUint64());
              need_post_to_platform_ = true;
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

  void ForEachUint64Config(
      const base::MoveOnlyClosure<uint64_t, const std::string&> func) {
    for (const auto& [name, pair] : GetFuncUint64Map()) {
      const auto [setter, getter] = pair;
      (this->*(setter))(func(name));
    }
  }

  std::unordered_map<std::string, std::string> GetPageConfigMap() {
    std::unordered_map<std::string, std::string> map;
    map.insert({"page_flatten", flatten_ ? "true" : "false"});
    map.insert({"target_sdk_version", target_sdk_version_});
    map.insert({"enable_lepus_ng", enable_lepus_ng_ ? "true" : "false"});
    map.insert({"react_version", react_version_});
    map.insert({"enable_css_parser", enable_css_parser_ ? "true" : "false"});
    map.insert({"absetting_disable_css_lazy_decode",
                absetting_disable_css_lazy_decode_});
    return map;
  }

  // TODO(yangguangzhao.solace): remove this function
  bool GetEnableSignalAPIBoolValue() {
    if (enable_signal_api_ == TernaryBool::UNDEFINE_VALUE) {
      enable_signal_api_ = LynxEnv::GetInstance().EnableSignalAPI()
                               ? TernaryBool::TRUE_VALUE
                               : TernaryBool::UNDEFINE_VALUE;
    }
    return enable_signal_api_ == TernaryBool::TRUE_VALUE;
  }

  // TODO(yangguangzhao.solace): remove this function
  bool GetEnableNativeScheduleCreateViewAsyncAsBool() const {
    if (enable_native_schedule_create_view_async_ ==
        TernaryBool::UNDEFINE_VALUE) {
      return LynxEnv::GetInstance().EnableNativeCreateViewAsync();
    } else {
      return enable_native_schedule_create_view_async_ ==
             TernaryBool::TRUE_VALUE;
    }
  }

  inline void SetOriginalConfig(std::string config_str) {
    original_config_ = std::move(config_str);
  }

  inline std::string GetOriginalConfig() { return original_config_; }

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

  inline void SetFontScaleEffectiveOnlyOnSp(bool font_scale) {
    layout_configs_.font_scale_sp_only_ = font_scale;
  }

  inline bool GetFontScaleEffectiveOnlyOnSp() {
    return layout_configs_.font_scale_sp_only_;
  }

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

  inline void SetEnableLepusNG(bool enable_lepus_ng) {
    enable_lepus_ng_ = enable_lepus_ng;
  }
  inline bool GetEnableLepusNG() { return enable_lepus_ng_; }

  void SetEnableSavePageData(bool enable) { enable_save_page_data_ = enable; }

  bool GetEnableSavePageData() { return enable_save_page_data_; }

  void SetListRemoveComponent(bool list_remove_component) {
    list_remove_component_ = list_remove_component;
  }
  bool GetListRemoveComponent() { return list_remove_component_; }

  void SetUnifyVWVHBehavior(bool unify) {
    css_configs_.unify_vw_vh_behavior_ = unify;
  }
  bool GetUnifyVWVHBehavior() { return css_configs_.unify_vw_vh_behavior_; }

  inline bool GetEnableZIndex() { return enable_z_index_; }
  inline void SetEnableZIndex(bool enable) { enable_z_index_ = enable; }

  inline bool GetEnableLynxAir() { return enable_lynx_air_; }
  inline void SetEnableLynxAir(bool enable) { enable_lynx_air_ = enable; }
  inline bool GetEnableFiberArch() { return enable_fiber_arch_; }
  inline void SetEnableFiberArch(bool enable) { enable_fiber_arch_ = enable; }

  inline bool GetEnableCSSParser() { return enable_css_parser_; }
  inline void SetEnableCSSParser(bool enable) { enable_css_parser_ = enable; }

  inline std::string GetAbSettingDisableCSSLazyDecode() {
    return absetting_disable_css_lazy_decode_;
  }
  inline void SetAbSettingDisableCSSLazyDecode(std::string disable) {
    absetting_disable_css_lazy_decode_ = disable;
  }

  inline void SetEnableEventRefactor(bool option) {
    enable_event_refactor_ = option;
  }

  bool GetEnableEventRefactor() const { return enable_event_refactor_; }

  int32_t GetIncludeFontPadding() const { return include_font_padding_; }

  void SetIncludeFontPadding(bool value) {
    include_font_padding_ = value ? 1 : -1;
  }

  inline void SetLynxAirMode(CompileOptionAirMode air_mode) {
    air_mode_ = air_mode;
  }

  inline CompileOptionAirMode GetLynxAirMode() { return air_mode_; }

  inline bool GetEnableRasterAnimation() const {
    return enable_raster_animation_;
  }
  inline void SetEnableRasterAnimation(bool value) {
    enable_raster_animation_ = value;
  }

  inline bool GetEnableCSSInvalidation() const {
    return enable_css_invalidation_;
  }

  inline void SetEnableCSSInvalidation(bool enable) {
    enable_css_invalidation_ = enable;
  }

  inline bool GetEnableParallelParseElementTemplate() {
    return pipeline_scheduler_config_ & kEnableParallelParseElementTemplate;
  }

  bool GetEnableParallelElement() const;

  inline void SetEnableParallelElement(bool enable) {
    enable_parallel_element_ = enable;
  }

  inline uint64_t GetPipelineSchedulerConfig() const {
    return pipeline_scheduler_config_;
  }

  inline void SetPipelineSchedulerConfig(uint64_t config) {
    pipeline_scheduler_config_ = config;
  }

  bool GetEnableStandardCSSSelector() const {
    return enable_standard_css_selector_;
  }

  void SetEnableStandardCSSSelector(bool enable) {
    enable_standard_css_selector_ = enable;
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

  void SetEnableComponentAsyncDecode(TernaryBool enable) {
    enable_component_async_decode_ = enable;
  }

  void SetEnableUseContextPool(TernaryBool enable) {
    enable_use_context_pool_ = enable;
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

  void SetEnableElementAPITypeCheckThrowWarning(bool enable) {
    enable_element_api_type_check_throw_warning_ = enable;
  }

  bool GetEnableElementAPITypeCheckThrowWarning() {
    return enable_element_api_type_check_throw_warning_;
  }

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

  // TODO(songshourui.null): move this function to testing file
  void PrintPageConfig(std::ostream& output) {
#define PAGE_CONFIG_DUMP(key) output << #key << ":" << key << ",";
    output << "page_version:" << version_ << ",";
    output << "page_flatten:" << flatten_ << ",";
    output << "page_implicit:" << implicit_ << ",";
    output << "dsl_:" << static_cast<int>(dsl_) << ",";
    output << "enable_auto_show_hide:" << auto_expose_ << ",";
    output << "bundle_module_mode_:" << static_cast<int>(bundle_module_mode_)
           << ",";
    PAGE_CONFIG_DUMP(enable_async_display_)
    PAGE_CONFIG_DUMP(enable_view_receive_touch_)
    output << "enable_lepus_strict_check_:" << lepus_strict_ << ",";
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

  bool NeedPostToPlatform() const { return need_post_to_platform_; }

  // TODO(zhoupeng.z): remove this method after pre-postings applied on all
  // platforms.
  void MarkPostToPlatform() { need_post_to_platform_ = false; }

 private:
  tasm::DynamicCSSConfigs css_configs_;
  CSSParserConfigs css_parser_configs_;
  std::string target_sdk_version_;
  std::string lepus_version_;
  std::string absetting_disable_css_lazy_decode_;
  std::string original_config_{};

  // BEGIN CONFIG MEMBER GEN
  lepus::Value extra_info_{};
  std::string version_{""};
  std::string cli_{""};
  std::string react_version_{""};
  std::string custom_data_{""};
  std::string tap_slop_{"50px"};
  std::string preferred_fps_{"auto"};
  int64_t lepus_gc_threshold_{256};
  int32_t observer_frame_rate_{20};
  int32_t long_press_duration_{-1};
  uint32_t lepus_quickjs_stacksize_{0};
  uint32_t log_box_image_size_warning_threshold_{1000000};
  PackageInstanceBundleModuleMode bundle_module_mode_{
      PackageInstanceBundleModuleMode::EVAL_REQUIRE_MODE};
  uint8_t map_container_type_{0};
  PackageInstanceDSL dsl_{PackageInstanceDSL::TT};
  TernaryBool use_new_image_{TernaryBool::UNDEFINE_VALUE};
  TernaryBool enable_text_boring_layout_{TernaryBool::UNDEFINE_VALUE};
  TernaryBool enable_text_layer_render_{TernaryBool::UNDEFINE_VALUE};
  TernaryBool enable_text_layout_cache_{TernaryBool::UNDEFINE_VALUE};
  TernaryBool enable_async_resolve_subtree_{TernaryBool::UNDEFINE_VALUE};
  TernaryBool enable_signal_api_{TernaryBool::UNDEFINE_VALUE};
  TernaryBool trail_new_image_{TernaryBool::UNDEFINE_VALUE};
  TernaryBool async_redirect_{TernaryBool::UNDEFINE_VALUE};
  TernaryBool enable_use_map_buffer_{TernaryBool::UNDEFINE_VALUE};
  TernaryBool enable_ui_operation_optimize_{TernaryBool::UNDEFINE_VALUE};
  TernaryBool enable_native_list_{TernaryBool::UNDEFINE_VALUE};
  TernaryBool enable_fiber_element_for_radon_diff_{TernaryBool::UNDEFINE_VALUE};
  TernaryBool enable_microtask_promise_polyfill_{TernaryBool::UNDEFINE_VALUE};
  TernaryBool enable_opt_push_style_to_bundle_{TernaryBool::UNDEFINE_VALUE};
  TernaryBool enable_native_schedule_create_view_async_{
      TernaryBool::UNDEFINE_VALUE};
  TernaryBool enable_unified_pipeline_{TernaryBool::UNDEFINE_VALUE};
  bool flatten_{true};
  bool implicit_{true};
  bool lepus_strict_{false};
  bool lepus_null_prop_as_undef_{false};
  bool data_strict_mode_{true};
  bool enable_async_display_{true};
  bool enable_image_downsampling_{false};
  bool enable_new_image_{true};
  bool enable_text_non_contiguous_layout_{true};
  bool enable_view_receive_touch_{false};
  bool enable_event_through_{false};
  bool remove_component_element_{false};
  bool strict_prop_type_{false};
  bool sync_image_attach_{true};
  bool use_image_post_processor_{false};
  bool use_new_swiper_{true};
  bool enable_async_init_video_engine_{false};
  bool enable_component_lifecycle_align_webview_{false};
  bool enable_list_new_architecture_{false};
  bool enable_new_list_container_{false};
  bool enable_list_plug_{false};
  bool enable_list_move_operation_{false};
  bool enable_create_view_async_{true};
  bool enable_vsync_aligned_flush_{false};
  bool enable_accessibility_element_{true};
  bool enable_overlap_for_accessibility_element_{true};
  bool enable_new_accessibility_{false};
  bool enable_new_layout_only_{true};
  bool enable_react_only_props_id_{false};
  bool enable_global_component_map_{false};
  bool enable_text_refactor_{false};
  bool enable_text_overflow_{false};
  bool enable_new_clip_mode_{true};
  bool auto_resume_animation_{true};
  bool enable_new_transform_origin_{true};
  bool enable_circular_data_check_{true};
  bool enable_reduce_init_data_copy_{false};
  bool enable_simultaneous_tap_{false};
  bool enable_component_layout_only_{false};
  bool extended_layout_only_opt_{false};
  bool enable_touch_refactor_{true};
  bool enable_end_gesture_at_last_finger_up_{false};
  bool disable_longpress_after_scroll_{false};
  bool keyboard_callback_pass_relative_height_{false};
  bool enable_new_intersection_observer_{false};
  bool enable_check_data_when_update_page_{true};
  bool force_calc_new_style_{true};
  bool enable_background_shape_layer_{true};
  bool compile_render_{false};
  bool enable_text_language_alignment_{false};
  bool enable_x_text_layout_reused_{false};
  bool enable_remove_component_extra_data_{false};
  bool enable_exposure_ui_margin_{false};
  bool enable_new_gesture_{false};
  bool enable_check_local_image_{true};
  bool enable_async_request_image_{false};
  bool enable_component_null_prop_{false};
  bool enable_cascade_pseudo_{false};
  bool remove_descendant_selector_scope_{true};
  bool auto_expose_{true};
  bool disable_quick_tracing_gc_{false};
  bool fix_css_import_rule_order_{true};
  bool enable_reload_lifecycle_{false};
  bool enable_a11y_{false};
  bool enable_a11y_id_mutation_observer_{false};
  bool enable_check_exposure_optimize_{false};
  bool enable_disexposure_when_lynx_hidden_{true};
  bool enable_exposure_when_layout_{false};
  bool enable_air_detect_removed_keys_when_update_data_{false};
  bool enable_js_data_processor_{false};
  bool enable_multi_touch_{false};
  bool enable_multi_touch_params_compatible_{false};
  bool enable_js_binding_api_throw_exception_{false};
  bool enable_icu_{false};
  bool enable_query_component_sync_{false};
  // END CONFIG MEMBER GEN

  bool css_align_with_legacy_w3c_{false};
  bool enable_lepus_ng_{true};
  bool default_overflow_visible_{false};
  bool enable_save_page_data_{false};
  bool list_remove_component_{false};
  bool enable_z_index_{false};
  bool enable_lynx_air_{false};
  bool enable_fiber_arch_{false};
  // Used for lynx config
  bool enable_css_parser_{false};
  // default include font padding
  // 1 means true
  // -1 means false
  int32_t include_font_padding_{0};

  // page's engine version controller
  bool is_target_sdk_verion_higher_than_2_1_{false};
  bool enable_event_refactor_{true};

  CompileOptionAirMode air_mode_{CompileOptionAirMode::AIR_MODE_OFF};

  // support CSS invalidation
  bool enable_css_invalidation_{false};

  // indicate that enable standard css selector
  bool enable_standard_css_selector_{false};

  // Enable lazy_bundles to be decoded in child threads before they are
  // delivered into tasm in async-loading.
  TernaryBool enable_component_async_decode_{TernaryBool::UNDEFINE_VALUE};

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

  // enable avoid throwing RenderFatal for element api when argument type
  // checking failed
  bool enable_element_api_type_check_throw_warning_{false};

  // CSSLazyImport
  TernaryBool enable_css_lazy_import_{TernaryBool::UNDEFINE_VALUE};

  // enableNewAnimator
  TernaryBool enable_new_animator_{TernaryBool::UNDEFINE_VALUE};

  // Composite config representing configs including enableParallelElement,
  // batch-rendering
  uint64_t pipeline_scheduler_config_{0};

  /**
   * Not a config but a marker to indicate whether the page config needs to be
   * posted to platform layer. In PreDecode, PageConfig will be set to platform
   * layer before `LoadBundle`, so that it does not need to be posted to
   * platform layer again, which can reduce the overhead of posting. This
   * optimization is only valid for Android now.
   * TODO(zhoupeng.z): Apply this optimization to all platforms.
   */
  bool need_post_to_platform_{true};

  template <typename T>
  using PageConfigSetter = void (PageConfig::*)(T);

  template <typename T>
  using PageConfigGetter = T (PageConfig::*)() const;

  template <typename T>
  using PageConfigPair = std::pair<PageConfigSetter<T>, PageConfigGetter<T>>;

  template <typename T>
  using PageConfigMap = std::unordered_map<std::string, PageConfigPair<T>>;

  static const PageConfigMap<TernaryBool>& GetFuncBoolMap();

  static const PageConfigMap<uint64_t>& GetFuncUint64Map();
};
}  // namespace tasm
}  // namespace lynx

#endif  // CORE_TEMPLATE_BUNDLE_TEMPLATE_CODEC_BINARY_DECODER_PAGE_CONFIG_H_
