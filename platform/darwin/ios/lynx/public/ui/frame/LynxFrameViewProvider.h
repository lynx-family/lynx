// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>

@class LynxUIContext;
@class LynxFrameView;

NS_ASSUME_NONNULL_BEGIN

/**
 * Protocol for providing LynxFrameView instances.
 * When a custom provider is set via LynxContext.lynxFrameViewProvider,
 * it will be used to create LynxFrameView instances instead of the default constructor.
 */
@protocol LynxFrameViewProvider <NSObject>

/**
 * Gets a LynxFrameView instance.
 * @param context The LynxContext to associate with the LynxFrameView
 * @return A LynxFrameView instance, or nil to fall back to default creation
 */
- (nullable LynxFrameView *)getLynxFrameView:(LynxUIContext *)context;

@end

NS_ASSUME_NONNULL_END
