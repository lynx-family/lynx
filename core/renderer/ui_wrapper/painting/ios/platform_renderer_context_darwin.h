// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_UI_WRAPPER_PAINTING_IOS_PLATFORM_RENDERER_CONTEXT_DARWIN_H_
#define CORE_RENDERER_UI_WRAPPER_PAINTING_IOS_PLATFORM_RENDERER_CONTEXT_DARWIN_H_

#import <Lynx/LynxRendererContext.h>
#import <UIKit/UIKit.h>

@protocol LUIBodyView;
@class LynxUIOwner;

namespace lynx {
namespace tasm {

class PlatformRendererContextDarwin {
 public:
  PlatformRendererContextDarwin(UIView<LUIBodyView>* container_view, LynxUIOwner* ui_owner = nil);
  ~PlatformRendererContextDarwin();

  UIView<LUIBodyView>* GetContainerView() { return _renderer_context.bodyView; }

  LynxRendererContext* GetRendererContext() { return _renderer_context; }

  LynxUIOwner* GetUIOwner() { return _ui_owner; }

  CGPoint GetRootViewLocationOnScreen();
  CGSize GetScreenSize();

 private:
  LynxRendererContext* _renderer_context;
  LynxUIOwner* _ui_owner;
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_UI_WRAPPER_PAINTING_IOS_PLATFORM_RENDERER_CONTEXT_DARWIN_H_
