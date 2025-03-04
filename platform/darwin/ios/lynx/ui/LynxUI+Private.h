// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxUI.h>

NS_ASSUME_NONNULL_BEGIN

// Used to mark that LynxUI-related functions may run on an asynchronous thread. It may be possible
// to use this macro to include some main thread API checkers in debug mode, ensuring that code
// wrapped by this macro do not call main thread APIs.
#define LYNX_MAYBE_ON_ASYNC_THREAD

// Use NSAssert to check that specific functions must run on the main thread.
#define LYNX_ASSERT_ON_MAIN_THREAD \
  NSAssert([NSThread isMainThread], @"This function must be called on main thread.");

typedef void (^LynxPropsDidUpdateBlockReadyBlock)(LynxUI*);

@interface LynxUI ()

// If you need to do something in propsDidUpdate later, add them as LynxNodeReadyBlock here.
@property(nonatomic) NSMutableArray<LynxPropsDidUpdateBlockReadyBlock>* propsDidUpdateBlockArray;

- (instancetype)initWithoutView NS_DESIGNATED_INITIALIZER;

- (void)setView:(UIView*)view;

- (void)initProperties;

// !!! This is a private API; do not override this API !!!
// Currently, LynxUI provides an onNodeReady callback to indicate that the property update and
// layout have ended. However, onNodeReady is a public interface, and many business-implemented
// native components have overridden this function but did not call [super onNodeReady]. This causes
// potential breaks when LynxUI involves changes related to onNodeReady. To solve this issue, a new
// private API named onNodeReadyForUIOwner has been added, allowing LynxUIOwner to call this API,
// and then onNodeReadyForUIOwner calls onNodeReady to ensure business logic is not broken.
- (void)onNodeReadyForUIOwner;

// !!! This is a private API; do not override this API !!!
// Currently, LynxUI provides a propsDidUpdate callback to indicate that the property update has
// ended. However, propsDidUpdate is a public interface, and many business-implemented native
// components have overridden this function but did not call [super propsDidUpdate]. This causes
// potential breaks when LynxUI involves changes related to propsDidUpdate. To solve this issue, a
// new private API named propsDidUpdateForUIOwner has been added, allowing LynxUIOwner to call this
// API, and then propsDidUpdateForUIOwner calls propsDidUpdate to ensure business logic is not
// broken.
- (void)propsDidUpdateForUIOwner;

@end
NS_ASSUME_NONNULL_END
