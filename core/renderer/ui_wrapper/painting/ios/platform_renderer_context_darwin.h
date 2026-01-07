// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_UI_WRAPPER_PAINTING_IOS_PLATFORM_RENDERER_CONTEXT_DARWIN_H_
#define CORE_RENDERER_UI_WRAPPER_PAINTING_IOS_PLATFORM_RENDERER_CONTEXT_DARWIN_H_

#import <UIKit/UIKit.h>

@protocol LUIBodyView;

namespace lynx {
namespace tasm {

class PlatformRendererContextDarwin {
 public:
  PlatformRendererContextDarwin(UIView<LUIBodyView>* container_view);
  ~PlatformRendererContextDarwin();

  UIView<LUIBodyView>* GetContainerView() { return _container_view; }

 private:
  __weak UIView<LUIBodyView>* _container_view;
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_UI_WRAPPER_PAINTING_IOS_PLATFORM_RENDERER_CONTEXT_DARWIN_H_
