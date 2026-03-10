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
@end

@implementation LynxMockView
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

  // Expectation: A layer should be added.
  // Since _refLayer is nil initially, it calls [view.layer addSublayer:newLayer]
  [[mockLayer expect] addSublayer:[OCMArg any]];

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

- (void)testBeginWithIntCountNotOne {
  id mockUIView = OCMClassMock([LynxMockView class]);
  id mockLayer = OCMClassMock([CALayer class]);
  id mockContext = OCMClassMock([LynxRendererContext class]);
  [[[mockUIView stub] andReturn:mockLayer] layer];

  LynxDisplayListApplier *applier = [[LynxDisplayListApplier alloc] initWithView:mockUIView
                                                                      andContext:mockContext];

  DisplayList list;
  // kBegin with int_count=0 -> record_offset should be false
  // float_count=4 -> x, y, w, h
  list.AddOperation(DisplayListOpType::kBegin, 0, 10.0f, 20.0f, 100.0f, 100.0f);

  // kRecordBox at 0,0
  list.AddOperation(DisplayListOpType::kRecordBox, 0.0f, 0.0f, 50.0f, 50.0f);

  // kFill
  list.AddOperation(DisplayListOpType::kFill, (int32_t)0xFFFFFFFF, 0);

  // Expectation: offset (10, 20) NOT applied
  [[mockLayer expect] addSublayer:[OCMArg checkWithBlock:^BOOL(CALayer *layer) {
                        CGRect frame = layer.frame;
                        return (frame.origin.x == 0.0f && frame.origin.y == 0.0f);
                      }]];

  [applier applyDisplayList:&list];
  [mockLayer verify];
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

- (void)testProcessContentOperationsWithClipRect {
  id mockUIView = OCMClassMock([LynxMockView class]);
  id mockContext = OCMClassMock([LynxRendererContext class]);
  LynxDisplayListApplier *applier = [[LynxDisplayListApplier alloc] initWithView:mockUIView
                                                                      andContext:mockContext];

  DisplayList list;
  // kClipRect with standard 4 floats
  list.AddOperation(DisplayListOpType::kClipRect, 10.0f, 10.0f, 100.0f, 100.0f);

  // kClipRect with 8 additional floats (rounded corners) -> total 12 floats
  list.AddOperation(DisplayListOpType::kClipRect, 10.0f, 10.0f, 100.0f, 100.0f,  // 4 base
                    5.0f, 5.0f, 5.0f, 5.0f, 5.0f, 5.0f, 5.0f, 5.0f);             // 8 radii

  [applier applyDisplayList:&list];
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

- (void)testBeginWithMatchingSign {
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

  [[mockLayer expect] addSublayer:[OCMArg checkWithBlock:^BOOL(CALayer *layer) {
                        CGRect frame = layer.frame;
                        // Verify offset was NOT applied
                        return (frame.origin.x == 0.0f && frame.origin.y == 0.0f);
                      }]];

  [applier applyDisplayList:&list];

  [mockLayer verify];
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
  [[mockLayer expect] addSublayer:[OCMArg checkWithBlock:^BOOL(CALayer *layer) {
                        CGRect frame = layer.frame;
                        return (frame.origin.x == 30.0f && frame.origin.y == 30.0f);
                      }]];

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
  [[mockLayer expect] addSublayer:[OCMArg checkWithBlock:^BOOL(CALayer *layer) {
                        return [layer isKindOfClass:[LynxTextLayer class]] &&
                               CGRectEqualToRect(layer.frame, CGRectMake(10, 10, 100, 50));
                      }]];

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
  [[mockLayer expect] addSublayer:[OCMArg checkWithBlock:^BOOL(CALayer *layer) {
                        // Verify it's a CALayer with contents (border image)
                        return [layer isKindOfClass:[CALayer class]] && layer.contents != nil;
                      }]];

  [applier applyDisplayList:&list];

  [mockLayer verify];
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
  OCMReject([mockLayer addSublayer:[OCMArg any]]);

  [applier applyDisplayList:&list];

  [mockLayer verify];
}

@end
