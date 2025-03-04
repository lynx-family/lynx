// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>
#import "AbsLynxUIScroller.h"
#import "LynxScrollView.h"
#import "LynxUI.h"

NS_ASSUME_NONNULL_BEGIN

@class LynxBounceView;
@class LynxScrollEventManager;

typedef NS_ENUM(NSInteger, HoverPosition) {
  HoverPositionTop = 0,
  HoverPositionBottom,
  HoverPositionCenter,
  HoverPositionLeft,
  HoverPositionRight
};

@protocol LynxBounceView <NSObject>

@optional
- (void)bdx_updateOverflowText:(nullable NSString *)text;

@end

@protocol LynxScrollViewUIDelegate <NSObject>

@optional
+ (UIView<LynxBounceView> *)LynxBounceView:(UIScrollView *)scrollView;

@end

@interface LynxUIScroller : AbsLynxUIScroller <LynxScrollView *> <UIScrollViewDelegate>
@property(nonatomic) BOOL enableSticky;
@property(nonatomic) BOOL enableScrollY;
@property(class) Class<LynxScrollViewUIDelegate> UIDelegate;
// for bounceView
//@property(nonatomic, strong) NSPointerArray *bounceUIArray;
@property(nonatomic, weak) LynxBounceView *upperBounceUI;
@property(nonatomic, weak) LynxBounceView *lowerBounceUI;
@property(nonatomic, strong, nullable) LynxBounceView *defaultBounceUI;
// Controls scrollToBounce event. Set to true before users' dragging ends.
@property(nonatomic, assign) BOOL isTransferring;
// List native storage
@property(nonatomic, assign) NSString *currentItemKey;
@property(nonatomic, strong, readonly) LynxScrollEventManager *scrollEventManager;

- (float)scrollLeftLimit;
- (float)scrollRightLimit;
- (float)scrollUpLimit;
- (float)scrollDownLimit;
- (void)updateContentSize;

@end

NS_ASSUME_NONNULL_END
