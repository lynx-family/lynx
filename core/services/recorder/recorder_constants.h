// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SERVICES_RECORDER_RECORDER_CONSTANTS_H_
#define CORE_SERVICES_RECORDER_RECORDER_CONSTANTS_H_

#include <cstdint>

namespace lynx {
namespace tasm {
namespace recorder {

// for TestBenchBaseRecorder
constexpr const char* kActionList = "Action List";
constexpr const char* kFunctionName = "Function Name";
constexpr const char* kInvokedMethodData = "Invoked Method Data";
constexpr const char* kScripts = "Scripts";
constexpr const char* kConfig = "Config";
constexpr const char* kComponentList = "Component List";
constexpr const char* kComponentName = "Name";
constexpr const char* kComponentType = "Type";
constexpr const char* kMethodName = "Method Name";
constexpr const char* kModuleName = "Module Name";
constexpr const char* kParams = "Params";
constexpr const char* kParamRecordTime = "Record Time";
constexpr const char* kParamRecordMillisecond = "RecordMillisecond";
constexpr const char* kCallback = "Callback";
constexpr uint32_t kFilenameBufferSize = 256;
constexpr uint32_t kFileDataBufferSize = 65536;
constexpr int64_t kRecordIDForGlobalEvent = -1;
constexpr const char* KJsbIgnoredInfo = "[]";
constexpr const char* kStrict = "strict";

// for LynxViewInitRecorder
constexpr const char* kParamThreadStrategy = "threadStrategy";
constexpr const char* kParamEnableJSRuntime = "enableJSRuntime";
constexpr const char* kParamLayoutHeightMode = "layoutHeightMode";
constexpr const char* kParamLayoutWidthMode = "layoutWidthMode";
constexpr const char* kParamPreferredLayoutHeight = "preferredLayoutHeight";
constexpr const char* kParamPreferredLayoutWidth = "preferredLayoutWidth";
constexpr const char* kParamPreferredMaxLayoutHeight =
    "preferredMaxLayoutHeight";
constexpr const char* kParamPreferredMaxLayoutWidth = "preferredMaxLayoutWidth";
constexpr const char* kParamRatio = "ratio";
constexpr const char* kFuncInitialLynxView = "initialLynxView";
constexpr const char* kFuncUpdateViewPort = "updateViewPort";
constexpr const char* kFuncSetThreadStrategy = "setThreadStrategy";

// for NativeModuleRecorder
constexpr const char* kParamArgc = "argc";
constexpr const char* kParamArgs = "args";
constexpr const char* kParamFunction = "function";
constexpr const char* kParamReturnValue = "returnValue";
constexpr const char* kCallBack = "callback";
constexpr const char* kFuncSendGlobalEvent = "sendGlobalEvent";
constexpr const char* kFuncSendEventAndroid = "sendEventAndroid";
constexpr const char* kKeyEventAndroidArgs[] = {"action", "keycode",
                                                "metaState"};
constexpr const char* kEventAndroidArgs[] = {"action", "x", "y", "metaState"};

// for TemplateAssemblerRecorder
constexpr const char* kParamComponentId = "component_id";
constexpr const char* kParamConfig = "config";
constexpr const char* kParamData = "data";
constexpr const char* kParamGlobalProps = "global_props";
constexpr const char* kParamNoticeDelegate = "noticeDelegate";
constexpr const char* kParamPreprocessorName = "preprocessorName";
constexpr const char* kParamReadOnly = "readOnly";
constexpr const char* kParamSource = "source";
constexpr const char* kParamTemplateData = "templateData";
constexpr const char* kParamUrl = "url";
constexpr const char* kParamValue = "value";
constexpr const char* kParamIsCSR = "isCSR";
constexpr const char* kFuncLoadTemplate = "loadTemplate";
constexpr const char* kFuncLoadTemplateBundle = "loadTemplateBundle";
constexpr const char* kFuncSetGlobalProps = "setGlobalProps";
constexpr const char* kFuncUpdateConfig = "updateConfig";
constexpr const char* kUpdatePageOption = "updatePageOption";
constexpr const char* kFuncUpdateDataByPreParsedData =
    "updateDataByPreParsedData";
constexpr const char* kFuncRecordReloadTemplate = "reloadTemplate";
constexpr const char* kFuncRecordUpdateMetaData = "updateMetaData";
constexpr const char* kFuncSendTouchEvent = "SendTouchEvent";
constexpr const char* kFuncSendCustomEvent = "SendCustomEvent";
constexpr const char* kFuncSendBubbleEvent = "SendBubbleEvent";
constexpr const char* kEventName = "name";
constexpr const char* kEventTag = "tag";
constexpr const char* kEventRootTag = "root_tag";
constexpr const char* kEventX = "x";
constexpr const char* kEventY = "y";
constexpr const char* kEventClientX = "client_x";
constexpr const char* kEventClientY = "client_y";
constexpr const char* kEventPageX = "page_x";
constexpr const char* kEventPageY = "page_y";
constexpr const char* kEventParaName = "pname";
constexpr const char* kEventParams = "params";
constexpr const char* kEventMsg = "msg";
constexpr const char* kSyncTag = "sync_tag";
constexpr const char* kCallbackId = "callback_id";
constexpr const char* kFuncRequireTemplate = "RequireTemplate";
constexpr const char* kFuncLoadComponentWithCallback =
    "LoadComponentWithCallback";
constexpr const char* kFuncUpdateFontScale = "updateFontScale";
constexpr const char* kFontScale = "scale";
constexpr const char* kFuncUpdateFontScaleType = "type";

}  // namespace recorder
}  // namespace tasm
}  // namespace lynx

#endif  // CORE_SERVICES_RECORDER_RECORDER_CONSTANTS_H_
