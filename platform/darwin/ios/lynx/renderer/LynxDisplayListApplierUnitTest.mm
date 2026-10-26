// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxBackgroundUtils.h>
#import <Lynx/LynxDisplayListApplier+Internal.h>
#import <Lynx/LynxImageManager.h>
#import <Lynx/LynxRenderer+Internal.h>
#import <Lynx/LynxRenderer.h>
#import <Lynx/LynxRendererContext.h>
#import <Lynx/LynxRendererHost.h>
#import <Lynx/LynxServiceTextProtocol.h>
#import <Lynx/LynxTextLayer.h>
#import <Lynx/LynxTextRenderManager.h>
#import <Lynx/UIView+Lynx.h>
#import <OCMock/OCMock.h>
#import <XCTest/XCTest.h>
#import "LynxRendererContext+Internal.h"
#include "core/public/platform_renderer_type.h"
#include "core/renderer/ui_wrapper/painting/paint_image.h"
#include "gfx/public/display_list.h"

using namespace lynx::tasm;

@interface LynxImageManager (LynxDisplayListApplierUnitTest)
- (void)updatePaintInfo:(const ImagePaintInfo &)paintInfo;
- (void)applyImage:(UIImage *)image withType:(LynxImageRequestType)type;
@end

@interface LynxDisplayListApplier (DrawingResourceUnitTest)
- (BOOL)isTextServiceModeOn;
@end

namespace {
constexpr int32_t kViewType = static_cast<int32_t>(PlatformRendererType::kView);

void UpdateMeasuredText(LynxRendererContext *context, NSString *text) {
  if (context.textRenderManager == nil) {
    context.textRenderManager = [LynxTextRenderManager new];
  }
  LynxAttributedTextBundle *bundle = [LynxAttributedTextBundle new];
  bundle.textStyle = [LynxTextStyle new];
  bundle.maxLineNum = -1;
  bundle.attributedString = [[NSAttributedString alloc]
      initWithString:text
          attributes:@{NSFontAttributeName : [UIFont systemFontOfSize:16]}];
  [context.textRenderManager putAttributedTextBundle:1 textBundle:bundle];
  [context.textRenderManager measureTextWithSign:1
                                           width:90
                                       widthMode:LynxMeasureModeDefinite
                                          height:50
                                      heightMode:LynxMeasureModeDefinite
                                 childrenSizeDic:nil];
}

UIImage *ResourceTestImage(UIColor *color) {
  UIGraphicsBeginImageContextWithOptions(CGSizeMake(8, 8), NO, 1);
  [color setFill];
  UIRectFill(CGRectMake(0, 0, 8, 8));
  UIImage *image = UIGraphicsGetImageFromCurrentImageContext();
  UIGraphicsEndImageContext();
  return image;
}

void AppendBegin(DisplayList &list, int32_t id, int32_t type, float x, float y, float w, float h) {
  DisplayListItem item;
  item.type = DisplayListOpType::kBegin;
  item.payload.begin.id = id;
  item.payload.begin.type = type;
  item.payload.begin.x = x;
  item.payload.begin.y = y;
  item.payload.begin.w = w;
  item.payload.begin.h = h;
  list.AppendItem(item);
}

void AppendEnd(DisplayList &list) {
  DisplayListItem item;
  item.type = DisplayListOpType::kEnd;
  list.AppendItem(item);
}

void AppendFill(DisplayList &list, uint32_t color, int32_t clip_index) {
  DisplayListItem item;
  item.type = DisplayListOpType::kFill;
  item.payload.fill.color = color;
  item.payload.fill.clip_index = clip_index;
  list.AppendItem(item);
}

void AppendBoxShadow(DisplayList &list, int32_t shadowIndex, int32_t clipIndex, bool inset,
                     float blur = 10.f) {
  DisplayListItem item;
  item.type = DisplayListOpType::kBoxShadow;
  item.payload.box_shadow.shadow_box_index = shadowIndex;
  item.payload.box_shadow.clip_box_index = clipIndex;
  item.payload.box_shadow.color = 0x80FF00FF;
  item.payload.box_shadow.blur_radius = blur;
  item.payload.box_shadow.clip_mode = inset ? 1 : 0;
  list.AppendItem(item);
}

void AppendDrawView(DisplayList &list, int32_t view_id, float offset_x = 0.0f,
                    float offset_y = 0.0f) {
  DisplayListItem item;
  item.type = DisplayListOpType::kDrawView;
  item.payload.draw_view.view_id = view_id;
  item.payload.draw_view.offset_x = offset_x;
  item.payload.draw_view.offset_y = offset_y;
  list.AppendItem(item);
}

void AppendRecordBox(DisplayList &list, float x, float y, float w, float h, bool has_radii = false,
                     const float *radii = nullptr) {
  DisplayListItem item;
  item.type = DisplayListOpType::kRecordBox;
  item.payload.record_box.x = x;
  item.payload.record_box.y = y;
  item.payload.record_box.w = w;
  item.payload.record_box.h = h;
  if (has_radii && radii != nullptr) {
    item.payload.record_box.has_radii = 1;
    for (int i = 0; i < 8; i++) {
      item.payload.record_box.radii[i] = radii[i];
    }
  } else {
    item.payload.record_box.has_radii = 0;
  }
  list.AppendItem(item);
}

void AppendText(DisplayList &list, int32_t text_id, int32_t box_index) {
  DisplayListItem item;
  item.type = DisplayListOpType::kText;
  item.payload.text.text_id = text_id;
  item.payload.text.box_index = box_index;
  list.AppendItem(item);
}

void AppendImage(DisplayList &list, int32_t image_id, int32_t box_index) {
  DisplayListItem item;
  item.type = DisplayListOpType::kImage;
  item.payload.image.image_id = image_id;
  item.payload.image.box_index = box_index;
  list.AppendItem(item);
}

void AppendBorder(DisplayList &list, int32_t out_index, int32_t inner_index,
                  const uint32_t *colors = nullptr, const int32_t *styles = nullptr) {
  DisplayListItem item{};
  item.type = DisplayListOpType::kBorder;
  item.payload.border.out_index = out_index;
  item.payload.border.inner_index = inner_index;
  for (int i = 0; i < 4; i++) {
    item.payload.border.colors[i] = colors == nullptr ? 0xFF000000 : colors[i];
    item.payload.border.styles[i] = styles == nullptr ? 0 : styles[i];
  }
  list.AppendItem(item);
}

void AppendClipRect(DisplayList &list, float x, float y, float w, float h, bool has_radii = false,
                    const float *radii = nullptr) {
  DisplayListItem item;
  item.type = DisplayListOpType::kClipRect;
  item.payload.clip_rect.x = x;
  item.payload.clip_rect.y = y;
  item.payload.clip_rect.w = w;
  item.payload.clip_rect.h = h;
  if (has_radii && radii != nullptr) {
    item.payload.clip_rect.has_radii = 1;
    for (int i = 0; i < 8; i++) {
      item.payload.clip_rect.radii[i] = radii[i];
    }
  } else {
    item.payload.clip_rect.has_radii = 0;
  }
  list.AppendItem(item);
}

}  // namespace

// Define a mock view class that implements the protocol
@interface LynxMockView : UIView <LynxRendererHost>
@property(nonatomic, strong) LynxRenderer *renderer;
@end

@implementation LynxMockView

@synthesize rendererContext = _rendererContext;

- (instancetype)initWithRendererContext:(LynxRendererContext *)context {
  self = [super init];
  return self;
}

- (void)setRenderer:(LynxRenderer *)renderer {
  _renderer = renderer;
}

- (LynxRenderer *)createRendererWithSign:(int32_t)sign andContext:(LynxRendererContext *)context {
  self.renderer = [[LynxRenderer alloc] initWithRenderHost:self andSign:sign andContext:context];
  return self.renderer;
}

- (UIView *)view {
  return self;
}

@end

@interface LynxActionProbeView : UIView
@property(nonatomic) BOOL actionsDisabledDuringMove;
@end

@implementation LynxActionProbeView
- (void)setFrame:(CGRect)frame {
  self.actionsDisabledDuringMove = [CATransaction disableActions];
  [super setFrame:frame];
}
@end

// Scrollable host mock: UIScrollView shifts its layer's bounds origin while scrolling.
@interface LynxMockScrollView : UIScrollView <LynxRendererHost>
@property(nonatomic, strong) LynxRenderer *renderer;
@end

@implementation LynxMockScrollView

@synthesize rendererContext = _rendererContext;

- (instancetype)initWithRendererContext:(LynxRendererContext *)context {
  self = [super init];
  return self;
}

- (void)setRenderer:(LynxRenderer *)renderer {
  _renderer = renderer;
}

- (LynxRenderer *)createRendererWithSign:(int32_t)sign andContext:(LynxRendererContext *)context {
  self.renderer = [[LynxRenderer alloc] initWithRenderHost:self andSign:sign andContext:context];
  return self.renderer;
}

- (UIView *)view {
  return self;
}

@end

@interface LynxCountingRenderer : LynxRenderer
@property(nonatomic, assign) NSInteger setFrameCount;
@end

@implementation LynxCountingRenderer

- (void)onSetFrame:(CGRect)frame {
  self.setFrameCount++;
}

@end

@interface LynxDisplayListApplierUnitTest : XCTestCase
@end

@implementation LynxDisplayListApplierUnitTest

- (void)setUp {
}

- (void)tearDown {
}

- (void)testBoxShadowCollapsedInsetAndReset {
  for (CGSize size : {CGSizeMake(0, 20), CGSizeMake(60, 0), CGSizeZero}) {
    LynxMockView *view = [[LynxMockView alloc] initWithFrame:CGRectMake(0, 0, 100, 80)];
    LynxDisplayListApplier *applier = [[LynxDisplayListApplier alloc] initWithView:view
                                                                        andContext:nil];
    DisplayList list;
    const float radii[] = {8, 8, 8, 8, 8, 8, 8, 8};
    AppendRecordBox(list, 10, 10, 60, 20, true, radii);
    AppendRecordBox(list, 15, 15, size.width, size.height);
    AppendBoxShadow(list, 1, 0, true);
    [applier applyDisplayList:&list];
    XCTAssertEqual(view.layer.sublayers.count, 1u);
    CAShapeLayer *fill = (CAShapeLayer *)view.layer.sublayers.firstObject.sublayers.firstObject;
    XCTAssertTrue([fill isKindOfClass:[CAShapeLayer class]]);
    XCTAssertTrue(CGRectEqualToRect(fill.frame, CGRectMake(10, 10, 60, 20)));
    XCTAssertEqualWithAccuracy(CGColorGetAlpha(fill.fillColor), 128.f / 255.f, 0.001f);
    XCTAssertTrue(CGPathContainsPoint(fill.path, nullptr, CGPointMake(30, 10), false));
    XCTAssertFalse(CGPathContainsPoint(fill.path, nullptr, CGPointZero, false));
    XCTAssertEqual(fill.shadowOpacity, 0.f);

    DisplayList empty;
    [applier applyDisplayList:&empty];
    XCTAssertEqual(view.layer.sublayers.count, 0u);
  }
}

- (void)testBoxShadowInsetAndOutsetPaths {
  for (bool inset : {false, true}) {
    LynxMockView *view = [[LynxMockView alloc] initWithFrame:CGRectMake(0, 0, 100, 80)];
    LynxDisplayListApplier *applier = [[LynxDisplayListApplier alloc] initWithView:view
                                                                        andContext:nil];
    DisplayList list;
    AppendBegin(list, 1, kViewType, 0, 0, 100, 80);
    AppendBegin(list, 2, kViewType, 7, 9, 60, 20);
    AppendRecordBox(list, 10, 10, 60, 20);
    AppendRecordBox(list, 15, 15, 50, 10);
    AppendBoxShadow(list, 1, 0, inset);
    AppendEnd(list);
    AppendEnd(list);
    [applier applyDisplayList:&list];
    CALayer *shadow = view.layer.sublayers.firstObject.sublayers.firstObject;
    XCTAssertNotNil(shadow);
    XCTAssertTrue(CGRectEqualToRect(shadow.frame, CGRectMake(17, 19, 60, 20)));
    XCTAssertTrue(CGSizeEqualToSize(shadow.shadowOffset, CGSizeZero));
    XCTAssertEqual(shadow.shadowRadius, 5.f);
    XCTAssertEqualWithAccuracy(CGColorGetAlpha(shadow.shadowColor), 128.f / 255.f, 0.001f);
    XCTAssertEqual(CGPathContainsPoint(shadow.shadowPath, nullptr, CGPointMake(30, 10), false),
                   !inset);
    XCTAssertEqual(CGPathContainsPoint(shadow.shadowPath, nullptr, CGPointMake(1, 1), false),
                   inset);
    CAShapeLayer *mask = (CAShapeLayer *)shadow.mask;
    CGPoint center = CGPointMake(30 - mask.frame.origin.x, 10 - mask.frame.origin.y);
    XCTAssertEqual(CGPathContainsPoint(mask.path, nullptr, center, !inset), inset);
  }
}

- (void)testBoxShadowHostDecorationSurvivesClipAndUpdates {
  UIView *parent = [[UIView alloc] initWithFrame:CGRectMake(0, 0, 200, 200)];
  LynxMockView *view = [[LynxMockView alloc] initWithFrame:CGRectMake(20, 30, 100, 80)];
  [parent addSubview:view];
  [view createRendererWithSign:1 andContext:nil];
  LynxDisplayListApplier *applier = [[LynxDisplayListApplier alloc] initWithView:view
                                                                      andContext:nil];
  DisplayList list;
  AppendBegin(list, 1, kViewType, 0, 0, 100, 80);
  AppendRecordBox(list, 0, 0, 100, 80);
  AppendRecordBox(list, -5, -5, 110, 90);
  AppendBoxShadow(list, 1, 0, false);
  AppendBoxShadow(list, 1, 0, false, 0);
  AppendEnd(list);
  [applier applyDisplayList:&list];
  [applier syncHostDecorationLayers];
  XCTAssertEqual(parent.layer.sublayers.count, 3u);
  XCTAssertEqual(parent.layer.sublayers.lastObject, view.layer);
  CALayer *first = parent.layer.sublayers[0];
  XCTAssertTrue(CGRectEqualToRect(first.frame, view.frame));
  XCTAssertEqual(first.sublayers.firstObject.shadowRadius, 5.f);
  XCTAssertEqual(parent.layer.sublayers[1].sublayers.firstObject.shadowRadius, 0.f);
  view.layer.masksToBounds = YES;
  XCTAssertEqual(first.superlayer, parent.layer);
  [applier applyDisplayList:&list];
  XCTAssertNil(first.superlayer);
  XCTAssertEqual(parent.layer.sublayers.count, 3u);
}

- (void)testBoxShadowHostDecorationKeepsViewportCoordinatesAfterScrolling {
  for (int mode = 0; mode < 3; ++mode) {
    UIView *parent = [[UIView alloc] initWithFrame:CGRectMake(0, 0, 200, 200)];
    LynxMockScrollView *view =
        [[LynxMockScrollView alloc] initWithFrame:CGRectMake(20, 30, 100, 80)];
    view.contentSize = CGSizeMake(200, 400);
    [parent addSubview:view];
    [view createRendererWithSign:1 andContext:nil];
    LynxRenderer *renderer = view.renderer;
    DisplayList list;
    AppendBegin(list, 1, kViewType, 0, 0, 100, 80);
    AppendRecordBox(list, 2, 3, 96, 74);
    AppendRecordBox(list, 5, 6, mode == 2 ? 0 : 90, 68);
    AppendBoxShadow(list, 1, 0, mode != 0);
    AppendEnd(list);
    [renderer updateDisplayList:&list];
    CALayer *container = parent.layer.sublayers.firstObject;
    CALayer *drawing = container.sublayers.firstObject;
    CGRect initialRect = [drawing convertRect:drawing.bounds toLayer:parent.layer];

    // UIScrollView represents contentOffset through its bounds origin.
    view.contentOffset = CGPointMake(7, 40);
    SubtreeProperty opacity;
    opacity.type = DisplayListSubtreePropertyOpType::kOpacity;
    opacity.data.opacity = 0.8f;
    [renderer applySubtreeProperties:&opacity count:1];
    XCTAssertTrue(CGPointEqualToPoint(container.bounds.origin, CGPointZero));
    XCTAssertEqualWithAccuracy(container.opacity, 0.8f, 0.001f);
    XCTAssertTrue(
        CGRectEqualToRect([drawing convertRect:drawing.bounds toLayer:parent.layer], initialRect));

    [renderer updateDisplayList:&list];
    XCTAssertNil(container.superlayer);
    container = parent.layer.sublayers.firstObject;
    drawing = container.sublayers.firstObject;
    XCTAssertEqual(parent.layer.sublayers.count, 2u);
    XCTAssertTrue(CGPointEqualToPoint(container.bounds.origin, CGPointZero));
    XCTAssertEqualWithAccuracy(container.opacity, 0.8f, 0.001f);
    XCTAssertTrue(
        CGRectEqualToRect([drawing convertRect:drawing.bounds toLayer:parent.layer], initialRect));
  }
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
  // We use LynxMockView to ensure it responds to 'renderer' (from protocol) and
  // 'layer'/'subviews' (from UIView)
  id mockUIView = OCMClassMock([LynxMockView class]);
  id mockLayer = OCMClassMock([CALayer class]);
  id mockRenderer = OCMClassMock([LynxRenderer class]);
  id mockContext = OCMClassMock([LynxRendererContext class]);

  // Stub properties
  [[[mockUIView stub] andReturn:mockLayer] layer];
  [[[mockUIView stub] andReturn:mockRenderer] renderer];

  // Stub subviews for kDrawView
  id mockSubView = OCMClassMock([UIView class]);
  id mockSubLayer = OCMClassMock([CALayer class]);
  [[[mockSubView stub] andReturn:mockSubLayer] layer];
  [[[mockUIView stub] andReturn:@[ mockSubView ]] subviews];

  // Stub Renderer Sign
  // [[_view renderer] sign]
  [OCMStub([(LynxRenderer *)mockRenderer sign]) andReturnValue:OCMOCK_VALUE((int32_t)1)];

  LynxDisplayListApplier *applier = [[LynxDisplayListApplier alloc] initWithView:mockUIView
                                                                      andContext:mockContext];

  // Construct DisplayList
  DisplayList list;

  // 1. kBegin
  // int_count=2 (sign and type), float_count=4 (x, y, w, h)
  // Logic: if (sign_arg != renderer_sign) -> record_offset = true
  // Renderer sign is 1. We pass 2. 2 != 1 -> true.
  // Offsets updated by x, y.
  AppendBegin(list, 2, kViewType, 10.0f, 20.0f, 100.0f, 100.0f);

  // 2. kRecordBox
  AppendRecordBox(list, 0.0f, 0.0f, 50.0f, 50.0f);

  // 3. kFill
  // argb = 0xFF0000FF (Red=0, Green=0, Blue=255, Alpha=1.0)
  AppendFill(list, 0xFF0000FF, 0);

  // Expectation: A layer should be added at the bottom of the host layer tree.
  [[mockLayer expect] insertSublayer:[OCMArg any] atIndex:0];

  // 4. kDrawView
  // Should update _refLayer to mockSubView.layer
  AppendDrawView(list, 123, 10.0f, 20.0f);

  // 5. Another kRecordBox and kFill to test _refLayer logic
  // This time _refLayer should be mockSubView.layer
  AppendRecordBox(list, 5.0f, 5.0f, 10.0f, 10.0f);
  AppendFill(list, 0xFF00FF00, 1);

  // Expectation: insertSublayer:newLayer above:mockSubLayer
  [[mockLayer expect] insertSublayer:[OCMArg any] above:mockSubLayer];

  // 6. kEnd
  AppendEnd(list);

  [applier applyDisplayList:&list];

  [mockLayer verify];
}

- (void)testProcessContentOperationsWithTODOs {
  id mockUIView = OCMClassMock([LynxMockView class]);
  id mockContext = OCMClassMock([LynxRendererContext class]);
  LynxDisplayListApplier *applier = [[LynxDisplayListApplier alloc] initWithView:mockUIView
                                                                      andContext:mockContext];

  DisplayList list;
  // kText, kImage, kBorder should be handled gracefully
  AppendText(list, 0, 0);
  AppendImage(list, 0, 0);
  AppendBorder(list, 0, 0);

  [applier applyDisplayList:&list];
  // Verify no crash
}

- (void)testDrawViewUpdatesOnlyChangedOffsetWithoutReapplyingCurrentOffset {
  LynxMockView *host = [[LynxMockView alloc] initWithRendererContext:nil];
  [host createRendererWithSign:1 andContext:nil];
  LynxMockView *child = [[LynxMockView alloc] initWithRendererContext:nil];
  child.frame = CGRectMake(1.0f, 2.0f, 12.0f, 13.0f);
  LynxCountingRenderer *childRenderer = [[LynxCountingRenderer alloc] initWithRenderHost:child
                                                                                 andSign:2
                                                                              andContext:nil];
  child.renderer = childRenderer;
  [host addSubview:child];

  LynxDisplayListApplier *applier = [[LynxDisplayListApplier alloc] initWithView:host
                                                                      andContext:nil];
  DisplayList list;
  AppendBegin(list, 1, kViewType, 0.0f, 0.0f, 100.0f, 100.0f);
  AppendBegin(list, 3, kViewType, 10.0f, 20.0f, 50.0f, 50.0f);
  AppendDrawView(list, 2, 15.0f, 26.0f);
  AppendEnd(list);
  AppendEnd(list);

  [applier applyDisplayList:&list];
  [applier applyDisplayList:&list];

  XCTAssertTrue(CGRectEqualToRect(child.frame, CGRectMake(15.0f, 26.0f, 12.0f, 13.0f)));
  XCTAssertEqual(childRenderer.setFrameCount, 1);
}

- (void)testDrawViewUsesTopLeftAnchorForTransformedChild {
  LynxMockView *host = [[LynxMockView alloc] initWithRendererContext:nil];
  [host createRendererWithSign:1 andContext:nil];
  LynxMockView *child = [[LynxMockView alloc] initWithRendererContext:nil];
  child.frame = CGRectMake(1.0f, 2.0f, 12.0f, 13.0f);
  child.layer.anchorPoint = CGPointMake(0.5f, 0.5f);
  child.layer.transform = CATransform3DMakeScale(2.0f, 2.0f, 1.0f);
  LynxCountingRenderer *childRenderer = [[LynxCountingRenderer alloc] initWithRenderHost:child
                                                                                 andSign:2
                                                                              andContext:nil];
  child.renderer = childRenderer;
  [host addSubview:child];

  LynxDisplayListApplier *applier = [[LynxDisplayListApplier alloc] initWithView:host
                                                                      andContext:nil];
  DisplayList list;
  AppendBegin(list, 1, kViewType, 0.0f, 0.0f, 100.0f, 100.0f);
  AppendDrawView(list, 2, 15.0f, 26.0f);
  AppendEnd(list);

  [applier applyDisplayList:&list];
  [applier applyDisplayList:&list];

  XCTAssertTrue(CGPointEqualToPoint(child.layer.anchorPoint, CGPointZero));
  XCTAssertTrue(CGPointEqualToPoint(child.layer.position, CGPointMake(15.0f, 26.0f)));
  XCTAssertTrue(CGSizeEqualToSize(child.layer.bounds.size, CGSizeMake(12.0f, 13.0f)));
  XCTAssertEqual(childRenderer.setFrameCount, 1);
}

- (void)testDrawViewUpdatesLegacyViewOffset {
  LynxMockView *host = [[LynxMockView alloc] initWithRendererContext:nil];
  [host createRendererWithSign:1 andContext:nil];
  UIView *child = [[UIView alloc] initWithFrame:CGRectMake(1.0f, 2.0f, 12.0f, 13.0f)];
  child.lynxSign = @2;
  [host addSubview:child];

  LynxDisplayListApplier *applier = [[LynxDisplayListApplier alloc] initWithView:host
                                                                      andContext:nil];
  DisplayList list;
  AppendBegin(list, 1, kViewType, 0.0f, 0.0f, 100.0f, 100.0f);
  AppendDrawView(list, 2, 15.0f, 26.0f);
  AppendEnd(list);

  [applier applyDisplayList:&list];
  [applier applyDisplayList:&list];

  XCTAssertTrue(CGRectEqualToRect(child.frame, CGRectMake(15.0f, 26.0f, 12.0f, 13.0f)));
}

- (void)testDrawViewPreservesAnchorForTransformedLegacyView {
  LynxMockView *host = [[LynxMockView alloc] initWithRendererContext:nil];
  [host createRendererWithSign:1 andContext:nil];
  UIView *child = [[UIView alloc] initWithFrame:CGRectMake(1.0f, 2.0f, 12.0f, 13.0f)];
  child.lynxSign = @2;
  child.layer.anchorPoint = CGPointMake(0.5f, 0.5f);
  child.layer.transform = CATransform3DMakeScale(2.0f, 2.0f, 1.0f);
  [host addSubview:child];

  LynxDisplayListApplier *applier = [[LynxDisplayListApplier alloc] initWithView:host
                                                                      andContext:nil];
  DisplayList list;
  AppendBegin(list, 1, kViewType, 0.0f, 0.0f, 100.0f, 100.0f);
  AppendDrawView(list, 2, 15.0f, 26.0f);
  AppendEnd(list);

  [applier applyDisplayList:&list];
  [applier applyDisplayList:&list];

  XCTAssertTrue(CGPointEqualToPoint(child.layer.anchorPoint, CGPointMake(0.5f, 0.5f)));
  XCTAssertTrue(CGPointEqualToPoint(child.layer.position, CGPointMake(21.0f, 32.5f)));
  XCTAssertTrue(CGSizeEqualToSize(child.layer.bounds.size, CGSizeMake(12.0f, 13.0f)));
}

- (void)testNestedTextContentOffsetAlignsTextLayerWithInlineView {
  LynxMockView *host = [[LynxMockView alloc] initWithRendererContext:nil];
  [host createRendererWithSign:1 andContext:nil];
  LynxMockView *inlineView = [[LynxMockView alloc] initWithRendererContext:nil];
  inlineView.frame = CGRectMake(0.0f, 0.0f, 58.0f, 16.0f);
  [inlineView createRendererWithSign:2 andContext:nil];
  [host addSubview:inlineView];

  id mockTextRenderer = OCMClassMock([LynxTextRenderer class]);
  id mockTextRenderManager = OCMClassMock([LynxTextRenderManager class]);
  OCMStub([mockTextRenderManager takeTextRender:1]).andReturn(mockTextRenderer);
  id mockContext = OCMClassMock([LynxRendererContext class]);
  OCMStub([mockContext textRenderManager]).andReturn(mockTextRenderManager);

  LynxDisplayListApplier *applier = [[LynxDisplayListApplier alloc] initWithView:host
                                                                      andContext:mockContext];
  DisplayList list;
  AppendBegin(list, 1, static_cast<int32_t>(PlatformRendererType::kText), 12.0f, 14.0f, 200.0f,
              40.0f);
  AppendBegin(list, 1, static_cast<int32_t>(PlatformRendererType::kText), 8.0f, 6.0f, 184.0f,
              28.0f);
  AppendRecordBox(list, 0.0f, 0.0f, 184.0f, 28.0f);
  AppendText(list, 1, 0);
  AppendEnd(list);
  AppendDrawView(list, 2, 8.0f, 6.0f);
  AppendEnd(list);

  [applier applyDisplayList:&list];

  CALayer *textLayer = host.layer.sublayers.firstObject;
  XCTAssertNotNil(textLayer);
  XCTAssertTrue(CGRectEqualToRect(textLayer.frame, CGRectMake(8.0f, 6.0f, 184.0f, 28.0f)));
  XCTAssertTrue(CGRectEqualToRect(inlineView.frame, CGRectMake(8.0f, 6.0f, 58.0f, 16.0f)));
}

- (void)testProcessContentOperationsWithUnknownOp {
  id mockUIView = OCMClassMock([LynxMockView class]);
  id mockContext = OCMClassMock([LynxRendererContext class]);
  LynxDisplayListApplier *applier = [[LynxDisplayListApplier alloc] initWithView:mockUIView
                                                                      andContext:mockContext];

  DisplayList list;
  // Add an unknown operation type to hit the default case
  // Assuming 9999 is not a valid op type
  DisplayListItem item;
  item.type = static_cast<DisplayListOpType>(9999);
  list.AppendItem(item);

  [applier applyDisplayList:&list];
  // Verify no crash
}

- (void)testProcessContentOperationsWithLinearGradientKeepsFollowingOpsAligned {
  LynxMockView *view = [[LynxMockView alloc] initWithFrame:CGRectMake(0, 0, 200, 200)];
  LynxDisplayListApplier *applier = [[LynxDisplayListApplier alloc] initWithView:view
                                                                      andContext:nil];

  DisplayList list;
  AppendRecordBox(list, 0.0f, 0.0f, 100.0f, 100.0f);
  AppendRecordBox(list, 10.0f, 12.0f, 80.0f, 60.0f);

  lynx::base::Vector<uint32_t> colors;
  colors.push_back(0xFFFF0000);
  colors.push_back(0xFF0000FF);
  lynx::base::Vector<float> stops;
  stops.push_back(0.0f);
  stops.push_back(1.0f);
  list.AddLinearGradient(90.0f, colors, stops, 0, 1, 1, 1);

  AppendRecordBox(list, 5.0f, 6.0f, 10.0f, 12.0f);
  AppendFill(list, 0xFF00FF00, 2);

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
  AppendRecordBox(list, 0.0f, 0.0f, 100.0f, 100.0f);

  lynx::base::Vector<uint32_t> colors;
  colors.push_back(0xFFFF0000);
  colors.push_back(0xFF0000FF);
  lynx::base::Vector<float> stops;
  stops.push_back(0.0f);
  stops.push_back(1.0f);
  list.AddLinearGradient(90.0f, colors, stops, 0, 99, 1, 1);

  AppendRecordBox(list, 5.0f, 6.0f, 10.0f, 12.0f);
  AppendFill(list, 0xFF00FF00, 1);

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
  AppendRecordBox(list, 0.0f, 0.0f, 100.0f, 100.0f);
  float radii[] = {4.0f, 4.0f, 8.0f, 8.0f, 12.0f, 12.0f, 16.0f, 16.0f};
  AppendRecordBox(list, 10.0f, 12.0f, 80.0f, 60.0f, true, radii);

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

- (void)testProcessContentOperationsWithRadialGradient {
  LynxMockView *view = [[LynxMockView alloc] initWithFrame:CGRectMake(0, 0, 200, 200)];
  LynxDisplayListApplier *applier = [[LynxDisplayListApplier alloc] initWithView:view
                                                                      andContext:nil];
  DisplayList list;
  AppendRecordBox(list, 0.0f, 0.0f, 100.0f, 100.0f);
  float radii[] = {4.0f, 4.0f, 8.0f, 8.0f, 12.0f, 12.0f, 16.0f, 16.0f};
  AppendRecordBox(list, 10.0f, 12.0f, 80.0f, 60.0f, true, radii);
  lynx::base::Vector<uint32_t> colors{0xFFFF0000, 0xFF0000FF};
  lynx::base::Vector<float> stops{0.0f, 1.0f};
  list.AddRadialGradient(50.0f, 50.0f, 70.0f, 70.0f, colors, stops, 0, 1, 1, 1);

  [applier applyDisplayList:&list];

  XCTAssertEqual(view.layer.sublayers.count, 1u);
  CALayer *gradientLayer = view.layer.sublayers.firstObject;
  XCTAssertTrue(CGRectEqualToRect(gradientLayer.frame, CGRectMake(10, 12, 80, 60)));
  XCTAssertNotNil(gradientLayer.mask);
  CALayer *horizontalLayer = gradientLayer.sublayers.firstObject;
  CAGradientLayer *radialLayer = (CAGradientLayer *)horizontalLayer.sublayers.firstObject;
  XCTAssertEqualObjects(radialLayer.type, kCAGradientLayerRadial);
}

- (void)testProcessContentOperationsWithZeroRadiusRadialGradient {
  LynxMockView *view = [[LynxMockView alloc] initWithFrame:CGRectMake(0, 0, 200, 200)];
  LynxDisplayListApplier *applier = [[LynxDisplayListApplier alloc] initWithView:view
                                                                      andContext:nil];
  DisplayList list;
  AppendRecordBox(list, 0.0f, 0.0f, 100.0f, 100.0f);
  AppendRecordBox(list, 0.0f, 0.0f, 100.0f, 100.0f);
  lynx::base::Vector<uint32_t> colors{0xFFFF0000, 0xFF0000FF};
  lynx::base::Vector<float> stops{0.0f, 1.0f};
  list.AddRadialGradient(0.0f, 50.0f, 0.0f, 50.0f, colors, stops, 0, 1, 1, 1);

  [applier applyDisplayList:&list];

  XCTAssertEqual(view.layer.sublayers.count, 1u);
  CALayer *gradientLayer = view.layer.sublayers.firstObject;
  XCTAssertNotNil(gradientLayer);
  CALayer *horizontalLayer = gradientLayer.sublayers.firstObject;
  CAGradientLayer *radialLayer = (CAGradientLayer *)horizontalLayer.sublayers.firstObject;
  XCTAssertEqualObjects(radialLayer.type, kCAGradientLayerRadial);
}

- (void)testProcessContentOperationsWithClipRectPartial {
  // Clip rect not equal to view bounds, should use mask
  id mockUIView = OCMClassMock([LynxMockView class]);
  id mockLayer = OCMClassMock([CALayer class]);
  [[[mockUIView stub] andReturn:mockLayer] layer];

  LynxDisplayListApplier *applier = [[LynxDisplayListApplier alloc] initWithView:mockUIView
                                                                      andContext:nil];

  DisplayList list;
  AppendClipRect(list, 10.0f, 10.0f, 50.0f, 50.0f);

  [applier applyDisplayList:&list];

  XCTAssertTrue(true);  // If it reaches here, no crash
}

- (void)testProcessContentOperationsWithClipRectFullView {
  // Clip rect equal to view bounds, should set cornerRadius and masksToBounds
  LynxMockView *view = [[LynxMockView alloc] initWithFrame:CGRectMake(0, 0, 100, 100)];
  LynxDisplayListApplier *applier = [[LynxDisplayListApplier alloc] initWithView:view
                                                                      andContext:nil];

  DisplayList list;
  AppendClipRect(list, 0.0f, 0.0f, 100.0f, 100.0f);

  [applier applyDisplayList:&list];

  XCTAssertTrue(view.layer.masksToBounds);
}

- (void)testProcessContentOperationsWithClipRectRoundedFullView {
  // Rounded clip rect equal to view bounds, should set cornerRadius
  LynxMockView *view = [[LynxMockView alloc] initWithFrame:CGRectMake(0, 0, 100, 100)];
  LynxDisplayListApplier *applier = [[LynxDisplayListApplier alloc] initWithView:view
                                                                      andContext:nil];

  DisplayList list;
  float radii[] = {10.0f, 10.0f, 10.0f, 10.0f, 10.0f, 10.0f, 10.0f, 10.0f};
  AppendClipRect(list, 0.0f, 0.0f, 100.0f, 100.0f, true, radii);

  [applier applyDisplayList:&list];

  XCTAssertEqualWithAccuracy(view.layer.cornerRadius, 10.0f, 0.001f);
  XCTAssertTrue(view.layer.masksToBounds);
}

- (void)testClipRectUniformRadiusInsetOnScrollViewPreservesPreciseMask {
  // A border-inset clip must retain its exact geometry so scroll-view content does not
  // cover the host decoration layer's rounded border corners.
  LynxMockScrollView *view = [[LynxMockScrollView alloc] initWithFrame:CGRectMake(0, 0, 100, 100)];
  LynxDisplayListApplier *applier = [[LynxDisplayListApplier alloc] initWithView:view
                                                                      andContext:nil];

  DisplayList list;
  float radii[] = {8.0f, 8.0f, 8.0f, 8.0f, 8.0f, 8.0f, 8.0f, 8.0f};
  AppendClipRect(list, 2.0f, 2.0f, 96.0f, 96.0f, true, radii);

  [applier applyDisplayList:&list];

  XCTAssertEqualWithAccuracy(view.layer.cornerRadius, 0.0f, 0.001f);
  XCTAssertFalse(view.layer.masksToBounds);
  XCTAssertTrue([view.layer.mask isKindOfClass:[CAShapeLayer class]]);
  CAShapeLayer *mask = (CAShapeLayer *)view.layer.mask;
  XCTAssertTrue(CGRectEqualToRect(mask.frame, view.layer.bounds));
  XCTAssertTrue(CGRectEqualToRect(CGPathGetBoundingBox(mask.path), CGRectMake(2, 2, 96, 96)));
}

- (void)testClipRectUniformRadiusInsetOnPlainViewUsesMask {
  // Non-scrollable hosts keep the precise border-inset clip via a mask.
  LynxMockView *view = [[LynxMockView alloc] initWithFrame:CGRectMake(0, 0, 100, 100)];
  LynxDisplayListApplier *applier = [[LynxDisplayListApplier alloc] initWithView:view
                                                                      andContext:nil];

  DisplayList list;
  float radii[] = {8.0f, 8.0f, 8.0f, 8.0f, 8.0f, 8.0f, 8.0f, 8.0f};
  AppendClipRect(list, 2.0f, 2.0f, 96.0f, 96.0f, true, radii);

  [applier applyDisplayList:&list];

  XCTAssertEqualWithAccuracy(view.layer.cornerRadius, 0.0f, 0.001f);
  XCTAssertFalse(view.layer.masksToBounds);
  XCTAssertTrue([view.layer.mask isKindOfClass:[CAShapeLayer class]]);
}

- (void)testClipRectNonUniformRadiusOnScrolledScrollViewPinsMaskToViewport {
  // Non-uniform radii still need a CAShapeLayer mask on scrollable hosts. When the
  // display list is applied while the view is already scrolled, the mask frame must
  // cover the current viewport (bounds), not the unscrolled origin.
  LynxMockScrollView *view = [[LynxMockScrollView alloc] initWithFrame:CGRectMake(0, 0, 100, 100)];
  view.contentSize = CGSizeMake(100, 1000);
  view.contentOffset = CGPointMake(0, 100);
  LynxDisplayListApplier *applier = [[LynxDisplayListApplier alloc] initWithView:view
                                                                      andContext:nil];

  DisplayList list;
  float radii[] = {8.0f, 8.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
  AppendClipRect(list, 0.0f, 0.0f, 100.0f, 100.0f, true, radii);

  [applier applyDisplayList:&list];

  XCTAssertTrue([view.layer.mask isKindOfClass:[CAShapeLayer class]]);
  XCTAssertTrue(CGRectEqualToRect(view.layer.mask.frame, view.layer.bounds));
  XCTAssertEqualWithAccuracy(view.layer.mask.frame.origin.y, 100.0f, 0.001f);
}

- (void)testHostBackgroundsShareContainerBelowBorderAndPreserveClip {
  UIView *parent = [[UIView alloc] initWithFrame:CGRectMake(0, 0, 200, 200)];
  LynxMockView *host = [[LynxMockView alloc] initWithFrame:CGRectMake(20, 30, 100, 80)];
  [host createRendererWithSign:1 andContext:nil];
  [parent addSubview:host];
  id context = OCMClassMock([LynxRendererContext class]);
  id manager = OCMClassMock([LynxImageManager class]);
  OCMStub([context imageManagerForID:123]).andReturn(manager);
  LynxDisplayListApplier *applier = [[LynxDisplayListApplier alloc] initWithView:host
                                                                      andContext:context];
  DisplayList list;
  AppendBegin(list, 1, kViewType, 0, 0, 100, 80);
  AppendRecordBox(list, 0, 0, 100, 80);
  AppendRecordBox(list, 10, 10, 80, 60);
  AppendRecordBox(list, 2, 3, 96, 74);
  AppendFill(list, 0xFFFFFFFF, 0);
  DisplayListItem item{};
  item.type = DisplayListOpType::kBackgroundImage;
  item.payload.background_image.image_id = 123;
  item.payload.background_image.clip_index = 2;
  item.payload.background_image.tiling_index = 0;
  item.payload.background_image.repeat_x = LynxBackgroundRepeatNoRepeat;
  item.payload.background_image.repeat_y = LynxBackgroundRepeatNoRepeat;
  list.AppendItem(item);
  list.AddLinearGradient(90, {0xFFFF0000, 0xFF0000FF}, {0, 1}, 0, 1, LynxBackgroundRepeatNoRepeat,
                         LynxBackgroundRepeatNoRepeat);
  list.AddRadialGradient(50, 40, 50, 40, {0xFFFFFFFF, 0xFF000000}, {0, 1}, 0, 2,
                         LynxBackgroundRepeatNoRepeat, LynxBackgroundRepeatNoRepeat);
  AppendBorder(list, 0, 1);
  AppendEnd(list);
  [applier applyDisplayList:&list];
  [applier syncHostDecorationLayers];

  NSArray<CALayer *> *layers = parent.layer.sublayers;
  XCTAssertEqual(layers.count, 4u);
  XCTAssertEqual(layers.lastObject, host.layer);
  CALayer *background = layers[1];
  XCTAssertTrue(CGRectEqualToRect(background.frame, host.frame));
  XCTAssertEqual(background.sublayers.count, 3u);
  XCTAssertTrue(CGRectEqualToRect(background.sublayers[1].frame, CGRectMake(10, 10, 80, 60)));
  XCTAssertTrue(CGRectEqualToRect(background.sublayers[2].frame, CGRectMake(2, 3, 96, 74)));
  CALayer *clip = background.sublayers.firstObject;
  XCTAssertTrue([clip isKindOfClass:CAReplicatorLayer.class]);
  XCTAssertTrue(CGRectEqualToRect(clip.frame, CGRectMake(2, 3, 96, 74)));
  XCTAssertTrue(clip.masksToBounds);
  XCTAssertEqual(host.layer.sublayers.count, 0u);

  // Simulate scrolling followed by an opacity update on the host.
  host.bounds = CGRectMake(7, 40, 100, 80);
  host.alpha = 0.8;
  [applier syncHostDecorationLayers];
  XCTAssertTrue(CGRectEqualToRect(background.bounds, CGRectMake(0, 0, 100, 80)));
  XCTAssertTrue(CGRectEqualToRect([clip convertRect:clip.bounds toLayer:parent.layer],
                                  CGRectMake(22, 33, 96, 74)));
  XCTAssertEqualWithAccuracy(background.opacity, 0.8, 0.001);

  [applier applyDisplayList:&list];
  [applier syncHostDecorationLayers];
  XCTAssertNil(background.superlayer);
  XCTAssertEqual(parent.layer.sublayers.count, 4u);
  XCTAssertEqual(parent.layer.sublayers[1].sublayers.count, 3u);
  CALayer *repaintedBackground = parent.layer.sublayers[1];
  CALayer *repaintedClip = repaintedBackground.sublayers.firstObject;
  XCTAssertTrue(CGRectEqualToRect(repaintedBackground.bounds, CGRectMake(0, 0, 100, 80)));
  XCTAssertTrue(CGRectEqualToRect([repaintedClip convertRect:repaintedClip.bounds
                                                     toLayer:parent.layer],
                                  CGRectMake(22, 33, 96, 74)));

  [applier reset];
  XCTAssertEqual(parent.layer.sublayers.count, 1u);
  XCTAssertNil(background.superlayer);
}

- (void)testBackgroundImageRepeatDirectionsAndReset {
  for (BOOL repeatX : {NO, YES}) {
    for (BOOL repeatY : {NO, YES}) {
      LynxMockView *view = [[LynxMockView alloc] initWithFrame:CGRectMake(0, 0, 100, 80)];
      view.contentScaleFactor = 2;
      id context = OCMClassMock([LynxRendererContext class]);
      id manager = OCMClassMock([LynxImageManager class]);
      OCMStub([context imageManagerForID:123]).andReturn(manager);
      __block CALayer *target = nil;
      OCMStub([manager setLayerTarget:[OCMArg any]]).andDo(^(NSInvocation *invocation) {
        __unsafe_unretained CALayer *imageLayer;
        [invocation getArgument:&imageLayer atIndex:2];
        target = imageLayer;
      });
      LynxDisplayListApplier *applier = [[LynxDisplayListApplier alloc] initWithView:view
                                                                          andContext:context];
      DisplayList list;
      float radii[] = {6, 6, 6, 6, 6, 6, 6, 6};
      AppendRecordBox(list, 2.25f, 3.25f, 88, 64, true, radii);
      AppendRecordBox(list, 10.3f, -3.7f, 15.3f, 11.7f);
      DisplayListItem item{};
      item.type = DisplayListOpType::kBackgroundImage;
      item.payload.background_image.image_id = 123;
      item.payload.background_image.clip_index = 0;
      item.payload.background_image.tiling_index = 1;
      item.payload.background_image.repeat_x =
          repeatX ? LynxBackgroundRepeatRepeat : LynxBackgroundRepeatNoRepeat;
      item.payload.background_image.repeat_y =
          repeatY ? LynxBackgroundRepeatRepeat : LynxBackgroundRepeatNoRepeat;
      list.AppendItem(item);
      [applier applyDisplayList:&list];

      CAReplicatorLayer *vertical = (CAReplicatorLayer *)view.layer.sublayers.firstObject;
      CAReplicatorLayer *horizontal = (CAReplicatorLayer *)vertical.sublayers.firstObject;
      XCTAssertNotNil(target);
      XCTAssertEqual(horizontal.sublayers.firstObject, target);
      XCTAssertTrue(CGRectEqualToRect(vertical.frame, CGRectMake(2.25, 3.25, 88, 64)));
      XCTAssertTrue(vertical.masksToBounds);
      XCTAssertEqualWithAccuracy(vertical.cornerRadius, 6, 0.001);
      XCTAssertEqualWithAccuracy(target.frame.size.width, repeatX ? 15.5 : 15.3, 0.001);
      XCTAssertEqualWithAccuracy(target.frame.size.height, repeatY ? 11.5 : 11.7, 0.001);
      XCTAssertEqualWithAccuracy(target.frame.origin.x, repeatX ? -7.25 : 8.05, 0.001);
      XCTAssertEqualWithAccuracy(target.frame.origin.y, repeatY ? -6.75 : -6.95, 0.001);
      XCTAssertEqual(horizontal.instanceCount, repeatX ? 7 : 1);
      XCTAssertEqual(vertical.instanceCount, repeatY ? 7 : 1);
      XCTAssertEqualWithAccuracy(horizontal.instanceTransform.m41, repeatX ? 15.5 : 15.3, 0.001);
      XCTAssertEqualWithAccuracy(vertical.instanceTransform.m42, repeatY ? 11.5 : 11.7, 0.001);

      [applier applyDisplayList:&list];
      XCTAssertNil(vertical.superlayer);
      XCTAssertEqual(view.layer.sublayers.count, 1u);
      [applier reset];
      XCTAssertEqual(view.layer.sublayers.count, 0u);
    }
  }
}

- (void)testBackgroundImageAutoSizeOnLoad {
  for (int32_t autoSize : {1, 2, 3}) {
    LynxMockView *view = [[LynxMockView alloc] initWithFrame:CGRectMake(0, 0, 100, 80)];
    view.contentScaleFactor = 2;
    id context = OCMClassMock([LynxRendererContext class]);
    LynxImageManager *manager = [[LynxImageManager alloc] initWithContext:nil];
    OCMStub([context imageManagerForID:123]).andReturn(manager);
    LynxDisplayListApplier *applier = [[LynxDisplayListApplier alloc] initWithView:view
                                                                        andContext:context];
    DisplayList list;
    AppendRecordBox(list, 0, 0, 100, 80);
    DisplayListItem item{};
    item.type = DisplayListOpType::kBackgroundImage;
    item.payload.background_image.image_id = 123;
    item.payload.background_image.auto_size = autoSize;
    item.payload.background_image.position_x = 0.5;
    item.payload.background_image.position_y = 0.5;
    item.payload.background_image.repeat_x = LynxBackgroundRepeatRepeat;
    item.payload.background_image.repeat_y = LynxBackgroundRepeatRepeat;
    list.AppendItem(item);
    [applier applyDisplayList:&list];
    XCTAssertTrue(view.layer.sublayers.firstObject.hidden);
    UIGraphicsBeginImageContextWithOptions(CGSizeMake(40, 20), NO, 2);
    UIImage *image = UIGraphicsGetImageFromCurrentImageContext();
    UIGraphicsEndImageContext();
    NSMutableDictionary *images = [manager valueForKey:@"images"];
    images[@(LynxImageRequestSrc)] = image;
    [manager applyImage:image withType:LynxImageRequestSrc];
    CAReplicatorLayer *vertical = (CAReplicatorLayer *)view.layer.sublayers.firstObject;
    CAReplicatorLayer *horizontal = (CAReplicatorLayer *)vertical.sublayers.firstObject;
    CALayer *target = horizontal.sublayers.firstObject;
    XCTAssertFalse(vertical.hidden);
    CGFloat width = autoSize == 1 ? 160 : (autoSize == 2 ? 100 : 40);
    CGFloat height = autoSize == 1 ? 80 : (autoSize == 2 ? 50 : 20);
    XCTAssertEqualWithAccuracy(target.frame.size.width, width, 0.001);
    XCTAssertEqualWithAccuracy(target.frame.size.height, height, 0.001);
    if (autoSize == 3) {
      XCTAssertEqualWithAccuracy(target.frame.origin.x, -10, 0.001);
      XCTAssertEqualWithAccuracy(target.frame.origin.y, -10, 0.001);
      XCTAssertEqual(horizontal.instanceCount, 3);
      XCTAssertEqual(vertical.instanceCount, 5);
    }
    [applier reset];
    [manager applyImage:image withType:LynxImageRequestSrc];
    XCTAssertEqual(view.layer.sublayers.count, 0u);
    // Rebinding a new target must immediately apply the cached intrinsic size.
    [applier applyDisplayList:&list];
    CALayer *cachedTarget =
        view.layer.sublayers.firstObject.sublayers.firstObject.sublayers.firstObject;
    XCTAssertEqualWithAccuracy(cachedTarget.frame.size.width, width, 0.001);
    XCTAssertEqualWithAccuracy(cachedTarget.frame.size.height, height, 0.001);
    [applier reset];
  }
}

- (void)testImageAppliesRoundedContentBox {
  LynxMockView *view = [[LynxMockView alloc] initWithFrame:CGRectMake(0, 0, 100, 100)];
  LynxDisplayListApplier *applier = [[LynxDisplayListApplier alloc] initWithView:view
                                                                      andContext:nil];

  DisplayList list;
  float radii[] = {6.0f, 6.0f, 6.0f, 6.0f, 6.0f, 6.0f, 6.0f, 6.0f};
  AppendRecordBox(list, 10.0f, 12.0f, 40.0f, 50.0f, true, radii);
  AppendImage(list, 123, 0);

  [applier applyDisplayList:&list];

  UIImageView *imageView = (UIImageView *)view.subviews.firstObject;
  XCTAssertNotNil(imageView);
  XCTAssertTrue(CGRectEqualToRect(imageView.frame, CGRectMake(10.0f, 12.0f, 40.0f, 50.0f)));
  XCTAssertEqualWithAccuracy(imageView.layer.cornerRadius, 6.0f, 0.001f);
  XCTAssertTrue(imageView.layer.masksToBounds);
}

- (void)testImageAppliesNonUniformRoundedContentBox {
  LynxMockView *view = [[LynxMockView alloc] initWithFrame:CGRectMake(0, 0, 100, 100)];
  LynxDisplayListApplier *applier = [[LynxDisplayListApplier alloc] initWithView:view
                                                                      andContext:nil];

  DisplayList list;
  float radii[] = {2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f};
  AppendRecordBox(list, 10.0f, 12.0f, 40.0f, 50.0f, true, radii);
  AppendImage(list, 123, 0);

  [applier applyDisplayList:&list];

  UIImageView *imageView = (UIImageView *)view.subviews.firstObject;
  XCTAssertNotNil(imageView);
  XCTAssertEqualWithAccuracy(imageView.layer.cornerRadius, 0.0f, 0.001f);
  XCTAssertFalse(imageView.layer.masksToBounds);
  XCTAssertNotNil(imageView.layer.mask);
  XCTAssertTrue([imageView.layer.mask isKindOfClass:[CAShapeLayer class]]);
  XCTAssertTrue(((CAShapeLayer *)imageView.layer.mask).path != NULL);
}

- (void)testImageManagerAppliesFitModeToTarget {
  NSDictionary<NSNumber *, NSNumber *> *expectations = @{
    @(static_cast<int32_t>(ImageFitMode::kScaleToFill)) : @(UIViewContentModeScaleToFill),
    @(static_cast<int32_t>(ImageFitMode::kAspectFit)) : @(UIViewContentModeScaleAspectFit),
    @(static_cast<int32_t>(ImageFitMode::kAspectFill)) : @(UIViewContentModeScaleAspectFill),
    @(static_cast<int32_t>(ImageFitMode::kCenter)) : @(UIViewContentModeCenter),
  };

  for (NSNumber *mode in expectations) {
    LynxImageManager *imageManager = [[LynxImageManager alloc] initWithContext:nil];
    ImagePaintInfo paintInfo;
    paintInfo.mode = static_cast<ImageFitMode>(mode.intValue);
    [imageManager updatePaintInfo:paintInfo];
    UIImageView *imageView = [UIImageView new];
    [imageManager setTarget:imageView];
    XCTAssertEqual(imageView.contentMode,
                   static_cast<UIViewContentMode>(expectations[mode].integerValue));
  }

  LynxImageManager *defaultManager = [[LynxImageManager alloc] initWithContext:nil];
  UIImageView *defaultView = [UIImageView new];
  [defaultManager setTarget:defaultView];
  XCTAssertEqual(defaultView.contentMode, UIViewContentModeScaleToFill);

  LynxImageManager *invalidManager = [[LynxImageManager alloc] initWithContext:nil];
  ImagePaintInfo invalidPaintInfo;
  invalidPaintInfo.mode = static_cast<ImageFitMode>(-1);
  [invalidManager updatePaintInfo:invalidPaintInfo];
  UIImageView *invalidView = [UIImageView new];
  [invalidManager setTarget:invalidView];
  XCTAssertEqual(invalidView.contentMode, UIViewContentModeScaleToFill);
}

- (void)testProcessContentOperationsWithBorder {
  LynxMockView *view = [[LynxMockView alloc] initWithFrame:CGRectMake(0, 0, 100, 100)];
  LynxDisplayListApplier *applier = [[LynxDisplayListApplier alloc] initWithView:view
                                                                      andContext:nil];

  DisplayList list;
  AppendRecordBox(list, 0.0f, 0.0f, 100.0f, 100.0f);
  AppendRecordBox(list, 5.0f, 5.0f, 90.0f, 90.0f);
  AppendBorder(list, 0, 1);

  [applier applyDisplayList:&list];

  XCTAssertEqual(view.layer.sublayers.count, 1u);
  CALayer *borderLayer = view.layer.sublayers.firstObject;
  XCTAssertFalse([borderLayer isKindOfClass:[CAShapeLayer class]]);
  XCTAssertEqualWithAccuracy(borderLayer.borderWidth, 5.0, 0.001);
  XCTAssertTrue(CGColorEqualToColor(borderLayer.borderColor,
                                    [UIColor colorWithRed:0 green:0 blue:0 alpha:1].CGColor));
  XCTAssertNil(borderLayer.contents);
}

- (void)testBorderWithInvalidOuterDimensionsDoesNotCreateLayer {
  struct InvalidSize {
    float width;
    float height;
  };
  const InvalidSize invalidSizes[] = {
      {0.0f, 100.0f}, {100.0f, 0.0f}, {-1.0f, 100.0f}, {100.0f, -1.0f}};

  for (const InvalidSize &size : invalidSizes) {
    LynxMockView *view = [[LynxMockView alloc] initWithFrame:CGRectMake(0, 0, 100, 100)];
    LynxDisplayListApplier *applier = [[LynxDisplayListApplier alloc] initWithView:view
                                                                        andContext:nil];

    DisplayList list;
    AppendRecordBox(list, 0.0f, 0.0f, size.width, size.height);
    AppendRecordBox(list, 0.0f, 0.0f, size.width, size.height);
    AppendBorder(list, 0, 1);

    [applier applyDisplayList:&list];

    XCTAssertEqual(view.layer.sublayers.count, 0u, @"width=%f height=%f", size.width, size.height);
  }
}

- (void)testSolidBorderWithNonUniformGeometryUsesShapeLayer {
  LynxMockView *view = [[LynxMockView alloc] initWithFrame:CGRectMake(0, 0, 100, 100)];
  LynxDisplayListApplier *applier = [[LynxDisplayListApplier alloc] initWithView:view
                                                                      andContext:nil];

  const float outerRadii[8] = {12.0f, 12.0f, 8.0f, 8.0f, 4.0f, 4.0f, 0.0f, 0.0f};
  const float innerRadii[8] = {10.0f, 10.0f, 6.0f, 6.0f, 2.0f, 2.0f, 0.0f, 0.0f};
  DisplayList list;
  AppendRecordBox(list, 0.0f, 0.0f, 100.0f, 100.0f, true, outerRadii);
  AppendRecordBox(list, 1.0f, 2.0f, 96.0f, 95.0f, true, innerRadii);
  AppendBorder(list, 0, 1);

  [applier applyDisplayList:&list];

  XCTAssertEqual(view.layer.sublayers.count, 1u);
  CAShapeLayer *borderLayer = (CAShapeLayer *)view.layer.sublayers.firstObject;
  XCTAssertTrue([borderLayer isKindOfClass:[CAShapeLayer class]]);
  XCTAssertEqualObjects(borderLayer.fillRule, kCAFillRuleEvenOdd);
  XCTAssertTrue(borderLayer.path != NULL);
  XCTAssertTrue(CGColorEqualToColor(borderLayer.fillColor,
                                    [UIColor colorWithRed:0 green:0 blue:0 alpha:1].CGColor));
  XCTAssertNil(borderLayer.contents);
}

- (void)testSolidBorderWithCompatibleUniformRadiusUsesLayerBorder {
  LynxMockView *view = [[LynxMockView alloc] initWithFrame:CGRectMake(0, 0, 100, 100)];
  LynxDisplayListApplier *applier = [[LynxDisplayListApplier alloc] initWithView:view
                                                                      andContext:nil];

  const float outerRadii[8] = {12.0f, 12.0f, 12.0f, 12.0f, 12.0f, 12.0f, 12.0f, 12.0f};
  const float innerRadii[8] = {7.0f, 7.0f, 7.0f, 7.0f, 7.0f, 7.0f, 7.0f, 7.0f};
  DisplayList list;
  AppendRecordBox(list, 0.0f, 0.0f, 100.0f, 100.0f, true, outerRadii);
  AppendRecordBox(list, 5.0f, 5.0f, 90.0f, 90.0f, true, innerRadii);
  AppendBorder(list, 0, 1);

  [applier applyDisplayList:&list];

  XCTAssertEqual(view.layer.sublayers.count, 1u);
  CALayer *borderLayer = view.layer.sublayers.firstObject;
  XCTAssertFalse([borderLayer isKindOfClass:[CAShapeLayer class]]);
  XCTAssertEqualWithAccuracy(borderLayer.cornerRadius, 12.0, 0.001);
  XCTAssertEqualWithAccuracy(borderLayer.borderWidth, 5.0, 0.001);
  XCTAssertNil(borderLayer.contents);
}

- (void)testFractionalUniformBorderAndRadiusUsesLayerBorder {
  LynxMockView *view = [[LynxMockView alloc] initWithFrame:CGRectMake(0, 0, 100, 100)];
  LynxDisplayListApplier *applier = [[LynxDisplayListApplier alloc] initWithView:view
                                                                      andContext:nil];

  const float borderWidth = 0.1f;
  const float outerRadius = 10.1f;
  const float innerRadius = outerRadius - borderWidth;
  const float innerSize = 100.0f - borderWidth - borderWidth;
  const float outerRadii[8] = {outerRadius, outerRadius, outerRadius, outerRadius,
                               outerRadius, outerRadius, outerRadius, outerRadius};
  const float innerRadii[8] = {innerRadius, innerRadius, innerRadius, innerRadius,
                               innerRadius, innerRadius, innerRadius, innerRadius};
  DisplayList list;
  AppendRecordBox(list, 0.0f, 0.0f, 100.0f, 100.0f, true, outerRadii);
  AppendRecordBox(list, borderWidth, borderWidth, innerSize, innerSize, true, innerRadii);
  AppendBorder(list, 0, 1);

  [applier applyDisplayList:&list];

  XCTAssertEqual(view.layer.sublayers.count, 1u);
  CALayer *borderLayer = view.layer.sublayers.firstObject;
  XCTAssertFalse([borderLayer isKindOfClass:[CAShapeLayer class]]);
  XCTAssertEqualWithAccuracy(borderLayer.cornerRadius, outerRadius, 0.001);
  XCTAssertEqualWithAccuracy(borderLayer.borderWidth, borderWidth, 0.001);
  XCTAssertNil(borderLayer.contents);
}

- (void)testSolidBorderWithMismatchedInnerRadiusUsesShapeLayer {
  LynxMockView *view = [[LynxMockView alloc] initWithFrame:CGRectMake(0, 0, 100, 100)];
  LynxDisplayListApplier *applier = [[LynxDisplayListApplier alloc] initWithView:view
                                                                      andContext:nil];

  const float outerRadii[8] = {12.0f, 12.0f, 12.0f, 12.0f, 12.0f, 12.0f, 12.0f, 12.0f};
  const float innerRadii[8] = {2.0f, 2.0f, 2.0f, 2.0f, 2.0f, 2.0f, 2.0f, 2.0f};
  DisplayList list;
  AppendRecordBox(list, 0.0f, 0.0f, 100.0f, 100.0f, true, outerRadii);
  AppendRecordBox(list, 5.0f, 5.0f, 90.0f, 90.0f, true, innerRadii);
  AppendBorder(list, 0, 1);

  [applier applyDisplayList:&list];

  XCTAssertEqual(view.layer.sublayers.count, 1u);
  CALayer *borderLayer = view.layer.sublayers.firstObject;
  XCTAssertTrue([borderLayer isKindOfClass:[CAShapeLayer class]]);
  XCTAssertNil(borderLayer.contents);
}

- (void)testNonUniformBorderColorsFallBackToRasterImage {
  LynxMockView *view = [[LynxMockView alloc] initWithFrame:CGRectMake(0, 0, 100, 100)];
  LynxDisplayListApplier *applier = [[LynxDisplayListApplier alloc] initWithView:view
                                                                      andContext:nil];

  const uint32_t colors[4] = {0xFFFF0000, 0xFF00FF00, 0xFF0000FF, 0xFF000000};
  DisplayList list;
  AppendRecordBox(list, 0.0f, 0.0f, 100.0f, 100.0f);
  AppendRecordBox(list, 5.0f, 5.0f, 90.0f, 90.0f);
  AppendBorder(list, 0, 1, colors);

  [applier applyDisplayList:&list];

  XCTAssertEqual(view.layer.sublayers.count, 1u);
  CALayer *borderLayer = view.layer.sublayers.firstObject;
  XCTAssertFalse([borderLayer isKindOfClass:[CAShapeLayer class]]);
  XCTAssertNotNil(borderLayer.contents);
}

- (void)testProcessContentOperationsWithText {
  LynxMockView *view = [[LynxMockView alloc] initWithFrame:CGRectMake(0, 0, 100, 100)];
  LynxDisplayListApplier *applier = [[LynxDisplayListApplier alloc] initWithView:view
                                                                      andContext:nil];

  DisplayList list;
  AppendRecordBox(list, 0.0f, 0.0f, 100.0f, 100.0f);
  AppendText(list, 0, 0);

  [applier applyDisplayList:&list];
  // Verify no crash
  XCTAssertTrue(true);
}

- (void)testProcessContentOperationsWithImage {
  LynxMockView *view = [[LynxMockView alloc] initWithFrame:CGRectMake(0, 0, 100, 100)];
  LynxDisplayListApplier *applier = [[LynxDisplayListApplier alloc] initWithView:view
                                                                      andContext:nil];

  DisplayList list;
  AppendRecordBox(list, 0.0f, 0.0f, 100.0f, 100.0f);
  AppendImage(list, 0, 0);

  [applier applyDisplayList:&list];
  // Verify no crash
  XCTAssertTrue(true);
}

- (void)testResetClearsState {
  LynxMockView *view = [[LynxMockView alloc] initWithFrame:CGRectMake(0, 0, 100, 100)];
  LynxDisplayListApplier *applier = [[LynxDisplayListApplier alloc] initWithView:view
                                                                      andContext:nil];

  DisplayList list1;
  AppendRecordBox(list1, 0.0f, 0.0f, 100.0f, 100.0f);
  AppendFill(list1, 0xFF0000FF, 0);
  [applier applyDisplayList:&list1];

  XCTAssertEqual(view.layer.sublayers.count, 1u);

  [applier reset];

  DisplayList list2;
  AppendFill(list2, 0xFF00FF00, 0);
  [applier applyDisplayList:&list2];

  // After reset, the old layers should be removed and new ones added
  XCTAssertEqual(view.layer.sublayers.count, 1u);
}

- (void)testFillRecreatesLayerAndMaskWhenPositionAndColorChange {
  LynxMockView *view = [[LynxMockView alloc] initWithFrame:CGRectMake(0, 0, 100, 100)];
  LynxDisplayListApplier *applier = [[LynxDisplayListApplier alloc] initWithView:view
                                                                      andContext:nil];
  float radii[] = {3, 4, 5, 6, 7, 8, 9, 10};
  DisplayList first;
  AppendRecordBox(first, 1, 2, 30, 40, true, radii);
  AppendFill(first, 0xFFFF0000, 0);
  [applier applyDisplayList:&first];
  CALayer *layer = view.layer.sublayers.firstObject;
  CALayer *mask = layer.mask;
  XCTAssertNotNil(mask);

  DisplayList second;
  AppendRecordBox(second, 5, 6, 30, 40, true, radii);
  AppendFill(second, 0xFF00FF00, 0);
  [applier applyDisplayList:&second];

  XCTAssertEqual(view.layer.sublayers.count, 1u);
  CALayer *newLayer = view.layer.sublayers.firstObject;
  XCTAssertNotEqual(newLayer, layer);
  XCTAssertNil(layer.superlayer);
  XCTAssertNotEqual(newLayer.mask, mask);
  XCTAssertTrue(CGRectEqualToRect(newLayer.frame, CGRectMake(5, 6, 30, 40)));
  XCTAssertTrue(CGColorEqualToColor(newLayer.backgroundColor, UIColor.greenColor.CGColor));
  XCTAssertEqual(newLayer.animationKeys.count, 0u);
  [applier applyDisplayList:&second];
  XCTAssertNil(newLayer.superlayer);
  XCTAssertNotEqual(view.layer.sublayers.firstObject, newLayer);
}

- (void)testFillUpdatesReferencedBoxAndClearsOldRoundedMask {
  LynxMockView *view = [[LynxMockView alloc] initWithFrame:CGRectMake(0, 0, 100, 100)];
  LynxDisplayListApplier *applier = [[LynxDisplayListApplier alloc] initWithView:view
                                                                      andContext:nil];
  float radii[] = {3, 4, 5, 6, 7, 8, 9, 10};
  DisplayList first;
  AppendRecordBox(first, 0, 0, 30, 40, true, radii);
  AppendFill(first, 0xFFFF0000, 0);
  [applier applyDisplayList:&first];
  CALayer *layer = view.layer.sublayers.firstObject;

  DisplayList second;
  AppendRecordBox(second, 5, 6, 50, 60);
  AppendFill(second, 0xFFFF0000, 0);
  [applier applyDisplayList:&second];

  CALayer *newLayer = view.layer.sublayers.firstObject;
  XCTAssertNotEqual(newLayer, layer);
  XCTAssertNil(layer.superlayer);
  XCTAssertTrue(CGRectEqualToRect(newLayer.frame, CGRectMake(5, 6, 50, 60)));
  XCTAssertNil(newLayer.mask);
  XCTAssertEqual(newLayer.cornerRadius, 0);
  XCTAssertFalse(newLayer.masksToBounds);
}

- (void)testFillRebuildsMaskWhenLocalSizeChanges {
  LynxMockView *view = [[LynxMockView alloc] initWithFrame:CGRectMake(0, 0, 100, 100)];
  LynxDisplayListApplier *applier = [[LynxDisplayListApplier alloc] initWithView:view
                                                                      andContext:nil];
  float radii[] = {3, 4, 5, 6, 7, 8, 9, 10};
  DisplayList first;
  AppendRecordBox(first, 0, 0, 30, 40, true, radii);
  AppendFill(first, 0xFFFF0000, 0);
  [applier applyDisplayList:&first];
  CALayer *layer = view.layer.sublayers.firstObject;
  CALayer *mask = layer.mask;

  DisplayList second;
  AppendRecordBox(second, 0, 0, 50, 60, true, radii);
  AppendFill(second, 0xFFFF0000, 0);
  [applier applyDisplayList:&second];

  CALayer *newLayer = view.layer.sublayers.firstObject;
  XCTAssertNotEqual(newLayer, layer);
  XCTAssertNil(layer.superlayer);
  XCTAssertNotEqual(newLayer.mask, mask);
  XCTAssertTrue(CGRectEqualToRect(newLayer.mask.frame, CGRectMake(0, 0, 50, 60)));
}

- (void)testFillStructureChangeAndExplicitResetRemoveRetainedLayers {
  LynxMockView *view = [[LynxMockView alloc] initWithFrame:CGRectMake(0, 0, 100, 100)];
  LynxDisplayListApplier *applier = [[LynxDisplayListApplier alloc] initWithView:view
                                                                      andContext:nil];
  DisplayList first;
  AppendRecordBox(first, 0, 0, 30, 40);
  AppendFill(first, 0xFFFF0000, 0);
  [applier applyDisplayList:&first];
  CALayer *oldLayer = view.layer.sublayers.firstObject;

  DisplayList second;
  AppendRecordBox(second, 0, 0, 30, 40);
  AppendFill(second, 0xFFFF0000, 0);
  AppendFill(second, 0xFF00FF00, 0);
  [applier applyDisplayList:&second];
  XCTAssertNil(oldLayer.superlayer);
  XCTAssertEqual(view.layer.sublayers.count, 2u);

  NSArray<CALayer *> *layers = [view.layer.sublayers copy];
  [applier reset];
  XCTAssertEqual(view.layer.sublayers.count, 0u);
  [applier applyDisplayList:&second];
  for (CALayer *layer in layers) {
    XCTAssertNil(layer.superlayer);
    XCTAssertFalse([view.layer.sublayers containsObject:layer]);
  }
}

- (void)testFillDoesNotReuseWithMissingTextResourcesOrBoxes {
  LynxMockView *view = [[LynxMockView alloc] initWithFrame:CGRectMake(0, 0, 100, 100)];
  LynxDisplayListApplier *applier = [[LynxDisplayListApplier alloc] initWithView:view
                                                                      andContext:nil];
  DisplayList list;
  AppendRecordBox(list, 0, 0, 30, 40);
  AppendFill(list, 0xFFFF0000, 0);
  AppendText(list, 123, 0);
  [applier applyDisplayList:&list];
  CALayer *layer = view.layer.sublayers.firstObject;
  [applier applyDisplayList:&list];
  XCTAssertNil(layer.superlayer);
  XCTAssertNotEqual(view.layer.sublayers.firstObject, layer);

  DisplayList invalid;
  AppendRecordBox(invalid, 0, 0, 30, 40);
  AppendFill(invalid, 0xFFFF0000, 99);
  [applier applyDisplayList:&invalid];
  layer = view.layer.sublayers.firstObject;
  [applier applyDisplayList:&invalid];
  XCTAssertNil(layer.superlayer);
}

- (void)testFillRebuildsHostDecorationWhileTextAndNativeViewsKeepPaintOrder {
  UIView *parent = [[UIView alloc] initWithFrame:CGRectMake(0, 0, 100, 100)];
  LynxMockView *host = [[LynxMockView alloc] initWithFrame:parent.bounds];
  [host createRendererWithSign:1 andContext:nil];
  [parent addSubview:host];
  LynxMockView *child = [[LynxMockView alloc] initWithFrame:CGRectMake(0, 0, 10, 10)];
  [child createRendererWithSign:2 andContext:nil];
  [host addSubview:child];
  LynxRendererContext *context = [LynxRendererContext new];
  UpdateMeasuredText(context, @"text");
  LynxDisplayListApplier *applier = [[LynxDisplayListApplier alloc] initWithView:host
                                                                      andContext:context];
  DisplayList list;
  AppendBegin(list, 1, kViewType, 0, 0, 100, 100);
  AppendRecordBox(list, 0, 0, 100, 100);
  AppendFill(list, 0xFFFF0000, 0);
  AppendBegin(list, 3, kViewType, 4, 5, 30, 40);
  AppendRecordBox(list, 0, 0, 30, 40);
  AppendFill(list, 0xFF00FF00, 1);
  AppendDrawView(list, 2, 4, 5);
  AppendFill(list, 0xFF0000FF, 1);
  AppendText(list, 1, 1);
  AppendEnd(list);
  AppendEnd(list);
  [applier applyDisplayList:&list];
  NSArray<CALayer *> *content = [host.layer.sublayers copy];
  NSArray<CALayer *> *decorations = [parent.layer.sublayers copy];
  [applier applyDisplayList:&list];
  XCTAssertEqual(host.layer.sublayers.count, content.count);
  XCTAssertEqual(host.layer.sublayers[1], child.layer);
  XCTAssertEqual(host.layer.sublayers.lastObject, content.lastObject);
  XCTAssertNil(content[0].superlayer);
  XCTAssertNil(content[2].superlayer);
  XCTAssertNil(decorations.firstObject.superlayer);
  XCTAssertEqual(parent.layer.sublayers.count, decorations.count);
  XCTAssertEqual(content[1], child.layer);
  XCTAssertTrue(CGRectEqualToRect(host.layer.sublayers[0].frame, CGRectMake(4, 5, 30, 40)));

  [applier onRebuildSubRenderers];
  [applier applyDisplayList:&list];
  XCTAssertNotEqual(host.layer.sublayers.firstObject, content.firstObject);
  XCTAssertNil(decorations.firstObject.superlayer);
}

- (void)testFillDoesNotReuseDetachedLayers {
  LynxMockView *view = [[LynxMockView alloc] initWithFrame:CGRectMake(0, 0, 100, 100)];
  LynxDisplayListApplier *applier = [[LynxDisplayListApplier alloc] initWithView:view
                                                                      andContext:nil];
  DisplayList list;
  AppendRecordBox(list, 0, 0, 30, 40);
  AppendFill(list, 0xFFFF0000, 0);
  [applier applyDisplayList:&list];
  CALayer *layer = view.layer.sublayers.firstObject;
  [layer removeFromSuperlayer];
  [applier applyDisplayList:&list];
  XCTAssertEqual(view.layer.sublayers.count, 1u);
  XCTAssertNotEqual(view.layer.sublayers.firstObject, layer);
}

- (NSData *)renderedPixelsForView:(UIView *)view {
  const size_t width = 100;
  const size_t height = 100;
  CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
  CGContextRef context = CGBitmapContextCreate(nullptr, width, height, 8, width * 4, colorSpace,
                                               kCGImageAlphaPremultipliedLast);
  CGColorSpaceRelease(colorSpace);
  [view.layer renderInContext:context];
  NSData *pixels = [NSData dataWithBytes:CGBitmapContextGetData(context) length:width * height * 4];
  CGContextRelease(context);
  return pixels;
}

- (void)testMixedResourcePixelsMatchFullRebuild {
  LynxMockView *retainedView = [[LynxMockView alloc] initWithFrame:CGRectMake(0, 0, 100, 100)];
  LynxMockView *rebuiltView = [[LynxMockView alloc] initWithFrame:retainedView.frame];
  LynxRendererContext *context = [LynxRendererContext new];
  UpdateMeasuredText(context, @"text");
  ImagePaintInfo paintInfo;
  [context createImageManager:2
                withSourceURL:nil
            andPlaceholderURL:nil
                    paintInfo:paintInfo
                    eventMask:0
                     imageKey:2];
  NSMutableDictionary *images = [[context imageManagerForID:2] valueForKey:@"images"];
  images[@(LynxImageRequestSrc)] = ResourceTestImage(UIColor.yellowColor);
  LynxDisplayListApplier *retained = [[LynxDisplayListApplier alloc] initWithView:retainedView
                                                                       andContext:context];
  LynxDisplayListApplier *rebuilt = [[LynxDisplayListApplier alloc] initWithView:rebuiltView
                                                                      andContext:context];
  float radii[] = {3, 4, 5, 6, 7, 8, 9, 10};
  CALayer *textLayer = nil;
  CALayer *imageLayer = nil;
  CALayer *fillLayer = nil;
  id textContents = nil;
  for (int phase = 0; phase < 4; ++phase) {
    DisplayList list;
    AppendRecordBox(list, phase * 3, 5, 90, 50, phase < 3, radii);
    AppendFill(list, phase % 2 == 0 ? 0xFFFF0000 : 0x8000FF00, 0);
    AppendText(list, 1, 0);
    AppendRecordBox(list, 20, phase * 2, 20, 30);
    AppendFill(list, 0x800000FF, 1);
    AppendImage(list, 2, 1);
    [retained applyDisplayList:&list];
    [rebuilt reset];
    [rebuilt applyDisplayList:&list];
    CALayer *currentText = retainedView.layer.sublayers[1];
    CALayer *currentImage = retainedView.layer.sublayers[3];
    [currentText displayIfNeeded];
    [rebuiltView.layer.sublayers[1] displayIfNeeded];
    if (phase > 0) {
      XCTAssertEqual(currentText, textLayer);
      XCTAssertEqual(currentImage, imageLayer);
      XCTAssertEqual(currentText.contents, textContents);
      XCTAssertNil(fillLayer.superlayer);
    }
    textLayer = currentText;
    imageLayer = currentImage;
    fillLayer = retainedView.layer.sublayers.firstObject;
    textContents = currentText.contents;
    XCTAssertEqualObjects([self renderedPixelsForView:retainedView],
                          [self renderedPixelsForView:rebuiltView]);
  }
}

- (void)testTextRetainsBackingStoreWhenMovingAndRedrawsNewRenderer {
  LynxMockView *view = [[LynxMockView alloc] initWithFrame:CGRectMake(0, 0, 100, 100)];
  LynxRendererContext *context = [LynxRendererContext new];
  UpdateMeasuredText(context, @"first");
  LynxDisplayListApplier *applier = [[LynxDisplayListApplier alloc] initWithView:view
                                                                      andContext:context];
  DisplayList first;
  AppendRecordBox(first, 0, 0, 90, 50);
  AppendText(first, 1, 0);
  [applier applyDisplayList:&first];
  CALayer *layer = view.layer.sublayers.firstObject;
  [layer displayIfNeeded];
  id contents = layer.contents;
  XCTAssertNotNil(contents);
  NSData *firstPixels = [self renderedPixelsForView:view];

  DisplayList moved;
  AppendRecordBox(moved, 3, 4, 90, 50);
  AppendText(moved, 1, 0);
  [applier applyDisplayList:&moved];
  XCTAssertEqual(view.layer.sublayers.firstObject, layer);
  XCTAssertFalse(layer.needsDisplay);
  [layer displayIfNeeded];
  XCTAssertEqual(layer.contents, contents);

  UpdateMeasuredText(context, @"changed");
  [applier applyDisplayList:&first];
  XCTAssertEqual(view.layer.sublayers.firstObject, layer);
  XCTAssertTrue(layer.needsDisplay);
  [layer displayIfNeeded];
  XCTAssertNotEqualObjects([self renderedPixelsForView:view], firstPixels);
}

- (void)testTextRedrawsForSizeScaleAndInPlaceStorageEdits {
  LynxMockView *view = [[LynxMockView alloc] initWithFrame:CGRectMake(0, 0, 100, 100)];
  LynxRendererContext *context = [LynxRendererContext new];
  UpdateMeasuredText(context, @"first");
  LynxDisplayListApplier *applier = [[LynxDisplayListApplier alloc] initWithView:view
                                                                      andContext:context];
  DisplayList first;
  AppendRecordBox(first, 0, 0, 90, 50);
  AppendText(first, 1, 0);
  [applier applyDisplayList:&first];
  CALayer *layer = view.layer.sublayers.firstObject;
  [layer displayIfNeeded];
  LynxTextRenderer *renderer = [context.textRenderManager takeTextRender:1];
  [renderer.textStorage replaceCharactersInRange:NSMakeRange(0, 5) withString:@"other"];
  XCTAssertTrue(layer.needsDisplay);
  [layer displayIfNeeded];

  layer.contentsScale = UIScreen.mainScreen.scale + 1;
  [applier applyDisplayList:&first];
  XCTAssertTrue(layer.needsDisplay);
  XCTAssertEqual(layer.contentsScale, UIScreen.mainScreen.scale);
  [layer displayIfNeeded];
  DisplayList resized;
  AppendRecordBox(resized, 0, 0, 80, 40);
  AppendText(resized, 1, 0);
  [applier applyDisplayList:&resized];
  XCTAssertEqual(view.layer.sublayers.firstObject, layer);
  XCTAssertTrue(layer.needsDisplay);
}

- (void)testTextServiceVersionInvalidatesContentsForTheSamePage {
  id service = OCMProtocolMock(@protocol(LynxServiceTextProtocol));
  id services = OCMClassMock([LynxServices class]);
  OCMStub(ClassMethod([services getInstanceWithProtocol:@protocol(LynxServiceTextProtocol)]))
      .andReturn(service);
  int pageStorage = 0;
  void *page = &pageStorage;
  __block NSUInteger draws = 0;
  OCMStub([service drawPage:page OnContext:(CGContextRef)[OCMArg anyPointer]])
      .andDo(^(NSInvocation *invocation) {
        ++draws;
      });
  LynxMockView *view = [[LynxMockView alloc] initWithFrame:CGRectMake(0, 0, 100, 100)];
  LynxRendererContext *context = [LynxRendererContext new];
  [context updateTextBundle:1 withBundle:page];
  id applier = OCMPartialMock([[LynxDisplayListApplier alloc] initWithView:view
                                                                andContext:context]);
  OCMStub([applier isTextServiceModeOn]).andReturn(YES);
  DisplayList list;
  AppendRecordBox(list, 0, 0, 90, 50);
  AppendText(list, 1, 0);
  [applier applyDisplayList:&list];
  CALayer *layer = view.layer.sublayers.firstObject;
  [layer displayIfNeeded];
  XCTAssertEqual(draws, 1u);
  [applier applyDisplayList:&list];
  [layer displayIfNeeded];
  XCTAssertEqual(draws, 1u);

  [context updateTextBundle:1 withBundle:page];
  [applier applyDisplayList:&list];
  XCTAssertEqual(view.layer.sublayers.firstObject, layer);
  [layer displayIfNeeded];
  XCTAssertEqual(draws, 2u);
  [context destroyTextBundle:1];
  [applier applyDisplayList:&list];
  XCTAssertNil(layer.superlayer);
  XCTAssertEqual(view.layer.sublayers.count, 0u);
  [applier stopMocking];
  [services stopMocking];
}

- (void)testImagePreservesViewContentsAndReceivesLaterImages {
  LynxMockView *view = [[LynxMockView alloc] initWithFrame:CGRectMake(0, 0, 100, 100)];
  LynxImageManager *manager = [[LynxImageManager alloc] initWithContext:nil];
  UIImage *firstImage = ResourceTestImage(UIColor.redColor);
  NSMutableDictionary *images = [manager valueForKey:@"images"];
  images[@(LynxImageRequestSrc)] = firstImage;
  id context = OCMClassMock([LynxRendererContext class]);
  OCMStub([context imageManagerForID:1]).andReturn(manager);
  LynxDisplayListApplier *applier = [[LynxDisplayListApplier alloc] initWithView:view
                                                                      andContext:context];
  float radii[] = {3, 4, 5, 6, 7, 8, 9, 10};
  DisplayList first;
  AppendRecordBox(first, 0, 0, 50, 50, true, radii);
  AppendImage(first, 1, 0);
  [applier applyDisplayList:&first];
  UIImageView *imageView = (UIImageView *)view.subviews.firstObject;
  CALayer *mask = imageView.layer.mask;
  XCTAssertEqual(imageView.image, firstImage);
  id target = OCMPartialMock(imageView);
  OCMReject([target setImage:[OCMArg any]]);
  DisplayList moved;
  AppendRecordBox(moved, 3, 4, 50, 50, true, radii);
  AppendImage(moved, 1, 0);
  [applier applyDisplayList:&moved];
  XCTAssertEqual(view.subviews.firstObject, imageView);
  XCTAssertEqual(imageView.layer.mask, mask);
  XCTAssertEqual(imageView.image, firstImage);
  [target stopMocking];

  // This is the same delivery path used by an asynchronous load completion.
  UIImage *nextImage = ResourceTestImage(UIColor.blueColor);
  images[@(LynxImageRequestSrc)] = nextImage;
  [manager applyImage:nextImage withType:LynxImageRequestSrc];
  [applier applyDisplayList:&moved];
  XCTAssertEqual(view.subviews.firstObject, imageView);
  XCTAssertEqual(imageView.image, nextImage);

  DisplayList square;
  AppendRecordBox(square, 3, 4, 50, 50);
  AppendImage(square, 1, 0);
  [applier applyDisplayList:&square];
  XCTAssertEqual(view.subviews.firstObject, imageView);
  XCTAssertNil(imageView.layer.mask);
  XCTAssertTrue(imageView.clipsToBounds);
}

- (void)testImageManagerReplacementDoesNotKeepOldTargetOrContents {
  LynxMockView *view = [[LynxMockView alloc] initWithFrame:CGRectMake(0, 0, 100, 100)];
  LynxRendererContext *context = [LynxRendererContext new];
  ImagePaintInfo paintInfo;
  [context createImageManager:1
                withSourceURL:nil
            andPlaceholderURL:nil
                    paintInfo:paintInfo
                    eventMask:0
                     imageKey:1];
  LynxImageManager *oldManager = [context imageManagerForID:1];
  NSMutableDictionary *images = [oldManager valueForKey:@"images"];
  images[@(LynxImageRequestSrc)] = ResourceTestImage(UIColor.redColor);
  LynxDisplayListApplier *applier = [[LynxDisplayListApplier alloc] initWithView:view
                                                                      andContext:context];
  DisplayList list;
  AppendRecordBox(list, 0, 0, 50, 50);
  AppendImage(list, 1, 0);
  [applier applyDisplayList:&list];
  UIImageView *oldView = (UIImageView *)view.subviews.firstObject;
  [context createImageManager:1
                withSourceURL:nil
            andPlaceholderURL:nil
                    paintInfo:paintInfo
                    eventMask:0
                     imageKey:1];
  [applier applyDisplayList:&list];
  UIImageView *newView = (UIImageView *)view.subviews.firstObject;
  XCTAssertNotEqual(oldView, newView);
  XCTAssertNil(newView.image);
  [oldManager applyImage:ResourceTestImage(UIColor.blueColor) withType:LynxImageRequestSrc];
  XCTAssertNil(newView.image);
}

- (void)testRetainedTextPixelsMatchFullRebuild {
  LynxMockView *retainedView = [[LynxMockView alloc] initWithFrame:CGRectMake(0, 0, 100, 100)];
  LynxMockView *rebuiltView = [[LynxMockView alloc] initWithFrame:retainedView.frame];
  LynxRendererContext *context = [LynxRendererContext new];
  UpdateMeasuredText(context, @"first");
  LynxDisplayListApplier *retained = [[LynxDisplayListApplier alloc] initWithView:retainedView
                                                                       andContext:context];
  LynxDisplayListApplier *rebuilt = [[LynxDisplayListApplier alloc] initWithView:rebuiltView
                                                                      andContext:context];
  CALayer *textLayer = nil;
  for (int phase = 0; phase < 4; ++phase) {
    if (phase == 3) {
      UpdateMeasuredText(context, @"updated");
    }
    DisplayList list;
    AppendClipRect(list, 0, 0, 95 - phase, 55);
    AppendRecordBox(list, phase * 2, phase, 90 - phase, 50);
    AppendText(list, 1, 0);
    [retained applyDisplayList:&list];
    [rebuilt reset];
    [rebuilt applyDisplayList:&list];
    if (textLayer == nil) {
      textLayer = retainedView.layer.sublayers.firstObject;
    } else {
      XCTAssertEqual(retainedView.layer.sublayers.firstObject, textLayer);
    }
    [retainedView.layer.sublayers.firstObject displayIfNeeded];
    [rebuiltView.layer.sublayers.firstObject displayIfNeeded];
    XCTAssertEqualObjects([self renderedPixelsForView:retainedView],
                          [self renderedPixelsForView:rebuiltView]);
  }
}

- (void)testMixedResourcesPreserveNativeViewAnchors {
  LynxMockView *view = [[LynxMockView alloc] initWithFrame:CGRectMake(0, 0, 100, 100)];
  LynxMockView *child = [[LynxMockView alloc] initWithFrame:CGRectMake(0, 0, 10, 10)];
  [child createRendererWithSign:3 andContext:nil];
  [view addSubview:child];
  LynxRendererContext *context = [LynxRendererContext new];
  UpdateMeasuredText(context, @"text");
  ImagePaintInfo paintInfo;
  [context createImageManager:2
                withSourceURL:nil
            andPlaceholderURL:nil
                    paintInfo:paintInfo
                    eventMask:0
                     imageKey:2];
  NSMutableDictionary *images = [[context imageManagerForID:2] valueForKey:@"images"];
  images[@(LynxImageRequestSrc)] = ResourceTestImage(UIColor.redColor);
  LynxDisplayListApplier *applier = [[LynxDisplayListApplier alloc] initWithView:view
                                                                      andContext:context];
  DisplayList list;
  AppendRecordBox(list, 0, 0, 90, 50);
  AppendFill(list, 0xFF00FF00, 0);
  AppendImage(list, 2, 0);
  AppendDrawView(list, 3, 5, 6);
  AppendText(list, 1, 0);
  [applier applyDisplayList:&list];
  NSArray *layers = [view.layer.sublayers copy];
  NSArray *subviews = [view.subviews copy];
  child.frame = CGRectMake(0, 0, 10, 10);
  [applier applyDisplayList:&list];
  XCTAssertEqual(view.layer.sublayers.count, layers.count);
  XCTAssertNil(((CALayer *)layers[0]).superlayer);
  for (NSUInteger i = 1; i < layers.count; ++i) {
    XCTAssertEqual(view.layer.sublayers[i], layers[i]);
  }
  XCTAssertEqualObjects(view.subviews, subviews);
  XCTAssertTrue(CGPointEqualToPoint(child.frame.origin, CGPointMake(5, 6)));
  XCTAssertEqual(layers[2], child.layer);
  [view insertSubview:child atIndex:0];
  [applier applyDisplayList:&list];
  XCTAssertNil(((CALayer *)layers.lastObject).superlayer);
}

- (void)testResourceStructureChangesAndExplicitResetDiscardTextLayers {
  LynxMockView *view = [[LynxMockView alloc] initWithFrame:CGRectMake(0, 0, 100, 100)];
  LynxRendererContext *context = [LynxRendererContext new];
  UpdateMeasuredText(context, @"text");
  LynxDisplayListApplier *applier = [[LynxDisplayListApplier alloc] initWithView:view
                                                                      andContext:context];
  DisplayList first;
  AppendRecordBox(first, 0, 0, 90, 50);
  AppendText(first, 1, 0);
  [applier applyDisplayList:&first];
  CALayer *textLayer = view.layer.sublayers.firstObject;
  [applier applyDisplayList:&first];
  XCTAssertEqual(view.layer.sublayers.firstObject, textLayer);

  DisplayList changed;
  AppendRecordBox(changed, 0, 0, 90, 50);
  AppendFill(changed, 0xFF00FF00, 0);
  AppendText(changed, 1, 0);
  [applier applyDisplayList:&changed];
  XCTAssertNil(textLayer.superlayer);
  CALayer *newTextLayer = view.layer.sublayers[1];
  XCTAssertNotEqual(newTextLayer, textLayer);
  [applier applyDisplayList:&changed];
  XCTAssertEqual(view.layer.sublayers[1], newTextLayer);
  [applier reset];
  XCTAssertEqual(view.layer.sublayers.count, 0u);
  XCTAssertNil(newTextLayer.superlayer);
  [applier applyDisplayList:&changed];
  XCTAssertNotEqual(view.layer.sublayers[1], newTextLayer);
}

- (void)testRetainedResourcesRestoreTransactionActionsBeforeNativeViews {
  const BOOL originalDisableActions = [CATransaction disableActions];
  for (BOOL disabled : {NO, YES}) {
    [CATransaction setDisableActions:disabled];
    LynxMockView *view = [[LynxMockView alloc] initWithFrame:CGRectMake(0, 0, 100, 100)];
    LynxActionProbeView *child =
        [[LynxActionProbeView alloc] initWithFrame:CGRectMake(0, 0, 10, 10)];
    child.lynxSign = @2;
    [view addSubview:child];
    LynxRendererContext *context = [LynxRendererContext new];
    UpdateMeasuredText(context, @"text");
    LynxDisplayListApplier *applier = [[LynxDisplayListApplier alloc] initWithView:view
                                                                        andContext:context];
    DisplayList first;
    AppendRecordBox(first, 0, 0, 50, 50);
    AppendFill(first, 0xFFFF0000, 0);
    AppendText(first, 1, 0);
    AppendDrawView(first, 2, 3, 4);
    [applier applyDisplayList:&first];
    CALayer *fillLayer = view.layer.sublayers.firstObject;
    CALayer *textLayer = view.layer.sublayers[1];
    [CATransaction flush];
    [CATransaction setDisableActions:disabled];
    DisplayList second;
    AppendRecordBox(second, 1, 2, 50, 50);
    AppendFill(second, 0xFF00FF00, 0);
    AppendText(second, 1, 0);
    AppendDrawView(second, 2, 5, 6);
    [applier applyDisplayList:&second];
    XCTAssertNil(fillLayer.superlayer);
    XCTAssertEqual(view.layer.sublayers[1], textLayer);
    XCTAssertTrue(CGPointEqualToPoint(child.frame.origin, CGPointMake(5, 6)));
    XCTAssertEqual(child.actionsDisabledDuringMove, disabled);
    XCTAssertEqual([CATransaction disableActions], disabled);
    XCTAssertEqual(textLayer.animationKeys.count, 0u);
  }
  [CATransaction setDisableActions:originalDisableActions];
}

@end
