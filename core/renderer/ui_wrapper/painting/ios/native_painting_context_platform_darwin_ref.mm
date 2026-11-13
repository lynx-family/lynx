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
#import <Lynx/LynxImageManager.h>
#import <Lynx/LynxRendererContext.h>
#import <Lynx/LynxRendererHost.h>
#import <Lynx/LynxService.h>
#import <Lynx/LynxServiceTextProtocol.h>
#import <Lynx/LynxTemplateData+Converter.h>
#import <Lynx/LynxTextRenderManager.h>
#import <Lynx/LynxTextRenderer.h>
#import <Lynx/LynxUI.h>
#import <Lynx/LynxUIContext.h>
#import <Lynx/LynxUIOwner.h>
#import "LynxTimingConstants.h"

@interface LynxRendererContext (ImageUIMethod)

- (LynxImageManager*)imageManagerForSign:(int32_t)sign;

@end

@interface LynxImageManager (UIMethod)

- (BOOL)invokeUIMethod:(NSString*)method
                params:(NSDictionary*)params
              callback:(LynxUIMethodCallbackBlock)callback;

@end

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

LynxUIOwner* NativePaintingCtxPlatformDarwinRef::GetUIOwner() {
  return static_cast<PlatformRendererDarwinFactory*>(view_factory_.get())
      ->GetContext()
      ->GetUIOwner();
}

void NativePaintingCtxPlatformDarwinRef::InsertListItemPaintingNode(int32_t list_id,
                                                                    int32_t child_id) {
  LynxUIOwner* uiOwner = GetUIOwner();
  if (uiOwner == nil) {
    return;
  }
  [uiOwner insertListComponent:list_id componentSign:child_id];
}

void NativePaintingCtxPlatformDarwinRef::RemoveListItemPaintingNode(int32_t list_id,
                                                                    int32_t child_id) {
  LynxUIOwner* uiOwner = GetUIOwner();
  if (uiOwner == nil) {
    return;
  }
  [uiOwner removeListComponent:list_id componentSign:child_id];
}

void NativePaintingCtxPlatformDarwinRef::UpdateContentOffsetForListContainer(
    int32_t container_id, float content_size, float delta_x, float delta_y,
    bool is_init_scroll_offset, bool from_layout) {
  (void)is_init_scroll_offset;
  (void)from_layout;
  LynxUIOwner* uiOwner = GetUIOwner();
  if (uiOwner == nil) {
    return;
  }
  [uiOwner updateContentOffsetForListContainer:container_id
                                   contentSize:content_size
                                        deltaX:delta_x
                                        deltaY:delta_y];
}

void NativePaintingCtxPlatformDarwinRef::GetPlatformRendererScrollOffset(int32_t sign,
                                                                         float offset[2]) {
  if (offset == nullptr) {
    return;
  }
  offset[0] = 0.f;
  offset[1] = 0.f;

  UIScrollView* view = GetPlatformRendererScrollView(sign);
  if (view != nil) {
    const auto content_offset = view.contentOffset;
    offset[0] = content_offset.x;
    offset[1] = content_offset.y;
  }
}

bool NativePaintingCtxPlatformDarwinRef::IsPlatformRendererScrollable(int32_t sign) {
  LynxUI* ui = [GetUIOwner() findUIBySign:sign];
  if (ui != nil) {
    return [ui isScrollContainer];
  }
  return GetPlatformRendererScrollView(sign) != nil;
}

UIScrollView* NativePaintingCtxPlatformDarwinRef::GetPlatformRendererScrollView(int32_t sign) {
  UIView* view = GetPlatformRendererView(sign);
  if ([view isKindOfClass:[UIScrollView class]]) {
    return (UIScrollView*)view;
  }
  LynxUI* ui = [GetUIOwner() findUIBySign:sign];
  if ([ui.tagName isEqualToString:@"swiper"] || [ui.tagName isEqualToString:@"x-swiper"]) {
    // Swiper wraps its collection view in an intermediate paging view.
    for (UIView* pager in view.subviews) {
      for (UIView* child in pager.subviews) {
        if ([child isKindOfClass:[UICollectionView class]]) {
          return (UIScrollView*)child;
        }
      }
    }
    return nil;
  }
  if (![ui.tagName isEqualToString:@"viewpager"] &&
      ![ui.tagName isEqualToString:@"x-viewpager-ng"] &&
      ![ui.tagName isEqualToString:@"scroll-coordinator"] &&
      ![ui.tagName isEqualToString:@"x-foldview-ng"]) {
    return nil;
  }
  for (UIView* child in view.subviews) {
    if ([child isKindOfClass:[UIScrollView class]]) {
      return (UIScrollView*)child;
    }
  }
  return nil;
}

bool NativePaintingCtxPlatformDarwinRef::GetPlatformRendererPosition(int32_t sign,
                                                                     float position[2]) {
  LynxUI* ui = [GetUIOwner() findUIBySign:sign];
  LynxUI* parent = ui.parent;
  const bool is_coordinator = [parent.tagName isEqualToString:@"scroll-coordinator"] ||
                              [parent.tagName isEqualToString:@"x-foldview-ng"];
  const bool is_pager_item = ([ui.tagName isEqualToString:@"viewpager-item"] ||
                              [ui.tagName isEqualToString:@"x-viewpager-item-ng"]) &&
                             ([parent.tagName isEqualToString:@"viewpager"] ||
                              [parent.tagName isEqualToString:@"x-viewpager-ng"]);
  const bool is_swiper_item =
      ([ui.tagName isEqualToString:@"swiper-item"] ||
       [ui.tagName isEqualToString:@"x-swiper-item"]) &&
      ([parent.tagName isEqualToString:@"swiper"] || [parent.tagName isEqualToString:@"x-swiper"]);
  const bool is_refresh_child = [parent.tagName isEqualToString:@"refresh"] ||
                                [parent.tagName isEqualToString:@"x-refresh-view"];
  if (!is_coordinator && !is_pager_item && !is_swiper_item && !is_refresh_child) {
    return false;
  }
  UIView* parent_view = GetPlatformRendererView(static_cast<int32_t>(parent.sign));
  UIView* view = GetPlatformRendererView(sign);
  if (parent_view == nil || view == nil) {
    return false;
  }
  UIView* host_view = view;
  CGPoint origin = CGPointZero;
  UIScrollView* scroll_view = GetPlatformRendererScrollView(static_cast<int32_t>(parent.sign));
  bool inside_scroll_view = false;
  // Include native wrappers, leaving contentOffset to the event tree's scroll
  // compensation. CUI applies host transforms separately.
  while (view != nil && view != parent_view) {
    inside_scroll_view |= view == scroll_view;
    origin.x += view.layer.position.x - view.layer.anchorPoint.x * view.bounds.size.width;
    origin.y += view.layer.position.y - view.layer.anchorPoint.y * view.bounds.size.height;
    if (is_refresh_child && view != host_view) {
      // Refresh reparents its header into the content scroll view. Its native
      // scroll ancestors are not ancestors in the event tree, so include their
      // bounds offsets here. The content host's own scrolling stays in CUI.
      origin.x -= view.bounds.origin.x;
      origin.y -= view.bounds.origin.y;
    }
    view = view.superview;
  }
  if (view == nil) {
    return false;
  }
  if ((is_coordinator || is_swiper_item) && scroll_view == nil) {
    return false;
  }
  if (is_swiper_item && !inside_scroll_view) {
    return false;
  }
  if (is_coordinator && !inside_scroll_view) {
    // Fixed children live outside the scroll view. Express their position in
    // content coordinates so the parent's scroll compensation keeps them fixed.
    origin.x += scroll_view.contentOffset.x;
    origin.y += scroll_view.contentOffset.y;
  }
  position[0] = origin.x;
  position[1] = origin.y;
  return true;
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

  LynxRendererContext* renderer_context = GetRendererContext();
  LynxImageManager* image_manager = [renderer_context imageManagerForSign:sign];
  if (image_manager != nil) {
    auto callback_holder =
        std::make_shared<base::MoveOnlyClosure<void, int32_t, const pub::Value&>>(
            std::move(callback));
    LynxUIMethodCallbackBlock block = ^(int code, id _Nullable data) {
      if (!callback_holder || !(*callback_holder)) {
        return;
      }
      auto image_callback = std::move(*callback_holder);
      image_callback(code, PubLepusValue(LynxConvertToLepusValue(data)));
    };
    BOOL handled = [image_manager invokeUIMethod:method_name params:params_dict callback:block];
    if (handled) {
      return;
    }
    if (!callback_holder || !(*callback_holder)) {
      return;
    }
    callback = std::move(*callback_holder);
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
