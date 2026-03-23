// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxDisplayListApplier+Internal.h>
#import <Lynx/LynxRenderer.h>
#import <Lynx/LynxRendererContext.h>
#import <Lynx/LynxRendererHost.h>
#import <Lynx/LynxTextLayer.h>
#import <Lynx/LynxTextRenderManager.h>
#import <OCMock/OCMock.h>
#import <XCTest/XCTest.h>
#include "core/renderer/dom/fragment/display_list.h"

using namespace lynx::tasm;

// Define a mock view class that implements the protocol
@interface LynxMockView : UIView <LynxRendererHost>
@property(nonatomic, strong) LynxRenderer *renderer;
@end

@implementation LynxMockView

- (void)setRenderer:(LynxRenderer *)renderer {
  _renderer = renderer;
}

- (LynxRenderer *)createRendererWithSign:(int32_t)sign andContext:(LynxRendererContext *)context {
  self.renderer = [[LynxRenderer alloc] initWithRenderHost:self andSign:sign andContext:context];
  return self.renderer;
}

- (LynxRenderer *)getRenderer {
  return self.renderer;
}

- (UIView *)getView {
  return self;
}

@end

@interface LynxDisplayListApplierUnitTest : XCTestCase
@end

@implementation LynxDisplayListApplierUnitTest

- (void)setUp {
}

- (void)tearDown {
}

- (void)testInitWithView {
  id mockView = OCMProtocolMock(@protocol(LynxRendererHost));
  id mockContext = OCMClassMock([LynxRendererContext class]);
  LynxDisplayListApplier *applier = [[LynxDisplayListApplier alloc] initWithView:mockView
                                                                      andContext:mockContext];
  XCTAssertNotNil(applier);
}

- (void)testApplyDisplayListWithNullInputs {
  id mockView = OCMProtocolMock(@protocol(LynxRendererHost));
  id mockContext = OCMClassMock([LynxRendererContext class]);
  LynxDisplayListApplier *applier = [[LynxDisplayListApplier alloc] initWithView:mockView
                                                                      andContext:mockContext];

  [applier applyDisplayList:nullptr];
  // Should not crash

  LynxDisplayListApplier *nullViewApplier =
      [[LynxDisplayListApplier alloc] initWithView:nil andContext:mockContext];
  DisplayList list;
  [nullViewApplier applyDisplayList:&list];
  // Should not crash
}

- (void)testProcessContentOperations {
  // Setup Mock View and Layer
  // We use LynxMockView to ensure it responds to 'getRenderer' (from protocol) and
  // 'layer'/'subviews' (from UIView)
  id mockUIView = OCMClassMock([LynxMockView class]);
  id mockLayer = OCMClassMock([CALayer class]);
  id mockRenderer = OCMClassMock([LynxRenderer class]);
  id mockContext = OCMClassMock([LynxRendererContext class]);

  // Stub properties
  [[[mockUIView stub] andReturn:mockLayer] layer];
  [[[mockUIView stub] andReturn:mockRenderer] getRenderer];

  // Stub subviews for kDrawView
  id mockSubView = OCMClassMock([UIView class]);
  id mockSubLayer = OCMClassMock([CALayer class]);
  [[[mockSubView stub] andReturn:mockSubLayer] layer];
  [[[mockUIView stub] andReturn:@[ mockSubView ]] subviews];

  // Stub Renderer Sign
  // [[_view getRenderer] getSign]
  [[[mockRenderer stub] andReturnValue:OCMOCK_VALUE((int32_t)1)] getSign];

  LynxDisplayListApplier *applier = [[LynxDisplayListApplier alloc] initWithView:mockUIView
                                                                      andContext:mockContext];

  // Construct DisplayList
  DisplayList list;

  // 1. kBegin
  // int_count=1 (sign to check), float_count=4 (x, y, w, h)
  // Logic: if (sign_arg != renderer_sign) -> record_offset = true
  // Renderer sign is 1. We pass 2. 2 != 1 -> true.
  // Offsets updated by x, y.
  list.AddOperation(DisplayListOpType::kBegin, 2, 10.0f, 20.0f, 100.0f, 100.0f);

  // 2. kRecordBox
  // float_count=4 (x, y, w, h)
  list.AddOperation(DisplayListOpType::kRecordBox, 0.0f, 0.0f, 50.0f, 50.0f);

  // 3. kFill
  // int_count=2 (argb, clip_index)
  // argb = 0xFF0000FF (Red=0, Green=0, Blue=255, Alpha=1.0)
  // Note: ARGB format in code:
  // a = ((argb >> 24) & 0xFF) / 255.0;
  // r = ((argb >> 16) & 0xFF) / 255.0;
  // ...
  // So 0xFF0000FF -> A=FF, R=00, G=00, B=FF.
  list.AddOperation(DisplayListOpType::kFill, (int32_t)0xFF0000FF, 0);

  // Expectation: A layer should be added at the bottom of the host layer tree.
  [[mockLayer expect] insertSublayer:[OCMArg any] atIndex:0];

  // 4. kDrawView
  // int_count=1 (view_id)
  // Should update _refLayer to mockSubView.layer
  list.AddOperation(DisplayListOpType::kDrawView, 123);

  // 5. Another kRecordBox and kFill to test _refLayer logic
  // This time _refLayer should be mockSubView.layer
  list.AddOperation(DisplayListOpType::kRecordBox, 5.0f, 5.0f, 10.0f, 10.0f);
  list.AddOperation(DisplayListOpType::kFill, (int32_t)0xFF00FF00, 1);

  // Expectation: insertSublayer:newLayer above:mockSubLayer
  [[mockLayer expect] insertSublayer:[OCMArg any] above:mockSubLayer];

  // 6. kEnd
  list.AddOperation(DisplayListOpType::kEnd);

  [applier applyDisplayList:&list];

  [mockLayer verify];
}

- (void)testProcessContentOperationsWithTODOs {
  id mockUIView = OCMClassMock([LynxMockView class]);
  id mockContext = OCMClassMock([LynxRendererContext class]);
  LynxDisplayListApplier *applier = [[LynxDisplayListApplier alloc] initWithView:mockUIView
                                                                      andContext:mockContext];

  DisplayList list;
  // kText, kImage, kBorder are currently empty but should be handled gracefully
  list.AddOperation(DisplayListOpType::kText);
  list.AddOperation(DisplayListOpType::kImage);
  list.AddOperation(DisplayListOpType::kBorder);

  [applier applyDisplayList:&list];
  // Verify no crash
}

- (void)testDrawViewWithIntCountNotOne {
  id mockUIView = OCMClassMock([LynxMockView class]);
  id mockSubView = OCMClassMock([UIView class]);
  id mockContext = OCMClassMock([LynxRendererContext class]);
  [[[mockUIView stub] andReturn:@[ mockSubView ]] subviews];

  LynxDisplayListApplier *applier = [[LynxDisplayListApplier alloc] initWithView:mockUIView
                                                                      andContext:mockContext];

  DisplayList list;
  // kDrawView with int_count=0 -> should NOT consume extra int
  list.AddOperation(DisplayListOpType::kDrawView, 0);

  [applier applyDisplayList:&list];
  // Verify execution completes (index advanced correctly)
}

- (void)testProcessContentOperationsWithUnknownOp {
  id mockUIView = OCMClassMock([LynxMockView class]);
  id mockContext = OCMClassMock([LynxRendererContext class]);
  LynxDisplayListApplier *applier = [[LynxDisplayListApplier alloc] initWithView:mockUIView
                                                                      andContext:mockContext];

  DisplayList list;
  // Add an unknown operation type to hit the default case
  // Assuming 9999 is not a valid op type
  list.AddOperation(static_cast<DisplayListOpType>(9999));

  [applier applyDisplayList:&list];
  // Verify no crash
}

- (void)testProcessContentOperationsWithLinearGradientKeepsFollowingOpsAligned {
  LynxMockView *view = [[LynxMockView alloc] initWithFrame:CGRectMake(0, 0, 200, 200)];
  LynxDisplayListApplier *applier = [[LynxDisplayListApplier alloc] initWithView:view
                                                                      andContext:nil];

  DisplayList list;
  list.AddOperation(DisplayListOpType::kRecordBox, 0.0f, 0.0f, 100.0f, 100.0f);
  list.AddOperation(DisplayListOpType::kRecordBox, 10.0f, 12.0f, 80.0f, 60.0f);

  lynx::base::Vector<uint32_t> colors;
  colors.push_back(0xFFFF0000);
  colors.push_back(0xFF0000FF);
  lynx::base::Vector<float> stops;
  stops.push_back(0.0f);
  stops.push_back(1.0f);
  list.AddLinearGradient(90.0f, colors, stops, 0, 1, 1, 1);

  list.AddOperation(DisplayListOpType::kRecordBox, 5.0f, 6.0f, 10.0f, 12.0f);
  list.AddOperation(DisplayListOpType::kFill, (int32_t)0xFF00FF00, 2);

  [applier applyDisplayList:&list];

  XCTAssertEqual(view.layer.sublayers.count, 2u);
  CALayer *gradientLayer = view.layer.sublayers[0];
  CALayer *fillLayer = view.layer.sublayers[1];

  XCTAssertEqualWithAccuracy(gradientLayer.frame.origin.x, 10.0f, 0.001f);
  XCTAssertEqualWithAccuracy(gradientLayer.frame.origin.y, 12.0f, 0.001f);
  XCTAssertEqualWithAccuracy(gradientLayer.frame.size.width, 80.0f, 0.001f);
  XCTAssertEqualWithAccuracy(gradientLayer.frame.size.height, 60.0f, 0.001f);
  XCTAssertTrue([gradientLayer isKindOfClass:[CAReplicatorLayer class]]);
  XCTAssertNotNil(gradientLayer.sublayers.firstObject);

  XCTAssertTrue(CGRectEqualToRect(fillLayer.frame, CGRectMake(5, 6, 10, 12)));
}

- (void)testProcessContentOperationsWithLinearGradientInvalidIndexKeepsFollowingOpsAligned {
  LynxMockView *view = [[LynxMockView alloc] initWithFrame:CGRectMake(0, 0, 200, 200)];
  LynxDisplayListApplier *applier = [[LynxDisplayListApplier alloc] initWithView:view
                                                                      andContext:nil];

  DisplayList list;
  list.AddOperation(DisplayListOpType::kRecordBox, 0.0f, 0.0f, 100.0f, 100.0f);

  lynx::base::Vector<uint32_t> colors;
  colors.push_back(0xFFFF0000);
  colors.push_back(0xFF0000FF);
  lynx::base::Vector<float> stops;
  stops.push_back(0.0f);
  stops.push_back(1.0f);
  list.AddLinearGradient(90.0f, colors, stops, 0, 99, 1, 1);

  list.AddOperation(DisplayListOpType::kRecordBox, 5.0f, 6.0f, 10.0f, 12.0f);
  list.AddOperation(DisplayListOpType::kFill, (int32_t)0xFF00FF00, 1);

  [applier applyDisplayList:&list];

  XCTAssertEqual(view.layer.sublayers.count, 1u);
  CALayer *fillLayer = view.layer.sublayers.firstObject;
  XCTAssertTrue(CGRectEqualToRect(fillLayer.frame, CGRectMake(5, 6, 10, 12)));
}

- (void)testProcessContentOperationsWithLinearGradientRoundedClipAppliesMask {
  LynxMockView *view = [[LynxMockView alloc] initWithFrame:CGRectMake(0, 0, 200, 200)];
  LynxDisplayListApplier *applier = [[LynxDisplayListApplier alloc] initWithView:view
                                                                      andContext:nil];

  DisplayList list;
  list.AddOperation(DisplayListOpType::kRecordBox, 0.0f, 0.0f, 100.0f, 100.0f);
  list.AddOperation(DisplayListOpType::kRecordBox, 10.0f, 12.0f, 80.0f, 60.0f, 4.0f, 4.0f, 8.0f,
                    8.0f, 12.0f, 12.0f, 16.0f, 16.0f);

  lynx::base::Vector<uint32_t> colors;
  colors.push_back(0xFFFF0000);
  colors.push_back(0xFF0000FF);
  lynx::base::Vector<float> stops;
  stops.push_back(0.0f);
  stops.push_back(1.0f);
  list.AddLinearGradient(90.0f, colors, stops, 0, 1, 1, 1);

  [applier applyDisplayList:&list];

  XCTAssertEqual(view.layer.sublayers.count, 1u);
  CALayer *gradientLayer = view.layer.sublayers.firstObject;
  XCTAssertNotNil(gradientLayer.mask);
  XCTAssertTrue([gradientLayer.mask isKindOfClass:[CAShapeLayer class]]);
  XCTAssertTrue(((CAShapeLayer *)gradientLayer.mask).path != nil);
}

- (void)testProcessContentOperationsWithClipRectPartial {
  // Clip rect not equal to view bounds, should use mask
  id mockUIView = OCMClassMock([LynxMockView class]);
  id mockLayer = OCMClassMock([CALayer class]);
  id mockContext = OCMClassMock([LynxRendererContext class]);
  [[[mockUIView stub] andReturn:mockLayer] layer];
  // maskLayer.frame is set to _view.bounds
  [[[mockUIView stub] andReturnValue:OCMOCK_VALUE(CGRectMake(0, 0, 200, 200))] bounds];

  LynxDisplayListApplier *applier = [[LynxDisplayListApplier alloc] initWithView:mockUIView
                                                                      andContext:mockContext];

  DisplayList list;
  // kClipRect with standard 4 floats (partial rect)
  list.AddOperation(DisplayListOpType::kClipRect, 10.0f, 10.0f, 100.0f, 100.0f);

  // First call: clear previous state; Second call: set new mask
  [[mockLayer expect] setMask:nil];
  [[mockLayer expect] setMask:[OCMArg checkWithBlock:^BOOL(CAShapeLayer *mask) {
                        return [mask isKindOfClass:[CAShapeLayer class]] &&
                               CGRectEqualToRect(mask.frame, CGRectMake(0, 0, 200, 200));
                      }]];

  [applier applyDisplayList:&list];
  [mockLayer verify];
}

- (void)testProcessContentOperationsWithClipRectFullView {
  // Clip rect equals view bounds, should use masksToBounds
  id mockUIView = OCMClassMock([LynxMockView class]);
  id mockLayer = OCMClassMock([CALayer class]);
  id mockContext = OCMClassMock([LynxRendererContext class]);
  [[[mockUIView stub] andReturn:mockLayer] layer];
  [[[mockUIView stub] andReturnValue:OCMOCK_VALUE(CGRectMake(0, 0, 100, 100))] bounds];

  LynxDisplayListApplier *applier = [[LynxDisplayListApplier alloc] initWithView:mockUIView
                                                                      andContext:mockContext];

  DisplayList list;
  // kClipRect with standard 4 floats (full view rect)
  list.AddOperation(DisplayListOpType::kClipRect, 0.0f, 0.0f, 100.0f, 100.0f);

  [[mockLayer expect] setMasksToBounds:YES];

  [applier applyDisplayList:&list];
  [mockLayer verify];
}

- (void)testProcessContentOperationsWithClipRectRoundedUniform {
  // Rounded rect with full view and uniform radius, should use cornerRadius
  id mockUIView = OCMClassMock([LynxMockView class]);
  id mockLayer = OCMClassMock([CALayer class]);
  id mockContext = OCMClassMock([LynxRendererContext class]);
  [[[mockUIView stub] andReturn:mockLayer] layer];
  [[[mockUIView stub] andReturnValue:OCMOCK_VALUE(CGRectMake(0, 0, 100, 100))] bounds];

  LynxDisplayListApplier *applier = [[LynxDisplayListApplier alloc] initWithView:mockUIView
                                                                      andContext:mockContext];

  DisplayList list;
  // kClipRect with 12 floats (full view + uniform radii)
  list.AddOperation(DisplayListOpType::kClipRect, 0.0f, 0.0f, 100.0f, 100.0f,  // rect
                    5.0f, 5.0f, 5.0f, 5.0f, 5.0f, 5.0f, 5.0f, 5.0f);           // uniform radii

  [[mockLayer expect] setCornerRadius:5.0f];
  [[mockLayer expect] setMasksToBounds:YES];

  [applier applyDisplayList:&list];
  [mockLayer verify];
}

- (void)testProcessContentOperationsWithClipRectRoundedPartial {
  // Rounded rect with non-uniform radii should use mask (not cornerRadius optimization)
  id mockUIView = OCMClassMock([LynxMockView class]);
  id mockLayer = OCMClassMock([CALayer class]);
  id mockContext = OCMClassMock([LynxRendererContext class]);
  [[[mockUIView stub] andReturn:mockLayer] layer];
  // maskLayer.frame is set to _view.bounds
  [[[mockUIView stub] andReturnValue:OCMOCK_VALUE(CGRectMake(0, 0, 100, 100))] bounds];

  LynxDisplayListApplier *applier = [[LynxDisplayListApplier alloc] initWithView:mockUIView
                                                                      andContext:mockContext];

  DisplayList list;
  // kClipRect with 12 floats (full view + non-uniform radii)
  list.AddOperation(DisplayListOpType::kClipRect, 0.0f, 0.0f, 100.0f, 100.0f,  // rect
                    5.0f, 5.0f, 10.0f, 10.0f, 2.0f, 2.0f, 8.0f, 8.0f);         // non-uniform radii

  // First call: clear previous state; Second call: set new mask
  [[mockLayer expect] setMask:nil];
  [[mockLayer expect] setMask:[OCMArg checkWithBlock:^BOOL(CAShapeLayer *mask) {
                        return [mask isKindOfClass:[CAShapeLayer class]] &&
                               CGRectEqualToRect(mask.frame, CGRectMake(0, 0, 100, 100)) &&
                               mask.path != nil;
                      }]];

  [applier applyDisplayList:&list];
  [mockLayer verify];
}

- (void)testResetClearsClip {
  // Verify reset clears mask and cornerRadius
  id mockUIView = OCMClassMock([LynxMockView class]);
  id mockLayer = OCMClassMock([CALayer class]);
  id mockContext = OCMClassMock([LynxRendererContext class]);
  [[[mockUIView stub] andReturn:mockLayer] layer];

  LynxDisplayListApplier *applier = [[LynxDisplayListApplier alloc] initWithView:mockUIView
                                                                      andContext:mockContext];

  // First draw with clip - kClipRect will clear state first, then set mask
  DisplayList list1;
  list1.AddOperation(DisplayListOpType::kClipRect, 10.0f, 10.0f, 100.0f, 100.0f);
  // Expect setMask:nil for clearing, then setMask:[OCMArg any] for setting
  [[mockLayer expect] setMask:nil];
  [[mockLayer expect] setMask:[OCMArg any]];
  [applier applyDisplayList:&list1];

  // Verify mask/cornerRadius/masksToBounds is cleared on next applyDisplayList (via reset)
  [[mockLayer expect] setMask:nil];
  [[mockLayer expect] setCornerRadius:0.0f];
  [[mockLayer expect] setMasksToBounds:NO];

  DisplayList list2;
  [applier applyDisplayList:&list2];
  [mockLayer verify];
}

- (void)testProcessContentOperationsWithRecordBoxRadii {
  id mockUIView = OCMClassMock([LynxMockView class]);
  id mockContext = OCMClassMock([LynxRendererContext class]);
  LynxDisplayListApplier *applier = [[LynxDisplayListApplier alloc] initWithView:mockUIView
                                                                      andContext:mockContext];

  DisplayList list;
  // kRecordBox with radii
  // x, y, w, h, then 8 radii
  list.AddOperation(DisplayListOpType::kRecordBox, 0.0f, 0.0f, 50.0f, 50.0f, 2.0f, 2.0f, 2.0f, 2.0f,
                    2.0f, 2.0f, 2.0f, 2.0f);

  [applier applyDisplayList:&list];
}

- (void)testFillAppliesUniformCornerRadius {
  LynxMockView *view = [[LynxMockView alloc] initWithFrame:CGRectMake(0, 0, 100, 100)];
  LynxDisplayListApplier *applier = [[LynxDisplayListApplier alloc] initWithView:view
                                                                      andContext:nil];

  DisplayList list;
  list.AddOperation(DisplayListOpType::kRecordBox, 10.0f, 12.0f, 40.0f, 50.0f, 6.0f, 6.0f, 6.0f,
                    6.0f, 6.0f, 6.0f, 6.0f, 6.0f);
  list.AddOperation(DisplayListOpType::kFill, (int32_t)0xFFFF0000, 0);

  [applier applyDisplayList:&list];

  CALayer *layer = view.layer.sublayers.firstObject;
  XCTAssertNotNil(layer);
  XCTAssertEqualWithAccuracy(layer.cornerRadius, 6.0f, 0.001f);
  XCTAssertTrue(layer.masksToBounds);
}

- (void)testFillAppliesNonUniformCornerRadiusMask {
  LynxMockView *view = [[LynxMockView alloc] initWithFrame:CGRectMake(0, 0, 100, 100)];
  LynxDisplayListApplier *applier = [[LynxDisplayListApplier alloc] initWithView:view
                                                                      andContext:nil];

  DisplayList list;
  list.AddOperation(DisplayListOpType::kRecordBox, 10.0f, 12.0f, 40.0f, 50.0f, 2.0f, 2.0f, 6.0f,
                    6.0f, 10.0f, 10.0f, 14.0f, 14.0f);
  list.AddOperation(DisplayListOpType::kFill, (int32_t)0xFFFF0000, 0);

  [applier applyDisplayList:&list];

  CALayer *layer = view.layer.sublayers.firstObject;
  XCTAssertNotNil(layer);
  XCTAssertEqualWithAccuracy(layer.cornerRadius, 0.0f, 0.001f);
  XCTAssertFalse(layer.masksToBounds);
  XCTAssertNotNil(layer.mask);
  XCTAssertTrue([layer.mask isKindOfClass:[CAShapeLayer class]]);
  XCTAssertTrue(((CAShapeLayer *)layer.mask).path != nil);
}

- (void)testBeginWithMatchingSign {
  id mockUIView = OCMClassMock([LynxMockView class]);
  id mockRenderer = OCMClassMock([LynxRenderer class]);
  id mockLayer = OCMClassMock([CALayer class]);
  id mockSuperLayer = OCMClassMock([CALayer class]);
  id mockContext = OCMClassMock([LynxRendererContext class]);

  [[[mockUIView stub] andReturn:mockRenderer] getRenderer];
  [[[mockUIView stub] andReturn:mockLayer] layer];
  [[[mockLayer stub] andReturn:mockSuperLayer] superlayer];

  // Renderer sign is 1
  [[[mockRenderer stub] andReturnValue:OCMOCK_VALUE((int32_t)1)] getSign];

  LynxDisplayListApplier *applier = [[LynxDisplayListApplier alloc] initWithView:mockUIView
                                                                      andContext:mockContext];

  DisplayList list;

  // kBegin with sign 1 (matches renderer)
  // x=10, y=20
  list.AddOperation(DisplayListOpType::kBegin, 1, 10.0f, 20.0f, 100.0f, 100.0f);

  // kRecordBox at 0,0
  list.AddOperation(DisplayListOpType::kRecordBox, 0.0f, 0.0f, 50.0f, 50.0f);

  // kFill
  list.AddOperation(DisplayListOpType::kFill, (int32_t)0xFFFFFFFF, 0);

  // Expectation:
  // Since signs match (1 == 1), record_offset is false.
  // Offsets (10, 20) should NOT be added.
  // Rect should be (0, 0, 50, 50) NOT (10, 20, 50, 50)

  [[mockSuperLayer expect] insertSublayer:[OCMArg checkWithBlock:^BOOL(CALayer *layer) {
                             CGRect frame = layer.frame;
                             // Verify offset was NOT applied
                             return (frame.origin.x == 0.0f && frame.origin.y == 0.0f);
                           }]
                                    below:mockLayer];

  [applier applyDisplayList:&list];

  [mockLayer verify];
  [mockSuperLayer verify];
}

- (void)testNestedBeginEnd {
  id mockUIView = OCMClassMock([LynxMockView class]);
  id mockRenderer = OCMClassMock([LynxRenderer class]);
  id mockLayer = OCMClassMock([CALayer class]);
  id mockContext = OCMClassMock([LynxRendererContext class]);

  [[[mockUIView stub] andReturn:mockRenderer] getRenderer];
  [[[mockUIView stub] andReturn:mockLayer] layer];

  // Renderer sign is 1
  [[[mockRenderer stub] andReturnValue:OCMOCK_VALUE((int32_t)1)] getSign];

  LynxDisplayListApplier *applier = [[LynxDisplayListApplier alloc] initWithView:mockUIView
                                                                      andContext:mockContext];

  DisplayList list;

  // Level 1: kBegin sign 2 (Mismatch -> Offset applied)
  // Offset += 10, 10
  list.AddOperation(DisplayListOpType::kBegin, 2, 10.0f, 10.0f, 100.0f, 100.0f);

  // Level 2: kBegin sign 3 (Mismatch -> Offset applied)
  // Offset += 20, 20 -> Total 30, 30
  list.AddOperation(DisplayListOpType::kBegin, 3, 20.0f, 20.0f, 50.0f, 50.0f);

  // kRecordBox at 0,0
  list.AddOperation(DisplayListOpType::kRecordBox, 0.0f, 0.0f, 10.0f, 10.0f);

  // kFill
  list.AddOperation(DisplayListOpType::kFill, (int32_t)0xFFFFFFFF, 0);

  // Expectation: Total offset 30, 30
  [[mockLayer expect] insertSublayer:[OCMArg checkWithBlock:^BOOL(CALayer *layer) {
                        CGRect frame = layer.frame;
                        return (frame.origin.x == 30.0f && frame.origin.y == 30.0f);
                      }]
                             atIndex:0];

  // kEnd (Pops 20, 20) -> Offset back to 10, 10
  list.AddOperation(DisplayListOpType::kEnd);

  // kEnd (Pops 10, 10) -> Offset back to 0, 0
  list.AddOperation(DisplayListOpType::kEnd);

  [applier applyDisplayList:&list];

  [mockLayer verify];
}

- (void)testProcessContentOperationsWithText {
  id mockUIView = OCMClassMock([LynxMockView class]);
  id mockLayer = OCMClassMock([CALayer class]);
  id mockContext = OCMClassMock([LynxRendererContext class]);
  id mockTextRenderManager = OCMClassMock([LynxTextRenderManager class]);
  id mockTextRenderer = OCMClassMock([LynxTextRenderer class]);

  [[[mockUIView stub] andReturn:mockLayer] layer];
  [[[mockContext stub] andReturn:mockTextRenderManager] textRenderManager];
  [[[mockTextRenderManager stub] andReturn:mockTextRenderer] takeTextRender:123];

  LynxDisplayListApplier *applier = [[LynxDisplayListApplier alloc] initWithView:mockUIView
                                                                      andContext:mockContext];

  DisplayList list;
  // 1. kRecordBox
  list.AddOperation(DisplayListOpType::kRecordBox, 10.0f, 10.0f, 100.0f, 50.0f);

  // 2. kText
  // int_count=2 (text_id=123, box_index=0)
  list.AddOperation(DisplayListOpType::kText, 123, 0);

  // Expectation: LynxTextLayer added with correct frame
  [[mockLayer expect] insertSublayer:[OCMArg checkWithBlock:^BOOL(CALayer *layer) {
                        return [layer isKindOfClass:[LynxTextLayer class]] &&
                               CGRectEqualToRect(layer.frame, CGRectMake(10, 10, 100, 50));
                      }]
                             atIndex:0];

  [applier applyDisplayList:&list];

  [mockLayer verify];
}

- (void)testProcessContentOperationsWithBorder {
  id mockUIView = OCMClassMock([LynxMockView class]);
  id mockLayer = OCMClassMock([CALayer class]);
  id mockContext = OCMClassMock([LynxRendererContext class]);

  [[[mockUIView stub] andReturn:mockLayer] layer];

  LynxDisplayListApplier *applier = [[LynxDisplayListApplier alloc] initWithView:mockUIView
                                                                      andContext:mockContext];

  DisplayList list;

  // 1. Record outer box (100x100 at 0,0)
  list.AddOperation(DisplayListOpType::kRecordBox, 0.0f, 0.0f, 100.0f, 100.0f);

  // 2. Record inner box (80x80 at 10,10) - creates 10px border
  list.AddOperation(DisplayListOpType::kRecordBox, 10.0f, 10.0f, 80.0f, 80.0f);

  // 3. kBorder with 10 int params
  // int_count=10 (out_box_index=0, inner_box_index=1, 4 colors, 4 styles)
  // Colors: Top=0xFFFF0000(Red), Right=0xFF00FF00(Green), Bottom=0xFF0000FF(Blue),
  // Left=0xFFFFFF00(Yellow) Styles: Top=1(Solid), Right=2(Dashed), Bottom=3(Dotted), Left=1(Solid)
  list.AddOperation(DisplayListOpType::kBorder, 0, 1,          // box indices
                    (int32_t)0xFFFF0000, (int32_t)0xFF00FF00,  // top, right colors
                    (int32_t)0xFF0000FF, (int32_t)0xFFFFFF00,  // bottom, left colors
                    1, 2, 3, 1);                               // top, right, bottom, left styles

  // Expectation: A layer should be added with border image
  [[mockLayer expect] insertSublayer:[OCMArg checkWithBlock:^BOOL(CALayer *layer) {
                        // Verify it's a CALayer with contents (border image)
                        return [layer isKindOfClass:[CALayer class]] && layer.contents != nil;
                      }]
                             atIndex:0];

  [applier applyDisplayList:&list];

  [mockLayer verify];
}

- (void)testProcessContentOperationsWithHostBorderDecoration {
  id mockUIView = OCMClassMock([LynxMockView class]);
  id mockLayer = OCMClassMock([CALayer class]);
  id mockSuperLayer = OCMClassMock([CALayer class]);
  id mockRenderer = OCMClassMock([LynxRenderer class]);
  id mockContext = OCMClassMock([LynxRendererContext class]);

  [[[mockUIView stub] andReturn:mockLayer] layer];
  [[[mockUIView stub] andReturn:mockRenderer] getRenderer];
  [[[mockLayer stub] andReturn:mockSuperLayer] superlayer];
  [[[mockRenderer stub] andReturnValue:OCMOCK_VALUE((int32_t)1)] getSign];

  LynxDisplayListApplier *applier = [[LynxDisplayListApplier alloc] initWithView:mockUIView
                                                                      andContext:mockContext];

  DisplayList list;
  list.AddOperation(DisplayListOpType::kBegin, 1, 0.0f, 0.0f, 100.0f, 100.0f);
  list.AddOperation(DisplayListOpType::kRecordBox, 0.0f, 0.0f, 100.0f, 100.0f);
  list.AddOperation(DisplayListOpType::kRecordBox, 10.0f, 10.0f, 80.0f, 80.0f);
  list.AddOperation(DisplayListOpType::kBorder, 0, 1, (int32_t)0xFFFF0000, (int32_t)0xFF00FF00,
                    (int32_t)0xFF0000FF, (int32_t)0xFFFFFF00, 1, 2, 3, 1);

  [[mockSuperLayer expect] insertSublayer:[OCMArg checkWithBlock:^BOOL(CALayer *layer) {
                             return [layer isKindOfClass:[CALayer class]] && layer.contents != nil;
                           }]
                                    below:mockLayer];

  [applier applyDisplayList:&list];

  [mockLayer verify];
  [mockSuperLayer verify];
}

- (void)testHostDecorationLayersDetachAndReattach {
  LynxMockView *view = [[LynxMockView alloc] initWithFrame:CGRectMake(0, 0, 100, 100)];
  LynxRenderer *renderer = [[LynxRenderer alloc] initWithRenderHost:view andSign:1 andContext:nil];
  [view setRenderer:renderer];

  UIView *firstSuperview = [[UIView alloc] initWithFrame:CGRectMake(0, 0, 200, 200)];
  [firstSuperview addSubview:view];

  LynxDisplayListApplier *applier = [[LynxDisplayListApplier alloc] initWithView:view
                                                                      andContext:nil];

  DisplayList list;
  list.AddOperation(DisplayListOpType::kBegin, 1, 0.0f, 0.0f, 100.0f, 100.0f);
  list.AddOperation(DisplayListOpType::kRecordBox, 0.0f, 0.0f, 50.0f, 50.0f);
  list.AddOperation(DisplayListOpType::kFill, (int32_t)0xFFFF0000, 0);

  [applier applyDisplayList:&list];

  CALayer *decorationLayer = nil;
  for (CALayer *layer in firstSuperview.layer.sublayers) {
    if (layer != view.layer) {
      decorationLayer = layer;
      break;
    }
  }

  XCTAssertNotNil(decorationLayer);
  XCTAssertEqual(decorationLayer.superlayer, firstSuperview.layer);
  XCTAssertLessThan([firstSuperview.layer.sublayers indexOfObject:decorationLayer],
                    [firstSuperview.layer.sublayers indexOfObject:view.layer]);

  [applier detachHostDecorationLayers];
  XCTAssertNil(decorationLayer.superlayer);

  UIView *secondSuperview = [[UIView alloc] initWithFrame:CGRectMake(0, 0, 200, 200)];
  [view removeFromSuperview];
  [secondSuperview addSubview:view];

  [applier reattachHostDecorationLayers];

  XCTAssertEqual(decorationLayer.superlayer, secondSuperview.layer);
  XCTAssertLessThan([secondSuperview.layer.sublayers indexOfObject:decorationLayer],
                    [secondSuperview.layer.sublayers indexOfObject:view.layer]);
}

- (void)testSyncHostDecorationLayersMatchesHostLayerGeometry {
  LynxMockView *view = [[LynxMockView alloc] initWithFrame:CGRectMake(20, 30, 100, 100)];
  LynxRenderer *renderer = [[LynxRenderer alloc] initWithRenderHost:view andSign:1 andContext:nil];
  [view setRenderer:renderer];

  UIView *superview = [[UIView alloc] initWithFrame:CGRectMake(0, 0, 300, 300)];
  [superview addSubview:view];

  LynxDisplayListApplier *applier = [[LynxDisplayListApplier alloc] initWithView:view
                                                                      andContext:nil];

  DisplayList list;
  list.AddOperation(DisplayListOpType::kBegin, 1, 0.0f, 0.0f, 100.0f, 100.0f);
  list.AddOperation(DisplayListOpType::kRecordBox, 10.0f, 15.0f, 40.0f, 50.0f);
  list.AddOperation(DisplayListOpType::kFill, (int32_t)0xFFFF0000, 0);

  [applier applyDisplayList:&list];

  CALayer *decorationLayer = nil;
  for (CALayer *layer in superview.layer.sublayers) {
    if (layer != view.layer) {
      decorationLayer = layer;
      break;
    }
  }

  XCTAssertNotNil(decorationLayer);

  view.alpha = 0.4f;
  view.layer.transform = CATransform3DMakeTranslation(15.0f, 25.0f, 0.0f);

  [applier syncHostDecorationLayers];

  XCTAssertTrue(CGRectEqualToRect(decorationLayer.bounds, view.layer.bounds));
  XCTAssertTrue(CGPointEqualToPoint(decorationLayer.anchorPoint, view.layer.anchorPoint));
  XCTAssertTrue(CGPointEqualToPoint(decorationLayer.position, view.layer.position));
  XCTAssertTrue(CATransform3DEqualToTransform(decorationLayer.transform, view.layer.transform));
  XCTAssertEqualWithAccuracy(decorationLayer.opacity, view.alpha, 0.001f);
}

- (void)testProcessContentOperationsWithBorderInvalidIndices {
  id mockUIView = OCMClassMock([LynxMockView class]);
  id mockContext = OCMClassMock([LynxRendererContext class]);

  LynxDisplayListApplier *applier = [[LynxDisplayListApplier alloc] initWithView:mockUIView
                                                                      andContext:mockContext];

  DisplayList list;

  // kBorder with invalid box indices (no boxes recorded)
  // int_count=10 but box indices are invalid
  list.AddOperation(DisplayListOpType::kBorder, 0, 0,  // invalid box indices
                    (int32_t)0xFFFF0000, (int32_t)0xFF00FF00, (int32_t)0xFF0000FF,
                    (int32_t)0xFFFFFF00, 1, 1, 1, 1);

  // Should not crash, just skip processing
  [applier applyDisplayList:&list];
}

- (void)testProcessContentOperationsWithBorderZeroWidth {
  id mockUIView = OCMClassMock([LynxMockView class]);
  id mockLayer = OCMClassMock([CALayer class]);
  id mockContext = OCMClassMock([LynxRendererContext class]);

  [[[mockUIView stub] andReturn:mockLayer] layer];

  LynxDisplayListApplier *applier = [[LynxDisplayListApplier alloc] initWithView:mockUIView
                                                                      andContext:mockContext];

  DisplayList list;

  // Record two identical boxes (zero border width)
  list.AddOperation(DisplayListOpType::kRecordBox, 0.0f, 0.0f, 100.0f, 100.0f);
  list.AddOperation(DisplayListOpType::kRecordBox, 0.0f, 0.0f, 100.0f, 100.0f);

  // kBorder with identical boxes (zero width)
  list.AddOperation(DisplayListOpType::kBorder, 0, 1, (int32_t)0xFFFF0000, (int32_t)0xFF00FF00,
                    (int32_t)0xFF0000FF, (int32_t)0xFFFFFF00, 1, 1, 1, 1);

  // No layer should be added since border width is zero
  OCMReject([mockLayer insertSublayer:[OCMArg any] atIndex:0]);

  [applier applyDisplayList:&list];

  [mockLayer verify];
}

@end
