// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import {
  LynxMapArc, LynxMapBounds, LynxMapCircle, LynxMapInitParams, LynxMapLatLng,
  LynxMapMarkerMetadata, LynxMapPoint, LynxMapPolygon, LynxMapPolyline, LynxMapRouteLabel
} from './LynxMapTypes';

export {
  LynxMapArc, LynxMapBounds, LynxMapCircle, LynxMapInitParams, LynxMapLatLng,
  LynxMapMarkerMetadata, LynxMapPoint, LynxMapPolygon, LynxMapPolyline, LynxMapRouteLabel
} from './LynxMapTypes';

export interface ILynxMapMarkerView {
  updateMetadata(metadata: LynxMapMarkerMetadata): void;
  // Resolves after the provider has consumed the latest child UI snapshot.
  refreshMarker(builder: CustomBuilder): Promise<void>;
  destroy(): void;
}

export interface ILynxMapViewListener {
  onMapReady(): void;
  onMapUnavailable(): void;
  onMapError(message: string, code?: number): void;
  onMapEvent(name: string, detail: Object): void;
  onMarkerClick(id: string): void;
}

export interface ILynxMapView {
  initializeMapView(params: LynxMapInitParams, listener: ILynxMapViewListener): void;
  setCenter(center: LynxMapLatLng, duration?: number, scale?: number): void;
  setRotate(rotate: number, duration?: number): void;
  setTilt(tilt: number, animated?: boolean, duration?: number): void;
  setScale(scale: number, duration?: number): void;
  getCenter(): LynxMapLatLng | undefined;
  getScale(): number | undefined;
  getBound(): LynxMapBounds | undefined;
  // Padding is [left, right, top, bottom] in Lynx logical pixels.
  setBound(points: LynxMapLatLng[], padding: number[], duration: number): Promise<void>;
  setPointToCenter(point: LynxMapPoint): void;
  latLngToPx(point: LynxMapLatLng): Promise<LynxMapPoint>;
  pxToLatLng(point: LynxMapPoint): Promise<LynxMapLatLng>;
  setMapStyle(style: string): void;
  setScaleRange(range: number[]): void;
  setRotationDegree(rotation: number): void;
  setEnableScaleControls(enable: boolean): void;
  setLogoPosition(position: string): void;
  setEnableScale(enable: boolean): void;
  setEnableDrag(enable: boolean): void;
  setEnableRotate(enable: boolean): void;
  setEnableTilt(enable: boolean): void;
  setNativeInteractionEnabled(enable: boolean): void;
  setPolyline(polylines: LynxMapPolyline[]): void;
  setRouteLabel(labels: LynxMapRouteLabel[]): void;
  setPolygons(polygons: LynxMapPolygon[]): void;
  setArcs(arcs: LynxMapArc[]): void;
  setCircles(circles: LynxMapCircle[]): void;
  addMarker(metadata: LynxMapMarkerMetadata, builder: CustomBuilder): Promise<ILynxMapMarkerView>;
  destroy(): void;
}
