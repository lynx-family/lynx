// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>
#import "LynxUIOwner.h"
@class LynxView;
@protocol LUIBodyView;
NS_ASSUME_NONNULL_BEGIN

@interface LynxUIOwner (Accessibility)

- (void)addA11yMutation:(NSString *_Nonnull)action
                   sign:(NSNumber *_Nonnull)sign
                 a11yID:(NSString *_Nullable)a11yID
                toArray:(NSMutableArray *)array;

- (void)addA11yPropsMutation:(NSString *_Nonnull)property
                        sign:(NSNumber *_Nonnull)sign
                      a11yID:(NSString *_Nullable)a11yID
                     toArray:(NSMutableArray *)array;

- (void)flushMutations:(NSMutableArray *)array withBodyView:(id<LUIBodyView> _Nullable)lynxView;

- (void)listenAccessibilityFocused;

- (void)setA11yFilter:(NSSet<NSString *> *)filter;

- (BOOL)checkNestedAccessibilityElements:(LynxUI *)subTree;

@end

NS_ASSUME_NONNULL_END
