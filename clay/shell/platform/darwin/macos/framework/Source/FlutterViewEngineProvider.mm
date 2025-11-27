// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "clay/shell/platform/darwin/macos/framework/Source/FlutterViewEngineProvider.h"
#import "clay/shell/platform/darwin/macos/framework/Source/FlutterEngine_Internal.h"
#import "clay/shell/platform/darwin/macos/framework/Source/FlutterViewController_Internal.h"

@interface FlutterViewEngineProvider () {
  __weak FlutterEngine* _engine;
}

@end

@implementation FlutterViewEngineProvider

- (instancetype)initWithEngine:(FlutterEngine*)engine {
  self = [super init];
  if (self != nil) {
    _engine = engine;
  }
  return self;
}

- (nullable FlutterView*)getView:(uint64_t)viewId {
  // TODO(dkwingsmt): This class only supports the first view for now. After
  // FlutterEngine supports multi-view, it should get the view associated to the
  // ID.
  if (viewId == kFlutterDefaultViewId) {
    // Clay may be doesn't init viewController, so we will get engine's view is VC is null
    FlutterView* view = _engine.viewController.flutterView;
    if (!view) {
      return _engine.view;
    }
  }
  return nil;
}

@end
