// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxUIView.h"

NS_ASSUME_NONNULL_BEGIN

@class LynxUIComponent;

@protocol LynxUIComponentLayoutObserver <NSObject>

- (void)onComponentLayoutUpdated:(LynxUIComponent*)component;
- (void)onAsyncComponentLayoutUpdated:(LynxUIComponent*)component operationID:(int64_t)operationID;

@end

@interface LynxUIComponent : LynxUIView

@property(weak) id<LynxUIComponentLayoutObserver> layoutObserver;
@property(nonatomic, strong) NSString* itemKey;
@property(nonatomic, assign, readonly) BOOL frameDidSet;
@property(nonatomic, assign) NSInteger zIndex;

- (void)asyncListItemRenderFinished:(int64_t)operationID;

@end

NS_ASSUME_NONNULL_END
