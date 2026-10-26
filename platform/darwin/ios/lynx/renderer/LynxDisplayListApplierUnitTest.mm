// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxDisplayListApplier+Internal.h>
#import <Lynx/LynxImageManager.h>
#import <Lynx/LynxRenderer.h>
#import <Lynx/LynxRendererContext.h>
#import <Lynx/LynxRendererHost.h>
#import <Lynx/LynxService.h>
#import <Lynx/LynxServiceTextProtocol.h>
#import <Lynx/LynxTextLayer.h>
#import <Lynx/LynxTextRenderManager.h>
#import <Lynx/UIView+Lynx.h>
#import <OCMock/OCMock.h>
#import <XCTest/XCTest.h>
#include "core/public/platform_renderer_type.h"
#include "core/renderer/dom/fragment/display_list.h"
#include "core/renderer/ui_wrapper/painting/paint_image.h"

using namespace lynx::tasm;

@interface LynxImageManager (LynxDisplayListApplierUnitTest)
- (void)updatePaintInfo:(const ImagePaintInfo &)paintInfo;
@end

@interface LynxTextraTestDisplayListApplier : LynxDisplayListApplier
@end

@implementation LynxTextraTestDisplayListApplier
- (BOOL)isTextServiceModeOn {
  return YES;
}
@end

namespace {
constexpr int32_t kViewType = static_cast<int32_t>(PlatformRendererType::kView);

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

@interface LynxCountingTextRenderer : LynxTextRenderer
@property(nonatomic, assign) NSInteger drawCount;
@end

@implementation LynxCountingTextRenderer
- (void)drawRect:(CGRect)rect padding:(UIEdgeInsets)padding border:(UIEdgeInsets)border {
  self.drawCount++;
  [[UIColor blackColor] setFill];
  UIRectFill(rect);
}
@end

@interface LynxTextraContentReuseUnitTest : XCTestCase
@end

@implementation LynxTextraContentReuseUnitTest {
  LynxMockView *_host;
  LynxRendererContext *_context;
  LynxDisplayListApplier *_applier;
  id _services;
  id _textService;
  NSInteger _drawCount;
  NSMutableArray<NSValue *> *_destroyedPages;
  int _page;
  int _replacementPage;
  int _updatedPage;
}

- (void)setUp {
  [super setUp];
  _drawCount = 0;
  _destroyedPages = [NSMutableArray new];
  _textService = OCMProtocolMock(@protocol(LynxServiceTextProtocol));
  _services = OCMClassMock([LynxServices class]);
  OCMStub(ClassMethod([_services getInstanceWithProtocol:@protocol(LynxServiceTextProtocol)]))
      .andReturn(_textService);
  __weak typeof(self) weakSelf = self;
  OCMStub([_textService drawPage:[OCMArg anyPointer] OnContext:(CGContextRef)[OCMArg anyPointer]])
      .andDo(^(NSInvocation *invocation) {
        typeof(self) self = weakSelf;
        if (!self) return;
        CGContextRef ctx;
        [invocation getArgument:&ctx atIndex:3];
        self->_drawCount++;
        CGContextSetRGBFillColor(ctx, 0, 0, 0, 1);
        CGContextFillRect(ctx, CGRectMake(0, 0, 100, 100));
      });
  OCMStub([_textService destroyPage:[OCMArg anyPointer]]).andDo(^(NSInvocation *invocation) {
    typeof(self) self = weakSelf;
    if (!self) return;
    void *page;
    [invocation getArgument:&page atIndex:2];
    [self->_destroyedPages addObject:[NSValue valueWithPointer:page]];
  });
  _host = [[LynxMockView alloc] initWithFrame:CGRectMake(0, 0, 100, 100)];
  _context = [LynxRendererContext new];
  _applier = [[LynxTextraTestDisplayListApplier alloc] initWithView:_host andContext:_context];
}

- (void)tearDown {
  [_applier reset];
  _applier = nil;
  _context = nil;
  [_services stopMocking];
  [_textService stopMocking];
  [super tearDown];
}

- (void)applyTextAtX:(float)x width:(float)width {
  DisplayList list;
  AppendRecordBox(list, x, 0, width, 20);
  AppendText(list, 1, 0);
  [_applier applyDisplayList:&list];
}

- (void)testUnchangedTextReusesBackingButSizeAndBackingLossRedraw {
  [_context updateTextBundle:1 withBundle:&_page];
  [self applyTextAtX:0 width:40];
  CALayer *layer = _host.layer.sublayers.firstObject;
  [layer display];
  XCTAssertNotNil(layer.contents);
  id contents = layer.contents;
  XCTAssertEqual(_drawCount, 1);

  [self applyTextAtX:0 width:40];
  XCTAssertEqual(_host.layer.sublayers.firstObject, layer);
  XCTAssertEqual(layer.contents, contents);
  XCTAssertFalse(layer.needsDisplay);
  [self applyTextAtX:10 width:40];
  XCTAssertEqual(layer.frame.origin.x, 10);
  XCTAssertFalse(layer.needsDisplay);
  XCTAssertEqual(layer.animationKeys.count, 0u);

  [self applyTextAtX:10 width:60];
  XCTAssertEqual(_host.layer.sublayers.firstObject, layer);
  XCTAssertTrue(layer.needsDisplay);
  [layer display];
  XCTAssertEqual(_drawCount, 2);
  layer.contents = nil;
  [self applyTextAtX:10 width:60];
  XCTAssertTrue(layer.needsDisplay);
  [layer display];
  XCTAssertEqual(_drawCount, 3);
  XCTAssertEqual(layer.animationKeys.count, 0u);
}

- (void)testRepeatedPagePublicationReusesBackingButRecycledAddressDoesNot {
  [_context updateTextBundle:1 withBundle:&_page];
  [self applyTextAtX:0 width:40];
  CALayer *first = _host.layer.sublayers.firstObject;
  [first display];

  // Layout republishes the existing Page when only a sibling has changed.
  [_context updateTextBundle:1 withBundle:&_page];
  [self applyTextAtX:0 width:40];
  CALayer *updated = _host.layer.sublayers.firstObject;
  XCTAssertEqual(updated, first);
  XCTAssertFalse(updated.needsDisplay);
  XCTAssertEqual(_drawCount, 1);
  XCTAssertEqual(_destroyedPages.count, 0u);

  // Reusing a destroyed page's address must also create a fresh cache identity.
  [_context destroyTextBundle:1];
  [_context updateTextBundle:1 withBundle:&_page];
  [self applyTextAtX:0 width:40];
  XCTAssertNotEqual(_host.layer.sublayers.firstObject, updated);
  XCTAssertTrue(_host.layer.sublayers.firstObject.needsDisplay);
  XCTAssertEqual(_destroyedPages.count, 1u);
}

- (void)testReplacementAndNullBundleKeepPageOwnershipInContext {
  [_context updateTextBundle:1 withBundle:&_page];
  [self applyTextAtX:0 width:40];
  CALayer *oldLayer = _host.layer.sublayers.firstObject;
  [oldLayer display];
  [_context updateTextBundle:1 withBundle:&_replacementPage];
  XCTAssertEqualObjects(_destroyedPages, (@[ [NSValue valueWithPointer:&_page] ]));
  [self applyTextAtX:0 width:40];
  XCTAssertNotEqual(_host.layer.sublayers.firstObject, oldLayer);
  XCTAssertTrue(_host.layer.sublayers.firstObject.needsDisplay);

  [_context updateTextBundle:1 withBundle:nullptr];
  XCTAssertEqual([_context getTextBundle:1], nullptr);
  [self applyTextAtX:0 width:40];
  XCTAssertEqual(_host.layer.sublayers.count, 0u);
  XCTAssertEqualObjects(
      _destroyedPages,
      (@[ [NSValue valueWithPointer:&_page], [NSValue valueWithPointer:&_replacementPage] ]));
  [_context destroyTextBundle:1];
  XCTAssertEqual(_destroyedPages.count, 2u);
}

- (void)testRemovedTextAndResetReleaseCachedLayers {
  [_context updateTextBundle:1 withBundle:&_page];
  __weak CALayer *removedLayer;
  @autoreleasepool {
    [self applyTextAtX:0 width:40];
    removedLayer = _host.layer.sublayers.firstObject;
    [removedLayer display];
    DisplayList empty;
    [_applier applyDisplayList:&empty];
    [CATransaction flush];
  }
  XCTAssertNil(removedLayer);
  XCTAssertEqual(_destroyedPages.count, 0u);
  @autoreleasepool {
    [self applyTextAtX:0 width:40];
    removedLayer = _host.layer.sublayers.firstObject;
    XCTAssertNotNil(removedLayer);
    [_applier reset];
    [CATransaction flush];
  }
  XCTAssertNil(removedLayer);
  XCTAssertEqual([_context getTextBundle:1], &_page);
  [_context destroyTextBundle:1];
  XCTAssertEqual(_destroyedPages.count, 1u);
}

- (void)testChangedSiblingRedrawsAndReorderingPreservesUnchangedBacking {
  [_context updateTextBundle:1 withBundle:&_page];
  [_context updateTextBundle:2 withBundle:&_replacementPage];
  auto makeList = [](bool reversed) {
    DisplayList list;
    AppendRecordBox(list, 0, 0, 40, 20);
    AppendText(list, reversed ? 2 : 1, 0);
    AppendText(list, reversed ? 1 : 2, 0);
    return list;
  };
  auto list = makeList(false);
  [_applier applyDisplayList:&list];
  CALayer *unchanged = _host.layer.sublayers[0];
  CALayer *changed = _host.layer.sublayers[1];
  [unchanged display];
  [changed display];
  [_context updateTextBundle:2 withBundle:&_updatedPage];
  auto reordered = makeList(true);
  [_applier applyDisplayList:&reordered];
  XCTAssertEqual(_host.layer.sublayers[1], unchanged);
  XCTAssertFalse(unchanged.needsDisplay);
  XCTAssertNotEqual(_host.layer.sublayers[0], changed);
  XCTAssertTrue(_host.layer.sublayers[0].needsDisplay);
  [_host.layer.sublayers[0] display];
  XCTAssertEqual(_drawCount, 3);
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
  LynxMockView *view = [[LynxMockView alloc] initWithFrame:CGRectMake(0, 0, 100, 100)];

  LynxDisplayListApplier *applier = [[LynxDisplayListApplier alloc] initWithView:view
                                                                      andContext:nil];

  DisplayList list;
  AppendClipRect(list, 10.0f, 10.0f, 50.0f, 50.0f);

  [applier applyDisplayList:&list];

  XCTAssertTrue([view.layer.mask isKindOfClass:[CAShapeLayer class]]);
  CAShapeLayer *maskLayer = (CAShapeLayer *)view.layer.mask;
  XCTAssertTrue(CGRectEqualToRect(maskLayer.frame, view.bounds));
  XCTAssertTrue(
      CGRectEqualToRect(CGPathGetBoundingBox(maskLayer.path), CGRectMake(10, 10, 50, 50)));
  XCTAssertFalse(view.layer.masksToBounds);
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

- (void)testUnchangedTextReusesBackingButResizesAndReplacementsRedraw {
  LynxMockView *host = [[LynxMockView alloc] initWithFrame:CGRectMake(0, 0, 100, 100)];
  LynxCountingTextRenderer *renderer = [LynxCountingTextRenderer new];
  __block LynxCountingTextRenderer *currentRenderer = renderer;
  id textManager = OCMClassMock([LynxTextRenderManager class]);
  OCMStub([textManager takeTextRender:1]).andDo(^(NSInvocation *invocation) {
    [invocation setReturnValue:&currentRenderer];
  });
  id context = OCMClassMock([LynxRendererContext class]);
  OCMStub([context textRenderManager]).andReturn(textManager);
  LynxDisplayListApplier *applier = [[LynxDisplayListApplier alloc] initWithView:host
                                                                      andContext:context];
  auto makeList = [](float x, float width) {
    DisplayList list;
    AppendBegin(list, 7, kViewType, 0, 0, 100, 100);
    AppendRecordBox(list, x, 0, width, 20);
    AppendText(list, 1, 0);
    AppendEnd(list);
    return list;
  };
  auto list = makeList(0, 40);
  [applier applyDisplayList:&list];
  CALayer *layer = host.layer.sublayers.firstObject;
  [layer display];
  XCTAssertNotNil(layer.contents);
  id contents = layer.contents;
  XCTAssertEqual(renderer.drawCount, 1);

  [applier applyDisplayList:&list];
  XCTAssertEqual(host.layer.sublayers.firstObject, layer);
  XCTAssertEqual(layer.contents, contents);
  XCTAssertFalse(layer.needsDisplay);

  auto moved = makeList(10, 40);
  [applier applyDisplayList:&moved];
  XCTAssertEqual(host.layer.sublayers.firstObject, layer);
  XCTAssertEqual(layer.frame.origin.x, 10);
  XCTAssertFalse(layer.needsDisplay);
  XCTAssertEqual(layer.animationKeys.count, 0u);

  auto resized = makeList(10, 60);
  [applier applyDisplayList:&resized];
  XCTAssertEqual(host.layer.sublayers.firstObject, layer);
  XCTAssertTrue(layer.needsDisplay);
  [layer display];
  XCTAssertEqual(renderer.drawCount, 2);
  XCTAssertEqual(layer.animationKeys.count, 0u);

  layer.contents = nil;
  [applier applyDisplayList:&resized];
  XCTAssertTrue(layer.needsDisplay);
  [layer display];
  XCTAssertEqual(renderer.drawCount, 3);

  currentRenderer = [LynxCountingTextRenderer new];
  [applier applyDisplayList:&resized];
  CALayer *replacement = host.layer.sublayers.firstObject;
  XCTAssertNotEqual(replacement, layer);
  [replacement display];
  XCTAssertEqual(currentRenderer.drawCount, 1);
  [applier reset];
  XCTAssertEqual(host.layer.sublayers.count, 0u);
  [applier applyDisplayList:&resized];
  XCTAssertNotEqual(host.layer.sublayers.firstObject, replacement);
}

- (void)testImageReuseSurvivesNativeChildRebuildAndPreservesOrder {
  LynxMockView *host = [[LynxMockView alloc] initWithFrame:CGRectMake(0, 0, 100, 100)];
  UIView *child = [UIView new];
  child.lynxSign = @9;
  [host addSubview:child];
  id manager = OCMClassMock([LynxImageManager class]);
  __block NSInteger targetCount = 0;
  __block UIImageView *target = nil;
  OCMStub([manager setTarget:[OCMArg any]]).andDo(^(NSInvocation *invocation) {
    __unsafe_unretained UIImageView *view;
    [invocation getArgument:&view atIndex:2];
    target = view;
    targetCount++;
  });
  id context = OCMClassMock([LynxRendererContext class]);
  OCMStub([context imageManagerForID:1]).andReturn(manager);
  LynxDisplayListApplier *applier = [[LynxDisplayListApplier alloc] initWithView:host
                                                                      andContext:context];
  auto makeList = [](bool before, bool rounded) {
    DisplayList list;
    AppendBegin(list, 7, kViewType, 0, 0, 100, 100);
    const float radii[] = {4, 4, 4, 4, 4, 4, 4, 4};
    AppendRecordBox(list, 0, 0, 40, 40, rounded, radii);
    if (before) AppendDrawView(list, 9);
    AppendImage(list, 1, 0);
    if (!before) AppendDrawView(list, 9);
    AppendEnd(list);
    return list;
  };
  auto first = makeList(false, true);
  [applier applyDisplayList:&first];
  UIImageView *imageView = target;
  XCTAssertEqual(targetCount, 1);
  XCTAssertEqual(host.subviews.firstObject, imageView);
  XCTAssertEqual(imageView.layer.cornerRadius, 4);

  [applier onRebuildSubRenderers];
  XCTAssertNil(imageView.superview);
  XCTAssertEqualObjects(host.subviews, (@[ child ]));
  auto reordered = makeList(true, false);
  [applier applyDisplayList:&reordered];
  XCTAssertEqual(host.subviews.lastObject, imageView);
  XCTAssertEqual(targetCount, 1);
  XCTAssertEqual(imageView.layer.cornerRadius, 0);
  XCTAssertNil(imageView.layer.mask);
  XCTAssertTrue(imageView.clipsToBounds);

  // Completion after a rebuild still targets the same image view.
  UIImage *loaded = [UIImage new];
  target.image = loaded;
  XCTAssertEqual(imageView.image, loaded);
  [applier applyDisplayList:&reordered];
  XCTAssertEqual(imageView.image, loaded);
  XCTAssertEqual(targetCount, 1);

  DisplayList empty;
  [applier applyDisplayList:&empty];
  XCTAssertNil(imageView.superview);
  [applier applyDisplayList:&first];
  XCTAssertNotEqual(target, imageView);
  XCTAssertEqual(targetCount, 2);
}

- (void)testRemovedImageIsNotRetainedByReuseCache {
  LynxMockView *host = [LynxMockView new];
  LynxImageManager *manager = [[LynxImageManager alloc] initWithContext:nil];
  id context = OCMClassMock([LynxRendererContext class]);
  OCMStub([context imageManagerForID:1]).andReturn(manager);
  LynxDisplayListApplier *applier = [[LynxDisplayListApplier alloc] initWithView:host
                                                                      andContext:context];
  __weak UIView *removedView;
  @autoreleasepool {
    DisplayList list;
    AppendRecordBox(list, 0, 0, 40, 40);
    AppendImage(list, 1, 0);
    [applier applyDisplayList:&list];
    removedView = host.subviews.firstObject;
    XCTAssertNotNil(removedView);
    DisplayList empty;
    [applier applyDisplayList:&empty];
  }
  XCTAssertNil(removedView);
}

- (void)testReplacementImageManagerDoesNotReusePreviousTarget {
  LynxMockView *host = [LynxMockView new];
  id firstManager = OCMClassMock([LynxImageManager class]);
  id secondManager = OCMClassMock([LynxImageManager class]);
  __block id currentManager = firstManager;
  id context = OCMClassMock([LynxRendererContext class]);
  OCMStub([context imageManagerForID:1]).andDo(^(NSInvocation *invocation) {
    [invocation setReturnValue:&currentManager];
  });
  LynxDisplayListApplier *applier = [[LynxDisplayListApplier alloc] initWithView:host
                                                                      andContext:context];
  DisplayList list;
  AppendRecordBox(list, 0, 0, 40, 40);
  AppendImage(list, 1, 0);
  [applier applyDisplayList:&list];
  UIView *first = host.subviews.firstObject;
  currentManager = secondManager;
  [applier applyDisplayList:&list];
  XCTAssertNotEqual(host.subviews.firstObject, first);
  OCMVerify([secondManager setTarget:[OCMArg any]]);
  XCTAssertNil(first.superview);
}

@end
