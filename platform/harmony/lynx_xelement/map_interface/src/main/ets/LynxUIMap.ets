// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { EventHandlerArray, LLog, LynxUIMethodConstants, NativeContent, UIBase, UIGroup } from '@lynx/lynx';
import {
  ILynxMapViewListener, LynxMapArc, LynxMapCircle, LynxMapInitParams, LynxMapLatLng,
  LynxMapPoint, LynxMapPolygon, LynxMapPolyline, LynxMapRouteLabel
} from './ILynxMapView';
import { LynxAbstractMapView } from './LynxAbstractMapView';
import { LynxXElementMapService } from './LynxXElementMapService';
import { LynxUIMapMarker } from './LynxUIMapMarker';
import { LynxMapErrorEvent } from './LynxMapTypes';

interface LynxMapMethodError {
  message: string;
}

interface LynxMapScaleResult {
  scale: number;
}

interface LynxMapLatLngResult {
  latLng: LynxMapLatLng;
}

@ObservedV2
class LynxMapParams {
  @Trace mapView?: LynxAbstractMapView;
}

@ComponentV2
struct LynxMapView {
  @Param @Require params: LynxMapParams;

  build() {
    Stack() {
      if (this.params.mapView) {
        NodeContainer(this.params.mapView).width('100%').height('100%')
      }
    }.width('100%').height('100%')
  }
}

@Builder
function buildMap(ui: UIBase) {
  LynxMapView({ params: (ui as LynxUIMap).mapParams })
}

export class LynxUIMap extends UIGroup implements ILynxMapViewListener {
  readonly mapParams: LynxMapParams = new LynxMapParams();
  readonly builder: WrappedBuilder<[UIBase]> = wrapBuilder<[UIBase]>(buildMap);
  private initParams: LynxMapInitParams = new LynxMapInitParams();
  private nodeReadyScales: number[] = [];
  private initialized: boolean = false;
  private ready: boolean = false;
  private disposed: boolean = false;

  override getMemoryUsageBytes(): number {
    return 100 * 1024 * 1024;
  }

  override hasCustomizedLayout(): boolean {
    return true;
  }

  override onNodeReload(): void {
    super.onNodeReload();
    this.onMapEvent('mapload', new Object());
  }

  override update(props: Record<string, Object>, events?: EventHandlerArray[]): void {
    super.update(props, events);
    for (const entry of Object.entries(props)) {
      const key = entry[0];
      const value = entry[1];
      if (key === 'scale') {
        continue;
      }
      if (!this.initialized) {
        if (key === 'map-type') {
          this.initParams.mapType = typeof value === 'number' ? value : -1;
        } else if (key === 'biz-name') {
          this.initParams.bizName = typeof value === 'string' ? value : 'undefined';
        } else if (key === 'extra-params') {
          this.initParams.extraParams = value as Record<string, Object> | undefined;
        }
      }
      if (key === 'center') {
        const center = value as LynxMapLatLng | undefined;
        if (center && Number.isFinite(center.latitude) && Number.isFinite(center.longitude) &&
          Math.abs(center.latitude) <= 90 && Math.abs(center.longitude) <= 180) {
          const pendingCenter: LynxMapLatLng = { latitude: center.latitude, longitude: center.longitude };
          this.addNodeReadyBlock(() => {
            this.mapParams.mapView?.setCenter(pendingCenter);
          });
        }
      } else {
        this.addNodeReadyBlock(() => this.updateMapProperty(key, value));
      }
    }
    // Android gives scale lower priority than every other property in the
    // entire node-ready batch, including properties from later update calls.
    const scale = props.scale;
    if (typeof scale === 'number' && Number.isFinite(scale)) {
      this.nodeReadyScales.push(scale);
    }
  }

  private updateMapProperty(key: string, value: Object): void {
    const map = this.mapParams.mapView;
    if (!map || this.disposed) {
      return;
    }
    switch (key) {
      case 'scale-range': {
        const range = value as number[] | undefined;
        if (range && range.length >= 2 && Number.isFinite(range[0]) &&
          Number.isFinite(range[1]) && range[0] <= range[1]) {
          map.setScaleRange([range[0], range[1]]);
        }
        break;
      }
      case 'map-style': map.setMapStyle(typeof value === 'string' ? value : 'normal-light'); break;
      case 'rotate':
        if (typeof value === 'number' && Number.isFinite(value)) {
          map.setRotationDegree(value);
        }
        break;
      case 'enable-scale-controls': map.setEnableScaleControls(value as boolean); break;
      case 'logo-position': map.setLogoPosition(typeof value === 'string' ? value : 'bottom-left'); break;
      case 'enable-scale': map.setEnableScale(value as boolean); break;
      case 'enable-drag': map.setEnableDrag(value as boolean); break;
      case 'enable-rotate': map.setEnableRotate(value as boolean); break;
      case 'enable-tilt': map.setEnableTilt(value as boolean); break;
      case 'native-interaction-enabled': map.setNativeInteractionEnabled(value as boolean); break;
      case 'polyline': map.setPolyline((value as LynxMapPolyline[]) ?? []); break;
      case 'route-label': map.setRouteLabel((value as LynxMapRouteLabel[]) ?? []); break;
      case 'polygons': map.setPolygons((value as LynxMapPolygon[]) ?? []); break;
      case 'arcs': map.setArcs((value as LynxMapArc[]) ?? []); break;
      case 'circles': map.setCircles((value as LynxMapCircle[]) ?? []); break;
    }
  }

  override onNodeReady(): void {
    if (this.disposed) {
      return;
    }
    // Detach this batch before invoking providers. Reentrant updates belong to
    // the next node-ready callback, just like UIBase's operation queue.
    const scales = this.nodeReadyScales;
    this.nodeReadyScales = [];
    if (!this.initialized) {
      this.initialized = true;
      const provider = LynxXElementMapService.provider;
      if (!provider) {
        this.onMapError('No map provider registered.');
      } else {
        const mapView = provider.createMapView();
        this.mapParams.mapView = mapView;
        mapView.initializeMapView(this.initParams, this);
      }
    }
    super.onNodeReady();
    for (const scale of scales) {
      if (this.disposed) {
        break;
      }
      this.mapParams.mapView?.setScale(scale);
    }
  }

  onMapReady(): void {
    if (this.disposed) {
      return;
    }
    this.ready = true;
    for (const child of this.children) {
      const marker = UIBase.getUIFromNativeContent(child);
      if (marker instanceof LynxUIMapMarker) {
        marker.bindIMapView(this.mapParams.mapView);
      }
    }
  }

  onMapUnavailable(): void {
    this.ready = false;
    for (const child of this.children) {
      const marker = UIBase.getUIFromNativeContent(child);
      if (marker instanceof LynxUIMapMarker) {
        marker.bindIMapView(undefined);
      }
    }
  }

  onMapError(message: string, code: number = 1): void {
    LLog.e('LynxUIMap', message);
    this.onMapEvent('error', { errCode: code, errMsg: message } as LynxMapErrorEvent);
  }

  onMapEvent(name: string, detail: Object): void {
    if (!this.disposed) {
      this.sendCustomEvent(name, detail, 'detail');
    }
  }

  onMarkerClick(id: string): void {
    if (this.disposed) {
      return;
    }
    for (const child of this.children) {
      const marker = UIBase.getUIFromNativeContent(child);
      if (marker instanceof LynxUIMapMarker && `${marker.sign}` === id) {
        marker.onMarkerClick();
        return;
      }
    }
  }

  override invokeMethod(method: string, params: Object, callback: (code: number, res: Object) => void): boolean {
    if (['setCenter', 'getCenter', 'setRotate', 'setTilt', 'setScale', 'getScale', 'setBound', 'getBound',
      'fitPoints', 'setAnchor', 'latLngToPx', 'pxToLatLng'].indexOf(method) < 0) {
      return super.invokeMethod(method, params, callback);
    }
    this.invokeMapMethod(method, params as Record<string, Object>, callback);
    return true;
  }

  private async invokeMapMethod(method: string, params: Record<string, Object>,
    callback: (code: number, res: Object) => void): Promise<void> {
    const map = this.mapParams.mapView;
    if (this.disposed || !this.ready || !map) {
      callback(LynxUIMethodConstants.INVALID_STATE_ERROR, this.methodError('map is not loaded'));
      return;
    }
    let result: Object = new Object();
    const duration = params?.animated === false ? 0 :
      (typeof params?.duration === 'number' && Number.isFinite(params.duration) ? Math.max(0, params.duration) : 250);
    try {
      switch (method) {
        case 'setCenter': {
          const center = params?.center as LynxMapLatLng | undefined;
          if (!this.isLatLng(center)) {
            callback(LynxUIMethodConstants.PARAM_INVALID, this.methodError('Missing or invalid center'));
            return;
          }
          const scale = params?.scale;
          if (scale !== undefined && (typeof scale !== 'number' || !Number.isFinite(scale))) {
            callback(LynxUIMethodConstants.PARAM_INVALID, this.methodError('Invalid scale'));
            return;
          }
          map.setCenter(center!, duration, scale as number | undefined);
          break;
        }
        case 'setRotate':
          if (typeof params?.rotate !== 'number' || !Number.isFinite(params.rotate)) {
            callback(LynxUIMethodConstants.PARAM_INVALID, this.methodError('Missing or invalid rotate'));
            return;
          }
          map.setRotate(params.rotate, duration);
          break;
        case 'setTilt':
          if (typeof params?.tilt !== 'number' || !Number.isFinite(params.tilt)) {
            callback(LynxUIMethodConstants.PARAM_INVALID, this.methodError('Missing or invalid tilt'));
            return;
          }
          map.setTilt(params.tilt, params?.animated !== false, duration);
          break;
        case 'getCenter': {
          const center = map.getCenter();
          if (!center) {
            throw new Error('map center is unavailable');
          }
          result = center;
          break;
        }
        case 'setScale':
          if (typeof params?.scale !== 'number' || !Number.isFinite(params.scale)) {
            callback(LynxUIMethodConstants.PARAM_INVALID, this.methodError('Missing or invalid scale'));
            return;
          }
          map.setScale(params.scale, duration);
          break;
        case 'getScale': {
          const scale = map.getScale();
          if (scale === undefined) {
            throw new Error('map scale is unavailable');
          }
          result = { scale: scale } as LynxMapScaleResult;
          break;
        }
        case 'getBound': {
          const bounds = map.getBound();
          if (!bounds) {
            throw new Error('map bounds are unavailable');
          }
          result = bounds;
          break;
        }
        case 'setBound': {
          const northeast = params?.northeast as LynxMapLatLng | undefined;
          const southwest = params?.southwest as LynxMapLatLng | undefined;
          if (!this.isLatLng(northeast) || !this.isLatLng(southwest)) {
            callback(LynxUIMethodConstants.PARAM_INVALID, this.methodError('Missing or invalid bounds'));
            return;
          }
          await map.setBound([northeast!, southwest!], [0, 0, 0, 0], duration);
          break;
        }
        case 'fitPoints': {
          const points = params?.points as LynxMapLatLng[] | undefined;
          if (!Array.isArray(points) || points.length === 0 ||
            !points.every((point: LynxMapLatLng) => this.isLatLng(point))) {
            callback(LynxUIMethodConstants.PARAM_INVALID, this.methodError('Missing or invalid points'));
            return;
          }
          const padding = params.padding as number[] | undefined;
          const insets = padding && padding.length >= 4 &&
            padding.slice(0, 4).every((value: number) => Number.isFinite(value)) ? padding.slice(0, 4) : [0, 0, 0, 0];
          await map.setBound(points, insets, duration);
          break;
        }
        case 'setAnchor': {
          const point = params?.position as LynxMapPoint | undefined;
          if (!point || !Number.isFinite(point.x) || !Number.isFinite(point.y)) {
            callback(LynxUIMethodConstants.PARAM_INVALID, this.methodError('Missing or invalid position'));
            return;
          }
          map.setPointToCenter(point);
          break;
        }
        case 'latLngToPx': {
          const point = params?.latLng as LynxMapLatLng | undefined;
          if (!this.isLatLng(point)) {
            callback(LynxUIMethodConstants.PARAM_INVALID, this.methodError('Missing or invalid latLng'));
            return;
          }
          result = await map.latLngToPx(point!);
          break;
        }
        case 'pxToLatLng':
          if (!params || !Number.isFinite(params.x as number) || !Number.isFinite(params.y as number)) {
            callback(LynxUIMethodConstants.PARAM_INVALID, this.methodError('Missing or invalid screen coordinates'));
            return;
          }
          result = { latLng: await map.pxToLatLng({ x: params.x as number, y: params.y as number }) } as LynxMapLatLngResult;
          break;
      }
      if (this.disposed || this.mapParams.mapView !== map || !this.ready) {
        callback(LynxUIMethodConstants.INVALID_STATE_ERROR, this.methodError('map is no longer available'));
      } else {
        callback(LynxUIMethodConstants.SUCCESS, result);
      }
    } catch (error) {
      callback(LynxUIMethodConstants.INVALID_STATE_ERROR, this.methodError(`${error}`));
    }
  }

  private methodError(message: string): LynxMapMethodError {
    return { message: message };
  }

  private isLatLng(point?: LynxMapLatLng): boolean {
    return point !== undefined && point !== null && Number.isFinite(point.latitude) &&
      Number.isFinite(point.longitude) && Math.abs(point.latitude) <= 90 && Math.abs(point.longitude) <= 180;
  }

  override insertChild(content: NativeContent, index: number): void {
    if (this.disposed) {
      return;
    }
    super.insertChild(content, index < 0 ? this.children.length : index);
    const marker = UIBase.getUIFromNativeContent(content);
    if (marker instanceof LynxUIMapMarker) {
      marker.bindNativeContent(content);
      if (this.ready) {
        marker.bindIMapView(this.mapParams.mapView);
      }
    }
  }

  override layout(x: number, y: number, width: number, height: number, paddingLeft: number, paddingTop: number,
    paddingRight: number, paddingBottom: number, marginLeft: number, marginTop: number, marginRight: number,
    marginBottom: number, sticky?: number[]): void {
    // The NodeContainer follows the native layout; map children use geographic positions.
  }

  override dispose(): void {
    if (this.disposed) {
      return;
    }
    this.disposed = true;
    this.nodeReadyScales = [];
    this.onMapUnavailable();
    this.mapParams.mapView?.destroy();
    this.mapParams.mapView = undefined;
    super.dispose();
  }
}
