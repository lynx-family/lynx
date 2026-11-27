// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxMapKitProvider.h"
#import <MapKit/MapKit.h>
#import <XElement/LynxUIMap.h>
#import <XElement/LynxUIMapMarker.h>

static NSDictionary *Coordinates(CLLocationCoordinate2D coordinate) {
  return @{@"latitude" : @(coordinate.latitude), @"longitude" : @(coordinate.longitude)};
}

@interface LynxMapKitMarker : UIView <ILynxMapMarkerView>
@property(nonatomic, weak) id<ILynxMapView> mapView;
@property(nonatomic, weak) LynxUIMapMarker *markerUI;
@property(nonatomic, copy) NSDictionary *metadata;
@end

@implementation LynxMapKitMarker
- (void)attachToMapView:(id<ILynxMapView>)mapView markerUI:(LynxUIMapMarker *)markerUI {
  self.mapView = mapView;
  self.markerUI = markerUI;
  [self refreshMarkerView];
}
- (NSString *)markerIdentifier {
  return [NSString stringWithFormat:@"%ld", (long)self.markerUI.sign];
}
- (void)updateMetadata:(NSDictionary *)metadata {
  self.metadata = metadata;
  [self refreshMarkerView];
}
- (void)refreshMarkerView {
  if (self.metadata) {
    [self.mapView addOrUpdateMarkerWithIdentifier:self.markerIdentifier
                                         metadata:self.metadata
                                       markerView:self];
  }
}
- (void)handleMarkerTap {
  [self.markerUI sendEvent:@"clickmarker" detail:nil];
}
- (void)destroy {
  [self.mapView removeMarkerWithIdentifier:self.markerIdentifier];
  self.mapView = nil;
}
@end

@interface LynxMapKitAnnotation : MKPointAnnotation
@property(nonatomic, strong) LynxMapKitMarker *content;
@end
@implementation LynxMapKitAnnotation
@end

@interface LynxMapKitView : MKMapView <ILynxMapView, MKMapViewDelegate>
@property(nonatomic, weak) LynxUIMap *mapUI;
@property(nonatomic, strong) NSMutableDictionary<NSString *, LynxMapKitAnnotation *> *markers;
@property(nonatomic) double minimumScale;
@property(nonatomic) double maximumScale;
@property(nonatomic) BOOL sentLoad;
@property(nonatomic, strong) NSValue *pendingCenter;
@property(nonatomic, strong) NSNumber *pendingScale;
@property(nonatomic) BOOL cameraUpdateScheduled;
@property(nonatomic) NSTimeInterval cameraDuration;
@end

@implementation LynxMapKitView
- (instancetype)initWithFrame:(CGRect)frame {
  if ((self = [super initWithFrame:frame])) {
    _markers = [NSMutableDictionary new];
    _minimumScale = 0;
    _maximumScale = 22;
    self.delegate = self;
  }
  return self;
}
- (void)initializeMapViewWithParams:(NSDictionary *)params uiMap:(LynxUIMap *)uiMap {
  self.mapUI = uiMap;
}
- (void)layoutDidFinished {
  if (!self.cameraUpdateScheduled) [self applyPendingCamera];
}
- (void)mapViewDidFinishRenderingMap:(MKMapView *)mapView fullyRendered:(BOOL)fullyRendered {
  if (!self.sentLoad && self.mapUI && fullyRendered) {
    self.sentLoad = YES;
    [self.mapUI sendEvent:@"mapload" detail:nil];
  }
}
- (void)destroy {
  self.pendingCenter = nil;
  self.pendingScale = nil;
  self.delegate = nil;
  [self removeAnnotations:self.markers.allValues];
  [self.markers removeAllObjects];
}
- (void)configureView:(MKAnnotationView *)view forMarker:(LynxMapKitMarker *)marker {
  CGRect bounds = marker.bounds;
  if (CGRectIsEmpty(bounds)) return;
  view.bounds = bounds;
  marker.frame = bounds;
  if (marker.superview != view) [view addSubview:marker];
  NSArray *anchor = marker.metadata[@"anchor"];
  CGFloat x = anchor.count >= 2 ? [anchor[0] doubleValue] : 0.5;
  CGFloat y = anchor.count >= 2 ? [anchor[1] doubleValue] : 0.5;
  view.centerOffset = CGPointMake((0.5 - x) * bounds.size.width, (0.5 - y) * bounds.size.height);
  view.enabled = !marker.metadata[@"clickable"] || [marker.metadata[@"clickable"] boolValue];
  view.alpha = marker.metadata[@"alpha"] ? [marker.metadata[@"alpha"] doubleValue] : 1;
  CGFloat zIndex = [marker.metadata[@"zIndex"] doubleValue];
  if (@available(iOS 14.0, *)) {
    view.zPriority = zIndex;
    view.selectedZPriority = zIndex;
  } else {
    view.layer.zPosition = zIndex;
  }
}
- (MKAnnotationView *)mapView:(MKMapView *)mapView viewForAnnotation:(id<MKAnnotation>)annotation {
  if (![annotation isKindOfClass:[LynxMapKitAnnotation class]]) return nil;
  MKAnnotationView *view = [[MKAnnotationView alloc] initWithAnnotation:annotation
                                                        reuseIdentifier:nil];
  [self configureView:view forMarker:((LynxMapKitAnnotation *)annotation).content];
  return view;
}
- (void)mapView:(MKMapView *)mapView didSelectAnnotationView:(MKAnnotationView *)view {
  if ([view.annotation isKindOfClass:[LynxMapKitAnnotation class]]) {
    [((LynxMapKitAnnotation *)view.annotation).content handleMarkerTap];
    [self deselectAnnotation:view.annotation animated:NO];
  }
}
- (void)addOrUpdateMarkerWithIdentifier:(NSString *)identifier
                               metadata:(NSDictionary *)metadata
                             markerView:(id<ILynxMapMarkerView>)markerView {
  NSDictionary *position = metadata[@"position"];
  if (!position[@"latitude"] || !position[@"longitude"]) {
    [self removeMarkerWithIdentifier:identifier];
    return;
  }
  CLLocationCoordinate2D coordinate = CLLocationCoordinate2DMake(
      [position[@"latitude"] doubleValue], [position[@"longitude"] doubleValue]);
  if (!CLLocationCoordinate2DIsValid(coordinate)) return;
  LynxMapKitAnnotation *annotation = self.markers[identifier];
  if (!annotation) {
    annotation = [LynxMapKitAnnotation new];
    annotation.coordinate = coordinate;
    annotation.content = (LynxMapKitMarker *)markerView;
    self.markers[identifier] = annotation;
    [self addAnnotation:annotation];
  } else {
    annotation.coordinate = coordinate;
    MKAnnotationView *view = [self viewForAnnotation:annotation];
    if (view) [self configureView:view forMarker:annotation.content];
  }
}
- (void)removeMarkerWithIdentifier:(NSString *)identifier {
  LynxMapKitAnnotation *annotation = self.markers[identifier];
  if (annotation) {
    [self removeAnnotation:annotation];
    [self.markers removeObjectForKey:identifier];
  }
}
- (void)setCenterWithLatitude:(double)latitude
                    longitude:(double)longitude
                     animated:(BOOL)animated
                     duration:(NSTimeInterval)duration {
  CLLocationCoordinate2D coordinate = CLLocationCoordinate2DMake(latitude, longitude);
  if (!CLLocationCoordinate2DIsValid(coordinate)) return;
  self.pendingCenter = [NSValue valueWithMKCoordinate:coordinate];
  [self scheduleCameraUpdateAnimated:animated duration:duration];
}
- (NSDictionary *)getCenter {
  [self applyPendingCamera];
  return Coordinates(self.pendingCenter ? self.pendingCenter.MKCoordinateValue
                                        : self.centerCoordinate);
}
- (void)setScale:(double)scale animated:(BOOL)animated duration:(NSTimeInterval)duration {
  if (!isfinite(scale)) return;
  self.pendingScale = @(MAX(self.minimumScale, MIN(self.maximumScale, scale)));
  [self scheduleCameraUpdateAnimated:animated duration:duration];
}
- (void)scheduleCameraUpdateAnimated:(BOOL)animated duration:(NSTimeInterval)duration {
  self.cameraDuration = animated ? MAX(0, duration) / 1000.0 : 0;
  if (self.cameraUpdateScheduled) return;
  self.cameraUpdateScheduled = YES;
  // Lynx can deliver center and scale separately in one update. Apply one MapKit camera change.
  dispatch_async(dispatch_get_main_queue(), ^{
    self.cameraUpdateScheduled = NO;
    [self applyPendingCamera];
  });
}
- (void)applyPendingCamera {
  if ((!self.pendingCenter && !self.pendingScale) || CGRectIsEmpty(self.bounds)) return;
  CLLocationCoordinate2D target =
      self.pendingCenter ? self.pendingCenter.MKCoordinateValue : self.centerCoordinate;
  NSNumber *scale = self.pendingScale;
  self.pendingCenter = nil;
  self.pendingScale = nil;
  BOOL animated = self.cameraDuration > 0;
  void (^update)(void) = ^{
    if (!scale) {
      [self setCenterCoordinate:target animated:animated];
      return;
    }
    double unitsPerPoint = MKMapRectWorld.size.width / (256.0 * exp2(scale.doubleValue));
    MKMapPoint center = MKMapPointForCoordinate(target);
    double width = self.bounds.size.width * unitsPerPoint;
    double height = self.bounds.size.height * unitsPerPoint;
    [self
        setVisibleMapRect:MKMapRectMake(center.x - width / 2, center.y - height / 2, width, height)
                 animated:animated];
  };
  if (animated) {
    [UIView animateWithDuration:self.cameraDuration animations:update];
  } else {
    update();
  }
}
- (double)getScale {
  [self applyPendingCamera];
  if (self.pendingScale) return self.pendingScale.doubleValue;
  if (self.bounds.size.width <= 0 || self.visibleMapRect.size.width <= 0) return 0;
  return log2(MKMapRectWorld.size.width * self.bounds.size.width /
              (256.0 * self.visibleMapRect.size.width));
}
- (void)setScaleRangeWithMinScale:(double)minScale maxScale:(double)maxScale {
  if (minScale <= maxScale) {
    self.minimumScale = minScale;
    self.maximumScale = maxScale;
  }
}
- (void)setRotate:(double)rotate animated:(BOOL)animated duration:(NSTimeInterval)duration {
  [self applyPendingCamera];
  MKMapCamera *camera = [self.camera copy];
  camera.heading = rotate;
  [self setCamera:camera animated:animated];
}
- (void)setTilt:(double)tilt animated:(BOOL)animated duration:(NSTimeInterval)duration {
  [self applyPendingCamera];
  MKMapCamera *camera = [self.camera copy];
  camera.pitch = tilt;
  [self setCamera:camera animated:animated];
}
- (CGPoint)latLngToPxWithLatitude:(double)latitude longitude:(double)longitude {
  [self applyPendingCamera];
  return [self convertCoordinate:CLLocationCoordinate2DMake(latitude, longitude)
                   toPointToView:self];
}
- (NSDictionary *)pxToLatLngWithX:(double)x y:(double)y {
  [self applyPendingCamera];
  return Coordinates([self convertPoint:CGPointMake(x, y) toCoordinateFromView:self]);
}
- (NSDictionary *)getBound {
  return @{
    @"northeast" : [self pxToLatLngWithX:CGRectGetMaxX(self.bounds) y:0],
    @"southwest" : [self pxToLatLngWithX:0 y:CGRectGetMaxY(self.bounds)]
  };
}
- (void)setBoundWithNortheast:(NSDictionary *)northeast
                    southwest:(NSDictionary *)southwest
                     animated:(BOOL)animated
                     duration:(NSTimeInterval)duration {
  [self fitPoints:@[ northeast, southwest ] padding:@[] animated:animated duration:duration];
}
- (void)fitPoints:(NSArray<NSDictionary *> *)points
          padding:(NSArray<NSNumber *> *)padding
         animated:(BOOL)animated
         duration:(NSTimeInterval)duration {
  MKMapRect rect = MKMapRectNull;
  for (NSDictionary *point in points) {
    CLLocationCoordinate2D coordinate = CLLocationCoordinate2DMake(
        [point[@"latitude"] doubleValue], [point[@"longitude"] doubleValue]);
    if (CLLocationCoordinate2DIsValid(coordinate)) {
      MKMapPoint p = MKMapPointForCoordinate(coordinate);
      rect = MKMapRectUnion(rect, MKMapRectMake(p.x, p.y, 1, 1));
    }
  }
  if (MKMapRectIsNull(rect)) return;
  UIEdgeInsets insets = padding.count == 4
                            ? UIEdgeInsetsMake(padding[1].doubleValue, padding[0].doubleValue,
                                               padding[3].doubleValue, padding[2].doubleValue)
                            : UIEdgeInsetsZero;
  [self applyPendingCamera];
  [self setVisibleMapRect:rect edgePadding:insets animated:animated];
}
- (void)setEnableScale:(BOOL)enable {
  self.zoomEnabled = enable;
}
- (void)setEnableDrag:(BOOL)enable {
  self.scrollEnabled = enable;
}
- (void)setEnableRotate:(BOOL)enable {
  self.rotateEnabled = enable;
}
- (void)setEnableTilt:(BOOL)enable {
  self.pitchEnabled = enable;
}
- (void)setRotationDegree:(double)rotation {
  [self setRotate:rotation animated:NO duration:0];
}
// These provider-specific controls and overlay formats are not exposed by the MapKit adapter.
- (void)setMapStyle:(NSString *)mapStyle {
}
- (void)setPointToCenterWithX:(double)x y:(double)y {
}
- (void)setEnableScaleControls:(BOOL)enable {
}
- (void)setLogoPosition:(NSString *)logoPosition {
}
- (void)setPolyline:(NSArray *)polyline {
}
- (void)setPolygons:(NSArray *)polygons {
}
- (void)setCircles:(NSArray *)circles {
}
- (void)setArcs:(NSArray *)arcs {
}
- (void)setRouteLabel:(NSArray *)routeLabel {
}
@end

@implementation LynxMapKitProvider

- (UIView<ILynxMapView> *)createMapView {
  return [[LynxMapKitView alloc] initWithFrame:CGRectZero];
}

- (UIView<ILynxMapMarkerView> *)createMapMarkerView {
  return [[LynxMapKitMarker alloc] initWithFrame:CGRectZero];
}

@end
