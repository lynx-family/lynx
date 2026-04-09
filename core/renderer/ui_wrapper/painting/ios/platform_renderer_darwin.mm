// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/ui_wrapper/painting/ios/platform_renderer_darwin.h"
#include "core/renderer/ui_wrapper/common/ios/prop_bundle_darwin.h"
#include "core/renderer/ui_wrapper/common/native_prop_bundle.h"
#include "core/renderer/ui_wrapper/painting/ios/platform_renderer_context_darwin.h"

#import <Lynx/LUIBodyView.h>
#import <Lynx/LynxComponentRegistry.h>
#import <Lynx/LynxContainerView.h>
#import <Lynx/LynxRenderer+Internal.h>
#import <Lynx/LynxRenderer.h>

namespace lynx {
namespace tasm {

namespace {

void LynxCUIApplyLayoutFrame(UIView* view, CGRect layout_frame) {
  if (CATransform3DIsIdentity(view.layer.transform)) {
    view.layer.anchorPoint = CGPointMake(0.5f, 0.5f);
    [view setFrame:layout_frame];
    return;
  }

  // CUI subtree transform matrices already bake transform-origin. Consume them
  // with top-left anchor semantics so iOS does not apply an extra center pivot.
  view.layer.anchorPoint = CGPointZero;
  CGRect bounds = view.bounds;
  bounds.size = layout_frame.size;
  view.bounds = bounds;
  view.layer.position = layout_frame.origin;
}

}  // namespace

PlatformRendererDarwin::PlatformRendererDarwin(PlatformRendererContextDarwin* context, int id,
                                               PlatformRendererType type)
    : PlatformRendererDarwin(context, id, type, base::String()) {}

PlatformRendererDarwin::PlatformRendererDarwin(PlatformRendererContextDarwin* context, int id,
                                               const base::String& tag_name)
    : PlatformRendererDarwin(context, id, PlatformRendererType::kUnknown, tag_name) {}

PlatformRendererDarwin::PlatformRendererDarwin(PlatformRendererContextDarwin* context, int id,
                                               PlatformRendererType type,
                                               const base::String& tag_name)
    : PlatformRendererImpl(id, type, tag_name), context_(context) {
  InitializeUIView();
}

void PlatformRendererDarwin::OnUpdateDisplayList(DisplayList display_list) {
  if (display_list.HasContent()) {
    display_list_ = std::move(display_list);

    if (_view != nil) {
      constexpr int kFrameValueCount = 4;
      if (display_list_.GetContentFloatData() &&
          display_list_.GetContentFloatDataSize() >= kFrameValueCount) {
        float frame[4];
        // The first four float values in the display list are the frame of the
        // layer's OP_BEGIN.
        memcpy(frame, display_list_.GetContentFloatData(), 4 * sizeof(float));

        CGRect layout_frame =
            CGRectMake(frame[0] + display_list_.GetRenderOffset()[0],
                       frame[1] + display_list_.GetRenderOffset()[1], frame[2], frame[3]);
        LynxCUIApplyLayoutFrame(_view, layout_frame);

        if ([_view conformsToProtocol:@protocol(LUIBodyView)]) {
          ((UIView<LUIBodyView>*)_view).intrinsicContentSize = CGSizeMake(frame[2], frame[3]);
        }
      }

      [[_view renderer] updateDisplayList:&display_list_];
      [_view setNeedsDisplay];
    }
  }
}

void PlatformRendererDarwin::OnUpdateAttributes(const fml::RefPtr<PropBundle>& attributes,
                                                bool tends_to_flatten) {
  if (attributes && attributes->IsNative()) {
    // Convert NativePropBundle to PropBundleDarwin
    // The attributes should be a NativePropBundle from the pipeline
    NativePropBundle* native_bundle = static_cast<NativePropBundle*>(attributes.get());
    PropBundleDarwin prop_bundle_darwin(*native_bundle);
    [_view.renderer updateAttributes:prop_bundle_darwin.dictionary()];
  }
}

void PlatformRendererDarwin::OnAddChild(PlatformRenderer* child) {
  if (_view == nil) {
    return;
  }

  auto* child_renderer = static_cast<PlatformRendererDarwin*>(child);
  UIView<LynxRendererHost>* child_view = child_renderer->GetUIView();
  [_view addSubview:child_view];
  [[child_view renderer] reattachHostDecorationLayers];
}

void PlatformRendererDarwin::OnRemoveFromParent() {
  if (_view == nil) {
    return;
  }

  [[_view renderer] detachHostDecorationLayers];
  [_view removeFromSuperview];
}

void PlatformRendererDarwin::OnUpdateSubtreeProperties(const DisplayList& subtree_properties) {
  size_t count = subtree_properties.GetSubtreePropertiesSize();
  if (count == 0 || _view == nil) {
    return;
  }

  const SubtreeProperty* props = subtree_properties.GetSubtreePropertiesData();
  if (props == nullptr) {
    return;
  }

  LynxRenderer* renderer = _view.renderer;
  if (renderer == nil) {
    return;
  }

  [renderer applySubtreeProperties:props count:count];
}

void PlatformRendererDarwin::InitializeUIView() {
  if (IsPlatformExtendedRenderer()) {
    NSString* tagName = [NSString stringWithUTF8String:GetTagName().str().c_str()];
    Class hostClass = [LynxComponentRegistry rendererHostClassWithName:tagName];

    if (hostClass && [hostClass conformsToProtocol:@protocol(LynxRendererHost)]) {
      LynxRendererContext* rendererContext = context_->GetRendererContext();
      id<LynxRendererHost> customHost = [[hostClass alloc] initWithRendererContext:rendererContext];
      if (customHost && [customHost isKindOfClass:[UIView class]]) {
        // Safe to cast after confirming it's a UIView
        _view = (UIView<LynxRendererHost>*)customHost;

        LynxRenderer* renderer = [_view createRendererWithSign:GetId() andContext:rendererContext];
        [_view setRenderer:renderer];
        return;
      }
    }

    _view = [[LynxContainerView alloc] init];
    return;
  }

  switch (GetPlatformRendererType()) {
    // TODO(songshourui.null): Consruct specific UIView for each type later.
    case PlatformRendererType::kView:
    case PlatformRendererType::kText:
    case PlatformRendererType::kImage:
    case PlatformRendererType::kScroll:
    case PlatformRendererType::kList:
    case PlatformRendererType::kListItem: {
      _view = [[LynxContainerView alloc] init];
      break;
    }
    case PlatformRendererType::kPage: {
      _view = context_ != nullptr ? (UIView<LynxRendererHost>*)context_->GetContainerView() : nil;
      break;
    }
    default:
      break;
  }

  if (_view != nil) {
    LynxRenderer* renderer = [_view createRendererWithSign:GetId()
                                                andContext:context_->GetRendererContext()];
    [_view setRenderer:renderer];
  }
}

void PlatformRendererDarwin::UpdatePlatformExtraBundle(id platform_extra_bundle) {
  if (_view != nil) {
    LynxRenderer* renderer = _view.renderer;
    if (renderer != nil) {
      [renderer updatePlatformExtraBundle:platform_extra_bundle];
    }
  }
}

}  // namespace tasm
}  // namespace lynx
