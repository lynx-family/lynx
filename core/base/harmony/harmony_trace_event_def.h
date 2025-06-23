// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_BASE_HARMONY_HARMONY_TRACE_EVENT_DEF_H_
#define CORE_BASE_HARMONY_HARMONY_TRACE_EVENT_DEF_H_

#include "core/base/lynx_trace_categories.h"

#if ENABLE_TRACE_PERFETTO || ENABLE_TRACE_SYSTRACE

inline constexpr const char* const IMAGE_HELPER_DECODE_IMAGE_SYNC =
    "LynxImageHelper::DecodeImageSync";
inline constexpr const char* const NAPI_CONVERT_HELPER_JSON_TO_LEPUS_VALUE =
    "NapiConvertHelper::JSONToLepusValue";
inline constexpr const char* const SETUP_ARK_TS_RUNTIME = "SetupArkTSRuntime";
inline constexpr const char* const FONT_FACE_PARSE_AND_ADD_SRC =
    "FontFace::ParseAndAddSrc";
inline constexpr const char* const LEPUS_VALUE_TO_NAPI_VALUE =
    "LepusValueToNapiValue";
inline constexpr const char* const CALL_JSB_ON_ARK_TS = "CallJSBOnArkTS";
inline constexpr const char* const PUB_VALUE_TO_LEPUS_VALUE =
    "PubValueToLepusValue";
inline constexpr const char* const HARMONY_INVOKE_NATIVE_MODULE =
    "InvokeNativeModule";
inline constexpr const char* const INVOKE_CALLBACK_ON_UI_THREAD =
    "InvokeCallbackOnUIThread";
inline constexpr const char* const NAPI_VALUE_TO_LEPUS_VALUE =
    "NapiValueToLepusValue";
inline constexpr const char* const
    NATIVE_MODULE_HARMONY_CALLBACK_THREAD_SWITCH_START =
        "JSBTiming::jsb_callback_thread_switch_start";
inline constexpr const char* const LOAD_JS_SOURCE = "LoadJSSource";
inline constexpr const char* const BASE_TEXT_SHADOW_NODE_LOAD_FONT_FAMILY =
    "BaseTextShadowNode::LoadFontFamily";
inline constexpr const char* const IMAGE_SHADOW_NODE_MEASURE =
    "ImageShadowNode::Measure";
inline constexpr const char* const IMAGE_SHADOW_NODE_JUST_SIZE =
    "ImageShadowNode::JustSize";
inline constexpr const char* const INPUT_SHADOW_NODE_MEASURE =
    "InputShadowNode::Measure";
inline constexpr const char* const JS_SHADOW_NODE_REQUEST_LAYOUT =
    "JSShadowNode::RequestLayout";
inline constexpr const char* const JS_SHADOW_NODE_MEASURE =
    "JSShadowNode::Measure";
inline constexpr const char* const OVERLAY_SHADOW_NODE_MEASURE =
    "OverlayShadowNode::Measure";
inline constexpr const char* const SHADOW_NODE_OWNER_CREATE_SHADOW_NODE =
    "ShadowNodeOwner::CreateShadowNode.";
inline constexpr const char* const TEXT_SHADOW_NODE_MEASURE =
    "TextShadowNode::Measure";
inline constexpr const char* const TEXT_SHADOW_NODE_LAYOUT =
    "TextShadowNode::Layout";
inline constexpr const char* const
    TEXT_SHADOW_NODE_HANDLE_TEXT_OVERFLOW_AND_TRUNCATION =
        "TextShadowNode::HandleTextOverflowAndTruncation";
inline constexpr const char* const UIBASE_INPUT_ON_MEASURE =
    "UIBaseInput::OnMeasure";
inline constexpr const char* const UIBASE_UPDATE_LAYOUT =
    "UIBase::UpdateLayout";
inline constexpr const char* const UIBASE_APPLY_TRANSFORM =
    "UIBase::ApplyTransform";
inline constexpr const char* const UIBASE_CREATE_OR_UPDATE_BACKGROUND =
    "UIBase::CreateOrUpdateBackground";
inline constexpr const char* const UIBASE_CREATE_ON_DRAW = "UIBase::OnDraw";
inline constexpr const char* const UIBASE_INIT_DRAW_NODE =
    "UIBase::InitDrawNode";
inline constexpr const char* const UI_EXPOSURE_EXEC = "UIExposure::Exec";
inline constexpr const char* const UI_FOLD_VIEW_UPDATE_LAYOUT =
    "UIFoldView::UpdateLayout";
inline constexpr const char* const UI_FOLD_VIEW_UPDATE_FOLD_VIEW_LAYOUT =
    "UIFoldView::UpdateFoldViewLayout";
inline constexpr const char* const UI_IMAGE_UPDATE_LAYOUT =
    "UIImage::UpdateLayout";
inline constexpr const char* const UI_LIST_ON_MEASURE = "UIList::OnMeasure";
inline constexpr const char* const UI_LIST_UPDATE_CONTENT_SIZE =
    "UIList::UpdateContentSize";
inline constexpr const char* const UI_OWNER_CREATE_JS_UI =
    "UIOwner::CreateJSUI";
inline constexpr const char* const UI_OWNER_CREATE_UI = "UIOwner::CreateUI.";
inline constexpr const char* const UI_OWNER_UPDATE_UI = "UIOwner::UpdateUI";
inline constexpr const char* const UI_OWNER_ON_NODE_READY =
    "UIOwner::OnNodeReady";
inline constexpr const char* const UI_OWNER_UPDATE_LAYOUT =
    "UIOwner::UpdateLayout";
inline constexpr const char* const UI_OWNER_ON_LAYOUT_FINISH =
    "UIOwner::OnLayoutFinish";
inline constexpr const char* const UI_OWNER_ATTACH_PAGE_ROOT =
    "UIOwner::AttachPageRoot";
inline constexpr const char* const UI_OWNER_START_FLUENCY_TRACE =
    "StartFluencyTrace";
inline constexpr const char* const UI_OWNER_STOP_FLUENCY_TRACE =
    "StopFluencyTrace";
inline constexpr const char* const UI_ROOT_CONSTRUCTOR = "UIRoot::UIRoot";
inline constexpr const char* const UI_SCROLL_ON_MEASURE = "UIScroll::OnMeasure";
inline constexpr const char* const UI_SCROLL_UPDATE_CONTENT_SIZE =
    "UIScroll::UpdateContentSize";
inline constexpr const char* const UI_SWIPER_ON_PROP_UPDATE =
    "UISwiper::OnPropUpdate";
inline constexpr const char* const UI_TEXT_AREA_ON_PROP_UPDATE =
    "UITextArea::OnPropUpdate";
inline constexpr const char* const NODE_MANAGER_MEASURE_NODE =
    "NodeManager::MeasureNode";
inline constexpr const char* const NODE_MANAGER_LAYOUT_NODE =
    "NodeManager::LayoutNode";
#endif  // #if ENABLE_TRACE_PERFETTO || ENABLE_TRACE_SYSTRACE

#endif  // CORE_BASE_HARMONY_HARMONY_TRACE_EVENT_DEF_H_
