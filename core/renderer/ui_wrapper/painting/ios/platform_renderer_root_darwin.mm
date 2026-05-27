// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/ui_wrapper/painting/ios/platform_renderer_root_darwin.h"

namespace lynx {
namespace tasm {

PlatformRendererRootDarwin::PlatformRendererRootDarwin(PlatformRendererContextDarwin* context,
                                                       int id, PlatformRendererType type)
    : PlatformRendererDarwin(context, id, type, base::String()) {
  // The parent constructor called InitializeUIView(), which set _view to the
  // container view (LynxView) for the kPage type. Transfer this to our weak
  // reference and clear the strong ref to break the retain cycle.
  _rootView = _view;
  _view = nil;
}

UIView<LynxRendererHost>* PlatformRendererRootDarwin::GetUIView() { return _rootView; }

}  // namespace tasm
}  // namespace lynx
