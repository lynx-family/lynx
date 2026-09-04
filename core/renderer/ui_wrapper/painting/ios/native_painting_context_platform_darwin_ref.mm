// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/ui_wrapper/painting/ios/native_painting_context_platform_darwin_ref.h"

#include "core/renderer/dom/ios/lepus_value_converter.h"
#include "core/renderer/ui_wrapper/painting/ios/platform_renderer_context_darwin.h"
#include "core/renderer/ui_wrapper/painting/ios/platform_renderer_darwin.h"
#include "core/value_wrapper/value_impl_lepus.h"

#import <Lynx/LynxRendererContext.h>
#import <Lynx/LynxService.h>
#import <Lynx/LynxServiceTextProtocol.h>
#import <Lynx/LynxTemplateData+Converter.h>
#import <Lynx/LynxTextRenderManager.h>
#import <Lynx/LynxTextRenderer.h>
#import <Lynx/LynxUIOwner.h>
#import "LynxTimingConstants.h"

@interface LynxTextRenderer (LynxInlineEventTarget)
- (nullable NSArray<NSNumber*>*)lynx_inlineEventTargetInfoAtPoint:(CGPoint)point;
@end

namespace lynx {
namespace tasm {

NativePaintingCtxPlatformDarwinRef::NativePaintingCtxPlatformDarwinRef(
    std::unique_ptr<PlatformRendererFactory> view_factory)
    : NativePaintingCtxPlatformRef(std::move(view_factory)) {}

void NativePaintingCtxPlatformDarwinRef::GetRootViewLocationOnScreen(float location[2]) {
  if (location == nullptr) {
    return;
  }
  location[0] = 0.f;
  location[1] = 0.f;

  auto* factory = static_cast<PlatformRendererDarwinFactory*>(view_factory_.get());
  if (factory == nullptr) {
    return;
  }
  auto* context = factory->GetContext();
  if (context == nullptr) {
    return;
  }
  const auto res = context->GetRootViewLocationOnScreen();
  location[0] = res.x;
  location[1] = res.y;
}

void NativePaintingCtxPlatformDarwinRef::GetScreenSize(float size[2]) {
  if (size == nullptr) {
    return;
  }
  size[0] = 0.f;
  size[1] = 0.f;

  auto* factory = static_cast<PlatformRendererDarwinFactory*>(view_factory_.get());
  if (factory == nullptr) {
    return;
  }
  auto* context = factory->GetContext();
  if (context == nullptr) {
    return;
  }
  const auto res = context->GetScreenSize();
  size[0] = res.width;
  size[1] = res.height;
}

bool NativePaintingCtxPlatformDarwinRef::HitTestTextEventTarget(
    int32_t text_id, float x, float y, PlatformTextEventTargetInfo* result) {
  if (result == nullptr) {
    return false;
  }
  LynxRendererContext* renderer_context = GetRendererContext();
  if (renderer_context == nil) {
    return false;
  }

  NSArray<NSNumber*>* target_info = nil;
  void* page = [renderer_context getTextBundle:text_id];
  if (page != nullptr) {
    id<LynxServiceTextProtocol> text_service = LynxService(LynxServiceTextProtocol);
    target_info = [text_service getHitTestEventTargetsOfPage:page
                                             ByTouchPosition:CGPointMake(x, y)];
    if (target_info.count >= 3 && [target_info[2] boolValue]) {
      return false;
    }
  } else {
    LynxTextRenderer* text_renderer = [renderer_context.textRenderManager takeTextRender:text_id];
    target_info = [text_renderer lynx_inlineEventTargetInfoAtPoint:CGPointMake(x, y)];
  }

  if (target_info.count < 2) {
    return false;
  }
  result->sign = [target_info[0] intValue];
  result->event_mask = [target_info[1] unsignedIntValue];
  return result->sign >= 0 && result->event_mask != 0;
}

LynxRendererContext* NativePaintingCtxPlatformDarwinRef::GetRendererContext() {
  return static_cast<PlatformRendererDarwinFactory*>(view_factory_.get())
      ->GetContext()
      ->GetRendererContext();
}

void NativePaintingCtxPlatformDarwinRef::SetNeedMarkPaintEndTiming(
    const tasm::PipelineID& pipeline_id) {
  LynxPerformanceController* performance_controller = perf_controller_;
  NSString* pipeline_id_string = [NSString stringWithUTF8String:pipeline_id.c_str()];
  dispatch_async(dispatch_get_main_queue(), ^{
    [performance_controller markTiming:kTimingPaintEnd pipelineID:pipeline_id_string];
  });
}

void NativePaintingCtxPlatformDarwinRef::UpdatePlatformRendererExtraBundle(
    int32_t sign, id platform_extra_bundle) {
  if (auto it = renderers_.find(sign); it != renderers_.end()) {
    auto* renderer = static_cast<PlatformRendererDarwin*>(it->second.get());
    renderer->UpdatePlatformExtraBundle(platform_extra_bundle);
  }
}

void NativePaintingCtxPlatformDarwinRef::InvokePlatformViewUIMethod(
    int32_t sign, const std::string& method, const lepus::Value& params,
    base::MoveOnlyClosure<void, int32_t, const pub::Value&> callback) {
  NSString* method_name = [[NSString alloc] initWithUTF8String:method.c_str()];
  id ns_params = convertLepusValueToNSObject(params);
  NSDictionary* params_dict =
      [ns_params isKindOfClass:[NSDictionary class]] ? (NSDictionary*)ns_params : nil;
  if (auto it = renderers_.find(sign); it != renderers_.end() && it->second) {
    auto* renderer = static_cast<PlatformRendererDarwin*>(it->second.get());
    UIView<LynxRendererHost>* view = renderer->GetUIView();
    if (view != nil && [view respondsToSelector:@selector(invokeUIMethod:params:callback:)]) {
      auto callback_holder =
          std::make_shared<base::MoveOnlyClosure<void, int32_t, const pub::Value&>>(
              std::move(callback));
      LynxUIMethodCallbackBlock block = ^(int code, id _Nullable data) {
        if (!callback_holder || !(*callback_holder)) {
          return;
        }
        auto callback = std::move(*callback_holder);
        callback(code, PubLepusValue(LynxConvertToLepusValue(data)));
      };
      BOOL handled = [view invokeUIMethod:method_name params:params_dict callback:block];
      if (handled) {
        return;
      }
      if (!callback_holder || !(*callback_holder)) {
        return;
      }
      callback = std::move(*callback_holder);
    }
  }

  auto* factory = static_cast<PlatformRendererDarwinFactory*>(view_factory_.get());
  if (factory == nullptr) {
    NativePaintingCtxPlatformRef::InvokePlatformViewUIMethod(sign, method, params,
                                                             std::move(callback));
    return;
  }
  auto* context = factory->GetContext();
  if (context == nullptr) {
    NativePaintingCtxPlatformRef::InvokePlatformViewUIMethod(sign, method, params,
                                                             std::move(callback));
    return;
  }
  LynxUIOwner* owner = context->GetUIOwner();
  if (owner == nil) {
    NativePaintingCtxPlatformRef::InvokePlatformViewUIMethod(sign, method, params,
                                                             std::move(callback));
    return;
  }
  auto callback_holder = std::make_shared<base::MoveOnlyClosure<void, int32_t, const pub::Value&>>(
      std::move(callback));
  LynxUIMethodCallbackBlock block = ^(int code, id _Nullable data) {
    if (!callback_holder || !(*callback_holder)) {
      return;
    }
    auto callback = std::move(*callback_holder);
    callback(code, PubLepusValue(LynxConvertToLepusValue(data)));
  };
  [owner invokeUIMethodForSelectorQuery:method_name params:params_dict callback:block toNode:sign];
}

void NativePaintingCtxPlatformDarwinRef::NotifyNodeReady(const std::vector<int32_t>& signs) {
  auto* factory = static_cast<PlatformRendererDarwinFactory*>(view_factory_.get());
  if (factory == nullptr) {
    return;
  }
  auto* context = factory->GetContext();
  if (context == nullptr) {
    return;
  }
  LynxUIOwner* owner = context->GetUIOwner();
  if (owner == nil) {
    return;
  }
  for (const auto sign : signs) {
    if ([owner findUIBySign:sign] == nil) {
      continue;
    }
    [owner onNodeReady:sign];
  }
}

}  // namespace tasm
}  // namespace lynx
