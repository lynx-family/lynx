// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxBackgroundUtils.h>
#import <Lynx/LynxCSSType.h>
#import <Lynx/LynxDisplayListApplier+Internal.h>
#import <Lynx/LynxImageManager.h>
#import <Lynx/LynxPropsProcessor.h>
#import <Lynx/LynxRenderer+Internal.h>
#import <Lynx/LynxRenderer.h>
#import <Lynx/LynxRendererContext.h>
#import <Lynx/LynxRendererHost.h>
#import <Lynx/LynxService.h>
#import <Lynx/LynxServiceTextProtocol.h>
#import <Lynx/LynxTextLayer.h>
#import <Lynx/LynxTextRenderManager.h>
#import <Lynx/LynxUI+Internal.h>
#import <Lynx/LynxUIScroller.h>
#import <Lynx/UIView+Lynx.h>
#import <OCMock/OCMock.h>
#import <XCTest/XCTest.h>
#import "../ui/LynxUIUnitTestUtils.h"
#include "core/public/platform_renderer_type.h"
#include "core/renderer/ui_wrapper/painting/ios/path_darwin.h"
#include "core/renderer/ui_wrapper/painting/public/paint_image.h"
#include "gfx/public/display_list.h"
#include "gfx/public/path.h"

using namespace lynx::tasm;

@interface LynxImageManager (LynxDisplayListApplierUnitTest)
- (void)updatePaintInfo:(const ImagePaintInfo &)paintInfo;
- (void)applyImage:(UIImage *)image withType:(LynxImageRequestType)type;
@end

@interface LynxTextraTestDisplayListApplier : LynxDisplayListApplier
@end

@implementation LynxTextraTestDisplayListApplier
- (BOOL)isTextServiceModeOn {
  return YES;
}
@end

namespace {
lynx::tasm::PathDarwinFactory ClipFactory;

void AppendPathClip(DisplayList &list, fml::RefPtr<lynx::gfx::Path> path,
                    lynx::gfx::PathFillRule rule = lynx::gfx::PathFillRule::kNonzero, float x = 0,
                    float y = 0) {
  DisplayListItem item{};
  item.type = DisplayListOpType::kClipPath;
  item.payload.clip_path.path_index = list.RetainPath(std::move(path));
  item.payload.clip_path.fill_rule = rule;
  item.payload.clip_path.x = x;
  item.payload.clip_path.y = y;
  list.AppendItem(item);
}

void AddOwnerClip(DisplayList &list, fml::RefPtr<lynx::gfx::Path> path,
                  lynx::gfx::PathFillRule rule = lynx::gfx::PathFillRule::kNonzero, float x = 0,
                  float y = 0) {
  SubtreeProperty property{};
  property.type = DisplayListSubtreePropertyOpType::kClipPath;
  property.data.clip_path.path_index = path ? list.RetainPath(std::move(path)) : UINT32_MAX;
  property.data.clip_path.fill_rule = rule;
  property.data.clip_path.x = x;
  property.data.clip_path.y = y;
  list.AddSubtreeProperty(property);
}

CGPoint ProjectPoint(CGPoint point, CATransform3D transform) {
  CGFloat w = point.x * transform.m14 + point.y * transform.m24 + transform.m44;
  return CGPointMake((point.x * transform.m11 + point.y * transform.m21 + transform.m41) / w,
                     (point.x * transform.m12 + point.y * transform.m22 + transform.m42) / w);
}

bool ClipMaskContains(CALayer *layer, CGPoint point) {
  for (CALayer *mask = layer.mask; mask != nil; mask = mask.mask) {
    CATransform3D local = CATransform3DMakeTranslation(
        -mask.bounds.origin.x - mask.anchorPoint.x * mask.bounds.size.width,
        -mask.bounds.origin.y - mask.anchorPoint.y * mask.bounds.size.height, 0);
    local = CATransform3DConcat(local, mask.transform);
    local = CATransform3DConcat(local,
                                CATransform3DMakeTranslation(mask.position.x, mask.position.y, 0));
    point = ProjectPoint(point, CATransform3DInvert(local));
    if (!CGRectContainsPoint(mask.bounds, point)) return false;
    if ([mask isKindOfClass:[CAShapeLayer class]]) {
      CAShapeLayer *shape = (CAShapeLayer *)mask;
      if (!CGPathContainsPoint(shape.path, nullptr, point,
                               [shape.fillRule isEqualToString:kCAFillRuleEvenOdd]))
        return false;
    }
  }
  return true;
}

uint8_t LayerPixelAlpha(CALayer *layer, size_t x, size_t y) {
  constexpr size_t size = 100;
  uint8_t pixels[size * size * 4]{};
  CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
  CGContextRef context = CGBitmapContextCreate(pixels, size, size, 8, size * 4, colorSpace,
                                               kCGImageAlphaPremultipliedLast);
  CGContextTranslateCTM(context, 0, size);
  CGContextScaleCTM(context, 1, -1);
  [layer renderInContext:context];
  uint8_t alpha = pixels[(y * size + x) * 4 + 3];
  CGContextRelease(context);
  CGColorSpaceRelease(colorSpace);
  return alpha;
}

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

- (void)testInvalidTextBoxDropsBackingAndValidBoxRestoresText {
  [_context updateTextBundle:1 withBundle:&_page];
  for (int32_t invalidIndex : {-1, 1}) {
    [self applyTextAtX:0 width:40];
    CALayer *previous = _host.layer.sublayers.firstObject;
    [previous display];
    XCTAssertNotNil(previous.contents);
    DisplayList invalid;
    AppendRecordBox(invalid, 0, 0, 40, 20);
    AppendText(invalid, 1, invalidIndex);
    [_applier applyDisplayList:&invalid];
    XCTAssertNil(previous.superlayer);
    XCTAssertEqual(_host.layer.sublayers.count, 0u);
    [self applyTextAtX:0 width:40];
    XCTAssertNotEqual(_host.layer.sublayers.firstObject, previous);
    XCTAssertTrue(_host.layer.sublayers.firstObject.needsDisplay);
  }
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

- (NSArray<NSNumber *> *)redValuesForImage:(UIImage *)image
                                    points:(NSArray<NSValue *> *)points
                                      name:(NSString *)name {
  XCTAttachment *attachment = [XCTAttachment attachmentWithImage:image];
  attachment.name = name;
  attachment.lifetime = XCTAttachmentLifetimeKeepAlways;
  [self addAttachment:attachment];
  CGImageRef cgImage = image.CGImage;
  XCTAssertNotEqual(cgImage, nullptr);
  XCTAssertEqual(CGImageGetBitsPerComponent(cgImage), 8u);
  XCTAssertEqual(CGImageGetBitsPerPixel(cgImage), 32u);
  CFDataRef pixels = CGDataProviderCopyData(CGImageGetDataProvider(cgImage));
  const uint8_t *bytes = CFDataGetBytePtr(pixels);
  CGImageAlphaInfo alpha = CGImageGetAlphaInfo(cgImage);
  BOOL alphaFirst = alpha == kCGImageAlphaPremultipliedFirst || alpha == kCGImageAlphaFirst ||
                    alpha == kCGImageAlphaNoneSkipFirst;
  BOOL littleEndian =
      (CGImageGetBitmapInfo(cgImage) & kCGBitmapByteOrderMask) == kCGBitmapByteOrder32Little;
  NSUInteger redOffset = littleEndian ? (alphaFirst ? 2 : 3) : (alphaFirst ? 1 : 0);
  NSMutableArray<NSNumber *> *values = [NSMutableArray new];
  NSMutableArray<NSValue *> *imagePoints = [NSMutableArray new];
  for (NSValue *value in points) {
    CGPoint point = value.CGPointValue;
    [imagePoints addObject:[NSValue valueWithCGPoint:point]];
    XCTAssertTrue(CGRectContainsPoint((CGRect){CGPointZero, image.size}, point));
    NSUInteger x = MAX(0, MIN(CGImageGetWidth(cgImage) - 1, floor(point.x)));
    NSUInteger y = MAX(0, MIN(CGImageGetHeight(cgImage) - 1, floor(point.y)));
    [values addObject:@(bytes[y * CGImageGetBytesPerRow(cgImage) + x * 4 + redOffset])];
  }
  XCTAttachment *coordinates = [XCTAttachment
      attachmentWithString:[NSString stringWithFormat:@"View points: %@\nImage points: %@\nRed "
                                                      @"values: %@\nBitmap info: %u",
                                                      points, imagePoints, values,
                                                      (unsigned)CGImageGetBitmapInfo(cgImage)]];
  coordinates.name = [name stringByAppendingString:@"-coordinates"];
  coordinates.lifetime = XCTAttachmentLifetimeKeepAlways;
  [self addAttachment:coordinates];
  CFRelease(pixels);
  return values;
}

- (NSArray<NSNumber *> *)renderedRedValuesForView:(UIView *)view
                                           points:(NSArray<NSValue *> *)points
                                             name:(NSString *)name {
  UIGraphicsImageRendererFormat *format = [UIGraphicsImageRendererFormat defaultFormat];
  format.scale = 1;
  format.opaque = YES;
  if (@available(iOS 12.0, *)) format.preferredRange = UIGraphicsImageRendererFormatRangeStandard;
  UIGraphicsImageRenderer *renderer = [[UIGraphicsImageRenderer alloc] initWithSize:view.bounds.size
                                                                             format:format];
  UIImage *image = [renderer imageWithActions:^(UIGraphicsImageRendererContext *context) {
    [UIColor.blackColor setFill];
    UIRectFill((CGRect){CGPointZero, view.bounds.size});
    CGContextTranslateCTM(context.CGContext, -view.bounds.origin.x, -view.bounds.origin.y);
    [view.layer renderInContext:context.CGContext];
  }];
  NSMutableArray<NSValue *> *imagePoints = [NSMutableArray new];
  for (NSValue *point in points) {
    [imagePoints
        addObject:[NSValue
                      valueWithCGPoint:CGPointMake(point.CGPointValue.x - view.bounds.origin.x,
                                                   point.CGPointValue.y - view.bounds.origin.y)]];
  }
  return [self redValuesForImage:image points:imagePoints name:name];
}

- (NSArray<NSNumber *> *)hostedRedValuesForView:(UIView *)view
                                         points:(NSArray<NSValue *> *)points
                                           name:(NSString *)name {
  UIWindow *previousWindow = nil;
  for (UIWindow *candidate in UIApplication.sharedApplication.windows) {
    if (candidate.isKeyWindow) previousWindow = candidate;
  }
  UIWindow *window = nil;
  if (@available(iOS 13.0, *)) {
    if (previousWindow.windowScene != nil) {
      window = [[UIWindow alloc] initWithWindowScene:previousWindow.windowScene];
    }
  }
  if (window == nil) window = [[UIWindow alloc] initWithFrame:UIScreen.mainScreen.bounds];
  window.frame = UIScreen.mainScreen.bounds;
  window.windowLevel = UIWindowLevelAlert + 1;
  window.backgroundColor = UIColor.blackColor;
  UIViewController *controller = [UIViewController new];
  controller.view.backgroundColor = UIColor.blackColor;
  window.rootViewController = controller;
  UIView *originalParent = view.superview;
  NSUInteger originalIndex = [originalParent.subviews indexOfObjectIdenticalTo:view];
  [controller.view addSubview:view];
  [window makeKeyAndVisible];
  [window layoutIfNeeded];
  [CATransaction flush];
  XCTestExpectation *composition = [self expectationWithDescription:name];
  dispatch_after(dispatch_time(DISPATCH_TIME_NOW, 100 * NSEC_PER_MSEC), dispatch_get_main_queue(),
                 ^{
                   [composition fulfill];
                 });
  [self waitForExpectationsWithTimeout:2 handler:nil];
  UIGraphicsImageRendererFormat *format = [UIGraphicsImageRendererFormat defaultFormat];
  format.scale = 1;
  format.opaque = YES;
  if (@available(iOS 12.0, *)) format.preferredRange = UIGraphicsImageRendererFormatRangeStandard;
  UIGraphicsImageRenderer *renderer =
      [[UIGraphicsImageRenderer alloc] initWithSize:window.bounds.size format:format];
  __block BOOL captured = NO;
  UIImage *image = [renderer imageWithActions:^(UIGraphicsImageRendererContext *context) {
    captured = [window drawViewHierarchyInRect:window.bounds afterScreenUpdates:YES];
  }];
  XCTAssertTrue(captured);
  NSMutableArray<NSValue *> *windowPoints = [NSMutableArray new];
  for (NSValue *point in points) {
    [windowPoints addObject:[NSValue valueWithCGPoint:[view convertPoint:point.CGPointValue
                                                                  toView:window]]];
  }
  NSArray<NSNumber *> *values = [self redValuesForImage:image points:windowPoints name:name];
  [view removeFromSuperview];
  if (originalParent != nil)
    [originalParent insertSubview:view atIndex:MIN(originalIndex, originalParent.subviews.count)];
  window.hidden = YES;
  [previousWindow makeKeyWindow];
  return values;
}

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
  LynxMockView *mockView = [[LynxMockView alloc] initWithFrame:CGRectMake(0, 0, 100, 100)];
  id mockContext = OCMClassMock([LynxRendererContext class]);
  LynxDisplayListApplier *applier = [[LynxDisplayListApplier alloc] initWithView:mockView
                                                                      andContext:mockContext];
  XCTAssertNotNil(applier);
}

- (void)testApplyDisplayListWithNullInputs {
  LynxMockView *mockView = [[LynxMockView alloc] initWithFrame:CGRectMake(0, 0, 100, 100)];
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

- (void)testGradientRingMaskUsesLocalCoordinatesAndDoesNotSurviveReplacement {
  LynxMockView *view = [[LynxMockView alloc] initWithFrame:CGRectMake(0, 0, 200, 200)];
  LynxDisplayListApplier *applier = [[LynxDisplayListApplier alloc] initWithView:view
                                                                      andContext:nil];
  DisplayList list;
  const float outer[] = {8, 8, 12, 12, 16, 16, 4, 4};
  const float inner[] = {4, 4, 8, 8, 12, 12, 0, 0};
  AppendRecordBox(list, 10, 12, 80, 60, true, outer);
  AppendRecordBox(list, 14, 16, 72, 52, true, inner);
  list.AddLinearGradient(90, {0x80ff0000, 0x800000ff}, {0, 1}, 0, 0, 1, 1, 1);
  [applier applyDisplayList:&list];
  XCTAssertEqual(view.layer.sublayers.count, 1u);
  CALayer *gradient = view.layer.sublayers.firstObject;
  XCTAssertTrue(CGRectEqualToRect(gradient.frame, CGRectMake(10, 12, 80, 60)));
  CAShapeLayer *mask = (CAShapeLayer *)gradient.mask;
  XCTAssertTrue([mask isKindOfClass:CAShapeLayer.class]);
  XCTAssertEqualObjects(mask.fillRule, kCAFillRuleEvenOdd);
  XCTAssertFalse(CGPathContainsPoint(mask.path, nullptr, CGPointMake(40, 30), true));
  XCTAssertTrue(CGPathContainsPoint(mask.path, nullptr, CGPointMake(40, 2), true));
  XCTAssertFalse(CGPathContainsPoint(mask.path, nullptr, CGPointMake(0, 0), true));

  DisplayList replacement;
  AppendRecordBox(replacement, 10, 12, 80, 60);
  replacement.AddLinearGradient(90, {0xffff0000, 0xff0000ff}, {0, 1}, 0, 0, 1, 1);
  [applier applyDisplayList:&replacement];
  XCTAssertNil(gradient.superlayer);
  XCTAssertEqual(view.layer.sublayers.count, 1u);
  XCTAssertNil(view.layer.sublayers.firstObject.mask);
}

- (void)testForegroundAfterNativeChildStaysAboveContentAndResetsForDecorationOnlyList {
  UIView *parent = [[UIView alloc] initWithFrame:CGRectMake(0, 0, 200, 200)];
  LynxMockView *host = [[LynxMockView alloc] initWithFrame:CGRectMake(20, 30, 100, 80)];
  [host createRendererWithSign:1 andContext:nil];
  [parent addSubview:host];
  UIView *child = [[UIView alloc] initWithFrame:CGRectMake(0, 0, 100, 80)];
  child.lynxSign = @2;
  [host addSubview:child];
  LynxDisplayListApplier *applier = [[LynxDisplayListApplier alloc] initWithView:host
                                                                      andContext:nil];
  DisplayList list;
  AppendBegin(list, 1, kViewType, 0, 0, 100, 80);
  AppendRecordBox(list, 0, 0, 100, 80);
  AppendRecordBox(list, 4, 4, 92, 72);
  AppendFill(list, 0xff0000ff, 0);
  AppendDrawView(list, 2);
  list.AddLinearGradient(90, {0x80ff0000, 0x800000ff}, {0, 1}, 0, 0, 1, 1, 1);
  AppendEnd(list);
  [applier applyDisplayList:&list];
  XCTAssertEqual(parent.layer.sublayers.count, 2u);
  XCTAssertEqual(parent.layer.sublayers.lastObject, host.layer);
  XCTAssertEqual(host.layer.sublayers.count, 2u);
  XCTAssertEqual(host.layer.sublayers.firstObject, child.layer);
  XCTAssertNotNil(host.layer.sublayers.lastObject.mask);

  DisplayList decorations;
  AppendBegin(decorations, 1, kViewType, 0, 0, 100, 80);
  AppendRecordBox(decorations, 0, 0, 100, 80);
  decorations.AddLinearGradient(90, {0xffff0000, 0xff0000ff}, {0, 1}, 0, 0, 1, 1);
  AppendEnd(decorations);
  [applier applyDisplayList:&decorations];
  XCTAssertEqual(host.layer.sublayers.count, 1u);
  XCTAssertEqual(host.layer.sublayers.firstObject, child.layer);
  XCTAssertEqual(parent.layer.sublayers.count, 2u);
  XCTAssertEqual(parent.layer.sublayers.lastObject, host.layer);
}

- (void)testForegroundFollowsFlattenedChildFill {
  UIView *parent = [[UIView alloc] initWithFrame:CGRectMake(0, 0, 100, 80)];
  LynxMockView *host = [[LynxMockView alloc] initWithFrame:parent.bounds];
  [host createRendererWithSign:1 andContext:nil];
  [parent addSubview:host];
  LynxDisplayListApplier *applier = [[LynxDisplayListApplier alloc] initWithView:host
                                                                      andContext:nil];
  DisplayList list;
  AppendBegin(list, 1, kViewType, 0, 0, 100, 80);
  AppendRecordBox(list, 0, 0, 100, 80);
  AppendBegin(list, 2, kViewType, 0, 0, 100, 80);
  AppendFill(list, 0xff00ff00, 0);
  AppendEnd(list);
  list.AddLinearGradient(90, {0x80ff0000, 0x800000ff}, {0, 1}, 0, 0, 1, 1);
  AppendEnd(list);
  [applier applyDisplayList:&list];
  XCTAssertEqual(parent.layer.sublayers.count, 1u);
  XCTAssertEqual(host.layer.sublayers.count, 2u);
  XCTAssertTrue(CGColorEqualToColor(host.layer.sublayers.firstObject.backgroundColor,
                                    UIColor.greenColor.CGColor));
  XCTAssertTrue([host.layer.sublayers.lastObject isKindOfClass:CAReplicatorLayer.class]);
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

- (void)testScopedClipContainsCSSBackgroundAndChildButNotEarlierShadow {
  for (bool gradient : {false, true}) {
    UIView *parent = [[UIView alloc] initWithFrame:CGRectMake(0, 0, 200, 200)];
    LynxMockView *host = [[LynxMockView alloc] initWithFrame:CGRectMake(20, 30, 100, 80)];
    [host createRendererWithSign:1 andContext:nil];
    [parent addSubview:host];
    UIView *child = [[UIView alloc] initWithFrame:host.bounds];
    child.lynxSign = @2;
    [host addSubview:child];
    LynxDisplayListApplier *applier = [[LynxDisplayListApplier alloc] initWithView:host
                                                                        andContext:nil];
    DisplayList list;
    AppendBegin(list, 1, kViewType, 0, 0, 100, 80);
    AppendRecordBox(list, 0, 0, 100, 80);
    AppendRecordBox(list, -5, -5, 110, 90);
    AppendBoxShadow(list, 1, 0, false);
    const float radii[] = {12, 12, 12, 12, 12, 12, 12, 12};
    AppendClipRect(list, 0, 0, 100, 80, true, radii);
    // CSS paint has a rectangular box: the preceding clip alone supplies its rounded shape.
    if (gradient) {
      list.AddLinearGradient(90, {0xffff0000, 0xff0000ff}, {0, 1}, 0, 0,
                             LynxBackgroundRepeatNoRepeat, LynxBackgroundRepeatNoRepeat);
    } else {
      AppendFill(list, 0xffff0000, 0);
    }
    AppendDrawView(list, 2);
    AppendEnd(list);
    [applier applyDisplayList:&list];
    host.alpha = 0.5;
    [applier syncHostDecorationLayers];

    XCTAssertEqual(parent.layer.sublayers.count, 2u);
    XCTAssertEqual(parent.layer.sublayers.lastObject, host.layer);
    CALayer *shadow = parent.layer.sublayers.firstObject;
    XCTAssertEqual(shadow.superlayer, parent.layer);
    XCTAssertNil(shadow.mask);
    XCTAssertFalse(shadow.masksToBounds);
    XCTAssertEqual(shadow.sublayers.firstObject.shadowRadius, 5.f);
    XCTAssertEqualWithAccuracy(shadow.opacity, 0.5, 0.001);
    XCTAssertTrue(CGRectEqualToRect(shadow.frame, host.frame));

    XCTAssertEqual(host.layer.sublayers.count, 2u);
    CALayer *background = host.layer.sublayers.firstObject;
    XCTAssertEqual(background.superlayer, host.layer);
    XCTAssertEqual(host.layer.sublayers.lastObject, child.layer);
    XCTAssertTrue(CGRectEqualToRect(background.frame, host.bounds));
    XCTAssertEqual(background.opacity, 1.f);  // Host opacity applies once, through ancestry.
    XCTAssertNil(background.mask);
    XCTAssertEqual(background.cornerRadius, 0.f);
    XCTAssertNil(host.layer.mask);
    XCTAssertTrue(host.layer.masksToBounds);
    XCTAssertEqual(host.layer.cornerRadius, 12.f);
    if (gradient) {
      CAGradientLayer *paint =
          (CAGradientLayer *)background.sublayers.firstObject.sublayers.firstObject;
      XCTAssertTrue([paint isKindOfClass:CAGradientLayer.class]);
      XCTAssertEqual(paint.colors.count, 2u);
    } else {
      XCTAssertTrue(CGColorEqualToColor(background.backgroundColor, UIColor.redColor.CGColor));
    }
    [applier reset];
    XCTAssertNil(background.superlayer);
    XCTAssertNil(shadow.superlayer);
    XCTAssertEqualObjects(host.layer.sublayers, (@[ child.layer ]));
    XCTAssertEqual(host.layer.cornerRadius, 0.f);
    XCTAssertFalse(host.layer.masksToBounds);
  }
}

- (void)testScopedClipCSSBackgroundMaskUpdatesAndLegacyDecorationPlacementReturns {
  for (bool gradient : {false, true}) {
    UIView *parent = [[UIView alloc] initWithFrame:CGRectMake(0, 0, 200, 200)];
    LynxMockView *host = [[LynxMockView alloc] initWithFrame:CGRectMake(20, 30, 100, 80)];
    [host createRendererWithSign:1 andContext:nil];
    [parent addSubview:host];
    UIView *child = [[UIView alloc] initWithFrame:host.bounds];
    child.lynxSign = @2;
    [host addSubview:child];
    LynxDisplayListApplier *applier = [[LynxDisplayListApplier alloc] initWithView:host
                                                                        andContext:nil];
    auto appendBackground = [gradient](DisplayList &list) {
      if (gradient) {
        list.AddLinearGradient(90, {0xffff0000, 0xff0000ff}, {0, 1}, 0, 0,
                               LynxBackgroundRepeatNoRepeat, LynxBackgroundRepeatNoRepeat);
      } else {
        AppendFill(list, 0xffff0000, 0);
      }
    };
    CALayer *background = nil;
    CAShapeLayer *mask = nil;
    for (CGFloat width : {100.f, 120.f}) {
      host.frame = CGRectMake(20, 30, width, 80);
      DisplayList list;
      AppendBegin(list, 1, kViewType, 0, 0, width, 80);
      const float radii[] = {12, 12, 8, 8, 4, 4, 16, 16};
      AppendClipRect(list, 5, 7, width - 10, 66, true, radii);
      AppendRecordBox(list, 0, 0, width, 80);
      appendBackground(list);
      AppendDrawView(list, 2);
      AppendEnd(list);
      [applier applyDisplayList:&list];
      [applier syncHostDecorationLayers];
      XCTAssertNil(background.superlayer);
      XCTAssertNotEqual(host.layer.mask, mask);
      background = host.layer.sublayers.firstObject;
      mask = (CAShapeLayer *)host.layer.mask;
      XCTAssertTrue([mask isKindOfClass:CAShapeLayer.class]);
      XCTAssertTrue(CGRectEqualToRect(mask.frame, host.bounds));
      XCTAssertTrue(
          CGRectEqualToRect(CGPathGetBoundingBox(mask.path), CGRectMake(5, 7, width - 10, 66)));
      XCTAssertTrue(CGPathContainsPoint(mask.path, nullptr, CGPointMake(50, 40), false));
      XCTAssertFalse(CGPathContainsPoint(mask.path, nullptr, CGPointMake(6, 8), false));
      XCTAssertFalse(CGPathContainsPoint(mask.path, nullptr, CGPointMake(1, 40), false));
      XCTAssertEqualObjects(parent.layer.sublayers, (@[ host.layer ]));
      XCTAssertEqualObjects(host.layer.sublayers, (@[ background, child.layer ]));
      XCTAssertEqual(background.superlayer, host.layer);
      XCTAssertTrue(CGRectEqualToRect(background.frame, host.bounds));
    }

    // Legacy CSS lists paint decorations before their content clip, not after it.
    DisplayList legacy;
    AppendBegin(legacy, 1, kViewType, 0, 0, 120, 80);
    AppendRecordBox(legacy, 0, 0, 120, 80);
    appendBackground(legacy);
    AppendClipRect(legacy, 0, 0, 120, 80);
    AppendDrawView(legacy, 2);
    AppendEnd(legacy);
    [applier applyDisplayList:&legacy];
    [applier syncHostDecorationLayers];
    XCTAssertNil(background.superlayer);
    XCTAssertNil(host.layer.mask);
    XCTAssertEqual(host.layer.cornerRadius, 0.f);
    XCTAssertTrue(host.layer.masksToBounds);
    XCTAssertEqualObjects(host.layer.sublayers, (@[ child.layer ]));
    XCTAssertEqual(parent.layer.sublayers.count, 2u);
    XCTAssertEqual(parent.layer.sublayers.lastObject, host.layer);
    CALayer *decoration = parent.layer.sublayers.firstObject;
    XCTAssertEqual(decoration.superlayer, parent.layer);
    XCTAssertNil(decoration.mask);
    XCTAssertTrue(CGRectEqualToRect(decoration.frame, host.frame));

    DisplayList empty;
    [applier applyDisplayList:&empty];
    XCTAssertNil(decoration.superlayer);
    XCTAssertEqualObjects(parent.layer.sublayers, (@[ host.layer ]));
    XCTAssertEqualObjects(host.layer.sublayers, (@[ child.layer ]));
    XCTAssertNil(host.layer.mask);
    XCTAssertEqual(host.layer.cornerRadius, 0.f);
    XCTAssertFalse(host.layer.masksToBounds);
  }
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

- (void)testCustomFillRetainsBoundsAfterHostPropertyUpdate {
  UIView *parent = [[UIView alloc] initWithFrame:CGRectMake(0, 0, 200, 200)];
  LynxMockView *host = [[LynxMockView alloc] initWithFrame:CGRectMake(20, 30, 100, 80)];
  [host createRendererWithSign:1 andContext:nil];
  [parent addSubview:host];
  LynxDisplayListApplier *applier = [[LynxDisplayListApplier alloc] initWithView:host
                                                                      andContext:nil];
  DisplayList list;
  AppendBegin(list, 1, kViewType, 0, 0, 100, 80);
  AppendRecordBox(list, 5, 7, 40, 16);
  AppendFill(list, 0x80FF0000, 0);
  AppendEnd(list);
  [applier applyDisplayList:&list];
  host.alpha = 0.5;
  host.bounds = CGRectMake(0, 20, 100, 80);
  [applier syncHostDecorationLayers];
  CALayer *container = parent.layer.sublayers.firstObject;
  CALayer *fill = container.sublayers.firstObject;
  XCTAssertTrue(CGRectEqualToRect(fill.frame, CGRectMake(5, 7, 40, 16)));
  XCTAssertTrue(CGRectEqualToRect([fill convertRect:fill.bounds toLayer:parent.layer],
                                  CGRectMake(25, 37, 40, 16)));
  XCTAssertEqualWithAccuracy(container.opacity, 0.5, 0.001);
}

- (void)testInterleavedGradientsAndFillsKeepCommandOrder {
  UIView *parent = [[UIView alloc] initWithFrame:CGRectMake(0, 0, 200, 200)];
  LynxMockView *host = [[LynxMockView alloc] initWithFrame:CGRectMake(20, 30, 100, 80)];
  [host createRendererWithSign:1 andContext:nil];
  [parent addSubview:host];
  LynxDisplayListApplier *applier = [[LynxDisplayListApplier alloc] initWithView:host
                                                                      andContext:nil];
  DisplayList list;
  AppendBegin(list, 1, kViewType, 0, 0, 100, 80);
  AppendRecordBox(list, 0, 0, 100, 80);
  list.AddLinearGradient(180, {0xFFFF0000, 0xFF0000FF}, {0, 1}, 0, 0, LynxBackgroundRepeatNoRepeat,
                         LynxBackgroundRepeatNoRepeat);
  AppendFill(list, 0xFF00FF00, 0);
  list.AddLinearGradient(180, {0x80FFFFFF, 0x80000000}, {0, 1}, 0, 0, LynxBackgroundRepeatNoRepeat,
                         LynxBackgroundRepeatNoRepeat);
  AppendEnd(list);
  [applier applyDisplayList:&list];
  [applier syncHostDecorationLayers];
  NSArray<CALayer *> *layers = parent.layer.sublayers;
  XCTAssertEqual(layers.count, 4u);
  XCTAssertEqual(layers.lastObject, host.layer);
  XCTAssertEqual(layers[0].sublayers.count, 1u);
  XCTAssertTrue(CGColorEqualToColor(layers[1].backgroundColor, UIColor.greenColor.CGColor));
  XCTAssertEqual(layers[2].sublayers.count, 1u);
  host.bounds = CGRectMake(7, 40, 100, 80);
  host.alpha = 0.5;
  [applier syncHostDecorationLayers];
  for (NSNumber *index in @[ @0, @2 ]) {
    CALayer *group = layers[index.unsignedIntegerValue];
    XCTAssertTrue(CGRectEqualToRect(group.bounds, CGRectMake(0, 0, 100, 80)));
    XCTAssertEqualWithAccuracy(group.opacity, 0.5, 0.001);
  }
  [applier reset];
  XCTAssertEqual(parent.layer.sublayers.count, 1u);
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
  for (BackgroundSizeMode sizeMode :
       {BackgroundSizeMode::kAutoWidth, BackgroundSizeMode::kAutoHeight,
        BackgroundSizeMode::kAutoBoth}) {
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
    item.payload.background_image.size_mode = sizeMode;
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
    CGFloat width = sizeMode == BackgroundSizeMode::kAutoWidth
                        ? 160
                        : (sizeMode == BackgroundSizeMode::kAutoHeight ? 100 : 40);
    CGFloat height = sizeMode == BackgroundSizeMode::kAutoWidth
                         ? 80
                         : (sizeMode == BackgroundSizeMode::kAutoHeight ? 50 : 20);
    XCTAssertEqualWithAccuracy(target.frame.size.width, width, 0.001);
    XCTAssertEqualWithAccuracy(target.frame.size.height, height, 0.001);
    if (sizeMode == BackgroundSizeMode::kAutoBoth) {
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

- (void)testBackgroundImageFitSizeOnLoadAndImageReplacement {
  struct TestCase {
    BackgroundSizeMode mode;
    CGRect landscapeFrame;
    CGRect portraitFrame;
  };
  const TestCase cases[] = {
      {BackgroundSizeMode::kCover, CGRectMake(-22, 17, 160, 80), CGRectMake(8, -43, 100, 200)},
      {BackgroundSizeMode::kContain, CGRectMake(8, 32, 100, 50), CGRectMake(38, 17, 40, 80)},
  };
  for (const auto &test : cases) {
    LynxMockView *view = [[LynxMockView alloc] initWithFrame:CGRectMake(0, 0, 140, 120)];
    view.contentScaleFactor = 3;
    id context = OCMClassMock([LynxRendererContext class]);
    LynxImageManager *manager = [[LynxImageManager alloc] initWithContext:nil];
    OCMStub([context imageManagerForID:123]).andReturn(manager);
    LynxDisplayListApplier *applier = [[LynxDisplayListApplier alloc] initWithView:view
                                                                        andContext:context];
    DisplayList list;
    AppendRecordBox(list, 10, 20, 100, 80);
    float radii[] = {6, 6, 6, 6, 6, 6, 6, 6};
    AppendRecordBox(list, 2, 3, 140, 120, true, radii);
    DisplayListItem item{};
    item.type = DisplayListOpType::kBackgroundImage;
    item.payload.background_image.image_id = 123;
    item.payload.background_image.tiling_index = 0;
    item.payload.background_image.clip_index = 1;
    item.payload.background_image.size_mode = test.mode;
    item.payload.background_image.position_x = 0.5;
    item.payload.background_image.position_y = 0.5;
    item.payload.background_image.repeat_x = LynxBackgroundRepeatNoRepeat;
    item.payload.background_image.repeat_y = LynxBackgroundRepeatNoRepeat;
    list.AppendItem(item);
    [applier applyDisplayList:&list];
    CAReplicatorLayer *vertical = (CAReplicatorLayer *)view.layer.sublayers.firstObject;
    CALayer *target = vertical.sublayers.firstObject.sublayers.firstObject;
    XCTAssertTrue(vertical.hidden);

    const CGSize imageSizes[] = {CGSizeMake(40, 20), CGSizeMake(20, 40)};
    const CGRect frames[] = {test.landscapeFrame, test.portraitFrame};
    for (size_t index = 0; index < 2; ++index) {
      UIGraphicsBeginImageContextWithOptions(imageSizes[index], NO, 2);
      UIImage *image = UIGraphicsGetImageFromCurrentImageContext();
      UIGraphicsEndImageContext();
      NSMutableDictionary *images = [manager valueForKey:@"images"];
      images[@(LynxImageRequestSrc)] = image;
      // Completion updates the existing layers without applying another display list.
      [manager applyImage:image withType:LynxImageRequestSrc];
      XCTAssertEqual(view.layer.sublayers.firstObject, vertical);
      XCTAssertFalse(vertical.hidden);
      XCTAssertTrue(CGRectEqualToRect(vertical.frame, CGRectMake(2, 3, 140, 120)));
      XCTAssertTrue(vertical.masksToBounds);
      XCTAssertEqualWithAccuracy(vertical.cornerRadius, 6, 0.001);
      XCTAssertTrue(CGRectEqualToRect(target.frame, frames[index]));
    }

    [manager applyImage:nil withType:LynxImageRequestSrc];
    XCTAssertTrue(vertical.hidden);
    [applier reset];
    [applier applyDisplayList:&list];
    CALayer *cachedTarget =
        view.layer.sublayers.firstObject.sublayers.firstObject.sublayers.firstObject;
    XCTAssertFalse(view.layer.sublayers.firstObject.hidden);
    XCTAssertTrue(CGRectEqualToRect(cachedTarget.frame, test.portraitFrame));
    [applier reset];
  }
}

- (void)testBackgroundImageContainRepeatUsesResolvedTile {
  LynxMockView *view = [[LynxMockView alloc] initWithFrame:CGRectMake(0, 0, 120, 100)];
  view.contentScaleFactor = 3;
  id context = OCMClassMock([LynxRendererContext class]);
  LynxImageManager *manager = [[LynxImageManager alloc] initWithContext:nil];
  OCMStub([context imageManagerForID:123]).andReturn(manager);
  LynxDisplayListApplier *applier = [[LynxDisplayListApplier alloc] initWithView:view
                                                                      andContext:context];
  DisplayList list;
  AppendRecordBox(list, 10, 20, 100, 80);
  AppendRecordBox(list, 0, 0, 120, 100);
  DisplayListItem item{};
  item.type = DisplayListOpType::kBackgroundImage;
  item.payload.background_image.image_id = 123;
  item.payload.background_image.clip_index = 1;
  item.payload.background_image.size_mode = BackgroundSizeMode::kContain;
  item.payload.background_image.position_x = 0.5;
  item.payload.background_image.position_y = 0.5;
  item.payload.background_image.repeat_x = LynxBackgroundRepeatRepeat;
  item.payload.background_image.repeat_y = LynxBackgroundRepeatRepeat;
  list.AppendItem(item);
  [applier applyDisplayList:&list];
  UIGraphicsBeginImageContextWithOptions(CGSizeMake(40, 20), NO, 2);
  UIImage *image = UIGraphicsGetImageFromCurrentImageContext();
  UIGraphicsEndImageContext();
  [manager applyImage:image withType:LynxImageRequestSrc];
  CAReplicatorLayer *vertical = (CAReplicatorLayer *)view.layer.sublayers.firstObject;
  CAReplicatorLayer *horizontal = (CAReplicatorLayer *)vertical.sublayers.firstObject;
  CALayer *target = horizontal.sublayers.firstObject;
  XCTAssertTrue(CGRectEqualToRect(target.frame, CGRectMake(-90, -15, 100, 50)));
  XCTAssertEqualWithAccuracy(horizontal.instanceTransform.m41, 100, 0.001);
  XCTAssertEqualWithAccuracy(vertical.instanceTransform.m42, 50, 0.001);
  XCTAssertEqual(horizontal.instanceCount, 3);
  XCTAssertEqual(vertical.instanceCount, 3);
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

  LynxImageManager *imageManager = [[LynxImageManager alloc] initWithContext:nil];
  UIImageView *imageView = [UIImageView new];
  for (NSNumber *mode in expectations) {
    ImagePaintInfo paintInfo;
    paintInfo.mode = static_cast<ImageFitMode>(mode.intValue);
    [imageManager updatePaintInfo:paintInfo];
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

- (void)testInvalidTextKitBoxDropsBackingAndValidBoxRestoresText {
  LynxMockView *host = [[LynxMockView alloc] initWithFrame:CGRectMake(0, 0, 100, 100)];
  LynxCountingTextRenderer *renderer = [LynxCountingTextRenderer new];
  id textManager = OCMClassMock([LynxTextRenderManager class]);
  OCMStub([textManager takeTextRender:1]).andReturn(renderer);
  id context = OCMClassMock([LynxRendererContext class]);
  OCMStub([context textRenderManager]).andReturn(textManager);
  LynxDisplayListApplier *applier = [[LynxDisplayListApplier alloc] initWithView:host
                                                                      andContext:context];
  DisplayList valid;
  AppendRecordBox(valid, 0, 0, 40, 20);
  AppendText(valid, 1, 0);
  for (int32_t invalidIndex : {-1, 1}) {
    [applier applyDisplayList:&valid];
    CALayer *previous = host.layer.sublayers.firstObject;
    [previous display];
    XCTAssertNotNil(previous.contents);
    DisplayList invalid;
    AppendRecordBox(invalid, 0, 0, 40, 20);
    AppendText(invalid, 1, invalidIndex);
    [applier applyDisplayList:&invalid];
    XCTAssertNil(previous.superlayer);
    XCTAssertEqual(host.layer.sublayers.count, 0u);
    [applier applyDisplayList:&valid];
    XCTAssertNotEqual(host.layer.sublayers.firstObject, previous);
    XCTAssertTrue(host.layer.sublayers.firstObject.needsDisplay);
  }
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
  XCTAssertEqual(targetCount, 2);
  XCTAssertEqual(imageView.layer.cornerRadius, 0);
  XCTAssertNil(imageView.layer.mask);
  XCTAssertTrue(imageView.clipsToBounds);

  // Preserve already displayed contents. Real loader completion is covered by
  // LynxEmbeddedImageLoadingUnitTest.
  UIImage *loaded = [UIImage new];
  target.image = loaded;
  XCTAssertEqual(imageView.image, loaded);
  [applier applyDisplayList:&reordered];
  XCTAssertEqual(imageView.image, loaded);
  XCTAssertEqual(targetCount, 3);

  DisplayList empty;
  [applier applyDisplayList:&empty];
  XCTAssertNil(imageView.superview);
  [applier applyDisplayList:&first];
  XCTAssertNotEqual(target, imageView);
  XCTAssertEqual(targetCount, 4);
}

- (void)testRetainedImagesReorderAroundNativeImageViewAndDrawingLayer {
  LynxMockView *host = [[LynxMockView alloc] initWithFrame:CGRectMake(0, 0, 100, 100)];
  UIImageView *child = [[UIImageView alloc] initWithFrame:CGRectMake(0, 0, 10, 10)];
  child.lynxSign = @9;
  [host addSubview:child];
  LynxImageManager *firstManager = [[LynxImageManager alloc] initWithContext:nil];
  LynxImageManager *secondManager = [[LynxImageManager alloc] initWithContext:nil];
  id context = OCMClassMock([LynxRendererContext class]);
  OCMStub([context imageManagerForID:1]).andReturn(firstManager);
  OCMStub([context imageManagerForID:2]).andReturn(secondManager);
  LynxDisplayListApplier *applier = [[LynxDisplayListApplier alloc] initWithView:host
                                                                      andContext:context];
  auto makeList = [](int32_t first, int32_t last) {
    DisplayList list;
    AppendRecordBox(list, 0, 0, 40, 40);
    AppendImage(list, first, 0);
    AppendFill(list, 0xFF00FF00, 0);
    AppendDrawView(list, 9, 7, 9);
    if (last != 0) AppendImage(list, last, 0);
    return list;
  };
  auto initial = makeList(1, 2);
  [applier applyDisplayList:&initial];
  UIView *first = host.subviews.firstObject;
  UIView *second = host.subviews.lastObject;
  auto reordered = makeList(2, 1);
  for (int i = 0; i < 2; ++i) {
    [applier applyDisplayList:&reordered];
    XCTAssertEqualObjects(host.subviews, (@[ second, child, first ]));
    NSArray<CALayer *> *layers = host.layer.sublayers;
    XCTAssertEqual(layers.count, 4u);
    XCTAssertEqual(layers[0], second.layer);
    XCTAssertEqual(layers[2], child.layer);
    XCTAssertEqual(layers[3], first.layer);
    XCTAssertTrue(CGPointEqualToPoint(child.frame.origin, CGPointMake(7, 9)));
  }
  auto removed = makeList(2, 0);
  [applier applyDisplayList:&removed];
  XCTAssertNil(first.superview);
  XCTAssertEqualObjects(host.subviews, (@[ second, child ]));
  [applier reset];
  XCTAssertEqualObjects(host.subviews, (@[ child ]));
  XCTAssertEqualObjects(host.layer.sublayers, (@[ child.layer ]));
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

- (void)testOwnerPathIntersectsOverflowAndEvenOddAndSurvivesListRelease {
  LynxMockView *host = [[LynxMockView alloc] initWithFrame:CGRectMake(0, 0, 100, 100)];
  [host createRendererWithSign:1 andContext:nil];
  host.backgroundColor = UIColor.redColor;
  DisplayList content;
  AppendBegin(content, 1, kViewType, 0, 0, 100, 100);
  AppendClipRect(content, 10, 10, 70, 70);
  AppendEnd(content);
  [host.renderer updateDisplayList:&content];
  CALayer *overflowMask = host.layer.mask;
  CGPathRef nativePath = nullptr;
  {
    DisplayList properties;
    auto path = lynx::gfx::Path::MakeSvgPathData(
        lynx::base::String("M0 0H90V90H0Z M30 30H60V60H30Z"), 1, ClipFactory);
    nativePath = static_cast<const PathDarwin &>(path->GetImpl()).GetCGPath();
    AddOwnerClip(properties, path, lynx::gfx::PathFillRule::kEvenOdd);
    [host.renderer applySubtreePropertiesFromList:&properties];
  }
  XCTAssertNotNil(host.layer.mask);
  XCTAssertNotEqual(nativePath, nullptr);
  XCTAssertEqual(host.layer.mask.mask.mask, overflowMask);
  XCTAssertTrue(ClipMaskContains(host.layer, CGPointMake(20, 20)));
  XCTAssertFalse(ClipMaskContains(host.layer, CGPointMake(40, 40)));
  XCTAssertFalse(ClipMaskContains(host.layer, CGPointMake(85, 20)));
  XCTAssertGreaterThan(LayerPixelAlpha(host.layer, 20, 20), 200);
  XCTAssertEqual(LayerPixelAlpha(host.layer, 40, 40), 0);
  DisplayList clear;
  AddOwnerClip(clear, nullptr);
  [host.renderer applySubtreePropertiesFromList:&clear];
  XCTAssertEqual(host.layer.mask, overflowMask);
}

- (void)testOwnerPathPropertyUpdateKeepsDecorationContentAndClearsOnReset {
  UIView *parent = [[UIView alloc] initWithFrame:CGRectMake(0, 0, 100, 100)];
  LynxMockView *host = [[LynxMockView alloc] initWithFrame:parent.bounds];
  [parent addSubview:host];
  [host createRendererWithSign:1 andContext:nil];
  DisplayList content;
  AppendBegin(content, 1, kViewType, 0, 0, 100, 100);
  AppendRecordBox(content, 0, 0, 100, 100);
  AppendFill(content, 0xffff0000, 0);
  AppendEnd(content);
  [host.renderer updateDisplayList:&content];
  CALayer *decoration = parent.layer.sublayers.firstObject;
  XCTAssertNotEqual(decoration, host.layer);
  DisplayList properties;
  AddOwnerClip(properties, lynx::gfx::Path::MakeRect(20, 30, ClipFactory),
               lynx::gfx::PathFillRule::kNonzero, 10, 15);
  [host.renderer applySubtreePropertiesFromList:&properties];
  XCTAssertEqual(parent.layer.sublayers.firstObject, decoration);
  XCTAssertTrue(ClipMaskContains(host.layer, CGPointMake(15, 20)));
  XCTAssertFalse(ClipMaskContains(host.layer, CGPointMake(35, 20)));
  XCTAssertTrue(ClipMaskContains(decoration, CGPointMake(15, 20)));
  DisplayList replacement;
  AddOwnerClip(replacement, lynx::gfx::Path::MakeRect(50, 50, ClipFactory));
  [host.renderer applySubtreePropertiesFromList:&replacement];
  XCTAssertEqual(parent.layer.sublayers.firstObject, decoration);
  XCTAssertTrue(ClipMaskContains(decoration, CGPointMake(35, 20)));
  DisplayList clear;
  AddOwnerClip(clear, nullptr);
  [host.renderer applySubtreePropertiesFromList:&clear];
  XCTAssertNil(host.layer.mask);
  XCTAssertNil(decoration.mask);
  LynxDisplayListApplier *applier = [host.renderer valueForKey:@"_applier"];
  [applier applyClipPathProperty:properties.GetSubtreePropertiesData()[0] fromList:&properties];
  [applier reset];
  XCTAssertNil(host.layer.mask);
}

- (void)testNestedPathScopesIntersectAndRestoreNativeChildrenAndOrder {
  LynxMockView *host = [[LynxMockView alloc] initWithFrame:CGRectMake(0, 0, 100, 100)];
  UIView *parent = [[UIView alloc] initWithFrame:CGRectMake(0, 0, 200, 100)];
  [parent addSubview:host];
  [host createRendererWithSign:1 andContext:nil];
  LynxMockView *first = [[LynxMockView alloc] initWithFrame:host.bounds];
  LynxMockView *second = [[LynxMockView alloc] initWithFrame:host.bounds];
  [first createRendererWithSign:2 andContext:nil];
  [second createRendererWithSign:3 andContext:nil];
  [host addSubview:first];
  [host addSubview:second];
  CAShapeLayer *intrinsic = [CAShapeLayer layer];
  intrinsic.frame = first.bounds;
  intrinsic.path = [UIBezierPath bezierPathWithRect:CGRectMake(0, 0, 100, 100)].CGPath;
  first.layer.mask = intrinsic;
  DisplayList list;
  AppendBegin(list, 1, kViewType, 0, 0, 100, 100);
  AppendBegin(list, 4, kViewType, 10, 20, 80, 70);
  AppendPathClip(list, lynx::gfx::Path::MakeRect(60, 50, ClipFactory));
  AppendBegin(list, 5, kViewType, 5, 5, 40, 40);
  AppendClipRect(list, 0, 0, 30, 30);
  AppendDrawView(list, 2);
  AppendEnd(list);
  AppendDrawView(list, 3);
  AppendEnd(list);
  AppendRecordBox(list, 0, 0, 100, 100);
  AppendFill(list, 0xff0000ff, 0);
  AppendEnd(list);
  [host.renderer updateDisplayList:&list];
  XCTAssertEqualObjects(host.subviews, (@[ first, second ]));
  XCTAssertEqual(host.layer.sublayers[0], first.layer);
  XCTAssertEqual(host.layer.sublayers[1], second.layer);
  XCTAssertTrue(ClipMaskContains(first.layer, CGPointMake(20, 30)));
  XCTAssertFalse(ClipMaskContains(first.layer, CGPointMake(50, 30)));
  XCTAssertTrue(ClipMaskContains(second.layer, CGPointMake(50, 30)));
  XCTAssertFalse(ClipMaskContains(second.layer, CGPointMake(80, 30)));
  XCTAssertEqual(parent.layer.sublayers.count, 1u);
  XCTAssertEqual(parent.layer.sublayers.firstObject, host.layer);
  XCTAssertEqual(host.layer.sublayers.count, 3u);
  CALayer *unscopedFill = host.layer.sublayers.lastObject;
  XCTAssertTrue(CGColorEqualToColor(unscopedFill.backgroundColor, UIColor.blueColor.CGColor));
  XCTAssertNil(unscopedFill.mask);
  DisplayList unclipped;
  AppendBegin(unclipped, 1, kViewType, 0, 0, 100, 100);
  AppendDrawView(unclipped, 2);
  AppendDrawView(unclipped, 3);
  AppendEnd(unclipped);
  [host.renderer updateDisplayList:&unclipped];
  XCTAssertEqual(first.layer.mask, intrinsic);
  XCTAssertNil(second.layer.mask);
  XCTAssertEqualObjects(host.subviews, (@[ first, second ]));
}

- (void)testParentPathCoordinatesForTransformedScrolledChildAndDecoration {
  LynxMockView *host = [[LynxMockView alloc] initWithFrame:CGRectMake(0, 0, 200, 200)];
  [host createRendererWithSign:1 andContext:nil];
  LynxMockScrollView *child = [[LynxMockScrollView alloc] initWithFrame:CGRectMake(0, 0, 100, 100)];
  [child createRendererWithSign:2 andContext:nil];
  [host addSubview:child];
  child.contentSize = CGSizeMake(100, 500);
  child.contentOffset = CGPointMake(0, 40);
  float transform[16] = {2, 0, 0, 0, 0, 2, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
  [child.renderer applyTransform:transform];
  DisplayList childList;
  AppendBegin(childList, 2, kViewType, 0, 0, 100, 100);
  AppendRecordBox(childList, 0, 0, 100, 100);
  AppendFill(childList, 0xffff0000, 0);
  AppendEnd(childList);
  [child.renderer updateDisplayList:&childList];
  CALayer *decoration = host.layer.sublayers.firstObject;
  DisplayList list;
  AppendBegin(list, 1, kViewType, 0, 0, 200, 200);
  AppendPathClip(list, lynx::gfx::Path::MakeRect(40, 40, ClipFactory),
                 lynx::gfx::PathFillRule::kNonzero, 20, 30);
  AppendDrawView(list, 2, 10, 20);
  AppendEnd(list);
  [host.renderer updateDisplayList:&list];
  CGPoint inside = CGPointMake(30, 40);
  CGPoint outside = CGPointMake(70, 40);
  XCTAssertTrue(ClipMaskContains(child.layer, [child.layer convertPoint:inside
                                                              fromLayer:host.layer]));
  XCTAssertFalse(ClipMaskContains(child.layer, [child.layer convertPoint:outside
                                                               fromLayer:host.layer]));
  XCTAssertTrue(ClipMaskContains(decoration, [decoration convertPoint:inside
                                                            fromLayer:host.layer]));
  XCTAssertFalse(ClipMaskContains(decoration, [decoration convertPoint:outside
                                                             fromLayer:host.layer]));
  XCTAssertEqual(child.superview, host);
  XCTAssertEqual(decoration.superlayer, host.layer);
  child.contentOffset = CGPointMake(0, 80);
  [child.renderer syncHostDecorationLayers];
  XCTAssertTrue(ClipMaskContains(child.layer, [child.layer convertPoint:inside
                                                              fromLayer:host.layer]));
  XCTAssertTrue(ClipMaskContains(decoration, [decoration convertPoint:inside
                                                            fromLayer:host.layer]));
  // Rotate by ninety degrees, scale by two, and retain the baked translation.
  float rotated[16] = {0, 2, 0, 0, -2, 0, 0, 0, 0, 0, 1, 0, 100, 0, 0, 1};
  [child.renderer applyTransform:rotated];
  [child.renderer syncHostDecorationLayers];
  XCTAssertTrue(ClipMaskContains(child.layer, [child.layer convertPoint:inside
                                                              fromLayer:host.layer]));
  XCTAssertFalse(ClipMaskContains(child.layer, [child.layer convertPoint:outside
                                                               fromLayer:host.layer]));
  XCTAssertTrue(ClipMaskContains(decoration, [decoration convertPoint:inside
                                                            fromLayer:host.layer]));
  XCTAssertFalse(ClipMaskContains(decoration, [decoration convertPoint:outside
                                                             fromLayer:host.layer]));
}

- (void)testMalformedAndMissingPathClipsFailClosedForNativeChildren {
  LynxMockView *host = [[LynxMockView alloc] initWithFrame:CGRectMake(0, 0, 100, 100)];
  [host createRendererWithSign:1 andContext:nil];
  LynxMockView *child = [[LynxMockView alloc] initWithFrame:host.bounds];
  [child createRendererWithSign:2 andContext:nil];
  [host addSubview:child];
  DisplayList list;
  AppendBegin(list, 1, kViewType, 0, 0, 100, 100);
  AppendPathClip(list, lynx::gfx::Path::MakeSvgPathData(lynx::base::String("M0 0H100V100H0Z L"), 1,
                                                        ClipFactory));
  AppendDrawView(list, 2);
  AppendEnd(list);
  [host.renderer updateDisplayList:&list];
  XCTAssertNotNil(child.layer.mask);
  XCTAssertTrue(CGPathIsEmpty(((CAShapeLayer *)child.layer.mask).path));
  XCTAssertFalse(ClipMaskContains(child.layer, CGPointMake(10, 10)));
  DisplayList property;
  SubtreeProperty missing{};
  missing.type = DisplayListSubtreePropertyOpType::kClipPath;
  missing.data.clip_path.path_index = 123;
  property.AddSubtreeProperty(missing);
  [host.renderer applySubtreePropertiesFromList:&property];
  XCTAssertNotNil(host.layer.mask);
  XCTAssertTrue(CGPathIsEmpty(((CAShapeLayer *)host.layer.mask).path));
}

- (void)testScopedPathPreservesRoundedImageMaskAndReuseAcrossRemoval {
  LynxMockView *host = [[LynxMockView alloc] initWithFrame:CGRectMake(0, 0, 100, 100)];
  id manager = OCMClassMock([LynxImageManager class]);
  id context = OCMClassMock([LynxRendererContext class]);
  OCMStub([context imageManagerForID:1]).andReturn(manager);
  LynxDisplayListApplier *applier = [[LynxDisplayListApplier alloc] initWithView:host
                                                                      andContext:context];
  float radii[] = {8, 4, 2, 6, 3, 7, 9, 1};
  DisplayList clipped;
  AppendBegin(clipped, 1, kViewType, 0, 0, 100, 100);
  AppendPathClip(clipped, lynx::gfx::Path::MakeRect(20, 30, ClipFactory));
  AppendRecordBox(clipped, 0, 0, 50, 50, true, radii);
  AppendImage(clipped, 1, 0);
  AppendEnd(clipped);
  [applier applyDisplayList:&clipped];
  UIImageView *image = (UIImageView *)host.subviews.firstObject;
  XCTAssertNotNil(image);
  CALayer *intrinsic = image.layer.mask.mask.mask;
  XCTAssertNotNil(intrinsic);
  XCTAssertTrue(ClipMaskContains(image.layer, CGPointMake(10, 10)));
  XCTAssertFalse(ClipMaskContains(image.layer, CGPointMake(35, 10)));
  DisplayList unscoped;
  AppendBegin(unscoped, 1, kViewType, 0, 0, 100, 100);
  AppendRecordBox(unscoped, 0, 0, 50, 50, true, radii);
  AppendImage(unscoped, 1, 0);
  AppendEnd(unscoped);
  [applier applyDisplayList:&unscoped];
  XCTAssertEqual(host.subviews.firstObject, image);
  XCTAssertTrue([image.layer.mask isKindOfClass:[CAShapeLayer class]]);
  XCTAssertNil(image.layer.mask.mask);
  XCTAssertTrue(ClipMaskContains(image.layer, CGPointMake(35, 10)));
  [applier reset];
  XCTAssertNil(image.superview);
}

- (void)testOwnerClipSurvivesContentReplacementAndApplierTeardownClearsChildClip {
  LynxMockView *host = [[LynxMockView alloc] initWithFrame:CGRectMake(0, 0, 100, 100)];
  LynxMockView *child = [[LynxMockView alloc] initWithFrame:host.bounds];
  [child createRendererWithSign:2 andContext:nil];
  [host addSubview:child];
  DisplayList content;
  AppendBegin(content, 1, kViewType, 0, 0, 100, 100);
  AppendDrawView(content, 2);
  AppendEnd(content);
  {
    LynxDisplayListApplier *applier = [[LynxDisplayListApplier alloc] initWithView:host
                                                                        andContext:nil];
    DisplayList properties;
    AddOwnerClip(properties, lynx::gfx::Path::MakeRect(30, 40, ClipFactory));
    [applier applyClipPathProperty:properties.GetSubtreePropertiesData()[0] fromList:&properties];
    [applier applyDisplayList:&content];
    XCTAssertTrue(ClipMaskContains(host.layer, CGPointMake(10, 10)));
    XCTAssertFalse(ClipMaskContains(host.layer, CGPointMake(50, 10)));
    DisplayList scoped;
    AppendBegin(scoped, 1, kViewType, 0, 0, 100, 100);
    AppendPathClip(scoped, lynx::gfx::Path::MakeRect(10, 10, ClipFactory));
    AppendDrawView(scoped, 2);
    AppendEnd(scoped);
    [applier applyDisplayList:&scoped];
    XCTAssertFalse(ClipMaskContains(host.layer, CGPointMake(50, 10)));
    XCTAssertNotNil(child.layer.mask);
  }
  XCTAssertNil(child.layer.mask);
  XCTAssertNil(host.layer.mask);
  XCTAssertEqual(child.superview, host);
}

- (void)testOldParentResetDoesNotClearMovedChildNewParentClip {
  LynxMockView *firstHost = [[LynxMockView alloc] initWithFrame:CGRectMake(0, 0, 100, 100)];
  LynxMockView *secondHost = [[LynxMockView alloc] initWithFrame:firstHost.bounds];
  LynxMockView *child = [[LynxMockView alloc] initWithFrame:firstHost.bounds];
  [child createRendererWithSign:2 andContext:nil];
  [firstHost addSubview:child];
  LynxDisplayListApplier *first = [[LynxDisplayListApplier alloc] initWithView:firstHost
                                                                    andContext:nil];
  LynxDisplayListApplier *second = [[LynxDisplayListApplier alloc] initWithView:secondHost
                                                                     andContext:nil];
  DisplayList firstList;
  AppendBegin(firstList, 1, kViewType, 0, 0, 100, 100);
  AppendPathClip(firstList, lynx::gfx::Path::MakeRect(10, 10, ClipFactory));
  AppendDrawView(firstList, 2);
  AppendEnd(firstList);
  [first applyDisplayList:&firstList];
  XCTAssertFalse(ClipMaskContains(child.layer, CGPointMake(30, 30)));
  [child.renderer detachHostDecorationLayers];
  XCTAssertNil(child.layer.mask);
  [secondHost addSubview:child];
  DisplayList secondList;
  AppendBegin(secondList, 3, kViewType, 0, 0, 100, 100);
  AppendPathClip(secondList, lynx::gfx::Path::MakeRect(50, 50, ClipFactory));
  AppendDrawView(secondList, 2);
  AppendEnd(secondList);
  [second applyDisplayList:&secondList];
  XCTAssertTrue(ClipMaskContains(child.layer, CGPointMake(30, 30)));
  [first reset];
  XCTAssertNotNil(child.layer.mask);
  XCTAssertTrue(ClipMaskContains(child.layer, CGPointMake(30, 30)));
  XCTAssertFalse(ClipMaskContains(child.layer, CGPointMake(60, 30)));
  [second reset];
  XCTAssertNil(child.layer.mask);
}

- (void)testPerspectiveParentClipProjectsNativeChildAndDecorationWithoutAffineTruncation {
  LynxMockView *host = [[LynxMockView alloc] initWithFrame:CGRectMake(0, 0, 200, 100)];
  [host createRendererWithSign:1 andContext:nil];
  LynxMockView *child = [[LynxMockView alloc] initWithFrame:CGRectMake(0, 0, 150, 100)];
  [child createRendererWithSign:2 andContext:nil];
  [host addSubview:child];
  float projective[16] = {1, 0, 0, .002f, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
  [child.renderer applyTransform:projective];
  DisplayList childContent;
  AppendBegin(childContent, 2, kViewType, 0, 0, 150, 100);
  AppendRecordBox(childContent, 0, 0, 150, 100);
  AppendFill(childContent, 0xffff0000, 0);
  AppendEnd(childContent);
  [child.renderer updateDisplayList:&childContent];
  CALayer *decoration = host.layer.sublayers.firstObject;
  child.backgroundColor = UIColor.redColor;
  DisplayList owner;
  AddOwnerClip(owner, lynx::gfx::Path::MakeRect(140, 100, ClipFactory));
  [child.renderer applySubtreePropertiesFromList:&owner];
  DisplayList parentContent;
  AppendBegin(parentContent, 1, kViewType, 0, 0, 200, 100);
  AppendPathClip(parentContent, lynx::gfx::Path::MakeRect(100, 100, ClipFactory));
  AppendDrawView(parentContent, 2);
  AppendEnd(parentContent);
  [host.renderer updateDisplayList:&parentContent];
  CGPoint localInside = CGPointMake(110, 20);
  CGPoint sourceInside = ProjectPoint(localInside, child.layer.transform);
  XCTAssertEqualWithAccuracy(sourceInside.x, 90.163934, .001);
  XCTAssertTrue(ClipMaskContains(child.layer, localInside));
  XCTAssertFalse(ClipMaskContains(child.layer, CGPointMake(130, 20)));
  XCTAssertTrue(ClipMaskContains(decoration, localInside));
  XCTAssertFalse(ClipMaskContains(decoration, CGPointMake(130, 20)));
  CAShapeLayer *parentMask = (CAShapeLayer *)child.layer.mask.mask;
  CGPoint projectedEdge = ProjectPoint(CGPointMake(100, 0), parentMask.transform);
  XCTAssertEqualWithAccuracy(projectedEdge.x, 125, .001);
  NSArray<NSNumber *> *pixels =
      [self hostedRedValuesForView:host
                            points:@[
                              [NSValue valueWithCGPoint:CGPointMake(90, 16)],
                              [NSValue valueWithCGPoint:CGPointMake(99, 90)]
                            ]
                              name:@"perspective-owner-parent"];
  XCTAssertGreaterThan(pixels[0].unsignedCharValue, 200);
  XCTAssertEqual(pixels[1].unsignedCharValue, 0);
  XCTAssertEqual(child.superview, host);
  XCTAssertEqual(decoration.superlayer, host.layer);
}

- (void)testProjectivePoleAndSingularChildClipFailClosed {
  LynxMockView *host = [[LynxMockView alloc] initWithFrame:CGRectMake(0, 0, 200, 100)];
  [host createRendererWithSign:1 andContext:nil];
  LynxMockView *child = [[LynxMockView alloc] initWithFrame:CGRectMake(0, 0, 150, 100)];
  [child createRendererWithSign:2 andContext:nil];
  [host addSubview:child];
  DisplayList content;
  AppendBegin(content, 1, kViewType, 0, 0, 200, 100);
  AppendPathClip(content, lynx::gfx::Path::MakeRect(100, 100, ClipFactory));
  AppendDrawView(content, 2);
  AppendEnd(content);
  float pole[16] = {1, 0, 0, .02f, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
  [child.renderer applyTransform:pole];
  [host.renderer updateDisplayList:&content];
  XCTAssertTrue(CGPathIsEmpty(((CAShapeLayer *)child.layer.mask).path));
  float singular[16] = {0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
  [child.renderer applyTransform:singular];
  [child.renderer syncHostDecorationLayers];
  XCTAssertTrue(CGPathIsEmpty(((CAShapeLayer *)child.layer.mask).path));
}

- (void)testIntrinsicMaskOffsetCoordinatesSurviveOwnerIntersectionAndRemoval {
  LynxMockView *host = [[LynxMockView alloc] initWithFrame:CGRectMake(0, 0, 100, 100)];
  [host createRendererWithSign:1 andContext:nil];
  host.backgroundColor = UIColor.redColor;
  CAShapeLayer *intrinsic = [CAShapeLayer layer];
  intrinsic.frame = CGRectMake(20, 10, 70, 80);
  intrinsic.bounds = CGRectMake(10, 20, 70, 80);
  intrinsic.path = [UIBezierPath bezierPathWithRect:CGRectMake(15, 25, 30, 30)].CGPath;
  host.layer.mask = intrinsic;
  XCTAssertTrue(ClipMaskContains(host.layer, CGPointMake(29, 25)));
  XCTAssertFalse(ClipMaskContains(host.layer, CGPointMake(20, 25)));
  NSArray<NSNumber *> *baseline =
      [self renderedRedValuesForView:host
                              points:@[
                                [NSValue valueWithCGPoint:CGPointMake(29, 25)],
                                [NSValue valueWithCGPoint:CGPointMake(20, 25)],
                                [NSValue valueWithCGPoint:CGPointMake(40, 25)]
                              ]
                                name:@"intrinsic-offset-baseline"];
  XCTAssertGreaterThan(baseline[0].unsignedCharValue, 200);
  XCTAssertEqual(baseline[1].unsignedCharValue, 0);
  XCTAssertGreaterThan(baseline[2].unsignedCharValue, 200);
  DisplayList properties;
  AddOwnerClip(properties, lynx::gfx::Path::MakeRect(35, 40, ClipFactory));
  [host.renderer applySubtreePropertiesFromList:&properties];
  XCTAssertTrue(ClipMaskContains(host.layer, CGPointMake(29, 25)));
  XCTAssertFalse(ClipMaskContains(host.layer, CGPointMake(40, 25)));
  XCTAssertFalse(ClipMaskContains(host.layer, CGPointMake(20, 25)));
  NSArray<NSNumber *> *clipped =
      [self renderedRedValuesForView:host
                              points:@[
                                [NSValue valueWithCGPoint:CGPointMake(29, 25)],
                                [NSValue valueWithCGPoint:CGPointMake(40, 25)]
                              ]
                                name:@"intrinsic-offset-intersection"];
  XCTAssertGreaterThan(clipped[0].unsignedCharValue, 200);
  XCTAssertEqual(clipped[1].unsignedCharValue, 0);
  DisplayList removal;
  AddOwnerClip(removal, nullptr);
  [host.renderer applySubtreePropertiesFromList:&removal];
  XCTAssertEqual(host.layer.mask, intrinsic);
  XCTAssertTrue(ClipMaskContains(host.layer, CGPointMake(40, 25)));
  NSArray<NSNumber *> *restored =
      [self renderedRedValuesForView:host
                              points:@[ [NSValue valueWithCGPoint:CGPointMake(40, 25)] ]
                                name:@"intrinsic-offset-restoration"];
  XCTAssertGreaterThan(restored[0].unsignedCharValue, 200);
}

- (void)testParentSublayerTransformAndNonDefaultChildAnchorAndBounds {
  LynxMockView *host = [[LynxMockView alloc] initWithFrame:CGRectMake(0, 0, 100, 100)];
  host.bounds = CGRectMake(7, 11, 100, 100);
  host.layer.anchorPoint = CGPointMake(.1, .3);
  CATransform3D children = CATransform3DMakeScale(.8, .9, 1);
  children = CATransform3DConcat(children, CATransform3DMakeTranslation(4, 6, 0));
  children.m14 = .001;
  host.layer.sublayerTransform = children;
  LynxMockView *child = [[LynxMockView alloc] initWithFrame:CGRectMake(0, 0, 80, 70)];
  [child createRendererWithSign:2 andContext:nil];
  [host addSubview:child];
  child.bounds = CGRectMake(5, 13, 80, 70);
  child.layer.anchorPoint = CGPointMake(.2, .7);
  child.layer.position = CGPointMake(40, 55);
  child.layer.transform = CATransform3DMakeRotation(.2, 0, 0, 1);
  child.backgroundColor = UIColor.redColor;
  CGPoint localInside = CGPointMake(45, 48);
  CGPoint sourceInside = [host.layer convertPoint:localInside fromLayer:child.layer];
  CGRect clipBounds = CGRectMake(sourceInside.x - 10, sourceInside.y - 10, 20, 20);
  CAShapeLayer *clip = [CAShapeLayer layer];
  clip.path = [UIBezierPath bezierPathWithRect:clipBounds].CGPath;
  [child.renderer applyParentClips:@[ clip ] fromLayer:host.layer];
  XCTAssertTrue(ClipMaskContains(child.layer, localInside));
  CGPoint sourceOutside = CGPointMake(sourceInside.x + 20, sourceInside.y);
  CGPoint localOutside = [child.layer convertPoint:sourceOutside fromLayer:host.layer];
  XCTAssertFalse(ClipMaskContains(child.layer, localOutside));
  XCTAssertEqualWithAccuracy(child.layer.anchorPoint.x, .2, .001);
  XCTAssertEqualWithAccuracy(child.bounds.origin.y, 13, .001);
  NSArray<NSNumber *> *pixels = [self hostedRedValuesForView:host
                                                      points:@[
                                                        [NSValue valueWithCGPoint:sourceInside],
                                                        [NSValue valueWithCGPoint:sourceOutside]
                                                      ]
                                                        name:@"sublayer-transform-anchor-bounds"];
  XCTAssertGreaterThan(pixels[0].unsignedCharValue, 200);
  XCTAssertEqual(pixels[1].unsignedCharValue, 0);
}

- (void)testPathClipKeepsFollowingFillInsideHostAndEarlierShadowOutside {
  UIView *parent = [[UIView alloc] initWithFrame:CGRectMake(0, 0, 200, 200)];
  LynxMockView *host = [[LynxMockView alloc] initWithFrame:CGRectMake(20, 30, 100, 80)];
  [host createRendererWithSign:1 andContext:nil];
  [parent addSubview:host];
  DisplayList content;
  AppendBegin(content, 1, kViewType, 0, 0, 100, 80);
  AppendRecordBox(content, 0, 0, 100, 80);
  AppendBoxShadow(content, 0, 0, false);
  AppendPathClip(content, lynx::gfx::Path::MakeRect(40, 40, ClipFactory));
  AppendFill(content, 0xffff0000, 0);
  AppendEnd(content);
  [host.renderer updateDisplayList:&content];
  XCTAssertEqual(parent.layer.sublayers.count, 2u);
  XCTAssertEqual(parent.layer.sublayers.lastObject, host.layer);
  XCTAssertNil(parent.layer.sublayers.firstObject.mask);
  XCTAssertEqual(host.layer.sublayers.count, 1u);
  CALayer *fill = host.layer.sublayers.firstObject;
  XCTAssertTrue(CGColorEqualToColor(fill.backgroundColor, UIColor.redColor.CGColor));
  XCTAssertTrue(ClipMaskContains(fill, CGPointMake(20, 20)));
  XCTAssertFalse(ClipMaskContains(fill, CGPointMake(60, 20)));
}

- (void)testUnclippedParentRedrawDoesNotSynchronizeChildClips {
  LynxMockView *host = [[LynxMockView alloc] initWithFrame:CGRectMake(0, 0, 100, 100)];
  [host createRendererWithSign:1 andContext:nil];
  for (BOOL hasOwnerClip : {NO, YES}) {
    LynxMockView *child = [[LynxMockView alloc] initWithFrame:CGRectMake(0, 0, 80, 80)];
    [child createRendererWithSign:2 andContext:nil];
    [host addSubview:child];
    [child.renderer updateLayoutOffsetIfNeeded:CGPointZero];
    DisplayList properties;
    if (hasOwnerClip) {
      AddOwnerClip(properties, lynx::gfx::Path::MakeRect(20, 20, ClipFactory));
      [child.renderer applySubtreePropertiesFromList:&properties];
    }
    CALayer *mask = child.layer.mask;
    id renderer = OCMPartialMock(child.renderer);
    OCMReject([renderer applyParentClips:[OCMArg any] fromLayer:[OCMArg any]]);
    OCMReject([renderer clearParentClipsFromLayer:[OCMArg any]]);
    DisplayList content;
    AppendBegin(content, 1, kViewType, 0, 0, 100, 100);
    AppendDrawView(content, 2);
    AppendEnd(content);
    [host.renderer updateDisplayList:&content];
    [host.renderer updateDisplayList:&content];
    XCTAssertEqual(child.layer.mask, mask);
    if (hasOwnerClip) {
      XCTAssertTrue(ClipMaskContains(child.layer, CGPointMake(10, 10)));
      XCTAssertFalse(ClipMaskContains(child.layer, CGPointMake(30, 10)));
    } else {
      XCTAssertNil([child.renderer valueForKey:@"applier"]);
    }
    [host.renderer updateDisplayList:nullptr];
    [renderer stopMocking];
    [child removeFromSuperview];
  }
}

- (void)testRendererContentUpdateSynchronizesClipMasksOnce {
  LynxMockView *host = [[LynxMockView alloc] initWithFrame:CGRectMake(0, 0, 100, 100)];
  [host createRendererWithSign:1 andContext:nil];
  DisplayList initial;
  [host.renderer updateDisplayList:&initial];
  LynxDisplayListApplier *applier = [host.renderer valueForKey:@"applier"];
  id mock = OCMPartialMock(applier);
  __block NSUInteger syncCount = 0;
  OCMStub([mock syncHostDecorationLayers])
      .andDo(^(NSInvocation *invocation) {
        syncCount++;
      })
      .andForwardToRealObject();
  DisplayList properties;
  AddOwnerClip(properties, lynx::gfx::Path::MakeRect(30, 30, ClipFactory));
  [host.renderer applySubtreePropertiesFromList:&properties];
  DisplayList content;
  AppendBegin(content, 1, kViewType, 0, 0, 100, 100);
  AppendEnd(content);
  for (int update = 0; update < 2; ++update) {
    syncCount = 0;
    [host.renderer updateDisplayList:&content];
    XCTAssertEqual(syncCount, 1u);
    XCTAssertTrue(ClipMaskContains(host.layer, CGPointMake(10, 10)));
    XCTAssertFalse(ClipMaskContains(host.layer, CGPointMake(40, 10)));
  }
  syncCount = 0;
  [host.renderer updateDisplayList:nullptr];
  XCTAssertEqual(syncCount, 1u);
  XCTAssertTrue(ClipMaskContains(host.layer, CGPointMake(10, 10)));
  XCTAssertFalse(ClipMaskContains(host.layer, CGPointMake(40, 10)));
  [mock stopMocking];
}

- (void)testRealScrollerCallbackPinsRootOverflowMaskWithPathIntersection {
  for (BOOL rounded : {NO, YES}) {
    for (BOOL hasPathClip : {NO, YES}) {
      LynxUIScroller *scroller = [[LynxUIScroller alloc] init];
      LynxUIMockContext *context = [LynxUIUnitTestUtils initUIMockContextWithUI:scroller];
      [scroller updateFrame:CGRectMake(0, 0, 100, 100)
                  withPadding:UIEdgeInsetsZero
                       border:UIEdgeInsetsZero
          withLayoutAnimation:NO];
      [LynxPropsProcessor updateProp:@YES withKey:@"scroll-y" forUI:scroller];
      [scroller propsDidUpdate];
      UIView<LynxRendererHost> *view = (UIView<LynxRendererHost> *)scroller.view;
      view.renderer = [[LynxRenderer alloc] initWithRenderHost:view andSign:1 andContext:nil];
      DisplayList content;
      AppendBegin(content, 1, kViewType, 0, 0, 100, 100);
      const float radii[] = {4, 6, 10, 8, 12, 14, 16, 18};
      AppendClipRect(content, 2, 3, 96, 94, rounded, radii);
      AppendEnd(content);
      [view.renderer updateDisplayList:&content];
      CAShapeLayer *overflow = (CAShapeLayer *)view.layer.mask;
      XCTAssertTrue([overflow isKindOfClass:[CAShapeLayer class]]);
      DisplayList properties;
      if (hasPathClip) {
        AddOwnerClip(properties, lynx::gfx::Path::MakeRect(80, 80, ClipFactory));
        [view.renderer applySubtreePropertiesFromList:&properties];
      }
      view.backgroundColor = UIColor.redColor;
      UIScrollView *scrollView = (UIScrollView *)view;
      scrollView.contentSize = CGSizeMake(100, 1000);
      for (CGFloat offset : {40., 120.}) {
        scrollView.delegate = nil;
        scrollView.contentOffset = CGPointMake(0, offset);
        scrollView.delegate = scroller;
        [scroller scrollViewDidScroll:scrollView];
        XCTAssertTrue(CGRectEqualToRect(overflow.frame, view.layer.bounds));
        XCTAssertTrue(ClipMaskContains(view.layer, CGPointMake(20, offset + 20)));
        XCTAssertFalse(ClipMaskContains(view.layer, CGPointMake(1, offset + 20)));
        if (hasPathClip) {
          XCTAssertFalse(ClipMaskContains(view.layer, CGPointMake(90, offset + 20)));
        }
        NSArray<NSNumber *> *pixels = [self
            renderedRedValuesForView:view
                              points:@[
                                [NSValue valueWithCGPoint:CGPointMake(20, offset + 20)],
                                [NSValue valueWithCGPoint:CGPointMake(1, offset + 20)],
                                [NSValue valueWithCGPoint:CGPointMake(90, offset + 20)]
                              ]
                                name:[NSString stringWithFormat:@"root-overflow-%d-%d-%g", rounded,
                                                                hasPathClip, offset]];
        XCTAssertGreaterThan(pixels[0].unsignedCharValue, 200);
        XCTAssertEqual(pixels[1].unsignedCharValue, 0);
        if (hasPathClip) {
          XCTAssertEqual(pixels[2].unsignedCharValue, 0);
        } else {
          XCTAssertGreaterThan(pixels[2].unsignedCharValue, 200);
        }
      }
      if (hasPathClip) {
        DisplayList removal;
        AddOwnerClip(removal, nullptr);
        [view.renderer applySubtreePropertiesFromList:&removal];
      }
      XCTAssertEqual(view.layer.mask, overflow);
      XCTAssertTrue(ClipMaskContains(view.layer, CGPointMake(90, 140)));
      XCTAssertEqual(scroller.context, context.mockUIContext);
    }
  }
}

- (void)testScrollHostExternalIntrinsicMaskKeepsItsGeometry {
  LynxMockScrollView *view = [[LynxMockScrollView alloc] initWithFrame:CGRectMake(0, 0, 100, 100)];
  view.renderer = [[LynxRenderer alloc] initWithRenderHost:view andSign:1 andContext:nil];
  CAShapeLayer *intrinsic = [CAShapeLayer layer];
  intrinsic.frame = CGRectMake(20, 10, 70, 80);
  intrinsic.bounds = CGRectMake(10, 20, 70, 80);
  intrinsic.path = [UIBezierPath bezierPathWithRect:CGRectMake(15, 25, 30, 30)].CGPath;
  CGRect frame = intrinsic.frame;
  CGRect bounds = intrinsic.bounds;
  view.layer.mask = intrinsic;
  DisplayList properties;
  AddOwnerClip(properties, lynx::gfx::Path::MakeRect(100, 100, ClipFactory));
  [view.renderer applySubtreePropertiesFromList:&properties];
  view.contentSize = CGSizeMake(100, 1000);
  view.contentOffset = CGPointMake(0, 40);
  [view.renderer syncHostDecorationLayers];
  XCTAssertTrue(CGRectEqualToRect(intrinsic.frame, frame));
  XCTAssertTrue(CGRectEqualToRect(intrinsic.bounds, bounds));
  DisplayList removal;
  AddOwnerClip(removal, nullptr);
  [view.renderer applySubtreePropertiesFromList:&removal];
  XCTAssertEqual(view.layer.mask, intrinsic);
  XCTAssertTrue(ClipMaskContains(view.layer, CGPointMake(29, 25)));
  XCTAssertFalse(ClipMaskContains(view.layer, CGPointMake(20, 25)));
}

- (void)testRealScrollerCallbackRefreshesProjectiveRendererMasksAndDecorations {
  LynxUIScroller *scroller = [[LynxUIScroller alloc] init];
  LynxUIMockContext *context = [LynxUIUnitTestUtils initUIMockContextWithUI:scroller];
  [scroller updateFrame:CGRectMake(0, 0, 100, 100)
              withPadding:UIEdgeInsetsZero
                   border:UIEdgeInsetsZero
      withLayoutAnimation:NO];
  [LynxPropsProcessor updateProp:@YES withKey:@"scroll-y" forUI:scroller];
  [scroller propsDidUpdate];
  UIView<LynxRendererHost> *view = (UIView<LynxRendererHost> *)scroller.view;
  view.renderer = [[LynxRenderer alloc] initWithRenderHost:view andSign:2 andContext:nil];
  UIView *parent = [[UIView alloc] initWithFrame:CGRectMake(0, 0, 200, 100)];
  [parent addSubview:view];
  float projective[16] = {1, 0, 0, .002f, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
  [view.renderer applyTransform:projective];
  DisplayList content;
  AppendBegin(content, 2, kViewType, 0, 0, 100, 100);
  AppendRecordBox(content, 0, 0, 100, 100);
  AppendFill(content, 0xffff0000, 0);
  AppendEnd(content);
  [view.renderer updateDisplayList:&content];
  CALayer *decoration = parent.layer.sublayers.firstObject;
  XCTAssertNotEqual(decoration, view.layer);
  DisplayList owner;
  AddOwnerClip(owner, lynx::gfx::Path::MakeRect(80, 80, ClipFactory));
  [view.renderer applySubtreePropertiesFromList:&owner];
  CAShapeLayer *parentClip = [CAShapeLayer layer];
  parentClip.path = [UIBezierPath bezierPathWithRect:CGRectMake(0, 0, 100, 100)].CGPath;
  [view.renderer applyParentClips:@[ parentClip ] fromLayer:parent.layer];
  UIScrollView *scrollView = (UIScrollView *)view;
  scrollView.contentSize = CGSizeMake(100, 1000);
  for (CGFloat offset : {40., 80.}) {
    if (offset == 80) {
      [LynxPropsProcessor updateProp:@[ @(LynxBasicShapeTypePath), @"M40 40H60V60H40Z" ]
                             withKey:@"clip-path"
                               forUI:scroller];
    }
    scrollView.delegate = nil;
    scrollView.contentOffset = CGPointMake(0, offset);
    scrollView.delegate = scroller;
    [scroller scrollViewDidScroll:scrollView];
    CGPoint inside = [view.layer convertPoint:CGPointMake(30, 30) fromLayer:parent.layer];
    CGPoint outside = [view.layer convertPoint:CGPointMake(110, 30) fromLayer:parent.layer];
    XCTAssertTrue(ClipMaskContains(view.layer, inside));
    XCTAssertFalse(ClipMaskContains(view.layer, outside));
    XCTAssertFalse(ClipMaskContains(view.layer, CGPointMake(80.5, view.bounds.origin.y + 20)));
    XCTAssertTrue(ClipMaskContains(decoration, [decoration convertPoint:CGPointMake(30, 30)
                                                              fromLayer:parent.layer]));
    XCTAssertEqual(parent.layer.sublayers.firstObject, decoration);
    XCTAssertTrue(CGPointEqualToPoint(view.layer.mask.position, CGPointZero));
  }
  DisplayList clear;
  AddOwnerClip(clear, nullptr);
  [view.renderer applySubtreePropertiesFromList:&clear];
  [view.renderer applyParentClips:@[] fromLayer:nil];
  [scroller scrollViewDidScroll:scrollView];
  XCTAssertNil(view.layer.mask);
  XCTAssertNil(decoration.mask);
  XCTAssertEqual(scroller.context, context.mockUIContext);
}

- (void)testRealScrollerCallbackKeepsLegacyExternalMaskAndClipPathBehavior {
  LynxUIScroller *scroller = [[LynxUIScroller alloc] init];
  LynxUIMockContext *context = [LynxUIUnitTestUtils initUIMockContextWithUI:scroller];
  [scroller updateFrame:CGRectMake(0, 0, 100, 100)
              withPadding:UIEdgeInsetsZero
                   border:UIEdgeInsetsZero
      withLayoutAnimation:NO];
  [LynxPropsProcessor updateProp:@YES withKey:@"scroll-y" forUI:scroller];
  [scroller propsDidUpdate];
  UIScrollView *view = (UIScrollView *)scroller.view;
  view.contentSize = CGSizeMake(100, 1000);
  CAShapeLayer *external = [CAShapeLayer layer];
  external.frame = view.bounds;
  external.path = [UIBezierPath bezierPathWithRect:CGRectMake(10, 10, 80, 80)].CGPath;
  view.layer.mask = external;
  view.delegate = nil;
  view.contentOffset = CGPointMake(0, 40);
  view.delegate = scroller;
  [scroller scrollViewDidScroll:view];
  XCTAssertEqual(view.layer.mask, external);
  XCTAssertTrue(CGRectEqualToRect(external.frame, view.bounds));
  [LynxPropsProcessor updateProp:@[ @(LynxBasicShapeTypePath), @"M40 40H60V60H40Z" ]
                         withKey:@"clip-path"
                           forUI:scroller];
  [scroller scrollViewDidScroll:view];
  XCTAssertNotEqual(view.layer.mask, external);
  XCTAssertTrue(CGRectEqualToRect(CGPathGetPathBoundingBox(((CAShapeLayer *)view.layer.mask).path),
                                  CGRectMake(40, 40, 20, 20)));
  XCTAssertEqual(scroller.context, context.mockUIContext);
}

- (void)testPositiveScalarHomogeneousTransformsPreserveNativeClipCoverage {
  LynxMockView *host = [[LynxMockView alloc] initWithFrame:CGRectMake(0, 0, 100, 100)];
  [host createRendererWithSign:1 andContext:nil];
  LynxMockView *child = [[LynxMockView alloc] initWithFrame:host.bounds];
  [child createRendererWithSign:2 andContext:nil];
  [host addSubview:child];
  DisplayList childContent;
  AppendBegin(childContent, 2, kViewType, 0, 0, 100, 100);
  AppendRecordBox(childContent, 0, 0, 100, 100);
  AppendFill(childContent, 0xffff0000, 0);
  AppendEnd(childContent);
  [child.renderer updateDisplayList:&childContent];
  CALayer *decoration = host.layer.sublayers.firstObject;
  DisplayList owner;
  AddOwnerClip(owner, lynx::gfx::Path::MakeRect(80, 80, ClipFactory));
  [child.renderer applySubtreePropertiesFromList:&owner];
  DisplayList content;
  AppendBegin(content, 1, kViewType, 0, 0, 100, 100);
  AppendPathClip(content, lynx::gfx::Path::MakeRect(40, 40, ClipFactory));
  AppendDrawView(content, 2);
  AppendEnd(content);
  for (float scale : {1.f, 1e-8f, 1e-12f}) {
    float scalar[16] = {scale, 0, 0, 0, 0, scale, 0, 0, 0, 0, scale, 0, 0, 0, 0, scale};
    [child.renderer applyTransform:scalar];
    [host.renderer updateDisplayList:&content];
    CGPoint native = [host.layer convertPoint:CGPointMake(20, 20) fromLayer:child.layer];
    XCTAssertEqualWithAccuracy(native.x, 20, .001);
    XCTAssertEqualWithAccuracy(native.y, 20, .001);
    XCTAssertTrue(ClipMaskContains(child.layer, CGPointMake(20, 20)));
    XCTAssertFalse(ClipMaskContains(child.layer, CGPointMake(50, 20)));
    XCTAssertTrue(ClipMaskContains(decoration, CGPointMake(20, 20)));
    XCTAssertFalse(ClipMaskContains(decoration, CGPointMake(50, 20)));
    XCTAssertFalse(CGPathIsEmpty(((CAShapeLayer *)child.layer.mask).path));
    NSArray<NSNumber *> *pixels =
        [self hostedRedValuesForView:host
                              points:@[
                                [NSValue valueWithCGPoint:CGPointMake(20, 20)],
                                [NSValue valueWithCGPoint:CGPointMake(50, 20)]
                              ]
                                name:[NSString stringWithFormat:@"homogeneous-scalar-%g", scale]];
    XCTAssertGreaterThan(pixels[0].unsignedCharValue, 200);
    XCTAssertEqual(pixels[1].unsignedCharValue, 0);
  }
}

- (void)testActualUIKitChildrenReceiveDrawViewScopesAcrossContentReplacement {
  for (BOOL useScroller : {NO, YES}) {
    LynxMockView *host = [[LynxMockView alloc] initWithFrame:CGRectMake(0, 0, 100, 100)];
    [host createRendererWithSign:1 andContext:nil];
    LynxUIMockContext *context = nil;
    UIView *child = nil;
    if (useScroller) {
      LynxUIScroller *scroller = [[LynxUIScroller alloc] init];
      context = [LynxUIUnitTestUtils initUIMockContextWithUI:scroller];
      [scroller updateFrame:CGRectMake(0, 0, 100, 100)
                  withPadding:UIEdgeInsetsZero
                       border:UIEdgeInsetsZero
          withLayoutAnimation:NO];
      child = scroller.view;
    } else {
      child = [[UIView alloc] initWithFrame:host.bounds];
    }
    UIView<LynxRendererHost> *rendererHost = (UIView<LynxRendererHost> *)child;
    rendererHost.renderer = [[LynxRenderer alloc] initWithRenderHost:rendererHost
                                                             andSign:2
                                                          andContext:nil];
    [host addSubview:child];
    DisplayList childContent;
    AppendBegin(childContent, 2, kViewType, 0, 0, 100, 100);
    AppendRecordBox(childContent, 0, 0, 100, 100);
    AppendFill(childContent, 0xffff0000, 0);
    AppendEnd(childContent);
    [rendererHost.renderer updateDisplayList:&childContent];
    CALayer *oldDecoration = host.layer.sublayers.firstObject;
    DisplayList parentContent;
    AppendBegin(parentContent, 1, kViewType, 0, 0, 100, 100);
    AppendBegin(parentContent, 3, kViewType, 10, 20, 40, 40);
    AppendPathClip(parentContent, lynx::gfx::Path::MakeRect(40, 40, ClipFactory));
    AppendDrawView(parentContent, 2);
    AppendEnd(parentContent);
    AppendEnd(parentContent);
    [host.renderer updateDisplayList:&parentContent];
    XCTAssertNotNil(child.layer.mask);
    XCTAssertNotNil(oldDecoration.mask);
    XCTAssertTrue(ClipMaskContains(child.layer, CGPointMake(20, 30)));
    XCTAssertFalse(ClipMaskContains(child.layer, CGPointMake(70, 30)));
    DisplayList replacement;
    AppendBegin(replacement, 2, kViewType, 0, 0, 100, 100);
    AppendRecordBox(replacement, 0, 0, 90, 100);
    AppendFill(replacement, 0xffff0000, 0);
    AppendEnd(replacement);
    [rendererHost.renderer updateDisplayList:&replacement];
    CALayer *decoration = host.layer.sublayers.firstObject;
    XCTAssertNotEqual(decoration, oldDecoration);
    XCTAssertNil(oldDecoration.superlayer);
    XCTAssertNotNil(decoration.mask);
    XCTAssertTrue(ClipMaskContains(child.layer, CGPointMake(20, 30)));
    XCTAssertFalse(ClipMaskContains(decoration, CGPointMake(70, 30)));
    NSArray<NSNumber *> *scoped =
        [self renderedRedValuesForView:host
                                points:@[
                                  [NSValue valueWithCGPoint:CGPointMake(20, 30)],
                                  [NSValue valueWithCGPoint:CGPointMake(70, 30)]
                                ]
                                  name:useScroller ? @"draw-view-scroller-replacement"
                                                   : @"draw-view-UIView-replacement"];
    XCTAssertGreaterThan(scoped[0].unsignedCharValue, 200);
    XCTAssertEqual(scoped[1].unsignedCharValue, 0);
    DisplayList unscoped;
    AppendBegin(unscoped, 1, kViewType, 0, 0, 100, 100);
    AppendDrawView(unscoped, 2);
    AppendEnd(unscoped);
    [host.renderer updateDisplayList:&unscoped];
    XCTAssertNil(child.layer.mask);
    XCTAssertNil(decoration.mask);
    XCTAssertEqual(host.layer.sublayers.firstObject, decoration);
    XCTAssertEqual(child.superview, host);
    NSArray<NSNumber *> *cleared =
        [self renderedRedValuesForView:host
                                points:@[ [NSValue valueWithCGPoint:CGPointMake(70, 30)] ]
                                  name:useScroller ? @"draw-view-scroller-cleared"
                                                   : @"draw-view-UIView-cleared"];
    XCTAssertGreaterThan(cleared[0].unsignedCharValue, 200);
    if (context != nil) XCTAssertEqual(context.mockUI.view, child);
  }
}

@end
