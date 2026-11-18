// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#import <Lynx/LynxViewInsets+Internal.h>
#include <math.h>

LynxInsetType const LynxInsetStatusBars = @"statusBars";
LynxInsetType const LynxInsetNavigationBars = @"navigationBars";
LynxInsetType const LynxInsetCaptionBar = @"captionBar";
LynxInsetType const LynxInsetSystemBars = @"systemBars";
LynxInsetType const LynxInsetSystemOverlays = @"systemOverlays";
LynxInsetType const LynxInsetIme = @"ime";
LynxInsetType const LynxInsetDisplayCutout = @"displayCutout";
LynxInsetType const LynxInsetWaterfall = @"waterfall";
LynxInsetType const LynxInsetRoundedCorners = @"roundedCorners";
LynxInsetType const LynxInsetSystemGestures = @"systemGestures";
LynxInsetType const LynxInsetMandatorySystemGestures = @"mandatorySystemGestures";
LynxInsetType const LynxInsetTappableElement = @"tappableElement";

static void RequireInset(BOOL condition) {
  if (!condition) {
    [NSException raise:NSInvalidArgumentException format:@"Invalid View inset"];
  }
}

@interface LynxInsetGeometry ()
- (instancetype)initGeometry;
- (NSDictionary*)payload;
@end
@implementation LynxInsetGeometry
- (instancetype)initGeometry {
  return [super init];
}
- (NSDictionary*)payload {
  [self doesNotRecognizeSelector:_cmd];
  return @{};
}
@end

@implementation LynxInsetEdges
- (instancetype)initWithLeft:(CGFloat)left
                         top:(CGFloat)top
                       right:(CGFloat)right
                      bottom:(CGFloat)bottom {
  RequireInset(isfinite(left) && isfinite(top) && isfinite(right) && isfinite(bottom) &&
               left >= 0 && top >= 0 && right >= 0 && bottom >= 0);
  if (self = [super initGeometry]) {
    _left = left;
    _top = top;
    _right = right;
    _bottom = bottom;
  }
  return self;
}
- (NSDictionary*)payload {
  return @{
    @"type" : @"edges",
    @"left" : @(_left),
    @"top" : @(_top),
    @"right" : @(_right),
    @"bottom" : @(_bottom)
  };
}
@end

@interface LynxInsetRect ()
- (NSDictionary*)payload;
@end
@implementation LynxInsetRect
- (instancetype)initWithX:(CGFloat)x y:(CGFloat)y width:(CGFloat)width height:(CGFloat)height {
  RequireInset(isfinite(x) && isfinite(y) && isfinite(width) && isfinite(height) && width >= 0 &&
               height >= 0);
  if (self = [super init]) {
    _x = x;
    _y = y;
    _width = width;
    _height = height;
  }
  return self;
}
- (NSDictionary*)payload {
  return @{@"x" : @(_x), @"y" : @(_y), @"width" : @(_width), @"height" : @(_height)};
}
@end

@implementation LynxInsetRects
- (instancetype)initWithRects:(NSArray<LynxInsetRect*>*)rects {
  RequireInset(rects != nil);
  for (id rect in rects) RequireInset([rect isMemberOfClass:LynxInsetRect.class]);
  if (self = [super initGeometry]) _rects = [rects copy];
  return self;
}
- (NSDictionary*)payload {
  NSMutableArray* values = [NSMutableArray new];
  for (LynxInsetRect* rect in _rects) [values addObject:[rect payload]];
  return @{@"type" : @"rects", @"rects" : [values copy]};
}
@end

@interface LynxInsetCorner ()
- (NSDictionary*)payload;
@end
@implementation LynxInsetCorner
- (instancetype)initWithCenterX:(CGFloat)x centerY:(CGFloat)y radius:(CGFloat)radius {
  RequireInset(isfinite(x) && isfinite(y) && isfinite(radius) && radius >= 0);
  if (self = [super init]) {
    _centerX = x;
    _centerY = y;
    _radius = radius;
  }
  return self;
}
- (NSDictionary*)payload {
  return @{@"centerX" : @(_centerX), @"centerY" : @(_centerY), @"radius" : @(_radius)};
}
@end

@implementation LynxInsetCorners
- (instancetype)initWithTopLeft:(LynxInsetCorner*)topLeft
                       topRight:(LynxInsetCorner*)topRight
                    bottomRight:(LynxInsetCorner*)bottomRight
                     bottomLeft:(LynxInsetCorner*)bottomLeft {
  RequireInset(!topLeft || [topLeft isMemberOfClass:LynxInsetCorner.class]);
  RequireInset(!topRight || [topRight isMemberOfClass:LynxInsetCorner.class]);
  RequireInset(!bottomRight || [bottomRight isMemberOfClass:LynxInsetCorner.class]);
  RequireInset(!bottomLeft || [bottomLeft isMemberOfClass:LynxInsetCorner.class]);
  if (self = [super initGeometry]) {
    _topLeft = topLeft;
    _topRight = topRight;
    _bottomRight = bottomRight;
    _bottomLeft = bottomLeft;
  }
  return self;
}
- (NSDictionary*)payload {
  NSMutableDictionary* value = [@{@"type" : @"corners"} mutableCopy];
  if (_topLeft) value[@"topLeft"] = [_topLeft payload];
  if (_topRight) value[@"topRight"] = [_topRight payload];
  if (_bottomRight) value[@"bottomRight"] = [_bottomRight payload];
  if (_bottomLeft) value[@"bottomLeft"] = [_bottomLeft payload];
  return [value copy];
}
@end

@interface LynxInsetState ()
- (NSDictionary*)payload;
@end
@implementation LynxInsetState
- (instancetype)initWithGeometry:(LynxInsetGeometry*)geometry visible:(NSNumber*)visible {
  RequireInset([geometry isMemberOfClass:LynxInsetEdges.class] ||
               [geometry isMemberOfClass:LynxInsetRects.class] ||
               [geometry isMemberOfClass:LynxInsetCorners.class]);
  RequireInset(!visible || CFGetTypeID((__bridge CFTypeRef)visible) == CFBooleanGetTypeID());
  if (self = [super init]) {
    _geometry = geometry;
    _visible = [visible copy];
  }
  return self;
}
- (NSDictionary*)payload {
  NSMutableDictionary* value = [@{@"geometry" : [_geometry payload]} mutableCopy];
  if (_visible) value[@"visible"] = _visible;
  return [value copy];
}
@end

@implementation LynxViewInsets
+ (instancetype)empty {
  return [[self alloc] initWithCategories:@{}];
}
- (instancetype)initWithCategories:(NSDictionary<LynxInsetType, LynxInsetState*>*)categories {
  RequireInset(categories != nil);
  NSArray* types = @[
    LynxInsetStatusBars, LynxInsetNavigationBars, LynxInsetCaptionBar, LynxInsetSystemBars,
    LynxInsetSystemOverlays, LynxInsetIme, LynxInsetDisplayCutout, LynxInsetWaterfall,
    LynxInsetRoundedCorners, LynxInsetSystemGestures, LynxInsetMandatorySystemGestures,
    LynxInsetTappableElement
  ];
  NSMutableDictionary* copy = [NSMutableDictionary new];
  [categories
      enumerateKeysAndObjectsUsingBlock:^(LynxInsetType key, LynxInsetState* state, BOOL* stop) {
        RequireInset([types containsObject:key] && [state isMemberOfClass:LynxInsetState.class]);
        if ([key isEqual:LynxInsetRoundedCorners]) {
          RequireInset([state.geometry isMemberOfClass:LynxInsetCorners.class]);
        } else if ([key isEqual:LynxInsetDisplayCutout]) {
          RequireInset([state.geometry isMemberOfClass:LynxInsetEdges.class] ||
                       [state.geometry isMemberOfClass:LynxInsetRects.class]);
        } else {
          RequireInset([state.geometry isMemberOfClass:LynxInsetEdges.class]);
        }
        copy[[key copy]] = state;
      }];
  if (self = [super init]) _categories = [copy copy];
  return self;
}
- (LynxViewInsets*)lynx_mergingInsets:(LynxViewInsets*)update {
  RequireInset(update != nil);
  NSMutableDictionary* next = [_categories mutableCopy];
  [next addEntriesFromDictionary:update.categories];
  return [[LynxViewInsets alloc] initWithCategories:next];
}
- (NSDictionary*)lynx_dictionaryRepresentation {
  NSMutableDictionary* value = [NSMutableDictionary new];
  [_categories
      enumerateKeysAndObjectsUsingBlock:^(LynxInsetType key, LynxInsetState* state, BOOL* stop) {
        value[key] = [state payload];
      }];
  return [value copy];
}
@end

@implementation LynxViewInsetsBuilder {
  NSMutableDictionary<LynxInsetType, LynxInsetState*>* _categories;
}
- (instancetype)init {
  if (self = [super init]) _categories = [NSMutableDictionary new];
  return self;
}
- (instancetype)setInset:(LynxInsetType)type state:(LynxInsetState*)state {
  RequireInset(type != nil);
  if (state) {
    // Validate before changing builder state.
    LynxViewInsets* validated = [[LynxViewInsets alloc] initWithCategories:@{type : state}];
    _categories[type] = validated.categories[type];
  } else {
    [_categories removeObjectForKey:type];
  }
  return self;
}
- (LynxViewInsets*)build {
  return [[LynxViewInsets alloc] initWithCategories:_categories];
}
@end
