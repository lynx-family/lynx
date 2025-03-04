// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxTemplateRender.h"
#import "LynxView.h"

@class LynxWeakProxy;
@class LynxLifecycleDispatcher;

@interface LynxView () {
 @protected
  LynxWeakProxy* _clientWeakProxy;
  BOOL _enableTextNonContiguousLayout;
  BOOL _enableLayoutOnly;
  CGSize _intrinsicContentSize;
  BOOL _dispatchingIntrinsicContentSizeChange;
  BOOL _enableSyncFlush;
  // property
  BOOL _attached;
  LynxLifecycleDispatcher* _lifecycleDispatcher;
  LynxTemplateRender* _templateRender;
}

@end
