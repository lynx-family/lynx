// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#import <Lynx/LynxEventEmitter.h>
#import <Lynx/LynxListViewLight.h>
#import <Lynx/LynxUIListScrollManager.h>

typedef NS_ENUM(NSUInteger, LynxListEventsScrollPosition) {
  LynxListEventsScrollPositionInit = 0,
  LynxListEventsScrollPositionTop,
  LynxListEventsScrollPositionMid,
  LynxListEventsScrollPositionBottom,
  LynxListEventsScrollPositionBothEnds,
};

static NSString *const kLynxListEventsScroll = @"scroll";
static NSString *const kLynxListEventsScrollToUpper = @"scrolltoupper";
static NSString *const kLynxListEventsScrollToLower = @"scrolltolower";

@interface LynxUIListScrollManager ()
@property(nonatomic, assign) NSInteger sign;
@property(nonatomic, weak) LynxEventEmitter *emitter;

/*
 Record the last status to make the comparison
 */
@property(nonatomic, assign) CGPoint lastContentOffset;
@property(nonatomic, assign) CGSize lastContentSize;
@property(nonatomic, assign) NSTimeInterval lastUpdateTime;
@property(nonatomic, strong) NSString *lastEvent;

/*
 Flags for scrollToLower and scrollToUpper.
 */
@property(nonatomic, assign) CGFloat scrollToStartOffset;
@property(nonatomic, assign) CGFloat scrollToEndOffset;
@property(nonatomic, assign) NSInteger scrollToStartItemCount;
@property(nonatomic, assign) NSInteger scrollToEndItemCount;

/*
 Throttle of event frequency in milliseconds.
 */
@property(nonatomic, assign) CGFloat throttle;

@property(nonatomic, assign) LynxListEventsScrollPosition position;
@end

@implementation LynxUIListScrollManager

#pragma mark init
- (instancetype)init {
  self = [super init];
  if (self) {
    self.scrollToEndItemCount = NSIntegerMax;
    self.scrollToStartItemCount = NSIntegerMin;
  }
  return self;
}

- (void)setSign:(NSInteger)sign {
  _sign = sign;
}

- (void)setEventEmitter:(LynxEventEmitter *)eventEmitter {
  _emitter = eventEmitter;
}

- (void)updateScrollThresholds:(LynxUIListScrollThresholds *)scrollThreSholds {
  self.scrollToStartOffset = scrollThreSholds.scrollToStartOffset
                                 ? scrollThreSholds.scrollToStartOffset.floatValue
                                 : self.scrollToStartOffset;
  self.scrollToEndOffset = scrollThreSholds.scrollToEndOffset
                               ? scrollThreSholds.scrollToEndOffset.floatValue
                               : self.scrollToEndOffset;
  self.scrollToStartItemCount = scrollThreSholds.scrollToStartItemCount
                                    ? scrollThreSholds.scrollToStartItemCount.integerValue
                                    : self.scrollToStartItemCount;
  self.scrollToEndItemCount = scrollThreSholds.scrollToEndItemCount
                                  ? scrollThreSholds.scrollToEndItemCount.integerValue
                                  : self.scrollToEndItemCount;
  self.throttle = scrollThreSholds.throttle ? scrollThreSholds.throttle.floatValue : self.throttle;
}

#pragma mark delegate
- (void)scrollViewDidScroll:(UIScrollView *)scrollView {
  [(LynxListViewLight *)scrollView
      dispatchInvalidationContext:[[LynxUIListInvalidationContext alloc] initWithBoundsChange]];

  // update scrollingDirection
  if (!self.horizontal) {
    if (self.lastContentOffset.y > scrollView.contentOffset.y) {
      self.scrollingDirection = LynxListScrollDirectionToLower;
    } else if (self.lastContentOffset.y < scrollView.contentOffset.y) {
      self.scrollingDirection = LynxListScrollDirectionToUpper;
    } else {
      self.scrollingDirection = LynxListScrollDirectionStop;
    }
  } else {
    if (self.lastContentOffset.x > scrollView.contentOffset.x) {
      self.scrollingDirection = LynxListScrollDirectionToLower;
    } else if (self.lastContentOffset.x < scrollView.contentOffset.x) {
      self.scrollingDirection = LynxListScrollDirectionToUpper;
    } else {
      self.scrollingDirection = LynxListScrollDirectionStop;
    }
  }

  [self sendScrollEvent:(LynxListViewLight *)scrollView];
}

- (void)scrollViewWillBeginDragging:(UIScrollView *)scrollView {
  self.scrollStatus = LynxListScrollStatusDragging;
}

- (void)scrollViewDidEndDecelerating:(UIScrollView *)scrollView {
  self.scrollStatus = LynxListScrollStatusIdle;
  self.scrollingDirection = LynxListScrollDirectionStop;
}

- (void)scrollViewWillBeginDecelerating:(UIScrollView *)scrollView {
  self.scrollStatus = LynxListScrollStatusFling;
}

#pragma mark event
- (void)sendScrollEvent:(LynxListViewLight *)view {
  CGPoint preOffset = self.lastContentOffset;
  NSString *eventType = [self fetchScrollEvent:view];
  if (eventType) {
    NSDictionary *detail = @{
      @"scrollTop" : @(view.contentOffset.y),
      @"scrollLeft" : @(view.contentOffset.x),
      @"scrollWidth" : @(view.contentSize.width),
      @"scrollHeight" : @(view.contentSize.height),
      @"deltaX" : @(view.contentOffset.x - preOffset.x),
      @"deltaY" : @(view.contentOffset.y - preOffset.y),
      @"attachedCells" : [view attachedCells] ?: @[],
      @"state" : @(self.scrollStatus)
    };
    [self sendCustomEvent:eventType detail:detail];
  }
}

- (void)sendCustomEvent:(NSString *)name detail:(NSDictionary *)detail {
  LynxCustomEvent *event = [[LynxDetailEvent alloc] initWithName:name
                                                      targetSign:self.sign
                                                          detail:detail];
  [self.emitter sendCustomEvent:event];
}

- (NSString *)fetchScrollEvent:(LynxListViewLight *)view {
  NSArray<NSNumber *> *visibleIndexes = [self visibleIndexes:[view visibleCells]];

  LynxListEventsScrollPosition currentPosition = [self position:view
                                                 visibleIndexes:visibleIndexes
                                                          total:[view totalItemsCount]];

  self.lastContentOffset = view.contentOffset;
  self.lastContentSize = view.contentSize;

  NSTimeInterval previousUpdateTime = self.lastUpdateTime;
  self.lastUpdateTime = CFAbsoluteTimeGetCurrent();

  NSString *event = nil;
  switch (currentPosition) {
    case LynxListEventsScrollPositionTop:
      event = self.position == LynxListEventsScrollPositionTop ? kLynxListEventsScroll
                                                               : kLynxListEventsScrollToUpper;
      break;
    case LynxListEventsScrollPositionBottom:
      event = self.position == LynxListEventsScrollPositionBottom ? kLynxListEventsScroll
                                                                  : kLynxListEventsScrollToLower;
      break;
    case LynxListEventsScrollPositionMid:
      event = kLynxListEventsScroll;
      break;
    default:
      break;
  }

  if (self.lastEvent == event && event == kLynxListEventsScroll &&
      self.lastUpdateTime - previousUpdateTime < self.throttle / 1000.0) {
    event = nil;
  }

  self.position = currentPosition;
  self.lastEvent = event;

  return event;
}

- (LynxListEventsScrollPosition)position:(UIScrollView *)scrollView
                          visibleIndexes:(NSArray<NSNumber *> *)visibleIndexes
                                   total:(NSInteger)totalItemsCount {
  if (scrollView.contentSize.width * scrollView.contentSize.height == 0 ||
      self.lastContentSize.width * self.lastContentSize.height == 0) {
    return LynxListEventsScrollPositionInit;
  }
  CGFloat top = scrollView.contentOffset.y + scrollView.contentInset.top;
  CGFloat bottom = scrollView.contentSize.height - scrollView.bounds.size.height -
                   scrollView.contentOffset.y + scrollView.contentInset.bottom;

  CGFloat left = scrollView.contentOffset.x + scrollView.contentInset.left;
  CGFloat right = scrollView.contentSize.width - scrollView.bounds.size.width -
                  scrollView.contentOffset.x + scrollView.contentInset.right;

  BOOL reachStart = NO;
  BOOL reachEnd = NO;

  NSArray<NSNumber *> *sortedVisibleIndexes = [visibleIndexes
      sortedArrayUsingDescriptors:[NSArray arrayWithObject:[NSSortDescriptor
                                                               sortDescriptorWithKey:@"self"
                                                                           ascending:YES]]];
  if (self.scrollToStartItemCount != NSIntegerMin) {
    reachStart =
        sortedVisibleIndexes.count
            ? ((sortedVisibleIndexes.firstObject.integerValue + 1) <= self.scrollToStartItemCount)
            : NO;
  } else {
    reachStart = (self.horizontal ? left : top) <= self.scrollToStartOffset;
  }

  if (self.scrollToEndItemCount != NSIntegerMax) {
    reachEnd = sortedVisibleIndexes
                   ? (totalItemsCount - sortedVisibleIndexes.lastObject.integerValue - 1 <=
                      self.scrollToEndItemCount)
                   : NO;
  } else {
    reachEnd = (self.horizontal ? right : bottom) <= self.scrollToEndOffset;
  }

  if (self.horizontal) {
    if (reachStart && reachEnd) {
      return scrollView.contentOffset.x / scrollView.contentSize.width >
                     self.lastContentOffset.x / self.lastContentSize.width
                 ? LynxListEventsScrollPositionBottom
                 : LynxListEventsScrollPositionTop;
    } else if (reachStart) {
      return LynxListEventsScrollPositionTop;
    } else if (reachEnd) {
      return LynxListEventsScrollPositionBottom;
    } else {
      return LynxListEventsScrollPositionMid;
    }
  } else {
    if (reachStart && reachEnd) {
      return scrollView.contentOffset.y / scrollView.contentSize.height >
                     self.lastContentOffset.y / self.lastContentSize.height
                 ? LynxListEventsScrollPositionBottom
                 : LynxListEventsScrollPositionTop;
    } else if (reachStart) {
      return LynxListEventsScrollPositionTop;
    } else if (reachEnd) {
      return LynxListEventsScrollPositionBottom;
    } else {
      return LynxListEventsScrollPositionMid;
    }
  }
}

- (NSArray<NSNumber *> *)visibleIndexes:(NSArray<id<LynxListCell>> *)cellArray {
  NSMutableArray<NSNumber *> *visibleIndexes = [NSMutableArray array];
  [cellArray enumerateObjectsUsingBlock:^(id<LynxListCell> _Nonnull obj, NSUInteger idx,
                                          BOOL *_Nonnull stop) {
    [visibleIndexes addObject:@(obj.updateToPath)];
  }];
  return visibleIndexes.copy;
}

@end
