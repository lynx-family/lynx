// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <UIKit/UIKit.h>

NS_ASSUME_NONNULL_BEGIN

@class LynxUIMap;
@class LynxUIMapMarker;

@protocol ILynxMapView;

@protocol ILynxMapMarkerView <NSObject>

- (void)attachToMapView:(id<ILynxMapView>)mapView markerUI:(LynxUIMapMarker *)markerUI;
- (void)updateMetadata:(NSDictionary<NSString *, id> *)metadata;
- (void)refreshMarkerView;
- (NSString *)markerIdentifier;
- (void)handleMarkerTap;
- (void)destroy;

@end

@protocol ILynxMapView <NSObject>

- (void)initializeMapViewWithParams:(NSDictionary<NSString *, id> *)params uiMap:(LynxUIMap *)uiMap;
- (void)addOrUpdateMarkerWithIdentifier:(NSString *)identifier
                               metadata:(NSDictionary<NSString *, id> *)metadata
                             markerView:(id<ILynxMapMarkerView>)markerView;
- (void)removeMarkerWithIdentifier:(NSString *)identifier;
- (void)layoutDidFinished;
- (void)destroy;

- (void)setMapStyle:(nullable NSString *)mapStyle;
- (void)setCenterWithLatitude:(double)latitude
                    longitude:(double)longitude
                     animated:(BOOL)animated
                     duration:(NSTimeInterval)duration;
- (NSDictionary<NSString *, NSNumber *> *)getCenter;
- (void)setRotate:(double)rotate animated:(BOOL)animated duration:(NSTimeInterval)duration;
- (void)setTilt:(double)tilt animated:(BOOL)animated duration:(NSTimeInterval)duration;
- (void)setScale:(double)scale animated:(BOOL)animated duration:(NSTimeInterval)duration;
- (double)getScale;
- (void)setScaleRangeWithMinScale:(double)minScale maxScale:(double)maxScale;
- (void)setBoundWithNortheast:(NSDictionary<NSString *, NSNumber *> *)northeast
                    southwest:(NSDictionary<NSString *, NSNumber *> *)southwest
                     animated:(BOOL)animated
                     duration:(NSTimeInterval)duration;
- (NSDictionary<NSString *, NSDictionary<NSString *, NSNumber *> *> *)getBound;
- (void)fitPoints:(NSArray<NSDictionary<NSString *, NSNumber *> *> *)points
          padding:(NSArray<NSNumber *> *)padding
         animated:(BOOL)animated
         duration:(NSTimeInterval)duration;
- (void)setPointToCenterWithX:(double)x y:(double)y;
- (void)setRotationDegree:(double)rotationDegree;
- (void)setEnableScaleControls:(BOOL)enable;
- (void)setLogoPosition:(nullable NSString *)logoPosition;
- (void)setEnableScale:(BOOL)enable;
- (void)setEnableDrag:(BOOL)enable;
- (void)setEnableRotate:(BOOL)enable;
- (void)setEnableTilt:(BOOL)enable;

- (void)setPolyline:(nullable NSArray<NSDictionary<NSString *, id> *> *)polyline;
- (void)setPolygons:(nullable NSArray<NSDictionary<NSString *, id> *> *)polygons;
- (void)setCircles:(nullable NSArray<NSDictionary<NSString *, id> *> *)circles;
- (void)setArcs:(nullable NSArray<NSDictionary<NSString *, id> *> *)arcs;
- (void)setRouteLabel:(nullable NSArray<NSDictionary<NSString *, id> *> *)routeLabel;

- (CGPoint)latLngToPxWithLatitude:(double)latitude longitude:(double)longitude;
- (NSDictionary<NSString *, NSNumber *> *)pxToLatLngWithX:(double)x y:(double)y;

@end

NS_ASSUME_NONNULL_END
