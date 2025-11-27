// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/fml/logging.h"

#import "clay/shell/platform/darwin/macos/framework/Source/FlutterPlatformViewController.h"

@implementation FlutterPlatformViewController {
  // NSDictionary maps platform view type identifiers to FlutterPlatformViewFactories.
  NSMutableDictionary<NSString*, NSObject<FlutterPlatformViewFactory>*>* _platformViewFactories;

  // Map from platform view id to the underlying NSView.
  std::map<int, NSView*> _platformViews;

  // View ids that are going to be disposed on the next present call.
  std::unordered_set<int64_t> _platformViewsToDispose;
}

- (instancetype)init {
  self = [super init];
  if (self) {
    _platformViewFactories = [[NSMutableDictionary alloc] init];
  }
  return self;
}

- (void)registerViewFactory:(nonnull NSObject<FlutterPlatformViewFactory>*)factory
                     withId:(nonnull NSString*)factoryId {
  _platformViewFactories[factoryId] = factory;
}

- (nullable NSView*)platformViewWithID:(int64_t)viewId {
  if (_platformViews.count(viewId)) {
    return _platformViews[viewId];
  } else {
    return nil;
  }
}

- (void)disposePlatformViews {
  if (_platformViewsToDispose.empty()) {
    return;
  }

  FML_DCHECK([[NSThread currentThread] isMainThread])
      << "Must be on the main thread to handle disposing platform views";
  for (int64_t viewId : _platformViewsToDispose) {
    NSView* view = _platformViews[viewId];
    [view removeFromSuperview];
    _platformViews.erase(viewId);
  }
  _platformViewsToDispose.clear();
}

@end
