// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_UI_WRAPPER_PAINTING_IOS_PLATFORM_RENDERER_CONTEXT_DARWIN_H_
#define CORE_RENDERER_UI_WRAPPER_PAINTING_IOS_PLATFORM_RENDERER_CONTEXT_DARWIN_H_

#import <Lynx/LynxRendererContext.h>
#import <UIKit/UIKit.h>

@protocol LUIBodyView;
@class LynxComponentScopeRegistry;
@class LynxUIOwner;

namespace lynx {
namespace tasm {

class PlatformRendererContextDarwin {
 public:
  PlatformRendererContextDarwin(UIView<LUIBodyView>* container_view, LynxUIOwner* ui_owner = nil,
                                LynxComponentScopeRegistry* component_registry = nil);
  ~PlatformRendererContextDarwin();

  UIView<LUIBodyView>* GetContainerView() { return renderer_context_.bodyView; }

  LynxRendererContext* GetRendererContext() { return renderer_context_; }

  LynxUIOwner* GetUIOwner() { return ui_owner_; }

  LynxComponentScopeRegistry* GetComponentRegistry() { return component_registry_; }

  CGPoint GetRootViewLocationOnScreen();
  CGSize GetScreenSize();

 private:
  LynxRendererContext* renderer_context_;
  LynxUIOwner* ui_owner_;
  LynxComponentScopeRegistry* component_registry_;
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_UI_WRAPPER_PAINTING_IOS_PLATFORM_RENDERER_CONTEXT_DARWIN_H_
