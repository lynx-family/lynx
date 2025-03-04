// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@class LynxUI;
@protocol LynxListScrollEventEmitterDelegate <NSObject>
@optional
/// implement this method if you want to customize `scrolltolower` event sending while scrolling
/// This method is queried in each `scrollViewDidScroll`
/// This method should return `YES` if you want to trigger the `scrolltolower` during scrolling.
- (BOOL)shouldForceSendLowerThresholdEvent;

/// implement this method if you want to customize `scrolltoupper` event sending while scrolling
/// if this method should return `YES` if you want to trigger the `scrolltoupper` during scrolling.
- (BOOL)shouldForceSendUpperThresholdEvent;

/// implement this method if you want to attach cells information when sending scrolling event.
/// This NSArray should contains `NSDictionary<NSString*, NSString*>*`
- (NSArray *)attachedCellsArray;
@end

@class LynxEventEmitter;
@class LynxListScrollEventEmitterHelper;
@interface LynxListScrollEventEmitter : NSObject <UIScrollViewDelegate>

- (instancetype)init;
// This method will be deprecated and removed after validating that there are no API breaks.
- (instancetype)initWithLynxUI:(LynxUI *)lynxUI;
- (void)attachToLynxUI:(LynxUI *)lynxUI;

@property(nonatomic, nullable, weak) id<LynxListScrollEventEmitterDelegate> delegate;

/// set to `YES` if you want to send scroll events
@property(nonatomic) BOOL enableScrollEvent;

/// set to `YES` if you want to send scrolltolower events
@property(nonatomic) BOOL enableScrollToLowerEvent;

/// set to `YES` if you want to send scrolltoupper events
@property(nonatomic) BOOL enableScrollToUpperEvent;

/// the minimum time between two scroll events, in milliseconds.
@property(nonatomic) CGFloat scrollEventThrottle;

// see the detailed explanation for scrollUpperThreshold and scrollLowerThreshold in
@property(nonatomic) CGFloat scrollUpperThreshold;
@property(nonatomic) CGFloat scrollLowerThreshold;

@property(nonatomic) BOOL horizontalLayout;

@property(nonatomic, strong, nullable) LynxListScrollEventEmitterHelper *helper;

// @optional methods in UIScrollViewDelegate that are implemented should be declared explicitly.
- (void)scrollViewDidEndDecelerating:(UIScrollView *)scrollView;
- (void)scrollViewDidEndDragging:(UIScrollView *)scrollView willDecelerate:(BOOL)decelerate;
- (void)scrollViewDidScroll:(UIScrollView *)scrollView;
- (void)scrollViewWillBeginDragging:(UIScrollView *)scrollView;
- (void)helperSendScrollEvent:(UIScrollView *)scrollView;
@end

typedef NS_ENUM(NSUInteger, LynxListScrollState) {
  LynxListScrollStateNone = 0,
  LynxListScrollStateDargging,
  LynxListScrollStateScrolling
};

typedef NS_ENUM(NSUInteger, LynxListScrollPosition) {
  LynxListScrollPositionInit = 0,
  LynxListScrollPositionTop,
  LynxListScrollPositionMid,
  LynxListScrollPositionBottom,
  LynxListScrollPositionBothEnds,
};

@interface LynxListScrollEventEmitterHelper : NSObject
@property(nonatomic, assign) LynxListScrollPosition scrollPosition;
@property(nonatomic, assign) LynxListScrollState scrollState;
@property(nonatomic) BOOL horizontalLayout;
- (instancetype)initWithEmitter:(LynxListScrollEventEmitter *)emitter;
- (NSString *)fetchScrollEvent:(UIScrollView *)scrollView;
@end

NS_ASSUME_NONNULL_END
