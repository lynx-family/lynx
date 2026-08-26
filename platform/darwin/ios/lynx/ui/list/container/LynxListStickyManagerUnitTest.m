// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxScrollEventManager.h>
#import <Lynx/LynxUIComponent.h>
#import <XCTest/XCTest.h>

#import "LynxListStickyManager.h"

@interface LynxListStickyManagerTestEventRecorder : LynxScrollEventManager
@property(nonatomic, strong) NSMutableArray<NSDictionary *> *recordedEvents;
@end

@implementation LynxListStickyManagerTestEventRecorder

- (instancetype)init {
  self = [super init];
  if (self) {
    _recordedEvents = [NSMutableArray array];
  }
  return self;
}

- (void)sendScrollEvent:(NSString *)name
             scrollView:(UIScrollView *)scrollView
                 detail:(NSDictionary *)detail {
  [self.recordedEvents addObject:@{
    @"name" : name,
    @"detail" : detail,
  }];
}

@end

@interface LynxListStickyManagerTestOwner : NSObject <LynxListStickyManagerOwner>
@property(nonatomic, strong) UIScrollView *listScrollView;
@property(nonatomic, assign, getter=isVertical) BOOL vertical;
@property(nonatomic, copy, nullable) NSArray<NSString *> *itemKeys;
@property(nonatomic, strong, nullable) LynxScrollEventManager *eventManager;
@end

@implementation LynxListStickyManagerTestOwner

- (UIScrollView *)scrollViewForListStickyManager {
  return self.listScrollView;
}

- (BOOL)isVerticalForListStickyManager {
  return self.vertical;
}

- (NSArray<NSString *> *)itemKeysForListStickyManager {
  return self.itemKeys;
}

- (LynxScrollEventManager *)eventManagerForListStickyManager {
  return self.eventManager;
}

@end

@interface LynxListStickyManagerUnitTest : XCTestCase

@end

@implementation LynxListStickyManagerUnitTest

- (LynxUIComponent *)stickyComponentWithKey:(NSString *)itemKey
                                      frame:(CGRect)frame
                                     zIndex:(NSInteger)zIndex
                               inScrollView:(UIScrollView *)scrollView {
  LynxUIComponent *component = [[LynxUIComponent alloc] init];
  component.itemKey = itemKey;
  component.zIndex = zIndex;
  [component updateFrame:frame
              withPadding:UIEdgeInsetsZero
                   border:UIEdgeInsetsZero
      withLayoutAnimation:NO];

  UIView *wrapper = [[UIView alloc] initWithFrame:frame];
  wrapper.layer.zPosition = zIndex;
  component.view.frame = (CGRect){CGPointZero, frame.size};
  [wrapper addSubview:component.view];
  [scrollView addSubview:wrapper];
  return component;
}

- (void)testStickyManagerStartClampsBouncePushesOffAndDeduplicatesEvents {
  UIScrollView *scrollView = [[UIScrollView alloc] initWithFrame:CGRectMake(0, 0, 100, 100)];
  scrollView.contentSize = CGSizeMake(100, 400);
  LynxListStickyManagerTestEventRecorder *eventRecorder =
      [[LynxListStickyManagerTestEventRecorder alloc] init];
  LynxListStickyManagerTestOwner *owner = [[LynxListStickyManagerTestOwner alloc] init];
  owner.listScrollView = scrollView;
  owner.vertical = YES;
  owner.itemKeys = @[ @"first", @"next" ];
  owner.eventManager = eventRecorder;

  LynxListStickyManager *manager = [[LynxListStickyManager alloc] initWithOwner:owner];
  manager.enabled = YES;
  manager.offset = 10;
  [manager setStickyStartIndexes:@[ @0, @1 ] endIndexes:nil];
  [manager propsDidUpdate];

  LynxUIComponent *first = [self stickyComponentWithKey:@"first"
                                                  frame:CGRectMake(0, 0, 100, 40)
                                                 zIndex:7
                                           inScrollView:scrollView];
  LynxUIComponent *next = [self stickyComponentWithKey:@"next"
                                                 frame:CGRectMake(0, 70, 100, 40)
                                                zIndex:8
                                          inScrollView:scrollView];
  [manager didAttachComponent:first];
  [manager didAttachComponent:next];

  // The raw bounce offset is -30. list-container behavior clamps it to zero before adding the
  // configured sticky offset, so the first item is positioned at y = 10.
  scrollView.contentOffset = CGPointMake(0, -30);
  XCTAssertEqualWithAccuracy(scrollView.contentOffset.y, -30, 0.001);
  [manager updateStickyItems];
  XCTAssertEqualWithAccuracy(CGRectGetMinY(first.view.superview.frame), 10, 0.001);
  XCTAssertTrue(first.view.superview.layer.zPosition > first.zIndex);
  XCTAssertTrue(CGRectEqualToRect(first.view.frame, CGRectMake(0, 0, 100, 40)));

  [manager updateStickyItems];
  XCTAssertEqualObjects([eventRecorder.recordedEvents valueForKey:@"name"],
                        (@[ LynxEventStickyTop, LynxEventStickyStart ]));
  XCTAssertEqualObjects(eventRecorder.recordedEvents[0][@"detail"], (@{@"top" : @"first"}));
  XCTAssertEqualObjects(eventRecorder.recordedEvents[1][@"detail"], (@{@"start" : @"first"}));

  // Effective offset is 60. The next sticky begins at 70, pushing the 40-point first item back
  // by 30 points instead of letting the two sticky items overlap.
  scrollView.contentOffset = CGPointMake(0, 50);
  [manager updateStickyItems];
  XCTAssertEqualWithAccuracy(CGRectGetMinY(first.view.superview.frame), 30, 0.001);
  XCTAssertEqual(eventRecorder.recordedEvents.count, 2U);
}

- (void)testStickyManagerEndPositionsItemAtViewportBottom {
  UIScrollView *scrollView = [[UIScrollView alloc] initWithFrame:CGRectMake(0, 0, 100, 100)];
  scrollView.contentSize = CGSizeMake(100, 400);
  scrollView.contentOffset = CGPointMake(0, 100);
  LynxListStickyManagerTestEventRecorder *eventRecorder =
      [[LynxListStickyManagerTestEventRecorder alloc] init];
  LynxListStickyManagerTestOwner *owner = [[LynxListStickyManagerTestOwner alloc] init];
  owner.listScrollView = scrollView;
  owner.vertical = YES;
  owner.itemKeys = @[ @"last" ];
  owner.eventManager = eventRecorder;

  LynxListStickyManager *manager = [[LynxListStickyManager alloc] initWithOwner:owner];
  manager.enabled = YES;
  [manager setStickyStartIndexes:nil endIndexes:@[ @0 ]];
  [manager propsDidUpdate];

  LynxUIComponent *last = [self stickyComponentWithKey:@"last"
                                                 frame:CGRectMake(0, 230, 100, 30)
                                                zIndex:5
                                          inScrollView:scrollView];
  [manager didAttachComponent:last];
  [manager updateStickyItems];

  // The viewport bottom is 200, so the 30-point item starts at 170.
  XCTAssertEqualWithAccuracy(CGRectGetMinY(last.view.superview.frame), 170, 0.001);
  XCTAssertTrue(last.view.superview.layer.zPosition > last.zIndex);
  XCTAssertTrue(CGRectEqualToRect(last.view.frame, CGRectMake(0, 0, 100, 30)));

  [manager updateStickyItems];
  XCTAssertEqualObjects([eventRecorder.recordedEvents valueForKey:@"name"],
                        (@[ LynxEventStickyBottom, LynxEventStickyEnd ]));
  XCTAssertEqualObjects(eventRecorder.recordedEvents[0][@"detail"], (@{@"bottom" : @"last"}));
  XCTAssertEqualObjects(eventRecorder.recordedEvents[1][@"detail"], (@{@"end" : @"last"}));
}

- (void)testStickyManagerDiffRemapResetsOldItemAndAttachesNewItem {
  UIScrollView *scrollView = [[UIScrollView alloc] initWithFrame:CGRectMake(0, 0, 100, 100)];
  scrollView.contentSize = CGSizeMake(100, 400);
  scrollView.contentOffset = CGPointMake(0, 20);
  LynxListStickyManagerTestOwner *owner = [[LynxListStickyManagerTestOwner alloc] init];
  owner.listScrollView = scrollView;
  owner.vertical = YES;
  owner.itemKeys = @[ @"first", @"second" ];

  LynxListStickyManager *manager = [[LynxListStickyManager alloc] initWithOwner:owner];
  manager.enabled = YES;
  [manager setStickyStartIndexes:@[ @0 ] endIndexes:nil];
  [manager propsDidUpdate];

  LynxUIComponent *first = [self stickyComponentWithKey:@"first"
                                                  frame:CGRectMake(0, 0, 100, 20)
                                                 zIndex:3
                                           inScrollView:scrollView];
  LynxUIComponent *second = [self stickyComponentWithKey:@"second"
                                                   frame:CGRectMake(0, 0, 100, 20)
                                                  zIndex:4
                                            inScrollView:scrollView];
  [manager didAttachComponent:first];
  [manager updateStickyItems];
  XCTAssertEqualWithAccuracy(CGRectGetMinY(first.view.superview.frame), 20, 0.001);

  // Diff moves "second" to sticky index zero. Updating props removes and resets the old key;
  // attaching the remapped component then makes it the active sticky item.
  owner.itemKeys = @[ @"second", @"first" ];
  [manager propsDidUpdate];
  XCTAssertEqualWithAccuracy(CGRectGetMinY(first.view.superview.frame), 0, 0.001);
  XCTAssertEqualWithAccuracy(first.view.superview.layer.zPosition, first.zIndex, 0.001);

  [manager didAttachComponent:second];
  [manager updateStickyItems];
  XCTAssertEqualWithAccuracy(CGRectGetMinY(second.view.superview.frame), 20, 0.001);
  XCTAssertTrue(second.view.superview.layer.zPosition > second.zIndex);
  XCTAssertEqualWithAccuracy(CGRectGetMinY(first.view.superview.frame), 0, 0.001);

  [manager willDetachComponent:second];
  XCTAssertEqualWithAccuracy(CGRectGetMinY(second.view.superview.frame), 0, 0.001);
  XCTAssertEqualWithAccuracy(second.view.superview.layer.zPosition, second.zIndex, 0.001);
  [manager updateStickyItems];
  XCTAssertEqualWithAccuracy(CGRectGetMinY(second.view.superview.frame), 0, 0.001);
}

@end
