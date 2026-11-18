// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#import <Lynx/LynxViewInsets+Internal.h>
#import <XCTest/XCTest.h>

@interface LynxViewInsetsUnitTest : XCTestCase
@end

@implementation LynxViewInsetsUnitTest
- (void)testCategoryUpdateAndExplicitClear {
  LynxInsetEdges* navEdges = [[LynxInsetEdges alloc] initWithLeft:0 top:0 right:0 bottom:24];
  LynxInsetState* nav = [[LynxInsetState alloc] initWithGeometry:navEdges visible:@YES];
  LynxViewInsetsBuilder* builder = [LynxViewInsetsBuilder new];
  [builder setInset:LynxInsetNavigationBars state:nav];
  LynxViewInsets* initial = [builder build];
  [builder setInset:LynxInsetNavigationBars state:nil];
  XCTAssertEqual([builder build].categories.count, 0u);
  LynxInsetState* ime = [[LynxInsetState alloc]
      initWithGeometry:[[LynxInsetEdges alloc] initWithLeft:0 top:0 right:0 bottom:300]
               visible:@YES];
  [builder setInset:LynxInsetIme state:ime];
  LynxViewInsets* next = [initial lynx_mergingInsets:[builder build]];
  XCTAssertEqualObjects(next.categories[LynxInsetNavigationBars], nav);
  XCTAssertNil(initial.categories[LynxInsetIme]);
  LynxInsetState* zero = [[LynxInsetState alloc]
      initWithGeometry:[[LynxInsetEdges alloc] initWithLeft:0 top:0 right:0 bottom:0]
               visible:@NO];
  LynxViewInsets* cleared =
      [next lynx_mergingInsets:[[LynxViewInsets alloc]
                                   initWithCategories:@{LynxInsetNavigationBars : zero}]];
  XCTAssertEqualObjects(cleared.categories[LynxInsetNavigationBars].visible, @NO);
  XCTAssertEqualObjects(cleared.categories[LynxInsetIme], ime);
  XCTAssertEqualObjects(
      [cleared lynx_dictionaryRepresentation],
      [[cleared lynx_mergingInsets:[LynxViewInsets empty]] lynx_dictionaryRepresentation]);
}

- (void)testZeroGeometryCanRemainVisible {
  LynxInsetState* state = [[LynxInsetState alloc]
      initWithGeometry:[[LynxInsetEdges alloc] initWithLeft:0 top:0 right:0 bottom:0]
               visible:@YES];
  LynxViewInsets* value = [[LynxViewInsets alloc] initWithCategories:@{LynxInsetIme : state}];
  NSDictionary* payload = [value lynx_dictionaryRepresentation];
  XCTAssertEqualObjects(payload[@"ime"][@"visible"], @YES);
  XCTAssertEqualObjects(payload[@"ime"][@"geometry"][@"bottom"], @0);
}

- (void)testCopiesCollectionsAndPreservesSignedCoordinates {
  LynxInsetRect* rect = [[LynxInsetRect alloc] initWithX:-10 y:-20 width:30 height:40];
  NSMutableArray* rects = [NSMutableArray arrayWithObject:rect];
  LynxInsetState* state =
      [[LynxInsetState alloc] initWithGeometry:[[LynxInsetRects alloc] initWithRects:rects]
                                       visible:nil];
  NSMutableDictionary* categories = [@{LynxInsetDisplayCutout : state} mutableCopy];
  LynxViewInsets* value = [[LynxViewInsets alloc] initWithCategories:categories];
  [rects removeAllObjects];
  [categories removeAllObjects];
  NSArray* captured =
      [value lynx_dictionaryRepresentation][@"displayCutout"][@"geometry"][@"rects"];
  XCTAssertEqual(captured.count, 1u);
  XCTAssertEqualObjects(captured[0][@"x"], @(-10));
  LynxInsetCorner* corner = [[LynxInsetCorner alloc] initWithCenterX:-20 centerY:10 radius:30];
  XCTAssertEqual(corner.centerX, -20);
}

- (void)testRejectsInvalidGeometryAndFloatingIme {
  XCTAssertThrows([[LynxInsetEdges alloc] initWithLeft:0 top:-1 right:0 bottom:0]);
  XCTAssertThrows([[LynxInsetRect alloc] initWithX:NAN y:0 width:1 height:1]);
  XCTAssertThrows([[LynxInsetCorner alloc] initWithCenterX:0 centerY:0 radius:-1]);
  LynxInsetState* rects =
      [[LynxInsetState alloc] initWithGeometry:[[LynxInsetRects alloc] initWithRects:@[]]
                                       visible:@YES];
  XCTAssertThrows([[LynxViewInsets alloc] initWithCategories:@{LynxInsetIme : rects}]);
}
@end
