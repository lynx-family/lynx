// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/ui_wrapper/painting/ios/platform_renderer_darwin.h"

#include <memory>

#include "core/renderer/ui_wrapper/common/ios/prop_bundle_darwin.h"
#include "core/renderer/ui_wrapper/common/native_prop_bundle.h"
#include "core/renderer/ui_wrapper/painting/ios/platform_renderer_context_darwin.h"

#import <Lynx/LUIBodyView.h>
#import <Lynx/LynxComponentRegistry.h>
#import <Lynx/LynxContainerView.h>
#import <Lynx/LynxRenderer+Internal.h>
#import <Lynx/LynxRenderer.h>
#import <Lynx/LynxUI.h>
#import <Lynx/LynxUIOwner+Private.h>
#import <Lynx/LynxUIOwner.h>

namespace lynx {
namespace tasm {

namespace {

bool IsOverlayRendererTag(const base::String& tag) {
  return tag.IsEqual("overlay") || tag.IsEqual("x-overlay-ng");
}

UIEdgeInsets UIEdgeInsetsFromLayoutMetrics(const float* metrics) {
  if (metrics == nullptr) {
    return UIEdgeInsetsZero;
  }
  return UIEdgeInsetsMake(metrics[1], metrics[0], metrics[3], metrics[2]);
}

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

std::unique_ptr<PropBundleDarwin> CreateDarwinPropBundle(const fml::RefPtr<PropBundle>& bundle) {
  if (!bundle || !bundle->IsNative()) {
    return nullptr;
  }
  return std::make_unique<PropBundleDarwin>(*static_cast<NativePropBundle*>(bundle.get()));
}

bool ShouldDetachThroughUIOwner(LynxUIOwner* owner, int sign) {
  if (owner == nil) {
    return false;
  }
  LynxUI* ui = [owner findUIBySign:sign];
  return ui != nil && ui.parent != nil;
}

}  // namespace

PlatformRendererDarwin::PlatformRendererDarwin(PlatformRendererContextDarwin* context, int id,
                                               PlatformRendererType type)
    : PlatformRendererDarwin(context, id, type, fml::RefPtr<PropBundle>()) {}

PlatformRendererDarwin::PlatformRendererDarwin(PlatformRendererContextDarwin* context, int id,
                                               PlatformRendererType type,
                                               const fml::RefPtr<PropBundle>& init_data,
                                               const PlatformRendererInitConfig& init_config)
    : PlatformRendererDarwin(context, id, type, base::String(), init_data, init_config) {}

PlatformRendererDarwin::PlatformRendererDarwin(PlatformRendererContextDarwin* context, int id,
                                               const base::String& tag_name)
    : PlatformRendererDarwin(context, id, tag_name, fml::RefPtr<PropBundle>()) {}

PlatformRendererDarwin::PlatformRendererDarwin(PlatformRendererContextDarwin* context, int id,
                                               const base::String& tag_name,
                                               const fml::RefPtr<PropBundle>& init_data,
                                               const PlatformRendererInitConfig& init_config)
    : PlatformRendererDarwin(context, id, PlatformRendererType::kUnknown, tag_name, init_data,
                             init_config) {}

PlatformRendererDarwin::PlatformRendererDarwin(PlatformRendererContextDarwin* context, int id,
                                               PlatformRendererType type,
                                               const base::String& tag_name)
    : PlatformRendererDarwin(context, id, type, tag_name, fml::RefPtr<PropBundle>()) {}

PlatformRendererDarwin::PlatformRendererDarwin(PlatformRendererContextDarwin* context, int id,
                                               PlatformRendererType type,
                                               const base::String& tag_name,
                                               const fml::RefPtr<PropBundle>& init_data,
                                               const PlatformRendererInitConfig& init_config)
    : PlatformRendererImpl(id, type, tag_name),
      context_(context),
      ui_owner_(context != nullptr ? context->GetUIOwner() : nil) {
  SetDirectChildOfCompatibleComponent(init_config.is_direct_child_of_compatible_component);
  SetFragmentParentId(init_config.fragment_parent_id);
  if (ShouldCreatePlatformExtendedRenderer(init_config)) {
    is_platform_extended_renderer_ = true;
  }
  InitializeUIView(init_data);
}

PlatformRendererDarwin::~PlatformRendererDarwin() { CleanupUIView(); }

void PlatformRendererDarwin::OnUpdateDisplayList(DisplayList display_list) {
  if (display_list.HasContent()) {
    display_list_ = std::move(display_list);

    UIView<LynxRendererHost>* view = GetUIView();
    if (view != nil) {
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
        layout_frame = ResolveLayoutFrame(layout_frame);
        UpdateUIOwnerLayout(CGRectMake(frame[0], frame[1], frame[2], frame[3]));
        LynxCUIApplyLayoutFrame(view, layout_frame);
        [[view renderer] onSetFrame:layout_frame];

        if ([view conformsToProtocol:@protocol(LUIBodyView)]) {
          ((UIView<LUIBodyView>*)view).intrinsicContentSize = CGSizeMake(frame[2], frame[3]);
        }
      }

      [[view renderer] updateDisplayList:&display_list_];
      [view setNeedsDisplay];
    }
  }
}

void PlatformRendererDarwin::OnUpdateAttributes(const fml::RefPtr<PropBundle>& attributes,
                                                bool tends_to_flatten) {
  if (GetUIView() != nil && attributes && attributes->IsNative()) {
    // Convert NativePropBundle to PropBundleDarwin
    // The attributes should be a NativePropBundle from the pipeline
    auto prop_bundle_darwin = CreateDarwinPropBundle(attributes);
    NSDictionary* props = prop_bundle_darwin->dictionary();
    LynxUIOwner* owner = ui_owner_;
    if (owner != nil && HasUIOwnerNode(GetId())) {
      [owner updateUIWithSign:GetId()
                        props:props
                     eventSet:prop_bundle_darwin->event_set()
                lepusEventSet:prop_bundle_darwin->lepus_event_set()
           gestureDetectorSet:prop_bundle_darwin->gesture_detector_set()];
    }
    UIView<LynxRendererHost>* update_view = GetUIView();
    [[update_view renderer] updateAttributes:props];
  }
}

void PlatformRendererDarwin::OnRebuildSubRenderers() {
  UIView<LynxRendererHost>* view = GetUIView();
  if (view == nil) {
    return;
  }

  [[view renderer] onRebuildSubRenderers];
}

void PlatformRendererDarwin::OnAddChild(PlatformRenderer* child, int index,
                                        bool should_update_ui_owner) {
  // TODO(linxs): Support indexed insertion on iOS after mapping the
  // platform renderer child index to the corresponding UIOwner/UIView child index.
  if (child == nullptr) {
    return;
  }

  auto* child_renderer = static_cast<PlatformRendererDarwin*>(child);
  UIView<LynxRendererHost>* child_view = child_renderer->GetUIView();
  LynxUIOwner* owner = ui_owner_;
  if (should_update_ui_owner && owner != nil && HasUIOwnerNode(GetId()) &&
      child_renderer->HasUIOwnerNode(child->GetId())) {
    [owner insertNode:child->GetId() toParent:GetId() atIndex:index];
    [[child_view renderer] reattachHostDecorationLayers];
    return;
  }

  if (child_renderer->IsOverlay()) {
    // Overlay owns its native hierarchy through LynxOverlayGlobalManager. The
    // rendererhost parent can be a plain host view, so never attach the overlay
    // view through the normal UIKit subview path.
    [[child_view renderer] reattachHostDecorationLayers];
    return;
  }

  UIView<LynxRendererHost>* view = GetUIView();
  if (view == nil) {
    return;
  }

  if (child_view == nil) {
    return;
  }

  if (index < 0) {
    [view addSubview:child_view];
  } else {
    [view insertSubview:child_view atIndex:index];
  }
  [[child_view renderer] reattachHostDecorationLayers];
}

void PlatformRendererDarwin::OnRemoveFromParent(bool should_update_ui_owner) {
  LynxUIOwner* owner = ui_owner_;
  if (should_update_ui_owner && ShouldDetachThroughUIOwner(owner, GetId())) {
    [owner detachNode:GetId()];
    UIView<LynxRendererHost>* detach_view = GetUIView();
    [[detach_view renderer] detachHostDecorationLayers];
    return;
  }

  if (!should_update_ui_owner && IsOverlay()) {
    return;
  }

  UIView<LynxRendererHost>* view = GetUIView();
  if (view == nil) {
    return;
  }

  [[view renderer] detachHostDecorationLayers];
  [view removeFromSuperview];
}

void PlatformRendererDarwin::OnUpdateSubtreeProperties(const DisplayList& subtree_properties) {
  size_t count = subtree_properties.GetSubtreePropertiesSize();
  if (count == 0) {
    return;
  }

  LynxRenderer* renderer = GetUIView().renderer;
  if (renderer == nil) {
    return;
  }

  const SubtreeProperty* props = subtree_properties.GetSubtreePropertiesData();
  if (props == nullptr) {
    return;
  }

  [renderer applySubtreeProperties:props count:count];
}

void PlatformRendererDarwin::InitializeUIView() { InitializeUIView(fml::RefPtr<PropBundle>()); }

void PlatformRendererDarwin::InitializeUIView(const fml::RefPtr<PropBundle>& init_data) {
  if (context_ == nullptr) {
    return;
  }

  if (IsPlatformExtendedRenderer()) {
    base::String extended_tag_name = GetExtendedRendererTagName();
    NSString* tagName = [NSString stringWithUTF8String:extended_tag_name.str().c_str()];
    Class hostClass = [LynxComponentRegistry rendererHostClassWithName:tagName];
    auto initial_prop_bundle = CreateDarwinPropBundle(init_data);
    NSDictionary* initial_props = initial_prop_bundle ? initial_prop_bundle->dictionary() : nil;

    if (hostClass && [hostClass conformsToProtocol:@protocol(LynxRendererHost)]) {
      LynxRendererContext* rendererContext = context_->GetRendererContext();
      id<LynxRendererHost> customHost = [[hostClass alloc] initWithRendererContext:rendererContext];
      if (customHost && [customHost isKindOfClass:[UIView class]]) {
        // Safe to cast after confirming it's a UIView
        _view = (UIView<LynxRendererHost>*)customHost;
        InitializeRendererForView(GetUIView(), initial_props);
        return;
      }
    }

    if (InitializeUIOwnerRenderer(extended_tag_name, init_data)) {
      return;
    }

    _view = [[LynxContainerView alloc] init];
    InitializeRendererForView(GetUIView(), initial_props);
    return;
  } else {
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
        if (!InitializeUIOwnerRenderer(GetExtendedRendererTagName(), init_data)) {
          _view =
              context_ != nullptr ? (UIView<LynxRendererHost>*)context_->GetContainerView() : nil;
        }
        break;
      }
      default:
        break;
    }
  }

  InitializeRendererForView(GetUIView());
}

bool PlatformRendererDarwin::ShouldCreatePlatformExtendedRenderer(
    const PlatformRendererInitConfig& init_config) const {
  if (init_config.is_direct_child_of_compatible_component) {
    return true;
  }
  if (type_ == PlatformRendererType::kText || type_ == PlatformRendererType::kImage ||
      type_ == PlatformRendererType::kView || type_ == PlatformRendererType::kPage) {
    return false;
  }
  if (type_ != PlatformRendererType::kUnknown) {
    return true;
  }
  return !tag_name_.empty();
}

bool PlatformRendererDarwin::InitializeUIOwnerRenderer(const base::String& tag_name,
                                                       const fml::RefPtr<PropBundle>& init_data) {
  LynxUIOwner* owner = ui_owner_;
  if (owner == nil || tag_name.empty()) {
    return false;
  }

  NSDictionary* props = nil;
  NSSet* event_set = nil;
  NSSet* lepus_event_set = nil;
  NSSet* gesture_detector_set = nil;
  auto prop_bundle = CreateDarwinPropBundle(init_data);
  if (prop_bundle) {
    props = prop_bundle->dictionary();
    event_set = prop_bundle->event_set();
    lepus_event_set = prop_bundle->lepus_event_set();
    gesture_detector_set = prop_bundle->gesture_detector_set();
  }

  NSString* tagName = [NSString stringWithUTF8String:tag_name.str().c_str()];
  TagSupportedState state = LynxUnsupportedTag;
  Class clazz = [owner getTargetClass:tagName props:props supportedState:&state];
  if (state == LynxUnsupportedTag) {
    return false;
  }

  [owner createUISyncWithSign:GetId()
                      tagName:tagName
                        clazz:clazz
               supportedState:state
                     eventSet:event_set
                lepusEventSet:lepus_event_set
                        props:props
                    nodeIndex:GetId()
           gestureDetectorSet:gesture_detector_set];

  LynxUI* ui = [owner findUIBySign:GetId()];
  if (ui == nil) {
    return false;
  }
  UIView* ui_view = ui.view;
  if (ui_view == nil || ![ui_view respondsToSelector:@selector(createRendererWithSign:
                                                                           andContext:)]) {
    [owner cleanupCreatedUIWithSign:GetId()];
    return false;
  }

  _view = (UIView<LynxRendererHost>*)ui_view;
  InitializeRendererForView(GetUIView(), props);
  return true;
}

void PlatformRendererDarwin::InitializeRendererForView(UIView<LynxRendererHost>* view,
                                                       NSDictionary* initial_props) {
  if (view == nil || context_ == nullptr) {
    return;
  }
  LynxRenderer* renderer = [view createRendererWithSign:GetId()
                                             andContext:context_->GetRendererContext()];
  [view setRenderer:renderer];
  if (initial_props != nil) {
    [renderer updateAttributes:initial_props];
  }
}

CGRect PlatformRendererDarwin::ResolveLayoutFrame(CGRect frame) const {
  if (!IsOverlayRendererTag(GetExtendedRendererTagName())) {
    return frame;
  }

  // Overlay keeps a zero-sized layout box, but its renderer host surface is full-screen.
  CGSize screen_size = context_ != nullptr ? context_->GetScreenSize() : CGSizeZero;
  if (screen_size.width <= 0.f || screen_size.height <= 0.f) {
    screen_size = UIScreen.mainScreen.bounds.size;
  }
  frame.origin = CGPointZero;
  frame.size = screen_size;
  return frame;
}

void PlatformRendererDarwin::UpdateUIOwnerLayout(CGRect frame) {
  LynxUIOwner* owner = ui_owner_;
  if (owner == nil || !HasUIOwnerNode(GetId())) {
    return;
  }
  LynxUI* ui = [owner findUIBySign:GetId()];
  if (ui == nil) {
    return;
  }
  UIEdgeInsets padding = ui.padding;
  UIEdgeInsets border = ui.border;
  UIEdgeInsets margin = ui.margin;
  if (HasLayoutMetrics()) {
    padding = UIEdgeInsetsFromLayoutMetrics(GetLayoutPaddings());
    border = UIEdgeInsetsFromLayoutMetrics(GetLayoutBorders());
    margin = UIEdgeInsetsFromLayoutMetrics(GetLayoutMargins());
  }
  [owner updateUI:GetId()
       layoutLeft:frame.origin.x
              top:frame.origin.y
            width:frame.size.width
           height:frame.size.height
          padding:padding
           border:border
           margin:margin
           sticky:ui.sticky];
}

bool PlatformRendererDarwin::HasUIOwnerNode(int sign) const {
  LynxUIOwner* owner = ui_owner_;
  return owner != nil && [owner findUIBySign:sign] != nil;
}

void PlatformRendererDarwin::CleanupUIView() {
  LynxUIOwner* owner = ui_owner_;
  bool should_remove_from_native_parent = true;
  if (HasUIOwnerNode(GetId())) {
    should_remove_from_native_parent = !ShouldDetachThroughUIOwner(owner, GetId());
    [owner recycleNode:GetId()];
  }

  UIView<LynxRendererHost>* view = GetUIView();
  if (view != nil) {
    [[view renderer] detachHostDecorationLayers];
    if (should_remove_from_native_parent) {
      [view removeFromSuperview];
    }
  }
}

void PlatformRendererDarwin::UpdatePlatformExtraBundle(id platform_extra_bundle) {
  UIView<LynxRendererHost>* view = GetUIView();
  if (view == nil) {
    return;
  }
  [view.renderer updatePlatformExtraBundle:platform_extra_bundle];
}

}  // namespace tasm
}  // namespace lynx
