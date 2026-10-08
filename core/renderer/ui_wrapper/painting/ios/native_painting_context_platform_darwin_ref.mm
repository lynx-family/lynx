// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/ui_wrapper/painting/ios/native_painting_context_platform_darwin_ref.h"

#include <algorithm>

#include "core/renderer/dom/ios/lepus_value_converter.h"
#include "core/renderer/ui_wrapper/painting/ios/platform_renderer_context_darwin.h"
#include "core/renderer/ui_wrapper/painting/ios/platform_renderer_darwin.h"
#include "core/value_wrapper/value_impl_lepus.h"

#import <Lynx/LynxContext+Internal.h>
#import <Lynx/LynxRendererContext.h>
#import <Lynx/LynxService.h>
#import <Lynx/LynxServiceTextProtocol.h>
#import <Lynx/LynxTemplateData+Converter.h>
#import <Lynx/LynxTextRenderManager.h>
#import <Lynx/LynxTextRenderer.h>
#import <Lynx/LynxUIContext.h>
#import <Lynx/LynxUIOwner.h>
#import "LynxTimingConstants.h"

namespace lynx {
namespace tasm {

NativePaintingCtxPlatformDarwinRef::NativePaintingCtxPlatformDarwinRef(
    std::unique_ptr<PlatformRendererFactory> view_factory)
    : NativePaintingCtxPlatformRef(std::move(view_factory)) {}

PlatformTextEventTargetRegions NativePaintingCtxPlatformDarwinRef::GetTextEventTargetRegions(
    int32_t text_id) {
  PlatformTextEventTargetRegions regions;
  const auto* ranges = GetTextEventTargetRanges(text_id);
  LynxRendererContext* context = GetRendererContext();
  if (ranges == nullptr || context == nil) {
    return regions;
  }
  auto append = [&](int32_t sign, CGRect rect) {
    if (!CGRectIsEmpty(rect) && !CGRectIsNull(rect) && !CGRectIsInfinite(rect)) {
      regions.push_back(PlatformTextEventTargetRegion{
          sign, static_cast<float>(rect.origin.x), static_cast<float>(rect.origin.y),
          static_cast<float>(rect.size.width), static_cast<float>(rect.size.height)});
    }
  };
  if (context.uiContext.lynxContext.isTextServiceModeOn) {
    void* page = [context getTextBundle:text_id];
    id<LynxServiceTextProtocol> service = LynxService(LynxServiceTextProtocol);
    if (page == nullptr || service == nil) {
      return regions;
    }
    for (const auto& range : *ranges) {
      if (range.start < 0 || range.end <= range.start) continue;
      NSArray* rects =
          [service getSelectionRectsOfPage:page
                               ByCharRange:NSMakeRange(range.start, range.end - range.start)];
      for (NSValue* value in rects) append(range.sign, value.CGRectValue);
    }
  } else {
    LynxTextRenderer* renderer = [context.textRenderManager takeTextRender:text_id];
    if (renderer == nil) return regions;
    NSLayoutManager* layout = renderer.layoutManager;
    NSTextContainer* container = layout.textContainers.firstObject;
    if (container == nil) return regions;
    const NSRange visible = [layout glyphRangeForTextContainer:container];
    for (const auto& range : *ranges) {
      if (range.start < 0 || range.end <= range.start ||
          static_cast<NSUInteger>(range.end) > renderer.textStorage.length)
        continue;
      // Exclude line terminators: their enclosing rects extend to the line edge.
      NSString* text = renderer.textStorage.string;
      const CGFloat offset = renderer.textContentOffsetX;
      NSUInteger position = range.start;
      while (position < static_cast<NSUInteger>(range.end)) {
        NSUInteger lineEnd, contentsEnd;
        [text getLineStart:nullptr
                       end:&lineEnd
               contentsEnd:&contentsEnd
                  forRange:NSMakeRange(position, 0)];
        const NSUInteger end = std::min(contentsEnd, static_cast<NSUInteger>(range.end));
        if (end > position) {
          NSRange glyphs = [layout glyphRangeForCharacterRange:NSMakeRange(position, end - position)
                                          actualCharacterRange:nullptr];
          glyphs = NSIntersectionRange(glyphs, visible);
          if (glyphs.length > 0) {
            [layout enumerateEnclosingRectsForGlyphRange:glyphs
                                withinSelectedGlyphRange:NSMakeRange(NSNotFound, 0)
                                         inTextContainer:container
                                              usingBlock:^(CGRect rect, BOOL* stop) {
                                                rect.origin.x += offset;
                                                append(range.sign, rect);
                                              }];
          }
        }
        if (lineEnd <= position) break;
        position = lineEnd;
      }
    }
  }
  return regions;
}

std::vector<float> NativePaintingCtxPlatformDarwinRef::GetTransformValue(
    int32_t sign, const std::vector<float>& offsets) {
  return GetTransformValueForEventTarget(sign, offsets);
}

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

LynxRendererContext* NativePaintingCtxPlatformDarwinRef::GetRendererContext() {
  return static_cast<PlatformRendererDarwinFactory*>(view_factory_.get())
      ->GetContext()
      ->GetRendererContext();
}

void NativePaintingCtxPlatformDarwinRef::GetPlatformRendererScrollOffset(int32_t sign,
                                                                         float offset[2]) {
  if (offset == nullptr) {
    return;
  }
  offset[0] = 0.f;
  offset[1] = 0.f;

  UIView* view = GetPlatformRendererView(sign);
  if ([view isKindOfClass:[UIScrollView class]]) {
    const auto content_offset = ((UIScrollView*)view).contentOffset;
    offset[0] = content_offset.x;
    offset[1] = content_offset.y;
  }
}

bool NativePaintingCtxPlatformDarwinRef::IsPlatformRendererScrollable(int32_t sign) {
  UIView* view = GetPlatformRendererView(sign);
  return [view isKindOfClass:[UIScrollView class]];
}

UIView* NativePaintingCtxPlatformDarwinRef::GetPlatformRendererView(int32_t sign) {
  auto it = renderers_.find(sign);
  if (it == renderers_.end() || !it->second) {
    return nil;
  }
  return static_cast<PlatformRendererDarwin*>(it->second.get())->GetUIView();
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
