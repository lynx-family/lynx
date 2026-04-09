// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxContainerView.h>
#import <Lynx/LynxDisplayListApplier+Internal.h>
#import <Lynx/LynxRenderer+Internal.h>
#import <Lynx/LynxRenderer.h>
#import <Lynx/LynxRendererContext.h>
#import <Lynx/LynxRendererHost.h>
#import <OCMock/OCMock.h>
#import <XCTest/XCTest.h>
#import <malloc/malloc.h>
#include <objc/runtime.h>
#include <utility>
#include "core/renderer/dom/fragment/display_list.h"
#include "core/renderer/ui_wrapper/painting/ios/platform_renderer_context_darwin.h"
#include "core/renderer/ui_wrapper/painting/ios/platform_renderer_darwin.h"

@interface LynxRenderer (Testing)
- (void)ensureLynxDisplayListApplier;
@end

@interface LynxRendererUnitTest : XCTestCase
@end

@implementation LynxRendererUnitTest

- (void)setUp {
  // Put setup code here. This method is called before the invocation of each test method in the
  // class.
}

- (void)tearDown {
  // Put teardown code here. This method is called after the invocation of each test method in the
  // class.
}

- (void)testUpdateDisplayList {
  id host = OCMProtocolMock(@protocol(LynxRendererHost));
  id context = OCMClassMock([LynxRendererContext class]);
  LynxRenderer* renderer = [[LynxRenderer alloc] initWithRenderHost:host
                                                            andSign:1
                                                         andContext:context];

  id mockApplier = OCMClassMock([LynxDisplayListApplier class]);
  OCMStub([mockApplier alloc]).andReturn(mockApplier);
  OCMStub([mockApplier initWithView:host andContext:context]).andReturn(mockApplier);

  lynx::tasm::DisplayList list;
  [[mockApplier expect] applyDisplayList:&list];
  [[mockApplier expect] syncHostDecorationLayers];

  [renderer updateDisplayList:&list];

  XCTAssertEqual([renderer getDisplayList], &list);
  [mockApplier verify];
  [mockApplier stopMocking];
}

- (void)testEnsureLynxDisplayListApplier {
  id host = OCMProtocolMock(@protocol(LynxRendererHost));
  id context = OCMClassMock([LynxRendererContext class]);
  LynxRenderer* renderer = [[LynxRenderer alloc] initWithRenderHost:host
                                                            andSign:1
                                                         andContext:context];

  id mockApplier = OCMClassMock([LynxDisplayListApplier class]);
  OCMStub([mockApplier alloc]).andReturn(mockApplier);

  // Verify initWithView:andContext: is called with the host and context
  (void)[[[mockApplier expect] andReturn:mockApplier] initWithView:host andContext:context];

  [renderer ensureLynxDisplayListApplier];

  [mockApplier verify];
  [mockApplier stopMocking];
}

- (void)testDetachHostDecorationLayers {
  id host = OCMProtocolMock(@protocol(LynxRendererHost));
  id context = OCMClassMock([LynxRendererContext class]);
  LynxRenderer* renderer = [[LynxRenderer alloc] initWithRenderHost:host
                                                            andSign:1
                                                         andContext:context];

  id mockApplier = OCMClassMock([LynxDisplayListApplier class]);
  OCMStub([mockApplier alloc]).andReturn(mockApplier);
  OCMStub([mockApplier initWithView:host andContext:context]).andReturn(mockApplier);
  [renderer ensureLynxDisplayListApplier];

  [[mockApplier expect] detachHostDecorationLayers];
  [renderer detachHostDecorationLayers];

  [mockApplier verify];
  [mockApplier stopMocking];
}

- (void)testReattachHostDecorationLayers {
  id host = OCMProtocolMock(@protocol(LynxRendererHost));
  id context = OCMClassMock([LynxRendererContext class]);
  LynxRenderer* renderer = [[LynxRenderer alloc] initWithRenderHost:host
                                                            andSign:1
                                                         andContext:context];

  id mockApplier = OCMClassMock([LynxDisplayListApplier class]);
  OCMStub([mockApplier alloc]).andReturn(mockApplier);
  OCMStub([mockApplier initWithView:host andContext:context]).andReturn(mockApplier);
  [renderer ensureLynxDisplayListApplier];

  [[mockApplier expect] reattachHostDecorationLayers];
  [[mockApplier expect] syncHostDecorationLayers];
  [renderer reattachHostDecorationLayers];

  [mockApplier verify];
  [mockApplier stopMocking];
}

- (void)testApplySubtreePropertiesSyncsHostDecorationLayersOnce {
  LynxContainerView* hostView = [[LynxContainerView alloc] init];
  id context = OCMClassMock([LynxRendererContext class]);
  LynxRenderer* renderer = [[LynxRenderer alloc] initWithRenderHost:hostView
                                                            andSign:1
                                                         andContext:context];

  id mockApplier = OCMClassMock([LynxDisplayListApplier class]);
  OCMStub([mockApplier alloc]).andReturn(mockApplier);
  OCMStub([mockApplier initWithView:hostView andContext:context]).andReturn(mockApplier);
  [renderer ensureLynxDisplayListApplier];

  lynx::tasm::SubtreeProperty props[2];

  props[0].type = lynx::tasm::DisplayListSubtreePropertyOpType::kTransform;
  float* transform = props[0].data.transform;
  float transformMatrix[16] = {
      1.0f, 0.0f, 0.0f, 0.0f, 0.0f,  1.0f,  0.0f, 0.0f,
      0.0f, 0.0f, 1.0f, 0.0f, 20.0f, 30.0f, 0.0f, 1.0f,
  };
  memcpy(transform, transformMatrix, sizeof(transformMatrix));

  props[1].type = lynx::tasm::DisplayListSubtreePropertyOpType::kOpacity;
  props[1].data.opacity = 0.5f;

  [[mockApplier expect] syncHostDecorationLayers];
  [renderer applySubtreeProperties:props count:2];

  XCTAssertEqual(hostView.alpha, 0.5f);
  XCTAssertEqual(hostView.layer.transform.m41, 20.0f);
  XCTAssertEqual(hostView.layer.transform.m42, 30.0f);

  [mockApplier verify];
  [mockApplier stopMocking];
}

- (void)testGetSign {
  LynxRenderer* renderer = [[LynxRenderer alloc] initWithRenderHost:nil andSign:1 andContext:nil];
  XCTAssertEqual([renderer sign], 1);
}

#pragma mark - SubtreeProperties Tests

- (void)testApplyTransformIdentity {
  // Use real LynxContainerView for SubtreeProperties tests
  LynxContainerView* hostView = [[LynxContainerView alloc] init];

  LynxRenderer* renderer = [[LynxRenderer alloc] initWithRenderHost:hostView
                                                            andSign:1
                                                         andContext:nil];

  // Identity matrix (column-major)
  float identity[16] = {
      1.0f, 0.0f, 0.0f, 0.0f,  // Column 0
      0.0f, 1.0f, 0.0f, 0.0f,  // Column 1
      0.0f, 0.0f, 1.0f, 0.0f,  // Column 2
      0.0f, 0.0f, 0.0f, 1.0f   // Column 3
  };

  [renderer applyTransform:identity];

  // Verify transform was applied
  CATransform3D expected = CATransform3DIdentity;
  XCTAssertTrue(CATransform3DEqualToTransform(hostView.layer.transform, expected));
}

- (void)testApplyTransformTranslation {
  LynxContainerView* hostView = [[LynxContainerView alloc] init];

  LynxRenderer* renderer = [[LynxRenderer alloc] initWithRenderHost:hostView
                                                            andSign:1
                                                         andContext:nil];

  // Translation matrix: translateX=100, translateY=50
  float translate[16] = {
      1.0f,   0.0f,  0.0f, 0.0f,  // Column 0
      0.0f,   1.0f,  0.0f, 0.0f,  // Column 1
      0.0f,   0.0f,  1.0f, 0.0f,  // Column 2
      100.0f, 50.0f, 0.0f, 1.0f   // Column 3 (m03, m13, m23, m33)
  };

  [renderer applyTransform:translate];

  // Verify translation was applied (CATransform3D uses row-vector: translation in m41/m42/m43)
  CATransform3D transform = hostView.layer.transform;
  XCTAssertEqual(transform.m41, 100.0f);  // translateX
  XCTAssertEqual(transform.m42, 50.0f);   // translateY
  XCTAssertEqual(transform.m43, 0.0f);    // translateZ
}

- (void)testPlatformRendererDarwinUsesTopLeftAnchorForTransformedLayout {
  lynx::tasm::PlatformRendererContextDarwin context(nil);
  lynx::tasm::PlatformRendererDarwin renderer(&context, 13, PlatformRendererType::kView);
  UIView<LynxRendererHost>* view = renderer.GetUIView();
  XCTAssertNotNil(view);

  float rotate[16] = {
      0.70710677f, 0.70710677f, 0.0f, 0.0f, -0.70710677f, 0.70710677f, 0.0f, 0.0f,
      0.0f,        0.0f,        1.0f, 0.0f, 60.0f,        -24.852814f, 0.0f, 1.0f,
  };
  [[view renderer] applyTransform:rotate];

  lynx::tasm::DisplayList list;
  list.AddOperation(lynx::tasm::DisplayListOpType::kBegin, 13,
                    static_cast<int32_t>(PlatformRendererType::kView), 135.0f, 45.0f, 120.0f,
                    120.0f);
  list.AddOperation(lynx::tasm::DisplayListOpType::kEnd);
  renderer.OnUpdateDisplayList(std::move(list));

  XCTAssertTrue(CGPointEqualToPoint(view.layer.anchorPoint, CGPointZero));
  XCTAssertTrue(CGPointEqualToPoint(view.layer.position, CGPointMake(135.0f, 45.0f)));
  XCTAssertTrue(CGRectEqualToRect(view.bounds, CGRectMake(0.0f, 0.0f, 120.0f, 120.0f)));
  XCTAssertEqualWithAccuracy(view.layer.transform.m11, 0.70710677f, 0.001f);
  XCTAssertEqualWithAccuracy(view.layer.transform.m12, 0.70710677f, 0.001f);
  XCTAssertEqualWithAccuracy(view.layer.transform.m21, -0.70710677f, 0.001f);
  XCTAssertEqualWithAccuracy(view.layer.transform.m22, 0.70710677f, 0.001f);
  XCTAssertEqualWithAccuracy(view.layer.transform.m41, 60.0f, 0.001f);
  XCTAssertEqualWithAccuracy(view.layer.transform.m42, -24.852814f, 0.001f);
}

- (void)testApplyTransformScale {
  LynxContainerView* hostView = [[LynxContainerView alloc] init];

  LynxRenderer* renderer = [[LynxRenderer alloc] initWithRenderHost:hostView
                                                            andSign:1
                                                         andContext:nil];

  // Scale matrix: scaleX=2, scaleY=3
  float scale[16] = {
      2.0f, 0.0f, 0.0f, 0.0f,  // Column 0: m00=2
      0.0f, 3.0f, 0.0f, 0.0f,  // Column 1: m11=3
      0.0f, 0.0f, 1.0f, 0.0f,  // Column 2
      0.0f, 0.0f, 0.0f, 1.0f   // Column 3
  };

  [renderer applyTransform:scale];

  // Verify scale was applied
  CATransform3D transform = hostView.layer.transform;
  XCTAssertEqual(transform.m11, 2.0f);  // scaleX
  XCTAssertEqual(transform.m22, 3.0f);  // scaleY
}

- (void)testApplyOpacityNormal {
  LynxContainerView* hostView = [[LynxContainerView alloc] init];

  LynxRenderer* renderer = [[LynxRenderer alloc] initWithRenderHost:hostView
                                                            andSign:1
                                                         andContext:nil];

  // Test 0.5
  [renderer applyOpacity:0.5f];
  XCTAssertEqual(hostView.alpha, 0.5f);

  // Test 0
  [renderer applyOpacity:0.0f];
  XCTAssertEqual(hostView.alpha, 0.0f);

  // Test 1
  [renderer applyOpacity:1.0f];
  XCTAssertEqual(hostView.alpha, 1.0f);
}

- (void)testApplyOpacityClamping {
  LynxContainerView* hostView = [[LynxContainerView alloc] init];

  LynxRenderer* renderer = [[LynxRenderer alloc] initWithRenderHost:hostView
                                                            andSign:1
                                                         andContext:nil];

  // Greater than 1 should be clamped to 1
  [renderer applyOpacity:1.5f];
  XCTAssertEqual(hostView.alpha, 1.0f);

  // Less than 0 should be clamped to 0
  [renderer applyOpacity:-0.5f];
  XCTAssertEqual(hostView.alpha, 0.0f);
}

- (void)testApplySubtreePropertiesBoth {
  LynxContainerView* hostView = [[LynxContainerView alloc] init];

  LynxRenderer* renderer = [[LynxRenderer alloc] initWithRenderHost:hostView
                                                            andSign:1
                                                         andContext:nil];

  // Create two properties
  lynx::tasm::SubtreeProperty props[2];

  // Property 1: Transform
  props[0].type = lynx::tasm::DisplayListSubtreePropertyOpType::kTransform;
  float* m = props[0].data.transform;
  // Identity matrix with translation
  memset(m, 0, sizeof(float) * 16);
  m[0] = m[5] = m[10] = m[15] = 1.0f;
  m[12] = 100.0f;  // translateX

  // Property 2: Opacity
  props[1].type = lynx::tasm::DisplayListSubtreePropertyOpType::kOpacity;
  props[1].data.opacity = 0.8f;

  [renderer applySubtreeProperties:props count:2];

  // Verify both were applied (translation now in m41)
  XCTAssertEqual(hostView.layer.transform.m41, 100.0f);
  XCTAssertEqual(hostView.alpha, 0.8f);
}

- (void)testApplySubtreePropertiesEmpty {
  LynxContainerView* hostView = [[LynxContainerView alloc] init];

  LynxRenderer* renderer = [[LynxRenderer alloc] initWithRenderHost:hostView
                                                            andSign:1
                                                         andContext:nil];

  // Empty array - should not crash
  [renderer applySubtreeProperties:nullptr count:0];

  // If we get here, test passed
  XCTAssertTrue(YES);
}

- (void)testApplySubtreePropertiesNullHost {
  LynxRenderer* renderer = [[LynxRenderer alloc] initWithRenderHost:nil andSign:1 andContext:nil];

  lynx::tasm::SubtreeProperty props[1];
  props[0].type = lynx::tasm::DisplayListSubtreePropertyOpType::kOpacity;
  props[0].data.opacity = 0.5f;

  // Should not crash with nil host
  [renderer applySubtreeProperties:props count:1];

  XCTAssertTrue(YES);
}

- (void)testApplyTransformNullMatrix {
  LynxContainerView* hostView = [[LynxContainerView alloc] init];

  LynxRenderer* renderer = [[LynxRenderer alloc] initWithRenderHost:hostView
                                                            andSign:1
                                                         andContext:nil];

  // nil matrix - should not crash
  [renderer applyTransform:nullptr];

  XCTAssertTrue(YES);
}

- (void)testApplyTransformRotation {
  LynxContainerView* hostView = [[LynxContainerView alloc] init];

  LynxRenderer* renderer = [[LynxRenderer alloc] initWithRenderHost:hostView
                                                            andSign:1
                                                         andContext:nil];

  // Rotation matrix: 90 degrees around Z axis (column-major, C++ column-vector convention)
  // M = [ cos -sin   0   0 ]
  //     [ sin  cos   0   0 ]
  //     [   0    0   1   0 ]
  //     [   0    0   0   1 ]
  // Column 0 = [cos, sin, 0, 0], Column 1 = [-sin, cos, 0, 0]
  float cos90 = 0.0f;
  float sin90 = 1.0f;
  float rotation[16] = {
      cos90,  sin90, 0.0f, 0.0f,  // Column 0: m00, m10, m20, m30
      -sin90, cos90, 0.0f, 0.0f,  // Column 1: m01, m11, m21, m31
      0.0f,   0.0f,  1.0f, 0.0f,  // Column 2: m02, m12, m22, m32
      0.0f,   0.0f,  0.0f, 1.0f   // Column 3: m03, m13, m23, m33
  };

  [renderer applyTransform:rotation];

  // After transpose for CATransform3D (row-vector convention):
  // Row 0 = [cos, sin, 0, 0], Row 1 = [-sin, cos, 0, 0]
  CATransform3D transform = hostView.layer.transform;
  XCTAssertEqualWithAccuracy(transform.m11, cos90, 0.001f);   // m11 = cos
  XCTAssertEqualWithAccuracy(transform.m12, sin90, 0.001f);   // m12 = sin (from col 0 row 1)
  XCTAssertEqualWithAccuracy(transform.m21, -sin90, 0.001f);  // m21 = -sin (from col 1 row 0)
  XCTAssertEqualWithAccuracy(transform.m22, cos90, 0.001f);   // m22 = cos
}

@end
