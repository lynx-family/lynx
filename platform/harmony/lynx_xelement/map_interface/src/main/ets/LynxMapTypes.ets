// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

export interface LynxMapLatLng {
  latitude: number;
  longitude: number;
}

// Screen coordinates use logical pixels, as in the Android Lynx map API.
export interface LynxMapPoint {
  x: number;
  y: number;
}

export interface LynxMapBounds {
  northeast: LynxMapLatLng;
  southwest: LynxMapLatLng;
}

export interface LynxMapErrorEvent {
  errCode: number;
  errMsg: string;
}

export interface LynxMapClickEvent {
  point: LynxMapLatLng;
}

export interface LynxMapPolylineClickEvent {
  id: string;
}

export interface LynxMapRegionChangeEvent {
  type: string;
  rotate: number;
  scale: number;
  center: LynxMapLatLng;
  region: LynxMapBounds;
  causedBy: string;
  triggeredBy: string;
}

export interface LynxMapPolylineTexture {
  id: string;
  url: string;
}

export interface LynxMapPolyline {
  id: string;
  points: LynxMapLatLng[];
  reducePoints?: boolean;
  collideDetection?: boolean;
  edgeColor?: string[];
  gradientVertexColor?: string[];
  edgeTexture?: string[];
  edgeTextureAssets?: LynxMapPolylineTexture[];
  width?: number;
  dottedLine?: boolean;
  lineCapType?: string;
  lineJoinType?: string;
  zIndex?: number;
}

export interface LynxMapRouteLabel {
  id: string;
  points: LynxMapLatLng[];
  text: string;
  textColor?: string;
  fontSize?: number;
  borderColor?: string;
  borderWidth?: number;
  fontWeight?: string;
  zIndex?: number;
}

export interface LynxMapPolygon {
  id: string;
  points: LynxMapLatLng[];
  dottedLine?: boolean;
  fillColor?: string;
  strokeWidth?: number;
  strokeColor?: string;
  zIndex?: number;
}

export interface LynxMapArc {
  id: string;
  start: LynxMapLatLng;
  mid: LynxMapLatLng;
  end: LynxMapLatLng;
  width?: number;
  color?: string;
  zIndex?: number;
}

export interface LynxMapCircle {
  id: string;
  center: LynxMapLatLng;
  radius: number;
  fillColor?: string;
  strokeWidth?: number;
  strokeColor?: string;
  zIndex?: number;
}

export interface LynxMapAnimationChannel {
  from?: number;
  to?: number;
  duration?: number;
  repeatCount?: number;
}

export interface LynxMapMarkerAnimation {
  scale?: LynxMapAnimationChannel;
  alpha?: LynxMapAnimationChannel;
}

export class LynxMapInitParams {
  bizName: string = 'undefined';
  mapType: number = -1;
  extraParams?: Record<string, Object>;
}

export class LynxMapMarkerMetadata {
  id: string = '';
  position?: LynxMapLatLng;
  containerPosition?: LynxMapPoint;
  anchor: number[] = [0.5, 0.5];
  enableDrag: boolean = false;
  enableClick: boolean = true;
  enableCollideWithPoi: boolean = false;
  alpha: number = 1;
  rotation: number = 0;
  zIndex: number = 1;
  animate?: LynxMapMarkerAnimation;
}
