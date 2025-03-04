// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>
#import "LynxHeroModifiers.h"
#import "LynxUI.h"

NS_ASSUME_NONNULL_BEGIN
@class LynxAnimationInfo;
@interface LynxHeroViewConfig : NSObject
// Only one of sharedElement or content animation will take effect, with sharedElement having higher
// priority. If not found, content animation will be used. Description of sharedElement animation
@property(nonatomic, copy) NSString* sharedElementName;
@property(nonatomic, assign) BOOL crossPage;
@property(nonatomic) LynxHeroModifiers* sharedElementModifiers;
// Lynx shortcut for describing content animation
@property(nonatomic, copy) LynxAnimationInfo* enterTransitionName;
@property(nonatomic, copy) LynxAnimationInfo* exitTransitionName;
@property(nonatomic, copy) LynxAnimationInfo* pauseTransiitonName;
@property(nonatomic, copy) LynxAnimationInfo* resumeTransitionName;
// Native UIView description for animations
@property(nonatomic) LynxHeroModifiers* enterTransitionModifiers;
@property(nonatomic) LynxHeroModifiers* exitTransitionModifiers;
// Whether to take a screenshot, only effective for shared-element animations
@property(nonatomic, assign) BOOL snapshot;
// Whether to elevate the hierarchy
@property(nonatomic, assign) BOOL merge;
@property(nonatomic, weak) LynxUI* lynxUI;
@property(nonatomic, readonly, weak) UIView* view;

- (instancetype)initWithView:(UIView*)view;

@end

@interface UIView (LynxHeroTransition)
@property(nonatomic, readonly) LynxHeroViewConfig* lynxHeroConfig;
@end

NS_ASSUME_NONNULL_END
