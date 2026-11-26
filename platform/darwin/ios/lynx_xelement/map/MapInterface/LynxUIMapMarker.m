// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <XElement/LynxUIMapMarker.h>

#import <Lynx/LynxComponentRegistry.h>
#import <Lynx/LynxContext.h>
#import <Lynx/LynxEvent.h>
#import <Lynx/LynxPropsProcessor.h>
#import <Lynx/LynxUIMethodProcessor.h>

#import "LynxXElementMapService.h"

static NSDictionary<NSString *, NSNumber *> *LynxMarkerLatLngFromValue(id value) {
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

@interface LynxUIMapMarker ()

@property(nonatomic, strong) NSMutableDictionary<NSString *, id> *metadata;
@property(nonatomic, assign) BOOL metadataDirty;
@property(nonatomic, assign) BOOL markerReady;
@property(nonatomic, assign) BOOL mapViewBound;

@end

@implementation LynxUIMapMarker

LYNX_LAZY_REGISTER_UI("map-marker")

- (instancetype)init {
  self = [super init];
  if (self) {
    _metadata = [NSMutableDictionary dictionary];
    _metadataDirty = NO;
    _markerReady = NO;
    _mapViewBound = NO;
  }
  return self;
}

- (UIView *)createView {
  return [[LynxXElementMapService provider] createMapMarkerView];
}

- (id<ILynxMapMarkerView>)markerViewImpl {
  return (id<ILynxMapMarkerView>)self.view;
}

- (void)bindMapView:(id<ILynxMapView>)mapView {
  self.mapViewBound = YES;
  [[self markerViewImpl] attachToMapView:mapView markerUI:self];
  [self flushMetadataIfPossible];
}

- (void)markMetadataDirty {
  self.metadataDirty = YES;
  [self flushMetadataIfPossible];
}

- (void)flushMetadataIfPossible {
  if (!self.markerReady || !self.mapViewBound || !self.metadataDirty) {
    return;
  }
  self.metadataDirty = NO;
  [[self markerViewImpl] updateMetadata:[self.metadata copy]];
}

- (void)onNodeReady {
  [super onNodeReady];
  self.markerReady = YES;
  [self flushMetadataIfPossible];
}

- (void)onNodeRemoved {
  [[self markerViewImpl] destroy];
  [super onNodeRemoved];
}

- (void)updateFrame:(CGRect)frame
            withPadding:(UIEdgeInsets)padding
                 border:(UIEdgeInsets)border
                 margin:(UIEdgeInsets)margin
    withLayoutAnimation:(BOOL)with {
  CGSize previousSize = self.frame.size;
  [super updateFrame:CGRectMake(0, 0, frame.size.width, frame.size.height)
              withPadding:padding
                   border:border
                   margin:margin
      withLayoutAnimation:with];

  if (!CGSizeEqualToSize(previousSize, self.frame.size) && self.markerReady && self.mapViewBound) {
    [self.view.superview layoutIfNeeded];
    [[self markerViewImpl] refreshMarkerView];
  }
}

- (void)sendEvent:(NSString *)name detail:(NSDictionary<NSString *, id> *)detail {
  LynxDetailEvent *event = [[LynxDetailEvent alloc] initWithName:name
                                                      targetSign:[self sign]
                                                          detail:detail ?: @{}];
  [self.context.eventEmitter sendCustomEvent:event];
}

LYNX_PROP_SETTER("metadata", metadata, NSDictionary *) {
  if (![value isKindOfClass:[NSDictionary class]]) {
    return;
  }
  [self.metadata addEntriesFromDictionary:value];
  [self markMetadataDirty];
}

LYNX_PROP_SETTER("position", position, NSDictionary *) {
  NSDictionary<NSString *, NSNumber *> *position = LynxMarkerLatLngFromValue(value);
  if (!position) {
    [self.metadata removeObjectForKey:@"position"];
    [self.metadata removeObjectForKey:@"points"];
    [self markMetadataDirty];
    return;
  }

  self.metadata[@"position"] = position;
  self.metadata[@"points"] = @[ @[ position[@"longitude"], position[@"latitude"] ] ];
  [self markMetadataDirty];
}

LYNX_PROP_SETTER("enable-drag", enableDrag, BOOL) {
  self.metadata[@"draggable"] = @(value);
  [self markMetadataDirty];
}

LYNX_PROP_SETTER("enable-click", enableClick, BOOL) {
  self.metadata[@"clickable"] = @(value);
  [self markMetadataDirty];
}

LYNX_PROP_SETTER("container-position", containerPosition, NSDictionary *) {
  if (![value isKindOfClass:[NSDictionary class]] ||
      ![value[@"x"] isKindOfClass:[NSNumber class]] ||
      ![value[@"y"] isKindOfClass:[NSNumber class]]) {
    [self.metadata removeObjectForKey:@"containerPosition"];
    [self.metadata removeObjectForKey:@"offset"];
    [self markMetadataDirty];
    return;
  }

  self.metadata[@"containerPosition"] = @{@"x" : value[@"x"], @"y" : value[@"y"]};
  self.metadata[@"offset"] = @{@"offsetX" : value[@"x"], @"offsetY" : value[@"y"]};
  [self markMetadataDirty];
}

LYNX_PROP_SETTER("anchor", anchor, NSArray<NSNumber *> *) {
  if (![value isKindOfClass:[NSArray class]] || value.count < 2) {
    [self.metadata removeObjectForKey:@"anchor"];
    [self markMetadataDirty];
    return;
  }

  self.metadata[@"anchor"] = @[ value[0], value[1] ];
  [self markMetadataDirty];
}

LYNX_PROP_SETTER("z-index", zIndex, NSInteger) {
  self.metadata[@"zIndex"] = @(value);
  [self markMetadataDirty];
}

LYNX_PROP_SETTER("alpha", alpha, CGFloat) {
  self.metadata[@"alpha"] = @(value);
  [self markMetadataDirty];
}

LYNX_PROP_SETTER("enable-collide-with-poi", enableCollideWithPoi, BOOL) {
  self.metadata[@"isCollideWithPoi"] = @(value);
  [self markMetadataDirty];
}

LYNX_PROP_SETTER("rotation", rotation, CGFloat) {
  if (value < 0 || value > 360) {
    return;
  }
  self.metadata[@"rotateAngle"] = @(360.0 - value);
  [self markMetadataDirty];
}

LYNX_PROP_SETTER("animate", animate, NSDictionary *) {
  if (![value isKindOfClass:[NSDictionary class]]) {
    [self.metadata removeObjectForKey:@"animate"];
    [self markMetadataDirty];
    return;
  }

  NSMutableDictionary *animateMetadata = [NSMutableDictionary dictionary];
  [self appendAnimationConfig:value[@"scale"] key:@"scale" target:animateMetadata];
  [self appendAnimationConfig:value[@"alpha"] key:@"alpha" target:animateMetadata];

  if (animateMetadata.count == 0) {
    [self.metadata removeObjectForKey:@"animate"];
  } else {
    self.metadata[@"animate"] = [animateMetadata copy];
  }
  [self markMetadataDirty];
}

LYNX_UI_METHOD(refreshMarker) {
  [self.view.superview layoutIfNeeded];
  [[self markerViewImpl] refreshMarkerView];
  if (callback) {
    callback(kUIMethodSuccess, nil);
  }
}

- (void)appendAnimationConfig:(id)config key:(NSString *)key target:(NSMutableDictionary *)target {
  if (![config isKindOfClass:[NSDictionary class]]) {
    return;
  }
  NSDictionary *configDict = (NSDictionary *)config;
  NSMutableDictionary *animationConfig = [NSMutableDictionary dictionary];
  if ([configDict[@"from"] isKindOfClass:[NSNumber class]]) {
    animationConfig[@"from"] = configDict[@"from"];
  }
  if ([configDict[@"to"] isKindOfClass:[NSNumber class]]) {
    animationConfig[@"to"] = configDict[@"to"];
  }
  if ([configDict[@"duration"] isKindOfClass:[NSNumber class]]) {
    animationConfig[@"duration"] = configDict[@"duration"];
  }
  if ([configDict[@"repeatCount"] isKindOfClass:[NSNumber class]]) {
    animationConfig[@"repeatCount"] = configDict[@"repeatCount"];
  }
  if (animationConfig.count > 0) {
    target[key] = [animationConfig copy];
  }
}

- (void)dealloc {
  [[self markerViewImpl] destroy];
}

@end
