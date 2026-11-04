// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxDisplayListSurfacePainter.h"
#import "scglayer/LynxSCGLayer.h"

#import <Lynx/LynxBackgroundUtils.h>

#import <Lynx/LynxBackgroundRenderer.h>
#import <Lynx/LynxContext+Internal.h>
#import <Lynx/LynxGradient.h>
#import <Lynx/LynxImageManager.h>
#import <Lynx/LynxRendererContext.h>
#import <Lynx/LynxService.h>
#import <Lynx/LynxServiceTextProtocol.h>
#import <Lynx/LynxTextRenderManager.h>
#import <Lynx/LynxTextRenderer.h>
#import <Lynx/LynxUIContext.h>

#include <algorithm>
#include <cmath>
#include <map>

using lynx::base::Vector;
using lynx::tasm::DisplayListItem;
using lynx::tasm::DisplayListOpType;
using lynx::tasm::DisplayListReader;
using lynx::tasm::RoundedRectangle;

namespace {

CGRect BoxRect(const RoundedRectangle &box, CGPoint offset = CGPointZero) {
  return CGRectMake(box.GetX() + offset.x, box.GetY() + offset.y, box.GetWidth(), box.GetHeight());
}

BOOL ValidRect(CGRect rect) {
  return std::isfinite(rect.origin.x) && std::isfinite(rect.origin.y) &&
         std::isfinite(CGRectGetMaxX(rect)) && std::isfinite(CGRectGetMaxY(rect)) &&
         std::isfinite(rect.size.width) && std::isfinite(rect.size.height) && rect.size.width > 0 &&
         rect.size.height > 0;
}

LynxBorderRadii BorderRadii(const RoundedRectangle &box) {
  LynxBorderRadii radii = {};
  radii.topLeftX.val = box.GetRadiusXTopLeft();
  radii.topLeftY.val = box.GetRadiusYTopLeft();
  radii.topRightX.val = box.GetRadiusXTopRight();
  radii.topRightY.val = box.GetRadiusYTopRight();
  radii.bottomRightX.val = box.GetRadiusXBottomRight();
  radii.bottomRightY.val = box.GetRadiusYBottomRight();
  radii.bottomLeftX.val = box.GetRadiusXBottomLeft();
  radii.bottomLeftY.val = box.GetRadiusYBottomLeft();
  return radii;
}

CGPathRef BoxPath(const RoundedRectangle &box, CGPoint offset = CGPointZero) {
  return [LynxBackgroundUtils createBezierPathWithRoundedRect:BoxRect(box, offset)
                                                  borderRadii:BorderRadii(box)];
}

void ClipBox(CGContextRef context, const RoundedRectangle &box, CGPoint offset) {
  CGPathRef path = BoxPath(box, offset);
  CGContextAddPath(context, path);
  CGContextClip(context);
  CGPathRelease(path);
}

UIColor *Color(uint32_t argb) {
  return [UIColor colorWithRed:((argb >> 16) & 255) / 255.0
                         green:((argb >> 8) & 255) / 255.0
                          blue:(argb & 255) / 255.0
                         alpha:((argb >> 24) & 255) / 255.0];
}

LynxBorderStyle BorderStyle(int32_t style) {
  const LynxBorderStyle styles[] = {
      LynxBorderStyleSolid,  LynxBorderStyleDashed, LynxBorderStyleDotted, LynxBorderStyleDouble,
      LynxBorderStyleGroove, LynxBorderStyleRidge,  LynxBorderStyleInset,  LynxBorderStyleOutset,
      LynxBorderStyleHidden, LynxBorderStyleNone};
  return style >= 0 && style < 10 ? styles[style] : LynxBorderStyleNone;
}

// Keep repeated edges and their phase on device pixels, including negative offsets.
void DrawTiles(CGContextRef context, CGRect tile, CGRect clip, BOOL repeatX, BOOL repeatY,
               CGFloat scale, void (^draw)(CGRect)) {
  if (!ValidRect(tile) || !ValidRect(clip)) return;
  if (repeatX) tile.size.width = MAX(1, round(tile.size.width * scale)) / scale;
  if (repeatY) tile.size.height = MAX(1, round(tile.size.height * scale)) / scale;
  auto first = [scale](CGFloat origin, CGFloat clipStart, CGFloat size) {
    CGFloat pixels = round(size * scale);
    CGFloat clipPixel = floor(clipStart * scale);
    CGFloat phase = fmod(clipPixel - round(origin * scale), pixels);
    if (phase < 0) phase += pixels;
    return (clipPixel - phase) / scale;
  };
  if (repeatX) tile.origin.x = first(tile.origin.x, clip.origin.x, tile.size.width);
  if (repeatY) tile.origin.y = first(tile.origin.y, clip.origin.y, tile.size.height);
  CGRect visible = CGRectIntersection(clip, CGContextGetClipBoundingBox(context));
  if (CGRectIsEmpty(visible) || CGRectIsNull(visible)) return;
  if (repeatX && tile.origin.x < visible.origin.x) {
    tile.origin.x += floor((visible.origin.x - tile.origin.x) / tile.size.width) * tile.size.width;
  }
  if (repeatY && tile.origin.y < visible.origin.y) {
    tile.origin.y +=
        floor((visible.origin.y - tile.origin.y) / tile.size.height) * tile.size.height;
  }
  CGFloat endX = repeatX ? CGRectGetMaxX(visible) : CGRectGetMaxX(tile);
  CGFloat endY = repeatY ? CGRectGetMaxY(visible) : CGRectGetMaxY(tile);
  for (CGFloat y = tile.origin.y; y < endY; y += tile.size.height) {
    for (CGFloat x = tile.origin.x; x < endX; x += tile.size.width) {
      CGRect rect = CGRectMake(x, y, tile.size.width, tile.size.height);
      if (!CGRectIntersectsRect(rect, visible)) continue;
      CGContextSaveGState(context);
      CGContextClipToRect(context, rect);
      draw(rect);
      CGContextRestoreGState(context);
    }
  }
}

}  // namespace

@interface LynxSCGPaintCommand : NSObject
@property(nonatomic) CGRect bounds;
@property(nonatomic, copy) void (^draw)(CGContextRef);
@end
@implementation LynxSCGPaintCommand
@end

@interface LynxSCGPaintGroup ()
@property(nonatomic) BOOL hostDecoration;
- (void)beginFrame;
- (void)append:(LynxSCGPaintCommand *)command;
- (BOOL)finishWithScale:(CGFloat)scale remainingPixels:(size_t *)remainingPixels;
- (void)invalidateDrawing;
- (void)drawInContext:(CGContextRef)context bounds:(CGRect)bounds;
@end

@implementation LynxSCGPaintGroup {
  NSMutableArray<LynxSCGPaintCommand *> *_commands;
  NSMutableArray<LynxSCGLayer *> *_tiles;
}

- (instancetype)init {
  if ((self = [super init])) {
    _commands = [NSMutableArray new];
    _tiles = [NSMutableArray new];
    self.anchorPoint = CGPointZero;
  }
  return self;
}

- (id<CAAction>)actionForKey:(NSString *)event {
  return (id<CAAction>)[NSNull null];
}

- (void)beginFrame {
  [_commands removeAllObjects];
}

- (void)append:(LynxSCGPaintCommand *)command {
  [_commands addObject:command];
}

- (BOOL)finishWithScale:(CGFloat)scale remainingPixels:(size_t *)remainingPixels {
  // Build only occupied tiles. Two distant fragments must not allocate backing
  // for the empty space between them. Keep each tile below the SPI size limit.
  constexpr int kTileSize = 2048;
  std::map<std::pair<int, int>, CGRect> rectangles;
  for (LynxSCGPaintCommand *command in _commands) {
    CGRect pixels = CGRectMake(floor(CGRectGetMinX(command.bounds) * scale),
                               floor(CGRectGetMinY(command.bounds) * scale), 0, 0);
    pixels.size = CGSizeMake(ceil(CGRectGetMaxX(command.bounds) * scale) - pixels.origin.x,
                             ceil(CGRectGetMaxY(command.bounds) * scale) - pixels.origin.y);
    // Unbounded CSS geometry should retain the existing layer renderer instead
    // of overflowing tile indices or allocating an unbounded surface pool.
    if (!ValidRect(pixels) || fabs(pixels.origin.x) > 1e9 || fabs(pixels.origin.y) > 1e9 ||
        pixels.size.width * pixels.size.height > *remainingPixels)
      return NO;
    int left = floor(CGRectGetMinX(pixels) / kTileSize);
    int top = floor(CGRectGetMinY(pixels) / kTileSize);
    int right = ceil(CGRectGetMaxX(pixels) / kTileSize);
    int bottom = ceil(CGRectGetMaxY(pixels) / kTileSize);
    for (int y = top; y < bottom; ++y) {
      for (int x = left; x < right; ++x) {
        CGRect rect = CGRectIntersection(
            pixels, CGRectMake(x * kTileSize, y * kTileSize, kTileSize, kTileSize));
        auto key = std::make_pair(y, x);
        auto found = rectangles.find(key);
        rectangles[key] = found == rectangles.end() ? rect : CGRectUnion(found->second, rect);
        if (rectangles.size() > 128) return NO;
      }
    }
  }
  size_t pixels = 0;
  for (const auto &entry : rectangles) pixels += entry.second.size.width * entry.second.size.height;
  if (pixels > *remainingPixels) return NO;
  *remainingPixels -= pixels;
  NSUInteger index = 0;
  for (const auto &entry : rectangles) {
    CGRect rect = entry.second;
    LynxSCGLayer *tile;
    if (index < _tiles.count) {
      tile = _tiles[index];
    } else {
      tile = [LynxSCGLayer layer];
      tile.anchorPoint = CGPointZero;
      __weak LynxSCGPaintGroup *weakSelf = self;
      tile.drawingBlock = ^(CGContextRef context, CGRect bounds) {
        [weakSelf drawInContext:context bounds:bounds];
      };
      [_tiles addObject:tile];
      [self addSublayer:tile];
    }
    tile.bounds = CGRectMake(rect.origin.x / scale, rect.origin.y / scale, rect.size.width / scale,
                             rect.size.height / scale);
    tile.position = tile.bounds.origin;
    tile.contentsScale = scale;
    [tile setNeedsDisplay];
    ++index;
  }
  while (_tiles.count > index) {
    [_tiles.lastObject removeFromSuperlayer];
    [_tiles removeLastObject];
  }
  return YES;
}

- (void)invalidateDrawing {
  for (LynxSCGLayer *tile in _tiles) [tile setNeedsDisplay];
}

- (void)drawInContext:(CGContextRef)context bounds:(CGRect)bounds {
  UIGraphicsPushContext(context);
  for (LynxSCGPaintCommand *command in _commands) {
    if (!CGRectIntersectsRect(command.bounds, bounds)) continue;
    CGContextSaveGState(context);
    command.draw(context);
    CGContextRestoreGState(context);
  }
  UIGraphicsPopContext();
}
@end

@interface LynxSCGImageObserver : NSObject <LynxImageManagerObserver>
@property(nonatomic, copy) dispatch_block_t invalidate;
@end
@implementation LynxSCGImageObserver
- (void)imageManagerDidChangeImage:(LynxImageManager *)manager {
  if (_invalidate) _invalidate();
}
@end

@interface LynxDisplayListSurfacePainter ()
- (BOOL)paintImage:(const DisplayListItem &)item
             boxes:(const Vector<RoundedRectangle> &)boxes
            offset:(CGPoint)offset
        decoration:(BOOL)decoration;
- (BOOL)paintGradient:(const DisplayListItem &)item
               reader:(const DisplayListReader &)reader
                boxes:(const Vector<RoundedRectangle> &)boxes
               offset:(CGPoint)offset
           decoration:(BOOL)decoration;
- (BOOL)paintShadow:(const DisplayListItem &)item
              boxes:(const Vector<RoundedRectangle> &)boxes
             offset:(CGPoint)offset
         decoration:(BOOL)decoration;
@end

@implementation LynxDisplayListSurfacePainter {
  LynxRendererContext *_context;
  void (^_insertLayer)(CALayer *, BOOL);
  dispatch_block_t _invalidate;
  NSMutableArray<LynxSCGPaintGroup *> *_groups;
  NSArray<LynxSCGPaintGroup *> *_previousGroups;
  LynxSCGPaintGroup *_group;
  NSMapTable<LynxImageManager *, LynxSCGImageObserver *> *_images;
  NSMapTable<LynxImageManager *, LynxSCGImageObserver *> *_previousImages;
  CGRect _bounds;
  CGFloat _scale;
}

- (instancetype)initWithContext:(LynxRendererContext *)context
                    insertLayer:(void (^)(CALayer *, BOOL))insertLayer
                     invalidate:(dispatch_block_t)invalidate {
  if ((self = [super init])) {
    _context = context;
    _insertLayer = [insertLayer copy];
    _invalidate = [invalidate copy];
  }
  return self;
}

- (void)beginFrameWithBounds:(CGRect)bounds scale:(CGFloat)scale {
  _bounds = (CGRect){CGPointZero, bounds.size};
  _scale = scale > 0 && std::isfinite(scale) ? scale : 1;
  _previousGroups = _groups;
  _groups = [NSMutableArray new];
  _previousImages = _images;
  _images = [NSMapTable strongToStrongObjectsMapTable];
  _group = nil;
}

- (void)appendBounds:(CGRect)bounds decoration:(BOOL)decoration draw:(void (^)(CGContextRef))draw {
  if (!ValidRect(bounds)) return;
  if (_group == nil || _group.hostDecoration != decoration) {
    NSUInteger index = _groups.count;
    _group = index < _previousGroups.count ? _previousGroups[index] : [LynxSCGPaintGroup layer];
    [_group beginFrame];
    _group.hostDecoration = decoration;
    _group.transform = CATransform3DIdentity;
    _group.anchorPoint = CGPointZero;
    _group.bounds = _bounds;
    _group.position = CGPointZero;
    _group.opacity = 1;
    _group.hidden = NO;
    _group.filters = nil;
    [_groups addObject:_group];
    _insertLayer(_group, decoration);
  }
  LynxSCGPaintCommand *command = [LynxSCGPaintCommand new];
  command.bounds = bounds;
  command.draw = draw;
  [_group append:command];
}

- (LynxImageManager *)imageManagerForID:(int32_t)imageID {
  LynxImageManager *manager = [_context imageManagerForID:imageID];
  if (manager == nil || [_images objectForKey:manager] != nil) return manager;
  LynxSCGImageObserver *observer = [_previousImages objectForKey:manager];
  if (observer == nil) {
    observer = [LynxSCGImageObserver new];
    observer.invalidate = _invalidate;
  }
  [_previousImages removeObjectForKey:manager];
  [_images setObject:observer forKey:manager];
  manager.drawingObserver = observer;
  return manager;
}

- (void)endGroup {
  _group = nil;
}

- (void)detachImageObservers:(NSMapTable<LynxImageManager *, LynxSCGImageObserver *> *)images {
  for (LynxImageManager *manager in images) {
    if (manager.drawingObserver == [images objectForKey:manager]) manager.drawingObserver = nil;
  }
}

- (BOOL)endFrame {
  [self detachImageObservers:_previousImages];
  _previousImages = nil;
  size_t remainingPixels = 16 * 1024 * 1024;
  for (LynxSCGPaintGroup *group in _groups) {
    if (![group finishWithScale:_scale remainingPixels:&remainingPixels]) return NO;
  }
  _previousGroups = nil;
  _group = nil;
  return YES;
}

- (void)invalidateImages {
  for (LynxSCGPaintGroup *group in _groups) [group invalidateDrawing];
}

- (void)reset {
  [self detachImageObservers:_images];
  [self detachImageObservers:_previousImages];
  _groups = nil;
  _previousGroups = nil;
  _images = nil;
  _previousImages = nil;
  _group = nil;
}

- (void)dealloc {
  [self detachImageObservers:_images];
  [self detachImageObservers:_previousImages];
}

- (BOOL)paintItem:(const DisplayListItem &)item
            reader:(const DisplayListReader &)reader
             boxes:(const Vector<RoundedRectangle> &)boxes
            offset:(CGPoint)offset
    hostDecoration:(BOOL)decoration {
  auto validBox = [&boxes](int32_t index) { return index >= 0 && (size_t)index < boxes.size(); };
  switch (item.type) {
    case DisplayListOpType::kFill: {
      if (!validBox(item.payload.fill.clip_index)) return YES;
      const auto box = boxes[item.payload.fill.clip_index];
      UIColor *color = Color(item.payload.fill.color);
      [self appendBounds:BoxRect(box, offset)
              decoration:decoration
                    draw:^(CGContextRef context) {
                      CGPathRef path = BoxPath(box, offset);
                      CGContextAddPath(context, path);
                      CGContextSetFillColorWithColor(context, color.CGColor);
                      CGContextFillPath(context);
                      CGPathRelease(path);
                    }];
      return YES;
    }
    case DisplayListOpType::kBorder: {
      const auto border = item.payload.border;
      if (!validBox(border.out_index) || !validBox(border.inner_index)) return YES;
      const auto outer = boxes[border.out_index];
      const auto inner = boxes[border.inner_index];
      const CGRect rect = BoxRect(outer, offset);
      NSArray<UIColor *> *colors = @[
        Color(border.colors[0]), Color(border.colors[1]), Color(border.colors[2]),
        Color(border.colors[3])
      ];
      [self
          appendBounds:rect
            decoration:decoration
                  draw:^(CGContextRef context) {
                    CGContextTranslateCTM(context, rect.origin.x, rect.origin.y);
                    CGRect local = (CGRect){CGPointZero, rect.size};
                    UIEdgeInsets insets = UIEdgeInsetsMake(
                        MAX(0, inner.GetY() - outer.GetY()), MAX(0, inner.GetX() - outer.GetX()),
                        MAX(0, outer.GetY() + outer.GetHeight() - inner.GetY() - inner.GetHeight()),
                        MAX(0, outer.GetX() + outer.GetWidth() - inner.GetX() - inner.GetWidth()));
                    LynxBorderColors borderColors = {colors[0].CGColor, colors[3].CGColor,
                                                     colors[2].CGColor, colors[1].CGColor};
                    LynxBorderStyles styles = {
                        BorderStyle(border.styles[0]), BorderStyle(border.styles[3]),
                        BorderStyle(border.styles[2]), BorderStyle(border.styles[1])};
                    CGPathRef outerPath = BoxPath(outer, CGPointMake(-outer.GetX(), -outer.GetY()));
                    CGPathRef innerPath = BoxPath(inner, CGPointMake(-outer.GetX(), -outer.GetY()));
                    auto radii = BorderRadii(outer);
                    auto corners = LynxGetCornerInsets(local, radii, UIEdgeInsetsZero);
                    LynxDrawBorders(context, styles, rect.size, radii, insets, borderColors, NO,
                                    outer.HasRadius(), outerPath, innerPath, corners);
                    CGPathRelease(innerPath);
                    CGPathRelease(outerPath);
                  }];
      return YES;
    }
    case DisplayListOpType::kText: {
      const auto text = item.payload.text;
      if (!validBox(text.box_index)) return YES;
      CGRect rect = BoxRect(boxes[text.box_index], offset);
      if (_context.uiContext.lynxContext.isTextServiceModeOn) {
        __weak LynxRendererContext *weakContext = _context;
        [self appendBounds:rect
                decoration:NO
                      draw:^(CGContextRef context) {
                        void *page = [weakContext getTextBundle:text.text_id];
                        if (page == nullptr) return;
                        CGContextClipToRect(context, rect);
                        CGContextTranslateCTM(context, rect.origin.x, rect.origin.y);
                        [LynxService(LynxServiceTextProtocol) drawPage:page OnContext:context];
                      }];
      } else {
        LynxTextRenderer *renderer =
            (LynxTextRenderer *)[_context.textRenderManager takeTextRender:text.text_id];
        if (renderer == nil) return YES;
        [self appendBounds:rect
                decoration:NO
                      draw:^(CGContextRef context) {
                        CGContextClipToRect(context, rect);
                        CGContextTranslateCTM(context, rect.origin.x, rect.origin.y);
                        CGContextSetShouldAntialias(context, YES);
                        [renderer drawRect:(CGRect){CGPointZero, rect.size}
                                   padding:UIEdgeInsetsZero
                                    border:UIEdgeInsetsZero];
                      }];
      }
      return YES;
    }
    case DisplayListOpType::kImage:
    case DisplayListOpType::kBackgroundImage:
      return [self paintImage:item boxes:boxes offset:offset decoration:decoration];
    case DisplayListOpType::kLinearGradient:
    case DisplayListOpType::kRadialGradient:
      return [self paintGradient:item
                          reader:reader
                           boxes:boxes
                          offset:offset
                      decoration:decoration];
    case DisplayListOpType::kBoxShadow:
      return [self paintShadow:item boxes:boxes offset:offset decoration:decoration];
    default:
      return NO;
  }
}

- (BOOL)paintImage:(const DisplayListItem &)item
             boxes:(const Vector<RoundedRectangle> &)boxes
            offset:(CGPoint)offset
        decoration:(BOOL)decoration {
  BOOL background = item.type == DisplayListOpType::kBackgroundImage;
  int32_t imageID =
      background ? item.payload.background_image.image_id : item.payload.image.image_id;
  int32_t clipIndex =
      background ? item.payload.background_image.clip_index : item.payload.image.box_index;
  if (clipIndex < 0 || (size_t)clipIndex >= boxes.size()) return YES;
  LynxImageManager *manager = [self imageManagerForID:imageID];
  if (manager == nil) return YES;
  // Native image services own animated decoding, playback controls and events.
  // Keep those images at their original position between surface paint groups.
  if ([manager imageRequiresNativeView]) {
    [self endGroup];
    return NO;
  }
  const auto clip = boxes[clipIndex];
  const CGRect clipRect = BoxRect(clip, offset);
  if (!background) {
    [self appendBounds:clipRect
            decoration:NO
                  draw:^(CGContextRef context) {
                    ClipBox(context, clip, offset);
                    [manager drawImageInContext:context rect:clipRect];
                  }];
    return YES;
  }
  const auto image = item.payload.background_image;
  if (image.tiling_index < 0 || (size_t)image.tiling_index >= boxes.size()) return YES;
  const CGRect fallback = BoxRect(boxes[image.tiling_index], offset);
  const CGFloat scale = _scale;
  [self appendBounds:clipRect
          decoration:decoration
                draw:^(CGContextRef context) {
                  UIImage *value = [manager imageForDrawing];
                  if (value == nil) return;
                  CGSize imageSize = value.size;
                  if (image.auto_size != 0 && (imageSize.width <= 0 || imageSize.height <= 0))
                    return;
                  CGRect tile = fallback;
                  BOOL autoWidth = (image.auto_size & 1) != 0;
                  BOOL autoHeight = (image.auto_size & 2) != 0;
                  if (autoWidth && autoHeight) {
                    tile.size = imageSize;
                  } else if (autoWidth) {
                    tile.size.width = tile.size.height * imageSize.width / imageSize.height;
                  } else if (autoHeight) {
                    tile.size.height = tile.size.width * imageSize.height / imageSize.width;
                  }
                  tile.origin.x += (fallback.size.width - tile.size.width) * image.position_x;
                  tile.origin.y += (fallback.size.height - tile.size.height) * image.position_y;
                  ClipBox(context, clip, offset);
                  DrawTiles(context, tile, clipRect, image.repeat_x == LynxBackgroundRepeatRepeat,
                            image.repeat_y == LynxBackgroundRepeatRepeat, scale, ^(CGRect rect) {
                              [value drawInRect:rect];
                            });
                }];
  return YES;
}

- (BOOL)paintGradient:(const DisplayListItem &)item
               reader:(const DisplayListReader &)reader
                boxes:(const Vector<RoundedRectangle> &)boxes
               offset:(CGPoint)offset
           decoration:(BOOL)decoration {
  BOOL linear = item.type == DisplayListOpType::kLinearGradient;
  int32_t colorCount =
      linear ? item.payload.linear_gradient.color_count : item.payload.radial_gradient.color_count;
  int32_t stopCount =
      linear ? item.payload.linear_gradient.stop_count : item.payload.radial_gradient.stop_count;
  int32_t tileIndex = linear ? item.payload.linear_gradient.tiling_index
                             : item.payload.radial_gradient.tiling_index;
  int32_t clipIndex =
      linear ? item.payload.linear_gradient.clip_index : item.payload.radial_gradient.clip_index;
  if (colorCount <= 0 || stopCount < 0 || tileIndex < 0 || clipIndex < 0 ||
      (size_t)tileIndex >= boxes.size() || (size_t)clipIndex >= boxes.size())
    return YES;
  NSMutableArray<NSNumber *> *colors = [NSMutableArray arrayWithCapacity:colorCount];
  NSMutableArray<NSNumber *> *stops = [NSMutableArray arrayWithCapacity:stopCount];
  for (int32_t i = 0; i < colorCount; ++i) [colors addObject:@(reader.Colors(item)[i])];
  for (int32_t i = 0; i < stopCount; ++i) [stops addObject:@(reader.Stops(item)[i] * 100.0f)];
  LynxGradient *gradient;
  BOOL repeatX;
  BOOL repeatY;
  if (linear) {
    const auto data = item.payload.linear_gradient;
    gradient = [[LynxLinearGradient alloc] initWithArray:@[ @(data.angle), colors, stops ]];
    repeatX = data.repeat_x == LynxBackgroundRepeatRepeat;
    repeatY = data.repeat_y == LynxBackgroundRepeatRepeat;
  } else {
    const auto data = item.payload.radial_gradient;
    NSArray *shape = @[
      @(LynxRadialGradientShapeEllipse), @(LynxRadialGradientSizeLength), @(LynxRadialCenterTypePX),
      @(data.center_x), @(LynxRadialCenterTypePX), @(data.center_y), @0, @0, @0, @0,
      @(data.radius_x), @(LynxPlatformLengthUnitNumber), @(data.radius_y),
      @(LynxPlatformLengthUnitNumber)
    ];
    gradient = [[LynxRadialGradient alloc] initWithArray:@[ shape, colors, stops ]];
    repeatX = data.repeat_x == LynxBackgroundRepeatRepeat;
    repeatY = data.repeat_y == LynxBackgroundRepeatRepeat;
  }
  const auto clip = boxes[clipIndex];
  const CGRect clipRect = BoxRect(clip, offset);
  const CGRect tile = BoxRect(boxes[tileIndex], offset);
  const CGFloat scale = _scale;
  [self appendBounds:clipRect
          decoration:decoration
                draw:^(CGContextRef context) {
                  ClipBox(context, clip, offset);
                  DrawTiles(context, tile, clipRect, repeatX, repeatY, scale, ^(CGRect rect) {
                    CGContextTranslateCTM(context, rect.origin.x, rect.origin.y);
                    [gradient draw:context withRect:(CGRect){CGPointZero, rect.size}];
                  });
                }];
  return YES;
}

- (BOOL)paintShadow:(const DisplayListItem &)item
              boxes:(const Vector<RoundedRectangle> &)boxes
             offset:(CGPoint)offset
         decoration:(BOOL)decoration {
  const auto shadow = item.payload.box_shadow;
  if (shadow.shadow_box_index < 0 || shadow.clip_box_index < 0 ||
      (size_t)shadow.shadow_box_index >= boxes.size() ||
      (size_t)shadow.clip_box_index >= boxes.size())
    return YES;
  const auto shape = boxes[shadow.shadow_box_index];
  const auto clip = boxes[shadow.clip_box_index];
  const CGRect shapeRect = BoxRect(shape, offset);
  const CGRect clipRect = BoxRect(clip, offset);
  BOOL inset = shadow.clip_mode == 1;
  BOOL empty = !ValidRect(shapeRect);
  if (!ValidRect(clipRect) || (!inset && empty)) return YES;
  CGFloat extent = MAX(0, shadow.blur_radius) * 3 + 1;
  CGRect outer = CGRectInset(CGRectUnion(clipRect, shapeRect), -extent, -extent);
  UIColor *color = Color(shadow.color);
  [self appendBounds:inset ? clipRect : outer
          decoration:decoration
                draw:^(CGContextRef context) {
                  if (inset && empty) {
                    ClipBox(context, clip, offset);
                    CGContextSetFillColorWithColor(context, color.CGColor);
                    CGContextFillRect(context, clipRect);
                    return;
                  }
                  CGPathRef clipPath = BoxPath(clip, offset);
                  CGContextAddPath(context, clipPath);
                  if (!inset) CGContextAddRect(context, outer);
                  CGContextEOClip(context);
                  CGPathRelease(clipPath);
                  // Move an opaque caster outside the clip and offset its shadow back. This
                  // draws only the shadow: filling the original shape would apply translucent
                  // shadow color twice wherever a spread/inset caster overlaps the clip.
                  CGFloat displacement = outer.size.width + extent + 1;
                  CGPathRef shapePath =
                      BoxPath(shape, CGPointMake(offset.x - displacement, offset.y));
                  CGContextAddPath(context, shapePath);
                  if (inset) CGContextAddRect(context, CGRectOffset(outer, -displacement, 0));
                  CGContextSetRGBFillColor(context, 0, 0, 0, 1);
                  CGContextSetShadowWithColor(context, CGSizeMake(displacement, 0),
                                              MAX(0, shadow.blur_radius) * 0.5, color.CGColor);
                  CGContextEOFillPath(context);
                  CGPathRelease(shapePath);
                }];
  return YES;
}

@end
