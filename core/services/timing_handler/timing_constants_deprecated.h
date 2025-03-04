// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef CORE_SERVICES_TIMING_HANDLER_TIMING_CONSTANTS_DEPRECATED_H_
#define CORE_SERVICES_TIMING_HANDLER_TIMING_CONSTANTS_DEPRECATED_H_
namespace lynx {
namespace tasm {
namespace timing {

// Timing Setter
/*
 * Polyfill Keys
 * All constants in this file are used for the old onSetup/onUpdate callbacks of
 * the Timing API, and they are no longer maintained starting from Lynx 3.1.
 * Please do not modify any constants in this file unless absolutely necessary.
 */
// come from framework
static constexpr const char* kUpdateSetStateTrigger =
    "update_set_state_trigger";
static constexpr const char kUpdateDiffVdomStart[] = "update_diff_vdom_start";
static constexpr const char kUpdateDiffVdomEnd[] = "update_diff_vdom_end";
// Setup & Update Timing
static constexpr const char kSetStateTrigger[] = "set_state_trigger";
static constexpr const char kOpenTimePolyfill[] = "open_time";
static constexpr const char kContainerInitStartPolyfill[] =
    "container_init_start";
static constexpr const char kContainerInitEndPolyfill[] = "container_init_end";
static constexpr const char kPrepareTemplateStartPolyfill[] =
    "prepare_template_start";
static constexpr const char kPrepareTemplateEndPolyfill[] =
    "prepare_template_end";
static constexpr const char kCreateLynxStartPolyfill[] = "create_lynx_start";
static constexpr const char kCreateLynxEndPolyfill[] = "create_lynx_end";
static constexpr const char kResolveStartPolyfill[] = "dispatch_start";
static constexpr const char kResolveEndPolyfill[] = "dispatch_end";
static constexpr const char kReloadFromBackgroundPolyfill[] = "reload_from_ks";
static constexpr const char kLoadBundleStartPolyfill[] = "load_template_start";
static constexpr const char kLoadBundleEndPolyfill[] = "load_template_end";
static constexpr const char kDataProcessorStartPolyfill[] =
    "data_processor_start";
static constexpr const char kDataProcessorEndPolyfill[] = "data_processor_end";
static constexpr const char kSetInitDataStartPolyfill[] = "set_init_data_start";
static constexpr const char kSetInitDataEndPolyfill[] = "set_init_data_end";
static constexpr const char kParseStartPolyfill[] = "decode_start";
static constexpr const char kParseEndPolyfill[] = "decode_end";
static constexpr const char kTemplateBundleParseStartPolyfill[] =
    "template_bundle_decode_start";
static constexpr const char kTemplateBundleParseEndPolyfill[] =
    "template_bundle_decode_end";
static constexpr const char kPaintEndPolyfill[] = "draw_end";  // paint
static constexpr const char kVmExecuteStartPolyfill[] = "lepus_excute_start";
static constexpr const char kVmExecuteEndPolyfill[] = "lepus_excute_end";
static constexpr const char kMtsRenderStartPolyfill[] = "create_vdom_start";
static constexpr const char kMtsRenderEndPolyfill[] = "create_vdom_end";
static constexpr const char kPaintingUiOperationExecuteStartPolyfill[] =
    "ui_operation_flush_start";
static constexpr const char kLayoutUiOperationExecuteEndPolyfill[] =
    "ui_operation_flush_end";
static constexpr const char kPaintingUiOperationExecuteEndPolyfill[] =
    "painting_ui_operation_flush_end";
static constexpr const char kLayoutUiOperationExecuteStartPolyfill[] =
    "layout_ui_operation_flush_start";
static constexpr const char kLayoutStartPolyfill[] = "layout_start";  // layout
static constexpr const char kLayoutEndPolyfill[] = "layout_end";
static constexpr const char kLoadBackgroundStartPolyfill[] = "load_app_start";
static constexpr const char kLoadBackgroundEndPolyfill[] = "load_app_end";
static constexpr const char kLoadCoreStartPolyfill[] = "load_core_start";
static constexpr const char kLoadCoreEndPolyfill[] = "load_core_end";
static constexpr const char kPipelineStartPolyfill[] = "pipeline_start";
static constexpr const char kPipelineEndPolyfill[] = "pipeline_end";
// metric
static constexpr const char kLynxTTIPolyfill[] = "lynx_tti";
static constexpr const char kTotalTTIPolyfill[] = "total_tti";
static constexpr const char kActualFMPPolyfill[] = "actual_fmp";
static constexpr const char kLynxActualFMPPolyfill[] = "lynx_actual_fmp";
static constexpr const char kTotalActualFMPPolyfill[] = "total_actual_fmp";
static constexpr const char kLynxFCPPolyfill[] = "lynx_fcp";
static constexpr const char kTotalFCPPolyfill[] = "total_fcp";

// List Timing
// TODO(zhangkaijie.9): PerformanceObserver Compatible List
static constexpr const char kListRenderChildrenStart[] =
    "list_render_children_start";
static constexpr const char kListRenderChildrenEnd[] =
    "list_render_children_end";
static constexpr const char kListPatchChangesStart[] =
    "list_patch_changes_start";
static constexpr const char kListPatchChangesEnd[] = "list_patch_changes_end";
static constexpr const char kListDiffVdomStart[] = "list_diff_vdom_start";
static constexpr const char kListDiffVdomEnd[] = "list_diff_vdom_end";

// Air Timing
static constexpr const char* kRenderPageStartAir = "render_page_start_air";
static constexpr const char* kRenderPageEndAir = "render_page_end_air";
static constexpr const char* kRefreshPageStartAir = "refresh_page_start_air";
static constexpr const char* kRefreshPageEndAir = "refresh_page_end_air";
// SSR Timing
static constexpr const char kRenderPageStartSSR[] = "render_page_start_ssr";
static constexpr const char kRenderPageEndSSR[] = "render_page_end_ssr";
static constexpr const char kDecodeStartSSR[] = "decode_start_ssr";
static constexpr const char kDecodeEndSSR[] = "decode_end_ssr";
static constexpr const char kDispatchStartSSR[] = "dispatch_start_ssr";
static constexpr const char kDispatchEndSSR[] = "dispatch_end_ssr";
static constexpr const char kCreateVDomStartSSR[] = "create_vdom_start_ssr";
static constexpr const char kCreateVDomEndSSR[] = "create_vdom_end_ssr";
static constexpr const char kDrawEndSSR[] = "draw_end_ssr";

// Duration of timing
static constexpr const char* kCreateLynxView = "create_lynx_view";
static constexpr const char* kPrepareTemplate = "prepare_template";
static constexpr const char* kLoadTemplate = "load_template";
static constexpr const char* kReloadTemplate = "reload_template";
static constexpr const char* kLoadSSRData = "load_ssr_data";
static constexpr const char* kDecode = "decode";
static constexpr const char* kSetupLepusExecute = "setup_lepus_execute";
static constexpr const char* kSetupDataProcessor = "setup_data_processor";
static constexpr const char* kSetupSetInitData = "setup_set_init_data";
static constexpr const char* kSetupCreateVDom = "setup_create_vdom";
static constexpr const char* kSetupDispatch = "setup_dispatch";
static constexpr const char* kSetupLayout = "setup_layout";
static constexpr const char* kSetupUiOperationFlush =
    "setup_ui_operation_flush";
static constexpr const char* kSetupPaintingUiOperationFlush =
    "setup_painting_ui_operation_flush";
static constexpr const char* kSetupLayoutUiOperationFlush =
    "setup_layout_ui_operation_flush";
static constexpr const char* kSetupDrawWaiting = "setup_draw_waiting";
static constexpr const char* kSetupLoadTemplateWaiting =
    "setup_load_template_waiting";
static constexpr const char* kLoadApp = "load_app";
static constexpr const char* kLoadCore = "load_core";
static constexpr const char* kUpdateCreateVDom = "update_create_vdom";
static constexpr const char* kUpdateDispatch = "update_dispatch";
static constexpr const char* kUpdateLayout = "update_layout";
static constexpr const char* kUpdateUiOperationFlush =
    "update_ui_operation_flush";
static constexpr const char* kUpdatePaintingUiOperationFlush =
    "update_painting_ui_operation_flush";
static constexpr const char* kUpdateLayoutUiOperationFlush =
    "update_layout_ui_operation_flush";
static constexpr const char* kUpdateDrawWaiting = "update_draw_waiting";
static constexpr const char* kUpdateTriggerWaiting = "update_trigger_waiting";
static constexpr const char* kUpdateWaiting = "update_waiting";
static constexpr const char* kUpdateTiming = "update_timing";
static constexpr const char* kLoadTemplateToUpdateDrawEnd =
    "load_template_to_update_draw_end";
static constexpr const char* kListRenderChildren = "list_render_children";
static constexpr const char* kListPatchChanges = "list_patch_changes";
static constexpr const char* kListDiffVdom = "list_diff_vdom";

// Events of timing
static constexpr const char* kLynxSDKSetupTiming = "lynxsdk_setup_timing";
static constexpr const char* kLynxSDKUpdateTiming = "lynxsdk_update_timing";
static constexpr const char kSetupRuntimeCallback[] =
    "lynx.performance.timing.onSetup";
static constexpr const char kUpdateRuntimeCallback[] =
    "lynx.performance.timing.onUpdate";

// Props of timing events
static constexpr const char kSetupTiming[] = "setup_timing";
static constexpr const char kExtraTiming[] = "extra_timing";
static constexpr const char kUpdateTimings[] = "update_timings";
static constexpr const char kMetrics[] = "metrics";
static constexpr const char kThreadStrategy[] = "thread_strategy";
static constexpr const char kHasReload[] = "has_reload";
static constexpr const char kUseNativeTiming[] = "use_native_timing";
static constexpr const char kURL[] = "url";
static constexpr const char kUpdateFlag[] = "update_flag";
static constexpr const char kTemplateBundleDecode[] = "template_bundle_decode";

// Props of timing events - SSR
static constexpr const char kSSRMetrics[] = "ssr_metrics";
static constexpr const char kSSRRenderPage[] = "ssr_render_page_timing";
static constexpr const char kSSRExtraInfo[] = "ssr_extra_info";
static constexpr const char kSSRLynxTTI[] = "lynx_tti_ssr";
static constexpr const char kSSRLynxFCP[] = "lynx_fcp_ssr";

// Others
static constexpr const char kSetupPrefix[] = "setup_";
static constexpr const char kUpdatePrefix[] = "update_";
static constexpr const char* kLynxTimingActualFMPFlag =
    "__lynx_timing_actual_fmp";
static constexpr const char* kTimingFlag = "__lynx_timing_flag";

// Others - SSR
static constexpr const char* kSSRSuffix = "_ssr";
static constexpr const char kSSRExtraInfoDataSize[] = "data_size";

}  // namespace timing
}  // namespace tasm
}  // namespace lynx

#endif  // CORE_SERVICES_TIMING_HANDLER_TIMING_CONSTANTS_DEPRECATED_H_
