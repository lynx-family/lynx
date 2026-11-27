// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { NodeController } from '@kit.ArkUI';
import {
  ILynxMapMarkerView, ILynxMapView, ILynxMapViewListener, LynxMapInitParams, LynxMapArc,
  LynxMapBounds, LynxMapCircle, LynxMapLatLng, LynxMapMarkerMetadata, LynxMapPoint,
  LynxMapPolygon, LynxMapPolyline, LynxMapRouteLabel
} from './ILynxMapView';

// Like the WebView adapter, each provider supplies its own NodeController.
export abstract class LynxAbstractMapView extends NodeController implements ILynxMapView {
  abstract initializeMapView(params: LynxMapInitParams, listener: ILynxMapViewListener): void;
  abstract setCenter(center: LynxMapLatLng, duration?: number, scale?: number): void;
  abstract setRotate(rotate: number, duration?: number): void;
  abstract setTilt(tilt: number, animated?: boolean, duration?: number): void;
  abstract setScale(scale: number, duration?: number): void;
  abstract getCenter(): LynxMapLatLng | undefined;
  abstract getScale(): number | undefined;
  abstract getBound(): LynxMapBounds | undefined;
  abstract setBound(points: LynxMapLatLng[], padding: number[], duration: number): Promise<void>;
  abstract setPointToCenter(point: LynxMapPoint): void;
  abstract latLngToPx(point: LynxMapLatLng): Promise<LynxMapPoint>;
  abstract pxToLatLng(point: LynxMapPoint): Promise<LynxMapLatLng>;
  abstract setMapStyle(style: string): void;
  abstract setScaleRange(range: number[]): void;
  abstract setRotationDegree(rotation: number): void;
  abstract setEnableScaleControls(enable: boolean): void;
  abstract setLogoPosition(position: string): void;
  abstract setEnableScale(enable: boolean): void;
  abstract setEnableDrag(enable: boolean): void;
  abstract setEnableRotate(enable: boolean): void;
  abstract setEnableTilt(enable: boolean): void;
  abstract setNativeInteractionEnabled(enable: boolean): void;
  abstract setPolyline(polylines: LynxMapPolyline[]): void;
  abstract setRouteLabel(labels: LynxMapRouteLabel[]): void;
  abstract setPolygons(polygons: LynxMapPolygon[]): void;
  abstract setArcs(arcs: LynxMapArc[]): void;
  abstract setCircles(circles: LynxMapCircle[]): void;
  abstract addMarker(metadata: LynxMapMarkerMetadata, builder: CustomBuilder): Promise<ILynxMapMarkerView>;
  abstract destroy(): void;
}
