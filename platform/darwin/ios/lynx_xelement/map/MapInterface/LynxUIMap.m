// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <XElement/LynxUIMap.h>

#import <Lynx/LynxComponentRegistry.h>
#import <Lynx/LynxContext.h>
#import <Lynx/LynxEvent.h>
#import <Lynx/LynxPropsProcessor.h>
#import <Lynx/LynxUIMethodProcessor.h>
#import <Lynx/UIView+Lynx.h>

#import <XElement/LynxUIMapMarker.h>
#import "LynxXElementMapService.h"

typedef void (^LynxMapOperationBlock)(void);

static NSDictionary<NSString *, NSNumber *> *LynxMapLatLngFromValue(id value) {
  if (![value isKindOfClass:[NSDictionary class]]) {
    return nil;
  }
  id latitude = value[@"latitude"];
  id longitude = value[@"longitude"];
  if (![latitude isKindOfClass:[NSNumber class]] || ![longitude isKindOfClass:[NSNumber class]]) {
    return nil;
  }
  return @{@"latitude" : latitude, @"longitude" : longitude};
}

static NSDictionary<NSString *, NSDictionary<NSString *, NSNumber *> *> *LynxMapZeroBound(void) {
  NSDictionary<NSString *, NSNumber *> *zero = @{@"latitude" : @0, @"longitude" : @0};
  return @{@"northeast" : zero, @"southwest" : zero};
}

@interface LynxUIMap ()

@property(nonatomic, strong) NSMutableDictionary<NSString *, id> *mapInitParams;

@end

@implementation LynxUIMap

LYNX_LAZY_REGISTER_UI("map")

- (instancetype)init {
  self = [super init];
  if (self) {
    _mapInitParams = [@{@"biz-name" : @"undefined", @"map-type" : @(-1)} mutableCopy];
  }
  return self;
}

- (UIView *)createView {
  self.firstRender = YES;
  return [[LynxXElementMapService provider] createMapView];
}

- (int64_t)memoryUsageBytes {
  return 100 * 1024 * 1024;
}

- (void)onNodeReload {
  [super onNodeReload];
  [self sendEvent:@"mapload" detail:nil];
}

- (id<ILynxMapView>)mapViewImpl {
  return (id<ILynxMapView>)self.view;
}

- (void)propsDidUpdate {
  [super propsDidUpdate];
  if (self.firstRender) {
    self.firstRender = NO;
    [[self mapViewImpl] initializeMapViewWithParams:[self.mapInitParams copy] uiMap:self];
    [self enableTapGestureSimultaneouslyRecursively:self.view];
  }
}

- (void)onNodeRemoved {
  [[self mapViewImpl] destroy];
  [super onNodeRemoved];
}

- (void)insertChild:(id)child atIndex:(NSInteger)index {
  if ([child isKindOfClass:[LynxUIMapMarker class]]) {
    // Marker views are managed by the map SDK, not the normal view hierarchy.
    // Only update the children array, skip view/layer manipulation.
    [self didInsertChild:child atIndex:index];
    [((LynxUIMapMarker *)child) bindMapView:[self mapViewImpl]];
  } else {
    [super insertChild:child atIndex:index];
  }
}

- (void)willRemoveComponent:(LynxUI *)child {
  if ([child isKindOfClass:[LynxUIMapMarker class]]) {
    // Don't remove marker view from SDK's annotation view container.
    // SDK cleanup is handled by onNodeRemoved -> destroy -> removeMarkerWithIdentifier.
    return;
  }
  [super willRemoveComponent:child];
}

- (void)removeChild:(id)child atIndex:(NSInteger)index {
  [super removeChild:child atIndex:index];
}

- (void)layoutDidFinished {
  [super layoutDidFinished];
  [[self mapViewImpl] layoutDidFinished];
}

- (void)sendEvent:(NSString *)name detail:(NSDictionary<NSString *, id> *)detail {
  LynxDetailEvent *event = [[LynxDetailEvent alloc] initWithName:name
                                                      targetSign:[self sign]
                                                          detail:detail ?: @{}];
  [self.context.eventEmitter sendCustomEvent:event];
}

LYNX_PROP_SETTER("biz-name", bizName, NSString *) {
  self.mapInitParams[@"biz-name"] = value ?: @"undefined";
}

LYNX_PROP_SETTER("map-type", mapType, NSInteger) { self.mapInitParams[@"map-type"] = @(value); }

LYNX_PROP_SETTER("extra-params", extraParams, NSDictionary *) {
  self.mapInitParams[@"extra-params"] = [value isKindOfClass:[NSDictionary class]] ? value : nil;
}

LYNX_PROP_SETTER("map-style", mapStyle, NSString *) {
  NSString *style = [value copy];
  [self.nodeReadyBlockArray addObject:^(LynxUI *ui) {
    [[(LynxUIMap *)ui mapViewImpl] setMapStyle:style];
  }];
}

LYNX_PROP_SETTER("center", center, NSDictionary *) {
  NSDictionary<NSString *, NSNumber *> *center = LynxMapLatLngFromValue(value);
  if (center) {
    [self.nodeReadyBlockArray addObject:^(LynxUI *ui) {
      [[(LynxUIMap *)ui mapViewImpl] setCenterWithLatitude:center[@"latitude"].doubleValue
                                                 longitude:center[@"longitude"].doubleValue
                                                  animated:YES
                                                  duration:250];
    }];
  }
}

LYNX_PROP_SETTER("scale", scale, CGFloat) {
  [self.nodeReadyBlockArray addObject:^(LynxUI *ui) {
    [[(LynxUIMap *)ui mapViewImpl] setScale:value animated:YES duration:250];
  }];
}

LYNX_PROP_SETTER("scale-range", scaleRange, NSArray<NSNumber *> *) {
  if (![value isKindOfClass:[NSArray class]] || value.count < 2) {
    return;
  }
  NSNumber *minScale = value[0];
  NSNumber *maxScale = value[1];
  if (![minScale isKindOfClass:[NSNumber class]] || ![maxScale isKindOfClass:[NSNumber class]]) {
    return;
  }
  if (minScale.doubleValue > maxScale.doubleValue) {
    return;
  }

  [self.nodeReadyBlockArray addObject:^(LynxUI *ui) {
    [[(LynxUIMap *)ui mapViewImpl] setScaleRangeWithMinScale:minScale.doubleValue
                                                    maxScale:maxScale.doubleValue];
  }];
}

LYNX_PROP_SETTER("rotate", rotate, CGFloat) {
  [self.nodeReadyBlockArray addObject:^(LynxUI *ui) {
    [[(LynxUIMap *)ui mapViewImpl] setRotationDegree:value];
  }];
}

LYNX_PROP_SETTER("enable-scale-controls", enableScaleControls, BOOL) {
  [self.nodeReadyBlockArray addObject:^(LynxUI *ui) {
    [[(LynxUIMap *)ui mapViewImpl] setEnableScaleControls:value];
  }];
}

LYNX_PROP_SETTER("logo-position", logoPosition, NSString *) {
  [self.nodeReadyBlockArray addObject:^(LynxUI *ui) {
    [[(LynxUIMap *)ui mapViewImpl] setLogoPosition:value];
  }];
}

LYNX_PROP_SETTER("enable-scale", enableScale, BOOL) {
  [self.nodeReadyBlockArray addObject:^(LynxUI *ui) {
    [[(LynxUIMap *)ui mapViewImpl] setEnableScale:value];
  }];
}

LYNX_PROP_SETTER("enable-drag", enableDrag, BOOL) {
  [self.nodeReadyBlockArray addObject:^(LynxUI *ui) {
    [[(LynxUIMap *)ui mapViewImpl] setEnableDrag:value];
  }];
}

LYNX_PROP_SETTER("enable-rotate", enableRotate, BOOL) {
  [self.nodeReadyBlockArray addObject:^(LynxUI *ui) {
    [[(LynxUIMap *)ui mapViewImpl] setEnableRotate:value];
  }];
}

LYNX_PROP_SETTER("enable-tilt", enableTilt, BOOL) {
  [self.nodeReadyBlockArray addObject:^(LynxUI *ui) {
    [[(LynxUIMap *)ui mapViewImpl] setEnableTilt:value];
  }];
}

LYNX_PROP_SETTER("polyline", polyline, NSArray<NSDictionary *> *) {
  [self.nodeReadyBlockArray addObject:^(LynxUI *ui) {
    [[(LynxUIMap *)ui mapViewImpl] setPolyline:value];
  }];
}

LYNX_PROP_SETTER("polygons", polygons, NSArray<NSDictionary *> *) {
  [self.nodeReadyBlockArray addObject:^(LynxUI *ui) {
    [[(LynxUIMap *)ui mapViewImpl] setPolygons:value];
  }];
}

LYNX_PROP_SETTER("circles", circles, NSArray<NSDictionary *> *) {
  [self.nodeReadyBlockArray addObject:^(LynxUI *ui) {
    [[(LynxUIMap *)ui mapViewImpl] setCircles:value];
  }];
}

LYNX_PROP_SETTER("arcs", arcs, NSArray<NSDictionary *> *) {
  [self.nodeReadyBlockArray addObject:^(LynxUI *ui) {
    [[(LynxUIMap *)ui mapViewImpl] setArcs:value];
  }];
}

LYNX_PROP_SETTER("route-label", routeLabel, NSArray<NSDictionary *> *) {
  [self.nodeReadyBlockArray addObject:^(LynxUI *ui) {
    [[(LynxUIMap *)ui mapViewImpl] setRouteLabel:value];
  }];
}

LYNX_UI_METHOD(setCenter) {
  NSDictionary<NSString *, NSNumber *> *center = LynxMapLatLngFromValue(params[@"center"]);
  if (!center) {
    if (callback) {
      callback(kUIMethodParamInvalid, nil);
    }
    return;
  }

  BOOL animated =
      [params[@"animated"] isKindOfClass:[NSNumber class]] ? [params[@"animated"] boolValue] : YES;
  NSTimeInterval duration = [params[@"duration"] isKindOfClass:[NSNumber class]]
                                ? [params[@"duration"] doubleValue]
                                : 250;
  [[self mapViewImpl] setCenterWithLatitude:center[@"latitude"].doubleValue
                                  longitude:center[@"longitude"].doubleValue
                                   animated:animated
                                   duration:duration];

  id scale = params[@"scale"];
  if ([scale isKindOfClass:[NSNumber class]]) {
    [[self mapViewImpl] setScale:[scale doubleValue] animated:animated duration:duration];
  }

  if (callback) {
    callback(kUIMethodSuccess, nil);
  }
}

LYNX_UI_METHOD(setRotate) {
  id rotate = params[@"rotate"];
  if (![rotate isKindOfClass:[NSNumber class]]) {
    if (callback) {
      callback(kUIMethodParamInvalid, nil);
    }
    return;
  }
  BOOL animated =
      [params[@"animated"] isKindOfClass:[NSNumber class]] ? [params[@"animated"] boolValue] : YES;
  NSTimeInterval duration = [params[@"duration"] isKindOfClass:[NSNumber class]]
                                ? [params[@"duration"] doubleValue]
                                : 250;
  [[self mapViewImpl] setRotate:[rotate doubleValue] animated:animated duration:duration];
  if (callback) {
    callback(kUIMethodSuccess, nil);
  }
}

LYNX_UI_METHOD(setTilt) {
  id tilt = params[@"tilt"];
  if (![tilt isKindOfClass:[NSNumber class]]) {
    if (callback) {
      callback(kUIMethodParamInvalid, nil);
    }
    return;
  }
  BOOL animated =
      [params[@"animated"] isKindOfClass:[NSNumber class]] ? [params[@"animated"] boolValue] : YES;
  NSTimeInterval duration = [params[@"duration"] isKindOfClass:[NSNumber class]]
                                ? [params[@"duration"] doubleValue]
                                : 250;
  [[self mapViewImpl] setTilt:[tilt doubleValue] animated:animated duration:duration];
  if (callback) {
    callback(kUIMethodSuccess, nil);
  }
}

LYNX_UI_METHOD(getCenter) {
  NSDictionary *center = [[self mapViewImpl] getCenter];
  if (callback) {
    callback(
        kUIMethodSuccess, center ?: @{
          @"latitude" : @0,
          @"longitude" : @0
        });
  }
}

LYNX_UI_METHOD(setScale) {
  id scale = params[@"scale"];
  if (![scale isKindOfClass:[NSNumber class]]) {
    if (callback) {
      callback(kUIMethodParamInvalid, nil);
    }
    return;
  }
  BOOL animated =
      [params[@"animated"] isKindOfClass:[NSNumber class]] ? [params[@"animated"] boolValue] : YES;
  NSTimeInterval duration = [params[@"duration"] isKindOfClass:[NSNumber class]]
                                ? [params[@"duration"] doubleValue]
                                : 250;
  [[self mapViewImpl] setScale:[scale doubleValue] animated:animated duration:duration];
  if (callback) {
    callback(kUIMethodSuccess, nil);
  }
}

LYNX_UI_METHOD(getScale) {
  if (callback) {
    callback(
        kUIMethodSuccess,
        @{@"scale" : @([[self mapViewImpl] getScale])});
  }
}

LYNX_UI_METHOD(setBound) {
  NSDictionary *northeast = LynxMapLatLngFromValue(params[@"northeast"]);
  NSDictionary *southwest = LynxMapLatLngFromValue(params[@"southwest"]);
  if (!northeast || !southwest) {
    if (callback) {
      callback(kUIMethodParamInvalid, nil);
    }
    return;
  }
  BOOL animated =
      [params[@"animated"] isKindOfClass:[NSNumber class]] ? [params[@"animated"] boolValue] : YES;
  NSTimeInterval duration = [params[@"duration"] isKindOfClass:[NSNumber class]]
                                ? [params[@"duration"] doubleValue]
                                : 250;
  [[self mapViewImpl] setBoundWithNortheast:northeast
                                  southwest:southwest
                                   animated:animated
                                   duration:duration];
  if (callback) {
    callback(kUIMethodSuccess, nil);
  }
}

LYNX_UI_METHOD(getBound) {
  NSDictionary *bound = [[self mapViewImpl] getBound];
  if (callback) {
    callback(kUIMethodSuccess, bound ?: LynxMapZeroBound());
  }
}

LYNX_UI_METHOD(fitPoints) {
  NSArray *points = params[@"points"];
  if (![points isKindOfClass:[NSArray class]]) {
    if (callback) {
      callback(kUIMethodParamInvalid, nil);
    }
    return;
  }

  NSMutableArray *validPoints = [NSMutableArray arrayWithCapacity:points.count];
  for (id point in points) {
    NSDictionary *latLng = LynxMapLatLngFromValue(point);
    if (!latLng) {
      if (callback) {
        callback(kUIMethodParamInvalid, nil);
      }
      return;
    }
    [validPoints addObject:latLng];
  }

  NSArray *padding = [params[@"padding"] isKindOfClass:[NSArray class]] ? params[@"padding"] : @[];
  // Frontend order: [left, right, top, bottom] -> iOS SDK order: [top, left, bottom, right]
  NSArray *convertedPadding = padding;
  if (padding.count >= 4) {
    convertedPadding = @[ padding[2], padding[0], padding[3], padding[1] ];
  }
  BOOL animated =
      [params[@"animated"] isKindOfClass:[NSNumber class]] ? [params[@"animated"] boolValue] : YES;
  NSTimeInterval duration = [params[@"duration"] isKindOfClass:[NSNumber class]]
                                ? [params[@"duration"] doubleValue]
                                : 250;
  [[self mapViewImpl] fitPoints:[validPoints copy]
                        padding:convertedPadding
                       animated:animated
                       duration:duration];
  if (callback) {
    callback(kUIMethodSuccess, nil);
  }
}

LYNX_UI_METHOD(setAnchor) {
  NSDictionary *position = params[@"position"];
  if (![position isKindOfClass:[NSDictionary class]] ||
      ![position[@"x"] isKindOfClass:[NSNumber class]] ||
      ![position[@"y"] isKindOfClass:[NSNumber class]]) {
    if (callback) {
      callback(kUIMethodParamInvalid, nil);
    }
    return;
  }

  [[self mapViewImpl] setPointToCenterWithX:[position[@"x"] doubleValue]
                                          y:[position[@"y"] doubleValue]];
  if (callback) {
    callback(kUIMethodSuccess, nil);
  }
}

LYNX_UI_METHOD(latLngToPx) {
  NSDictionary *latLng = params[@"latLng"];
  if (![latLng isKindOfClass:[NSDictionary class]] ||
      ![latLng[@"latitude"] isKindOfClass:[NSNumber class]] ||
      ![latLng[@"longitude"] isKindOfClass:[NSNumber class]]) {
    if (callback) {
      callback(kUIMethodParamInvalid, nil);
    }
    return;
  }

  CGPoint point = [[self mapViewImpl] latLngToPxWithLatitude:[latLng[@"latitude"] doubleValue]
                                                   longitude:[latLng[@"longitude"] doubleValue]];
  if (callback) {
    callback(
        kUIMethodSuccess,
        @{@"x" : @(point.x),
          @"y" : @(point.y)});
  }
}

LYNX_UI_METHOD(pxToLatLng) {
  if (![params[@"x"] isKindOfClass:[NSNumber class]] ||
      ![params[@"y"] isKindOfClass:[NSNumber class]]) {
    if (callback) {
      callback(kUIMethodParamInvalid, nil);
    }
    return;
  }

  NSDictionary *latLng = [[self mapViewImpl] pxToLatLngWithX:[params[@"x"] doubleValue]
                                                           y:[params[@"y"] doubleValue]];
  if (callback) {
    callback(kUIMethodSuccess, @{@"latLng" : latLng});
  }
}

- (void)enableTapGestureSimultaneouslyRecursively:(UIView *)view {
  view.lynxEnableTapGestureSimultaneously = YES;
  [view.subviews enumerateObjectsUsingBlock:^(__kindof UIView *_Nonnull obj, NSUInteger idx,
                                              BOOL *_Nonnull stop) {
    [self enableTapGestureSimultaneouslyRecursively:obj];
  }];
}

@end
