// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxUI+Internal.h>
#import <Lynx/LynxUIComponent.h>
#import <XCTest/XCTest.h>

#import "LynxListItemHelper.h"

@interface LynxListItemHelperTestOwner : NSObject <LynxListItemHelperOwner>
@property(nonatomic, strong) UIScrollView *listScrollView;
@property(nonatomic, assign, getter=isVertical) BOOL vertical;
@property(nonatomic, copy, nullable) NSArray<NSString *> *itemKeys;
@end

@implementation LynxListItemHelperTestOwner

- (UIScrollView *)scrollViewForListItemHelper {
  return self.listScrollView;
}

- (BOOL)isVerticalForListItemHelper {
  return self.vertical;
}

- (NSArray<NSString *> *)itemKeysForListItemHelper {
  return self.itemKeys;
}

@end

@interface LynxListItemHelperTestWrapper : UIView <LynxListItemWrapper>
@property(nonatomic, weak, nullable) LynxUIComponent *holdingUI;
@end

@implementation LynxListItemHelperTestWrapper
@end

@interface LynxListItemHelperTestBackgroundManager : LynxBackgroundManager
@property(nonatomic, strong) LynxBorderLayer *testBorderLayer;
@property(nonatomic, strong) LynxBackgroundSubBackgroundLayer *testBackgroundLayer;
@end

@implementation LynxListItemHelperTestBackgroundManager

- (LynxBorderLayer *)borderLayer {
  return self.testBorderLayer;
}

- (LynxBackgroundSubBackgroundLayer *)backgroundLayer {
  return self.testBackgroundLayer;
}

@end

@interface LynxListItemHelperTestComponent : LynxUIComponent
@property(nonatomic, assign) BOOL acceptsHitTest;
@property(nonatomic, strong) LynxListItemHelperTestBackgroundManager *testBackgroundManager;
@property(nonatomic, strong) id lastLayerValue;
@property(nonatomic, copy) NSString *lastLayerKeyPath;
@property(nonatomic, assign) BOOL lastLayerUpdateIncludedAllLayers;
@end

@implementation LynxListItemHelperTestComponent

- (BOOL)containsPoint:(CGPoint)point inHitTestFrame:(CGRect)frame {
  return self.acceptsHitTest && CGRectContainsPoint(frame, point);
}

- (id<LynxEventTarget>)hitTest:(CGPoint)point withEvent:(UIEvent *)event {
  return self.acceptsHitTest ? self : nil;
}

- (LynxBackgroundManager *)backgroundManager {
  return self.testBackgroundManager ?: [super backgroundManager];
}

- (void)setLayerValue:(id)value forKeyPath:(NSString *)keyPath forAllLayers:(BOOL)forAllLayers {
  self.lastLayerValue = value;
  self.lastLayerKeyPath = keyPath;
  self.lastLayerUpdateIncludedAllLayers = forAllLayers;
  [super setLayerValue:value forKeyPath:keyPath forAllLayers:forAllLayers];
}

@end

@interface LynxListItemHelperUnitTest : XCTestCase
@property(nonatomic, strong) LynxListItemHelperTestOwner *owner;
@property(nonatomic, strong) LynxListItemHelper *helper;
@property(nonatomic, strong) NSMutableArray<LynxUIComponent *> *retainedComponents;
@end

@implementation LynxListItemHelperUnitTest

- (void)setUp {
  [super setUp];
  self.owner = [[LynxListItemHelperTestOwner alloc] init];
  self.owner.listScrollView = [[UIScrollView alloc] initWithFrame:CGRectMake(0, 0, 100, 100)];
  self.owner.listScrollView.contentSize = CGSizeMake(500, 500);
  self.owner.vertical = YES;
  self.helper = [[LynxListItemHelper alloc] initWithOwner:self.owner];
  self.retainedComponents = [NSMutableArray array];
}

- (LynxListItemHelperTestWrapper *)addWrapperWithItemKey:(NSString *)itemKey
                                              idSelector:(NSString *)idSelector
                                                   frame:(CGRect)frame
                                               zPosition:(CGFloat)zPosition
                                          acceptsHitTest:(BOOL)acceptsHitTest {
  LynxListItemHelperTestComponent *component = [[LynxListItemHelperTestComponent alloc] init];
  component.itemKey = itemKey;
  component.idSelector = idSelector;
  component.acceptsHitTest = acceptsHitTest;
  component.view.frame = (CGRect){CGPointZero, frame.size};
  [self.retainedComponents addObject:component];

  LynxListItemHelperTestWrapper *wrapper =
      [[LynxListItemHelperTestWrapper alloc] initWithFrame:frame];
  wrapper.holdingUI = component;
  wrapper.layer.zPosition = zPosition;
  [wrapper addSubview:component.view];
  [self.owner.listScrollView addSubview:wrapper];
  return wrapper;
}

- (void)testAddsListItemViewAndAlignsFrames {
  LynxListItemHelperTestComponent *component = [[LynxListItemHelperTestComponent alloc] init];
  component.testBackgroundManager =
      [[LynxListItemHelperTestBackgroundManager alloc] initWithUI:component];
  component.testBackgroundManager.testBorderLayer = [LynxBorderLayer layer];
  component.testBackgroundManager.testBackgroundLayer = [LynxBackgroundSubBackgroundLayer layer];
  [component.view.layer addSublayer:component.testBackgroundManager.testBackgroundLayer];
  [component.view.layer addSublayer:component.testBackgroundManager.testBorderLayer];
  [self.retainedComponents addObject:component];
  LynxListItemHelperTestWrapper *wrapper = [[LynxListItemHelperTestWrapper alloc] init];
  wrapper.holdingUI = component;
  CGRect itemFrame = CGRectMake(12, 24, 80, 40);

  component.frame = itemFrame;
  [self.helper attachComponent:component toWrapper:wrapper];

  XCTAssertTrue(CGRectEqualToRect(wrapper.frame, itemFrame));
  XCTAssertTrue(CGRectEqualToRect(component.view.frame, CGRectMake(0, 0, 80, 40)));
  XCTAssertEqual(component.view.superview, wrapper);
  XCTAssertEqual(component.testBackgroundManager.testBorderLayer.superlayer, wrapper.layer);
  XCTAssertEqual(component.testBackgroundManager.testBackgroundLayer.superlayer, wrapper.layer);

  NSArray<CALayer *> *wrapperLayers = wrapper.layer.sublayers;
  XCTAssertTrue(
      [wrapperLayers indexOfObjectIdenticalTo:component.testBackgroundManager.testBackgroundLayer] <
      [wrapperLayers indexOfObjectIdenticalTo:component.view.layer]);
  XCTAssertTrue([wrapperLayers indexOfObjectIdenticalTo:component.view.layer] <
                [wrapperLayers
                    indexOfObjectIdenticalTo:component.testBackgroundManager.testBorderLayer]);

  component.frame = CGRectMake(24, 36, 60, 30);
  [self.helper updateLayoutForComponent:component inWrapper:wrapper];

  XCTAssertTrue(CGRectEqualToRect(wrapper.frame, component.frame));
  XCTAssertTrue(CGRectEqualToRect(component.view.frame, CGRectMake(0, 0, 60, 30)));
  XCTAssertEqualObjects(component.lastLayerKeyPath, @"frame");
  XCTAssertTrue(component.lastLayerUpdateIncludedAllLayers);
  XCTAssertTrue(CGRectEqualToRect([component.lastLayerValue CGRectValue], component.view.frame));
}

- (void)testLayoutUsesTheComponentCurrentlyHeldByTheWrapperForLayerUpdates {
  LynxListItemHelperTestComponent *holdingComponent =
      [[LynxListItemHelperTestComponent alloc] init];
  holdingComponent.testBackgroundManager =
      [[LynxListItemHelperTestBackgroundManager alloc] initWithUI:holdingComponent];
  LynxListItemHelperTestComponent *layoutComponent = [[LynxListItemHelperTestComponent alloc] init];
  layoutComponent.frame = CGRectMake(12, 24, 80, 40);
  [self.retainedComponents addObjectsFromArray:@[ holdingComponent, layoutComponent ]];

  LynxListItemHelperTestWrapper *wrapper = [[LynxListItemHelperTestWrapper alloc] init];
  wrapper.holdingUI = holdingComponent;

  [self.helper updateLayoutForComponent:layoutComponent inWrapper:wrapper];

  XCTAssertTrue(CGRectEqualToRect(wrapper.frame, layoutComponent.frame));
  XCTAssertEqual(layoutComponent.view.superview, wrapper);
  XCTAssertEqualObjects(holdingComponent.lastLayerKeyPath, @"frame");
  XCTAssertNil(layoutComponent.lastLayerKeyPath);
}

- (void)testItemKeyLookupUsesFirstMatchAndReturnsMinusOneWhenMissing {
  self.owner.itemKeys = @[ @"duplicate", @"duplicate" ];

  XCTAssertEqual([self.helper indexForItemKey:@"duplicate"], 0);
  XCTAssertEqual([self.helper indexForItemKey:@"missing"], -1);
  XCTAssertEqual([self.helper indexForItemKey:nil], -1);
}

- (void)testVisibleItemWrappersAndInfoAreSortedByItemKeys {
  self.owner.itemKeys = @[ @"first", @"second", @"hidden" ];
  self.owner.listScrollView.contentOffset = CGPointMake(0, 50);

  LynxListItemHelperTestWrapper *second = [self addWrapperWithItemKey:@"second"
                                                           idSelector:@"second-id"
                                                                frame:CGRectMake(10, 130, 50, 20)
                                                            zPosition:1
                                                       acceptsHitTest:NO];
  [self.owner.listScrollView addSubview:[[UIView alloc] initWithFrame:CGRectMake(0, 50, 100, 100)]];
  LynxListItemHelperTestWrapper *first = [self addWrapperWithItemKey:@"first"
                                                          idSelector:@"first-id"
                                                               frame:CGRectMake(5, 80, 40, 20)
                                                           zPosition:2
                                                      acceptsHitTest:NO];
  [self addWrapperWithItemKey:@"hidden"
                   idSelector:@"hidden-id"
                        frame:CGRectMake(0, 151, 100, 20)
                    zPosition:3
               acceptsHitTest:NO];

  XCTAssertEqualObjects(self.helper.visibleItemWrappers, (@[ first, second ]));
  XCTAssertEqualObjects(self.helper.visibleItemInfo, (@[
                          @{
                            @"id" : @"first-id",
                            @"position" : @0,
                            @"index" : @0,
                            @"itemKey" : @"first",
                            @"top" : @30,
                            @"bottom" : @50,
                            @"left" : @5,
                            @"right" : @45,
                          },
                          @{
                            @"id" : @"second-id",
                            @"position" : @1,
                            @"index" : @1,
                            @"itemKey" : @"second",
                            @"top" : @80,
                            @"bottom" : @100,
                            @"left" : @10,
                            @"right" : @60,
                          },
                        ]));
}

- (void)testVisibleItemWrappersUseHorizontalViewport {
  self.owner.vertical = NO;
  self.owner.itemKeys = @[ @"visible", @"hidden" ];
  self.owner.listScrollView.contentOffset = CGPointMake(40, 0);

  LynxListItemHelperTestWrapper *visible = [self addWrapperWithItemKey:@"visible"
                                                            idSelector:@"visible-id"
                                                                 frame:CGRectMake(120, 0, 20, 100)
                                                             zPosition:1
                                                        acceptsHitTest:NO];
  [self addWrapperWithItemKey:@"hidden"
                   idSelector:@"hidden-id"
                        frame:CGRectMake(141, 0, 20, 100)
                    zPosition:2
               acceptsHitTest:NO];

  XCTAssertEqualObjects(self.helper.visibleItemWrappers, (@[ visible ]));
}

- (void)testHitTestingUsesHighestZPositionAndPreservesLegacyWrapperLookup {
  UIView *decorativeView = [[UIView alloc] initWithFrame:CGRectMake(0, 0, 50, 50)];
  [self.owner.listScrollView addSubview:decorativeView];
  [self addWrapperWithItemKey:@"lower"
                   idSelector:@"lower-id"
                        frame:CGRectMake(0, 0, 50, 50)
                    zPosition:1
               acceptsHitTest:YES];
  LynxListItemHelperTestWrapper *higher = [self addWrapperWithItemKey:@"higher"
                                                           idSelector:@"higher-id"
                                                                frame:CGRectMake(0, 0, 50, 50)
                                                            zPosition:2
                                                       acceptsHitTest:YES];

  XCTAssertEqual([self.helper findHitTargetAtPoint:CGPointMake(10, 10) withEvent:nil],
                 higher.holdingUI);
  XCTAssertEqual([self.helper firstSubviewAtPoint:CGPointMake(10, 10)], decorativeView);
}

- (void)testHitTestingUsesSubviewOrderForEqualZPositions {
  LynxListItemHelperTestWrapper *first = [self addWrapperWithItemKey:@"first"
                                                          idSelector:@"first-id"
                                                               frame:CGRectMake(0, 0, 50, 50)
                                                           zPosition:1
                                                      acceptsHitTest:YES];
  [self addWrapperWithItemKey:@"second"
                   idSelector:@"second-id"
                        frame:CGRectMake(0, 0, 50, 50)
                    zPosition:1
               acceptsHitTest:YES];

  XCTAssertEqual([self.helper findHitTargetAtPoint:CGPointMake(10, 10) withEvent:nil],
                 first.holdingUI);
}

@end
