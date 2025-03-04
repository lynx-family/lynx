// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxListLayoutManager.h>
#import <Lynx/LynxUIComponent.h>
#import <Lynx/LynxUIListInvalidationContext.h>

@class LynxListLayoutManager;
@class LynxUIListScrollThresholds;

NS_ASSUME_NONNULL_BEGIN

@interface LynxUIListLight : LynxUI <UIScrollView*> <LynxUIComponentLayoutObserver>
@property(nonatomic, strong, nullable) LynxUIListInvalidationContext* generalPropsInfo;
@property(nonatomic, weak) LynxListLayoutManager* layout;
@property(nonatomic, strong) LynxUIListScrollThresholds* scrollThreSholds;
@property(nonatomic, strong) NSNumber* _Nullable initialScrollIndex;
@property(nonatomic, strong) NSDictionary* _Nullable listNoDiffInfo;
// Receives diff result from platform-info
@property(nonatomic, copy) NSDictionary* _Nullable diffResultFromTasm;
@property(nonatomic, copy, nullable) NSDictionary<NSString*, NSArray*>* curComponents;

- (void)setVerticalOrientation:(BOOL)value;
@end

NS_ASSUME_NONNULL_END
