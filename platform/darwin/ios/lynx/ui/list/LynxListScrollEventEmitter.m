// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxListScrollEventEmitter.h"
#import "LynxEventEmitter.h"
#import "LynxScrollEventManager.h"
#import "LynxUI.h"
#import "LynxUIContext.h"

static const int SCROLL_STATE_STOP = 1;
static const int SCROLL_STATE_DRAGGING = 2;
static const int SCROLL_STATE_DECELERATE = 3;

static const NSInteger BORDER_UPPER = 1 << 0;
static const NSInteger BORDER_LOWER = 1 << 1;
static const NSInteger BORDER_READY_TO_UPPER = 1 << 2;
static const NSInteger BORDER_READY_TO_LOWER = 1 << 3;

@interface LynxListScrollEventEmitter ()
@property(nonatomic, weak, readonly) LynxUI *lynxUI;
@property(nonatomic, weak, readonly) LynxEventEmitter *emitter;
@property(nonatomic, retain) NSDate *lastScrollEventTimestamp;
@property(nonatomic) CGFloat preScrollTop;
@property(nonatomic) CGFloat preScrollLeft;
// This flag makes sure that `SCROLL_STATE_STOP` could only be sent after a `SCROLL_STATE_DRAGGING`
// It is added to avoid the following bug:
// On iPhone 12 pro max with `pagingEnabled` set to `YES`
// `scrollViewDidEndDecelerating` will be triggered by __tapping__ the UIScrollView.
// This makes an __unexpected__ `SCROLL_STATE_STOP` emitted.
@property(nonatomic) BOOL didSendScrollStateDragging;
@property(nonatomic) NSInteger lastBorderStatus;
@property(nonatomic) BOOL nestedUpdated;
@end

@implementation LynxListScrollEventEmitter

- (instancetype)init {
  self = [super init];
  if (self) {
    _didSendScrollStateDragging = NO;
    _lastBorderStatus = BORDER_UPPER;
    _nestedUpdated = NO;
  }
  return self;
}

- (instancetype)initWithLynxUI:(LynxUI *)lynxUI {
  self = [super init];
  if (self) {
    _lynxUI = lynxUI;
    _emitter = _lynxUI.context.eventEmitter;
    _didSendScrollStateDragging = NO;
    _lastBorderStatus = BORDER_UPPER;
    _nestedUpdated = NO;
  }
  return self;
}

- (void)attachToLynxUI:(LynxUI *)lynxUI {
  _lynxUI = lynxUI;
  _emitter = _lynxUI.context.eventEmitter;
}

- (BOOL)reachLowerThresholdByUserDefinedCondition {
  if ([self.delegate respondsToSelector:@selector(shouldForceSendLowerThresholdEvent)]) {
    return [self.delegate shouldForceSendLowerThresholdEvent];
  } else {
    return NO;
  }
}

- (BOOL)reachUpperThresholdByUserDefinedCondition {
  if ([self.delegate respondsToSelector:@selector(shouldForceSendUpperThresholdEvent)]) {
    return [self.delegate shouldForceSendUpperThresholdEvent];
  } else {
    return NO;
  }
}

- (NSArray *)attachedCellsArray {
  if ([self.delegate respondsToSelector:@selector(attachedCellsArray)]) {
    return [self.delegate attachedCellsArray];
  } else {
    return nil;
  }
}

- (void)sendScrollEvent:(UIScrollView *)scrollView force:(BOOL)force {
  if (self.helper) {
    if (_lynxUI.context && (scrollView.isDragging || scrollView.isDecelerating)) {
      [_lynxUI.context onGestureRecognizedByUI:_lynxUI];
    }
    return;
  }
  NSInteger status = 0;

  CGFloat distanceYFromBottom = scrollView.contentSize.height - scrollView.contentOffset.y;
  CGFloat distanceXFromEdge = scrollView.contentSize.width - scrollView.contentOffset.x;

  CGFloat deltaX = scrollView.contentOffset.x - self.preScrollLeft;
  CGFloat deltaY = scrollView.contentOffset.y - self.preScrollTop;

  if (ABS(deltaX) <= CGFLOAT_EPSILON && ABS(deltaY) <= CGFLOAT_EPSILON) {
    return;
  }

  NSString *eventType = self.enableScrollEvent ? LynxEventScroll : nil;
  if (_horizontalLayout) {
    if (deltaX > 0) {
      const BOOL reachLowerThresholdByWidth =
          distanceXFromEdge < scrollView.frame.size.width + self.scrollLowerThreshold;
      const BOOL userDefinedCondition = [self reachLowerThresholdByUserDefinedCondition];

      if (userDefinedCondition) {
        status |= BORDER_READY_TO_LOWER;
      }
      if (reachLowerThresholdByWidth) {
        status |= BORDER_LOWER;
        if (userDefinedCondition) {
          status &= (~BORDER_READY_TO_LOWER);
        }
      }
      if (_enableScrollToLowerEvent) {
        BOOL isLower = [self isLower:status] && ![self isLower:_lastBorderStatus];
        BOOL isReadyToLower =
            [self isReadyToLower:status] && ![self isReadyToLower:_lastBorderStatus];
        if (isLower || isReadyToLower) {
          eventType = LynxEventScrollToLower;
        }
      }
    }

    if (deltaX < 0) {
      const BOOL reachUpperThresholdByWidth =
          scrollView.contentOffset.x <= self.scrollUpperThreshold;
      const BOOL userDefinedCondition = [self reachUpperThresholdByUserDefinedCondition];

      if (userDefinedCondition) {
        status |= BORDER_READY_TO_UPPER;
      }
      if (reachUpperThresholdByWidth) {
        status |= BORDER_UPPER;
        if (userDefinedCondition) {
          status &= (~BORDER_READY_TO_UPPER);
        }
      }
      if (_enableScrollToUpperEvent) {
        BOOL isUpper = [self isUpper:status] && ![self isUpper:_lastBorderStatus];
        BOOL isReadyToUpper =
            [self isReadyToUpper:status] && ![self isReadyToUpper:_lastBorderStatus];
        if (isUpper || isReadyToUpper) {
          eventType = LynxEventScrollToUpper;
        }
      }
    }
  } else {
    if (deltaY > 0) {
      const BOOL reachLowerThresholdByHeight =
          distanceYFromBottom < scrollView.frame.size.height + self.scrollLowerThreshold;
      const BOOL userDefinedCondition = [self reachLowerThresholdByUserDefinedCondition];

      if (userDefinedCondition) {
        status |= BORDER_READY_TO_LOWER;
      }
      if (reachLowerThresholdByHeight) {
        status |= BORDER_LOWER;
        if (userDefinedCondition) {
          status &= (~BORDER_READY_TO_LOWER);
        }
      }
      if (_enableScrollToLowerEvent) {
        BOOL isLower = [self isLower:status] && ![self isLower:_lastBorderStatus];
        BOOL isReadyToLower =
            [self isReadyToLower:status] && ![self isReadyToLower:_lastBorderStatus];
        if (isLower || isReadyToLower) {
          eventType = LynxEventScrollToLower;
        }
      }
    }

    if (deltaY < 0) {
      const BOOL reachUpperThresholdByHeight =
          scrollView.contentOffset.y <= self.scrollUpperThreshold;
      const BOOL userDefinedCondition = [self reachUpperThresholdByUserDefinedCondition];

      if (userDefinedCondition) {
        status |= BORDER_READY_TO_UPPER;
      }
      if (reachUpperThresholdByHeight) {
        status |= BORDER_UPPER;
        if (userDefinedCondition) {
          status &= (~BORDER_READY_TO_UPPER);
        }
      }
      if (_enableScrollToUpperEvent) {
        BOOL isUpper = [self isUpper:status] && ![self isUpper:_lastBorderStatus];
        BOOL isReadyToUpper =
            [self isReadyToUpper:status] && ![self isReadyToUpper:_lastBorderStatus];
        if (isUpper || isReadyToUpper) {
          eventType = LynxEventScrollToUpper;
        }
      }
    }
  }
  if ([eventType isEqualToString:LynxEventScroll]) {
    NSDate *now = [NSDate date];
    if (!force && [now timeIntervalSinceDate:self.lastScrollEventTimestamp] <
                      self.scrollEventThrottle / 1000.f) {
      return;
    }
    self.lastScrollEventTimestamp = now;
  }

  if (eventType) {
    [self sendScrollEvent:eventType
                scrollTop:scrollView.contentOffset.y
                scollleft:scrollView.contentOffset.x
             scrollHeight:scrollView.contentSize.height
              scrollWidth:scrollView.contentSize.width
                   deltaX:deltaX
                   deltaY:deltaY];
  }

  self.preScrollTop = scrollView.contentOffset.y;
  self.preScrollLeft = scrollView.contentOffset.x;

  if (_lynxUI.context && (scrollView.isDragging || scrollView.isDecelerating)) {
    [_lynxUI.context onGestureRecognizedByUI:_lynxUI];
  }
  _lastBorderStatus = status;
}

- (void)helperSendScrollEvent:(UIScrollView *)scrollView {
  if (self.helper) {
    NSString *eventType = [self.helper fetchScrollEvent:scrollView];
    if (eventType) {
      [self sendScrollEvent:eventType
                  scrollTop:scrollView.contentOffset.y
                  scollleft:scrollView.contentOffset.x
               scrollHeight:scrollView.contentSize.height
                scrollWidth:scrollView.contentSize.width
                     deltaX:scrollView.contentOffset.x - self.preScrollLeft
                     deltaY:scrollView.contentOffset.y - self.preScrollTop
                      state:self.helper.scrollState];
    }

    self.preScrollTop = scrollView.contentOffset.y;
    self.preScrollLeft = scrollView.contentOffset.x;
  }
}

- (void)scrollViewDidScroll:(UIScrollView *)scrollView {
  [self sendScrollEvent:scrollView force:NO];
  if ([scrollView respondsToSelector:@selector(triggerNestedScrollView:)]) {
    if (scrollView.enableNested && !_nestedUpdated) {
      _nestedUpdated = YES;
      if (!scrollView.parentScrollView &&
          (!scrollView.childrenScrollView || scrollView.childrenScrollView.count == 0)) {
        [scrollView updateChildren];
      }
    }
  }
  [scrollView triggerNestedScrollView:!_horizontalLayout];
}

- (void)scrollViewWillBeginDragging:(UIScrollView *)scrollView {
  [self sendScrollStateEvent:SCROLL_STATE_DRAGGING];
}

- (void)scrollViewDidEndDragging:(UIScrollView *)scrollView willDecelerate:(BOOL)decelerate {
  if (decelerate) {
    [self sendScrollStateEvent:SCROLL_STATE_DECELERATE];
  } else {
    [self sendScrollEvent:scrollView force:NO];
    BOOL dragToDragStop = scrollView.tracking && !scrollView.dragging && !scrollView.decelerating;

    // the following condition `dragToDragStop` prevents the SCROLL_STATE_STOP from emitting while
    // the bounces of the scrollview is NO on real iOS Devices (NOT SIMULATORS!). add
    // `noBouncesScrollViewEndDragging` to circumvent it.
    BOOL noBouncesScrollViewEndDragging =
        (!scrollView.bounces && scrollView.tracking && !scrollView.decelerating);
    if (dragToDragStop || noBouncesScrollViewEndDragging) {
      [self scrollStop:scrollView];
    }
  }
}

- (void)scrollViewDidEndDecelerating:(UIScrollView *)scrollView {
  BOOL scrollToScrollStop =
      !scrollView.tracking && !scrollView.dragging && !scrollView.decelerating;
  if (scrollToScrollStop) {
    [self scrollStop:scrollView];
  }
}

- (void)scrollStop:(UIScrollView *)scrollView {
  [self sendScrollStateEvent:SCROLL_STATE_STOP];
}

- (void)sendScrollStateEvent:(int)state {
  // all `scrollstatechange` should start with `SCROLL_STATE_DRAGGING` and end with
  // `SCROLL_STATE_STOP`
  if (!_didSendScrollStateDragging && state == SCROLL_STATE_STOP) {
    // No `SCROLL_STATE_STOP` event should be sent without a precedent `SCROLL_STATE_DRAGGING`
    return;
  } else if (!_didSendScrollStateDragging && state == SCROLL_STATE_DRAGGING) {
    _didSendScrollStateDragging = YES;
  } else if (_didSendScrollStateDragging && state == SCROLL_STATE_STOP) {
    _didSendScrollStateDragging = NO;
  }

  NSMutableDictionary *detail = [[NSMutableDictionary alloc] init];
  detail[@"state"] = @(state);
  NSArray *attachedCellsArray = [self attachedCellsArray];
  if (attachedCellsArray) {
    detail[@"attachedCells"] = attachedCellsArray;
  }
  LynxCustomEvent *scrollEventInfo = [[LynxDetailEvent alloc] initWithName:@"scrollstatechange"
                                                                targetSign:_lynxUI.sign
                                                                    detail:detail];
  [_emitter sendCustomEvent:scrollEventInfo];
}

- (void)sendScrollEvent:(NSString *)name
              scrollTop:(float)top
              scollleft:(float)left
           scrollHeight:(float)height
            scrollWidth:(float)width
                 deltaX:(float)x
                 deltaY:(float)y {
  [self sendScrollEvent:name
              scrollTop:top
              scollleft:left
           scrollHeight:height
            scrollWidth:width
                 deltaX:x
                 deltaY:y
                  state:LynxListScrollStateNone];
}

- (void)sendScrollEvent:(NSString *)name
              scrollTop:(float)top
              scollleft:(float)left
           scrollHeight:(float)height
            scrollWidth:(float)width
                 deltaX:(float)x
                 deltaY:(float)y
                  state:(LynxListScrollState)state {
  NSMutableDictionary *detail = [[NSMutableDictionary alloc] init];
  detail[@"deltaX"] = [NSNumber numberWithFloat:x];
  detail[@"deltaY"] = [NSNumber numberWithFloat:y];
  detail[@"scrollLeft"] = [NSNumber numberWithFloat:left];
  detail[@"scrollTop"] = [NSNumber numberWithFloat:top];
  detail[@"scrollHeight"] = [NSNumber numberWithFloat:height];
  detail[@"scrollWidth"] = [NSNumber numberWithFloat:width];
  detail[@"state"] = @(state);
  NSArray *attachedCellsArray = [self attachedCellsArray];
  if (attachedCellsArray) {
    detail[@"attachedCells"] = attachedCellsArray;
  }

  LynxCustomEvent *scrollEventInfo = [[LynxDetailEvent alloc] initWithName:name
                                                                targetSign:_lynxUI.sign
                                                                    detail:detail];

  [_emitter sendCustomEvent:scrollEventInfo];
}

- (BOOL)isUpper:(NSInteger)status {
  return (status & BORDER_UPPER) != 0;
}

- (BOOL)isLower:(NSInteger)status {
  return (status & BORDER_LOWER) != 0;
}

- (BOOL)isReadyToUpper:(NSInteger)status {
  return (status & BORDER_READY_TO_UPPER) != 0;
}

- (BOOL)isReadyToLower:(NSInteger)status {
  return (status & BORDER_READY_TO_LOWER) != 0;
}

@end

@interface LynxListScrollEventEmitterHelper ()
@property(nonatomic, assign) CGPoint lastContentOffset;
@property(nonatomic, assign) CGSize lastContentSize;
@property(nonatomic, weak) LynxListScrollEventEmitter *emitter;
@property(nonatomic, assign) NSTimeInterval lastUpdateTime;
@property(nonatomic, strong) NSString *lastEvent;
@end

@implementation LynxListScrollEventEmitterHelper

- (instancetype)initWithEmitter:(LynxListScrollEventEmitter *)emitter {
  if (self = [super init]) {
    self.lastContentOffset = CGPointZero;
    self.lastContentSize = CGSizeZero;
    self.scrollPosition = LynxListScrollPositionInit;
    self.scrollState = LynxListScrollStateNone;
    self.emitter = emitter;
    self.horizontalLayout = emitter.horizontalLayout;
  }
  return self;
}

- (NSString *)fetchScrollEvent:(UIScrollView *)scrollView {
  LynxListScrollPosition currentPosition = [self position:scrollView];

  self.lastContentOffset = scrollView.contentOffset;
  self.lastContentSize = scrollView.contentSize;

  NSTimeInterval previousUpdateTime = self.lastUpdateTime;
  self.lastUpdateTime = CFAbsoluteTimeGetCurrent();

  NSString *event = nil;
  switch (currentPosition) {
    case LynxListScrollPositionTop:
      event = self.scrollPosition == LynxListScrollPositionTop ? LynxEventScroll
                                                               : LynxEventScrollToUpper;
      break;
    case LynxListScrollPositionBottom:
      event = self.scrollPosition == LynxListScrollPositionBottom ? LynxEventScroll
                                                                  : LynxEventScrollToLower;
      break;
    case LynxListScrollPositionMid:
      event = LynxEventScroll;
      break;
    default:
      break;
  }

  if (self.lastEvent == event && event == LynxEventScroll &&
      self.lastUpdateTime - previousUpdateTime < self.emitter.scrollEventThrottle / 1000.0) {
    event = nil;
  }

  self.scrollPosition = currentPosition;
  self.lastEvent = event;

  return event;
}

- (LynxListScrollPosition)position:(UIScrollView *)scrollView {
  CGFloat top = scrollView.contentOffset.y + scrollView.contentInset.top;
  CGFloat bottom = scrollView.contentSize.height - scrollView.bounds.size.height -
                   scrollView.contentOffset.y + scrollView.contentInset.bottom;

  CGFloat left = scrollView.contentOffset.x + scrollView.contentInset.left;
  CGFloat right = scrollView.contentSize.width - scrollView.bounds.size.width -
                  scrollView.contentOffset.x + scrollView.contentInset.right;

  BOOL reachTop = [self.emitter reachUpperThresholdByUserDefinedCondition] ||
                  top <= self.emitter.scrollUpperThreshold;
  BOOL reachBottom = [self.emitter reachLowerThresholdByUserDefinedCondition] ||
                     bottom <= self.emitter.scrollLowerThreshold;
  BOOL reachLeft = [self.emitter reachUpperThresholdByUserDefinedCondition] ||
                   left <= self.emitter.scrollUpperThreshold;
  BOOL reachRight = [self.emitter reachLowerThresholdByUserDefinedCondition] ||
                    right <= self.emitter.scrollLowerThreshold;

  if (_horizontalLayout) {
    if (reachLeft && reachRight) {
      return scrollView.contentOffset.x / scrollView.contentSize.width >
                     self.lastContentOffset.x / self.lastContentSize.width
                 ? LynxListScrollPositionBottom
                 : LynxListScrollPositionTop;
    } else if (reachLeft) {
      return LynxListScrollPositionTop;
    } else if (reachRight) {
      return LynxListScrollPositionBottom;
    } else {
      return LynxListScrollPositionMid;
    }
  } else {
    if (reachTop && reachBottom) {
      return scrollView.contentOffset.y / scrollView.contentSize.height >
                     self.lastContentOffset.y / self.lastContentSize.height
                 ? LynxListScrollPositionBottom
                 : LynxListScrollPositionTop;
    } else if (reachTop) {
      return LynxListScrollPositionTop;
    } else if (reachBottom) {
      return LynxListScrollPositionBottom;
    } else {
      return LynxListScrollPositionMid;
    }
  }
}

@end
