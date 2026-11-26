// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { BuilderNode, FrameNode, UIContext } from '@kit.ArkUI';
import { AsyncCallback } from '@kit.BasicServicesKit';
import { image } from '@kit.ImageKit';
import { map, mapCommon, MapComponent } from '@kit.MapKit';
import {
  ILynxMapMarkerView, ILynxMapViewListener, LynxAbstractMapView, LynxMapArc, LynxMapBounds, LynxMapCircle,
  LynxMapInitParams, LynxMapLatLng, LynxMapMarkerMetadata, LynxMapPoint, LynxMapPolygon, LynxMapPolyline,
  LynxMapProvider, LynxMapRouteLabel
} from '@lynx/xelement_map_interface';

class SystemMapParams {
  options: mapCommon.MapOptions;
  callback: AsyncCallback<map.MapComponentController>;
  constructor(options: mapCommon.MapOptions, callback: AsyncCallback<map.MapComponentController>) {
    this.options = options;
    this.callback = callback;
  }
}

@Builder
function buildMap(params: SystemMapParams) {
  MapComponent({ mapOptions: params.options, mapCallback: params.callback }).width('100%').height('100%')
}

async function hasMarkerPixels(icon: image.PixelMap): Promise<boolean> {
  const format = icon.getImageInfoSync().pixelFormat;
  if (format !== image.PixelMapFormat.RGBA_8888 && format !== image.PixelMapFormat.BGRA_8888) { return true; }
  const buffer = new ArrayBuffer(icon.getPixelBytesNumber());
  await icon.readPixelsToBuffer(buffer);
  const pixels = new Uint8Array(buffer);
  for (let alpha = 3; alpha < pixels.length; alpha += 4) {
    if (pixels[alpha] !== 0) { return true; }
  }
  return false;
}

async function snapshotMarkerIcon(context: UIContext, builder: CustomBuilder): Promise<image.PixelMap> {
  const snapshot = context.getComponentSnapshot();
  const icon = await snapshot.createFromBuilder(builder, 0, false, { waitUntilRenderFinished: true });
  try {
    if (await hasMarkerPixels(icon)) { return icon; }
  } catch (_) {
    // A PixelMap that cannot be read by ArkTS can still be passed to Map Kit.
    return icon;
  }
  await icon.release();
  // The offscreen ArkUI tree can render before Lynx commits its child content. Give that
  // commit a frame before mounting a fresh builder. A deliberately transparent marker stays transparent.
  await new Promise<void>((resolve) => setTimeout(resolve, 120));
  return snapshot.createFromBuilder(builder, 100, false, { waitUntilRenderFinished: true });
}

class SystemMapMarker implements ILynxMapMarkerView {
  private context: UIContext;
  private marker?: map.Marker;
  private remove: () => void;

  constructor(context: UIContext, marker: map.Marker, remove: () => void) {
    this.context = context;
    this.marker = marker;
    this.remove = remove;
  }

  updateMetadata(metadata: LynxMapMarkerMetadata): void {
    const marker = this.marker;
    if (!marker) { return; }
    if (metadata.position) { marker.setPosition(metadata.position); }
    marker.setMarkerAnchor(metadata.anchor[0], metadata.anchor[1]);
    marker.setZIndex(metadata.zIndex);
    marker.setDraggable(metadata.enableDrag);
    marker.setClickable(metadata.enableClick);
    marker.setAlpha(metadata.alpha);
    marker.setRotation(metadata.rotation);
  }

  async refreshMarker(builder: CustomBuilder): Promise<void> {
    const icon = await snapshotMarkerIcon(this.context, builder);
    try {
      await this.marker?.setIcon(icon);
    } finally {
      await icon.release();
    }
  }

  destroy(): void {
    if (this.marker) {
      this.remove();
      this.marker.remove();
      this.marker = undefined;
    }
  }
}

class SystemMapView extends LynxAbstractMapView {
  private node?: BuilderNode<[SystemMapParams]>;
  private context?: UIContext;
  private controller?: map.MapComponentController;
  private listener?: ILynxMapViewListener;
  private markers: Map<string, string> = new Map();
  private generation: number = 0;
  private disposed: boolean = false;
  private options: mapCommon.MapOptions = {
    position: { target: { latitude: 0, longitude: 0 }, zoom: 2 }
  };
  private gesturesEnabled: boolean = true;
  private readonly loaded = (): void => { this.listener?.onMapEvent('mapload', new Object()); };
  private readonly markerClicked = (marker?: map.Marker): void => {
    const id = marker ? this.markers.get(marker.getId()) : undefined;
    if (id !== undefined) { this.listener?.onMarkerClick(id); }
  };

  initializeMapView(params: LynxMapInitParams, listener: ILynxMapViewListener): void {
    this.listener = listener;
  }

  makeNode(context: UIContext): FrameNode | null {
    if (this.disposed) { return null; }
    this.releaseController();
    this.node?.dispose();
    this.context = context;
    const generation = ++this.generation;
    const params = new SystemMapParams(this.options, (error, controller) => {
      if (this.disposed || generation !== this.generation) { return; }
      if (error || !controller) {
        this.listener?.onMapError(error?.message ?? 'Map Kit did not return a controller.', error?.code);
        return;
      }
      this.controller = controller;
      controller.getEventManager().on('mapLoad', this.loaded);
      controller.getEventManager().on('markerClick', this.markerClicked);
      controller.moveCamera(map.newCameraPosition(this.options.position));
      this.applyGestures();
      this.listener?.onMapReady();
    });
    this.node = new BuilderNode(context);
    this.node.build(wrapBuilder<[SystemMapParams]>(buildMap), params);
    return this.node.getFrameNode();
  }

  private releaseController(): void {
    if (this.controller) {
      this.listener?.onMapUnavailable();
      const events = this.controller.getEventManager();
      events.off('markerClick', this.markerClicked as () => void);
      events.off('mapLoad', this.loaded);
      this.controller = undefined;
    }
    this.markers.clear();
  }

  destroy(): void {
    if (this.disposed) { return; }
    this.disposed = true;
    this.generation++;
    this.releaseController();
    this.node?.dispose();
    this.node = undefined;
    this.context = undefined;
    this.listener = undefined;
  }

  private camera(): mapCommon.CameraPosition {
    return this.controller?.getCameraPosition() ?? this.options.position;
  }

  private updateCamera(position: mapCommon.CameraPosition, duration: number): void {
    this.options.position = position;
    const update = map.newCameraPosition(position);
    if (duration > 0) {
      this.controller?.animateCamera(update, duration);
    } else {
      this.controller?.moveCamera(update);
    }
  }

  setCenter(center: LynxMapLatLng, duration: number = 0, scale?: number): void {
    const camera = this.camera();
    camera.target = center;
    if (scale !== undefined) { camera.zoom = scale; }
    this.updateCamera(camera, duration);
  }
  setScale(scale: number, duration: number = 0): void {
    const camera = this.camera();
    camera.zoom = scale;
    this.updateCamera(camera, duration);
  }
  setRotate(rotate: number, duration: number = 0): void {
    const camera = this.camera();
    camera.bearing = rotate;
    this.updateCamera(camera, duration);
  }
  setTilt(tilt: number, animated: boolean = false, duration: number = 0): void {
    const camera = this.camera();
    camera.tilt = tilt;
    this.updateCamera(camera, animated ? duration : 0);
  }
  getCenter(): LynxMapLatLng { return this.camera().target; }
  getScale(): number { return this.camera().zoom; }
  getBound(): LynxMapBounds | undefined { return this.controller?.getProjection().getVisibleRegion().bounds; }

  async setBound(points: LynxMapLatLng[], padding: number[], duration: number): Promise<void> {
    if (!this.controller || points.length === 0) { return; }
    let bounds = map.LatLngBoundsUtils.include(points[0]);
    for (let i = 1; i < points.length; i++) { bounds = map.LatLngBoundsUtils.include(points[i], bounds); }
    const update = map.newLatLngBounds(bounds, {
      left: this.context!.vp2px(padding[0] ?? 0), top: this.context!.vp2px(padding[1] ?? 0),
      right: this.context!.vp2px(padding[2] ?? 0), bottom: this.context!.vp2px(padding[3] ?? 0)
    });
    if (duration > 0) { this.controller.animateCamera(update, duration); }
    else { this.controller.moveCamera(update); }
  }

  setPointToCenter(point: LynxMapPoint): void {
    if (this.context) {
      this.controller?.setPointToCenter({ positionX: this.context.vp2px(point.x), positionY: this.context.vp2px(point.y) });
    }
  }
  async latLngToPx(point: LynxMapLatLng): Promise<LynxMapPoint> {
    if (!this.controller || !this.context) { throw new Error('Map is not ready'); }
    const projected = this.controller.getProjection().toScreenLocation(point);
    return { x: this.context.px2vp(projected.positionX), y: this.context.px2vp(projected.positionY) };
  }
  async pxToLatLng(point: LynxMapPoint): Promise<LynxMapLatLng> {
    if (!this.controller || !this.context) { throw new Error('Map is not ready'); }
    return this.controller.getProjection().fromScreenLocation({
      positionX: this.context.vp2px(point.x), positionY: this.context.vp2px(point.y)
    });
  }
  setScaleRange(range: number[]): void {
    this.options.minZoom = range[0];
    this.options.maxZoom = range[1];
    this.controller?.setMinZoom(range[0]);
    this.controller?.setMaxZoom(range[1]);
  }
  setRotationDegree(rotation: number): void { this.setRotate(rotation); }
  setEnableScaleControls(enable: boolean): void {
    this.options.zoomControlsEnabled = enable;
    this.controller?.setZoomControlsEnabled(enable);
  }
  private applyGestures(): void {
    this.controller?.setScrollGesturesEnabled(this.gesturesEnabled && (this.options.scrollGesturesEnabled ?? true));
    this.controller?.setZoomGesturesEnabled(this.gesturesEnabled && (this.options.zoomGesturesEnabled ?? true));
    this.controller?.setRotateGesturesEnabled(this.gesturesEnabled && (this.options.rotateGesturesEnabled ?? true));
    this.controller?.setTiltGesturesEnabled(this.gesturesEnabled && (this.options.tiltGesturesEnabled ?? true));
  }
  setEnableScale(enable: boolean): void { this.options.zoomGesturesEnabled = enable; this.applyGestures(); }
  setEnableDrag(enable: boolean): void { this.options.scrollGesturesEnabled = enable; this.applyGestures(); }
  setEnableRotate(enable: boolean): void { this.options.rotateGesturesEnabled = enable; this.applyGestures(); }
  setEnableTilt(enable: boolean): void { this.options.tiltGesturesEnabled = enable; this.applyGestures(); }
  setNativeInteractionEnabled(enable: boolean): void { this.gesturesEnabled = enable; this.applyGestures(); }

  async addMarker(metadata: LynxMapMarkerMetadata, builder: CustomBuilder): Promise<ILynxMapMarkerView> {
    const controller = this.controller;
    const context = this.context;
    if (!controller || !context || !metadata.position) { throw new Error('Map or marker position is unavailable'); }
    const icon = await snapshotMarkerIcon(context, builder);
    try {
      if (controller !== this.controller) { throw new Error('Map was detached'); }
      const marker = await controller.addMarker({ position: metadata.position, icon: icon,
        anchorU: metadata.anchor[0], anchorV: metadata.anchor[1], clickable: metadata.enableClick });
      if (controller !== this.controller) {
        marker.remove();
        throw new Error('Map was detached');
      }
      const nativeId = marker.getId();
      this.markers.set(nativeId, metadata.id);
      return new SystemMapMarker(context, marker, () => { this.markers.delete(nativeId); });
    } finally {
      await icon.release();
    }
  }

  // These provider-specific styles and overlay formats are outside the system adapter's public API.
  setMapStyle(style: string): void {}
  setLogoPosition(position: string): void {}
  setPolyline(polylines: LynxMapPolyline[]): void {}
  setRouteLabel(labels: LynxMapRouteLabel[]): void {}
  setPolygons(polygons: LynxMapPolygon[]): void {}
  setArcs(arcs: LynxMapArc[]): void {}
  setCircles(circles: LynxMapCircle[]): void {}
}

export class SystemMapProvider extends LynxMapProvider {
  createMapView(): LynxAbstractMapView { return new SystemMapView(); }
}
