// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "core/shell/ios/native_facade_darwin.h"
#import "LynxError.h"
#import "LynxLog.h"
#import "LynxPerformance.h"
#import "LynxTemplateBundle+Converter.h"
#import "LynxTemplateBundle.h"
#import "LynxTheme.h"

#include "base/include/float_comparison.h"
#include "base/include/string/string_utils.h"
#include "core/renderer/ui_wrapper/common/ios/prop_bundle_darwin.h"

#include "core/renderer/dom/ios/lepus_value_converter.h"
#include "core/renderer/template_assembler.h"
#include "core/runtime/bindings/lepus/ios/lynx_lepus_module_darwin.h"
#include "core/runtime/vm/lepus/table.h"

#include "base/trace/native/trace_event.h"
#include "core/base/lynx_trace_categories.h"
#include "core/renderer/utils/darwin/event_converter_darwin.h"
#include "core/runtime/bindings/common/event/message_event.h"

namespace lynx {
namespace shell {

void NativeFacadeDarwin::OnDataUpdated() {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "NativeFacadeDarwin::OnDataUpdated");
  __strong id<TemplateRenderCallbackProtocol> render = _render;
  [render onDataUpdated];
}

void NativeFacadeDarwin::OnPageChanged(bool is_first_screen) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "NativeFacadeDarwin::OnPageChanged");
  __strong id<TemplateRenderCallbackProtocol> render = _render;
  [render onPageChanged:is_first_screen];
}

void NativeFacadeDarwin::OnTasmFinishByNative() {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "NativeFacadeDarwin::OnTasmFinishByNative");
  __strong id<TemplateRenderCallbackProtocol> render = _render;
  [render onTasmFinishByNative];
}

void NativeFacadeDarwin::OnTemplateLoaded(const std::string& url) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "NativeFacadeDarwin::OnTemplateLoaded", "url", url);
  __strong id<TemplateRenderCallbackProtocol> render = _render;
  [render onTemplateLoaded:[NSString stringWithUTF8String:url.c_str()]];
}

void NativeFacadeDarwin::OnSSRHydrateFinished(const std::string& url) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "NativeFacadeDarwin::OnSSRHydrateFinished", "url", url);
  __strong id<TemplateRenderCallbackProtocol> render = _render;
  [render onSSRHydrateFinished:[NSString stringWithUTF8String:url.c_str()]];
}

void NativeFacadeDarwin::OnRuntimeReady() {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "NativeFacadeDarwin::OnRuntimeReady");
  __strong id<TemplateRenderCallbackProtocol> render = _render;
  [render onRuntimeReady];
}

void NativeFacadeDarwin::ReportError(const base::LynxError& error) {
  __strong id<TemplateRenderCallbackProtocol> render = _render;
  NSMutableDictionary* customInfo = [NSMutableDictionary new];
  for (const auto& [key, value] : error.custom_info_) {
    [customInfo setValue:[NSString stringWithUTF8String:value.c_str()]
                  forKey:[NSString stringWithUTF8String:key.c_str()]];
  }
  std::string level_std_str =
      base::LynxError::GetLevelString(static_cast<int32_t>(error.error_level_));
  NSString* level = [NSString stringWithUTF8String:level_std_str.c_str()];
  LynxError* lynxError =
      [LynxError lynxErrorWithCode:error.error_code_
                           message:[NSString stringWithUTF8String:error.error_message_.c_str()]
                     fixSuggestion:[NSString stringWithUTF8String:error.fix_suggestion_.c_str()]
                             level:level
                        customInfo:customInfo
                      isLogBoxOnly:error.is_logbox_only_ ? YES : NO];
  [render onErrorOccurred:lynxError];
}

// issue: #1510
void NativeFacadeDarwin::OnModuleMethodInvoked(const std::string& module, const std::string& method,
                                               int32_t code) {
  __strong id<TemplateRenderCallbackProtocol> render = _render;
  [render didInvokeMethod:[NSString stringWithUTF8String:method.c_str()]
                 inModule:[NSString stringWithUTF8String:module.c_str()]
                errorCode:code];
}

void NativeFacadeDarwin::OnTimingSetup(const lepus::Value& timing_info) {
  __strong id<TemplateRenderCallbackProtocol> render = _render;
  [render onTimingSetup:lynx::tasm::convertLepusValueToNSObject(timing_info)];
}

void NativeFacadeDarwin::OnTimingUpdate(const lepus::Value& timing_info,
                                        const lepus::Value& update_timing,
                                        const std::string& update_flag) {
  __strong id<TemplateRenderCallbackProtocol> render = _render;
  [render onTimingUpdate:lynx::tasm::convertLepusValueToNSObject(timing_info)
            updateTiming:lynx::tasm::convertLepusValueToNSObject(update_timing)];
}

void NativeFacadeDarwin::OnDynamicComponentPerfReady(const lepus::Value& perf_info) {
  __strong id<TemplateRenderCallbackProtocol> render = _render;
  [render onDynamicComponentPerf:lynx::tasm::convertLepusValueToNSObject(perf_info)];
}

void NativeFacadeDarwin::OnConfigUpdated(const lepus::Value& data) {
  if (!data.IsTable() || data.Table()->size() == 0) {
    return;
  }

  __strong id<TemplateRenderCallbackProtocol> render = _render;
  for (const auto& prop : *(data.Table())) {
    if (!prop.first.IsEqual(tasm::CARD_CONFIG_THEME) || !prop.second.IsTable()) {
      continue;
    }

    LynxTheme* themeConfig = [LynxTheme new];
    for (const auto& sub_prop : *(prop.second.Table())) {
      if (sub_prop.second.IsString()) {
        NSString* key = [NSString stringWithUTF8String:sub_prop.first.c_str()];
        NSString* value = [NSString stringWithUTF8String:sub_prop.second.CString()];
        [themeConfig updateValue:value forKey:key];
      }
    }
    [render setLocalTheme:themeConfig];
  }
}

void NativeFacadeDarwin::OnUpdateDataWithoutChange() {}

void NativeFacadeDarwin::TriggerLepusMethodAsync(const std::string& js_method_name,
                                                 const lepus::Value& args) {
  lynx::piper::TriggerLepusMethodAsync(js_method_name, args, _render);
}

void NativeFacadeDarwin::InvokeUIMethod(const tasm::LynxGetUIResult& ui_result,
                                        const std::string& method,
                                        std::unique_ptr<tasm::PropBundle> params,
                                        piper::ApiCallBack callback) {
#if !defined(OS_OSX)
  __strong id<TemplateRenderCallbackProtocol> render = _render;
  NSString* method_string = [NSString stringWithUTF8String:method.c_str()];
  NSDictionary* params_dict = ((tasm::PropBundleDarwin*)params.get())->dictionary();

  [render invokeUIMethod:method_string
                  params:params_dict
                callback:callback.id()
                  toNode:ui_result.UiImplIds()[0]];
#endif
}

void NativeFacadeDarwin::FlushJSBTiming(piper::NativeModuleInfo timing) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY_JSB, "JSBTiming::FlushJSBTiming",
              [&timing](lynx::perfetto::EventContext ctx) {
                ctx.event()->add_debug_annotations("module_name", timing.module_name_);
                ctx.event()->add_debug_annotations("method_name", timing.method_name_);
                ctx.event()->add_debug_annotations("method_first_arg_name",
                                                   timing.method_first_arg_name_);
              });
  __strong id<TemplateRenderCallbackProtocol> render = _render;
  NSDictionary* info = @{
    @"jsb_module_name" : [NSString stringWithUTF8String:timing.module_name_.c_str()],
    @"jsb_method_name" : [NSString stringWithUTF8String:timing.method_name_.c_str()],
    @"jsb_name" : [NSString stringWithUTF8String:timing.method_first_arg_name_.c_str()],
    // "jsb_protocol_version" web field, lynx default 0.
    @"jsb_protocol_version" : @(0),
    @"jsb_bridgesdk" : @"lynx"
  };
  NSMutableDictionary* invokedInfo = [NSMutableDictionary dictionaryWithDictionary:info];
  [invokedInfo setObject:@(static_cast<int64_t>(timing.status_code_)) forKey:@"jsb_status_code"];
  [render onJSBInvoked:invokedInfo.copy];
  if (timing.status_code_ != piper::NativeModuleStatusCode::SUCCESS) {
    return;
  }
  NSDictionary* timing_info = @{
    @"perf" : @{
      @"jsb_call" : @(timing.jsb_call_),
      @"jsb_func_call" : @(timing.jsb_func_call_),
      @"jsb_func_convert_params" : @(timing.jsb_func_convert_params_),
      @"jsb_func_platform_method" : @(timing.jsb_func_platform_method_),
      @"jsb_callback_thread_switch" : @(timing.jsb_callback_thread_switch_),
      @"jsb_callback_thread_switch_waiting" : @(timing.jsb_callback_thread_switch_waiting_),
      @"jsb_callback_call" : @(timing.jsb_callback_call_),
      @"jsb_callback_convert_params" : @(timing.jsb_callback_convert_params_),
      @"jsb_callback_invoke" : @(timing.jsb_callback_invoke_),
      @"jsb_func_call_start" : @(timing.jsb_func_call_start_),
      @"jsb_func_call_end" : @(timing.jsb_func_call_end_),
      @"jsb_callback_thread_switch_start" : @(timing.jsb_callback_thread_switch_start_),
      @"jsb_callback_thread_switch_end" : @(timing.jsb_callback_thread_switch_end_),
      @"jsb_callback_call_start" : @(timing.jsb_callback_call_start_),
      @"jsb_callback_call_end" : @(timing.jsb_callback_call_end_),
    },
    @"info" : info
  };
  [render onCallJSBFinished:timing_info];
}

void NativeFacadeDarwin::OnTemplateBundleReady(tasm::LynxTemplateBundle bundle) {
  __strong id<TemplateRenderCallbackProtocol> render = _render;
  LynxTemplateBundle* templateBundle = ConstructTemplateBundleFromNative(std::move(bundle));
  [render onTemplateBundleReady:templateBundle];
}

void NativeFacadeDarwin::OnReceiveMessageEvent(runtime::MessageEvent event) {
  if (event.IsSendingToDevTool()) {
    __strong id<TemplateRenderCallbackProtocol> render = _render;
    [render
        onReceiveMessageEvent:tasm::darwin::EventConverterDarwin::ConverMessageEventToNSDictionary(
                                  event)];
  } else {
    // TODO(songshourui.null): impl this after UIContext is supported.
  }
}

}  // namespace shell
}  // namespace lynx
