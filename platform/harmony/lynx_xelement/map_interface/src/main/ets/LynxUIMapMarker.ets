// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { EventHandlerArray, LLog, LynxUIMethodConstants, NativeContent, UIBase, UIGroup } from '@lynx/lynx';
import { ILynxMapMarkerView, ILynxMapView, LynxMapLatLng, LynxMapMarkerMetadata, LynxMapPoint } from './ILynxMapView';
import { LynxMapMarkerAnimation } from './LynxMapTypes';

export class LynxUIMapMarker extends UIGroup {
  readonly builder: WrappedBuilder<[UIBase]> = wrapBuilder<[UIBase]>(buildMapMarkerHost);
  private metadata: LynxMapMarkerMetadata = new LynxMapMarkerMetadata();
  private mapView?: ILynxMapView;
  private markerView?: ILynxMapMarkerView;
  private nativeContent?: NativeContent;
  private customMarkerBuilder?: CustomBuilder;
  private refreshTask?: number;
  private revision: number = 0;
  private attemptedRevision: number = 0;
  private epoch: number = 0;
  private refreshing: boolean = false;
  private foreground: boolean = true;
  private disposed: boolean = false;

  hasCustomizedLayout(): boolean {
    return false;
  }

  onCreate(): void {
    // Native UIBase owns the child tree and its Lynx layout. Do not register
    // UIGroup's ETS child-management callbacks for this native layout path.
  }

  protected observeInvalidation(): boolean {
    return true;
  }

  protected needWindowStateChangeEvent(): boolean {
    return true;
  }

  bindNativeContent(content?: NativeContent): void {
    if (this.disposed || this.nativeContent === content) {
      return;
    }
    this.nativeContent = content;
    this.removeMarkerView();
    this.refreshMarker();
  }

  getNativeContent(): NativeContent | undefined {
    return this.nativeContent;
  }

  setCustomMarkerBuilder(builder: CustomBuilder): void {
    if (this.disposed) {
      return;
    }
    this.customMarkerBuilder = builder;
    this.refreshMarker();
  }

  bindIMapView(mapView?: ILynxMapView): void {
    if (this.disposed || this.mapView === mapView) {
      return;
    }
    this.mapView = mapView;
    this.removeMarkerView();
    this.refreshMarker();
  }

  override onNodeRemoved(): void {
    super.onNodeRemoved();
    // Only a real detach reaches this lifecycle. A remove/insert used for a
    // move keeps the map binding and its native marker untouched.
    this.mapView = undefined;
    if (this.refreshTask !== undefined) {
      clearTimeout(this.refreshTask);
      this.refreshTask = undefined;
    }
    this.removeMarkerView();
  }

  update(props: Record<string, Object>, events?: EventHandlerArray[]): void {
    super.update(props, events);
    let snapshotChanged = false;
    for (const entry of Object.entries(props)) {
      const key = entry[0];
      const value = entry[1];
      if (key === 'position') {
        const position = value as LynxMapLatLng | undefined;
        this.metadata.position = position && Number.isFinite(position.latitude) && Number.isFinite(position.longitude) ?
          { latitude: position.latitude, longitude: position.longitude } : undefined;
        if (!this.metadata.position) {
          this.removeMarkerView();
        }
      } else if (key === 'anchor') {
        const anchor = value as number[] | undefined;
        this.metadata.anchor = anchor && anchor.length >= 2 && Number.isFinite(anchor[0]) && Number.isFinite(anchor[1]) ?
          [anchor[0], anchor[1]] : [0.5, 0.5];
      } else if (key === 'container-position') {
        const point = value as LynxMapPoint | undefined;
        this.metadata.containerPosition = point && Number.isFinite(point.x) && Number.isFinite(point.y) ?
          { x: point.x, y: point.y } : undefined;
      } else if (key === 'enable-drag') {
        this.metadata.enableDrag = typeof value === 'boolean' ? value : false;
      } else if (key === 'enable-click') {
        this.metadata.enableClick = typeof value === 'boolean' ? value : true;
      } else if (key === 'enable-collide-with-poi') {
        this.metadata.enableCollideWithPoi = typeof value === 'boolean' ? value : false;
      } else if (key === 'alpha') {
        this.metadata.alpha = typeof value === 'number' && Number.isFinite(value) ? value : 1;
      } else if (key === 'rotation') {
        if (typeof value === 'number' && Number.isFinite(value) && value >= 0 && value <= 360) {
          this.metadata.rotation = value;
        } else if (value === undefined || value === null) {
          this.metadata.rotation = 0;
        }
      } else if (key === 'z-index') {
        this.metadata.zIndex = typeof value === 'number' && Number.isFinite(value) ? value : 1;
      } else if (key === 'animate') {
        this.metadata.animate = value ? JSON.parse(JSON.stringify(value)) as LynxMapMarkerAnimation : undefined;
      } else {
        snapshotChanged = true;
      }
    }
    // Metadata changes do not need a new bitmap. Root styles still do.
    if (snapshotChanged || !this.markerView) {
      this.refreshMarker();
    }
  }

  onNodeReady(): void {
    super.onNodeReady();
    if (!this.markerView) {
      this.refreshMarker();
    } else if (!this.refreshing) {
      this.markerView.updateMetadata(this.captureMetadata());
    }
  }

  layout(x: number, y: number, width: number, height: number, paddingLeft: number, paddingTop: number,
    paddingRight: number, paddingBottom: number, marginLeft: number, marginTop: number, marginRight: number,
    marginBottom: number, sticky?: number[]): void {
    this.width = width;
    this.height = height;
    if (!Number.isFinite(width) || !Number.isFinite(height) || width <= 0 || height <= 0) {
      this.removeMarkerView();
    }
    this.refreshMarker();
  }

  invalidate(): void {
    this.refreshMarker();
  }

  refreshMarker(): void {
    if (!this.disposed) {
      ++this.revision;
      this.scheduleRefresh();
    }
  }

  private scheduleRefresh(): void {
    if (this.disposed || this.refreshing || this.refreshTask !== undefined) {
      return;
    }
    // Merge native notifications before asking the provider for a snapshot.
    this.refreshTask = setTimeout(() => {
      this.refreshTask = undefined;
      this.flushMarker();
    }, 0);
  }

  private async flushMarker(): Promise<void> {
    const mapView = this.mapView;
    const content = this.nativeContent;
    const builder = this.customMarkerBuilder;
    if (this.disposed || !this.foreground || this.refreshing || !mapView || !content || !builder ||
      !Number.isFinite(this.width) || !Number.isFinite(this.height) || this.width <= 0 || this.height <= 0 ||
      !this.metadata.position || this.revision <= this.attemptedRevision) {
      return;
    }
    this.refreshing = true;
    this.attemptedRevision = this.revision;
    const epoch = this.epoch;
    const metadata = this.captureMetadata();
    try {
      if (!this.markerView) {
        const marker = await mapView.addMarker(metadata, builder);
        if (this.disposed || epoch !== this.epoch) {
          marker.destroy();
        } else {
          this.markerView = marker;
        }
      } else {
        await this.markerView.refreshMarker(builder);
      }
    } catch (error) {
      LLog.e('LynxUIMapMarker', `Failed to refresh marker: ${error}`);
    } finally {
      // Metadata updates received while busy do not request another bitmap.
      // Apply them even when the snapshot failed, without touching a new handle
      // created after detach. A provider error must not block future refreshes.
      if (!this.disposed && epoch === this.epoch) {
        try {
          this.markerView?.updateMetadata(this.captureMetadata());
        } catch (error) {
          LLog.e('LynxUIMapMarker', `Failed to update marker metadata: ${error}`);
        }
      }
      // Never overlap snapshots, including across detach/reattach. A change
      // received while busy remains pending for one trailing refresh.
      this.refreshing = false;
      if (!this.disposed && this.revision > this.attemptedRevision) {
        this.scheduleRefresh();
      }
    }
  }

  private captureMetadata(): LynxMapMarkerMetadata {
    const metadata = new LynxMapMarkerMetadata();
    metadata.id = `${this.sign}`;
    metadata.position = this.metadata.position;
    metadata.containerPosition = this.metadata.containerPosition;
    metadata.anchor = this.metadata.anchor.slice();
    metadata.enableDrag = this.metadata.enableDrag;
    metadata.enableClick = this.metadata.enableClick;
    metadata.enableCollideWithPoi = this.metadata.enableCollideWithPoi;
    metadata.alpha = this.metadata.alpha;
    metadata.rotation = this.metadata.rotation;
    metadata.zIndex = this.metadata.zIndex;
    metadata.animate = this.metadata.animate;
    return metadata;
  }

  onMarkerClick(): void {
    if (!this.disposed && this.metadata.enableClick) {
      this.sendCustomEvent('clickmarker', new Object(), 'detail');
    }
  }

  onEnterBackground(): void {
    this.foreground = false;
  }

  onEnterForeground(): void {
    this.foreground = true;
    this.refreshMarker();
  }

  invokeMethod(method: string, params: Object, callback: (code: number, res: Object) => void): boolean {
    if (method !== 'refreshMarker') {
      return super.invokeMethod(method, params, callback);
    }
    this.refreshMarker();
    callback(this.disposed ? LynxUIMethodConstants.INVALID_STATE_ERROR : LynxUIMethodConstants.SUCCESS, new Object());
    return true;
  }

  dispose(): void {
    this.disposed = true;
    if (this.refreshTask !== undefined) {
      clearTimeout(this.refreshTask);
      this.refreshTask = undefined;
    }
    this.removeMarkerView();
    this.mapView = undefined;
    this.nativeContent = undefined;
    this.customMarkerBuilder = undefined;
    super.dispose();
  }

  private removeMarkerView(): void {
    // Also invalidate a provider creation that is still awaiting its snapshot.
    const marker = this.markerView;
    this.markerView = undefined;
    ++this.epoch;
    marker?.destroy();
  }
}

@ComponentV2
struct MapMarkerBuilderHost {
  @Param @Require ui: LynxUIMapMarker;

  @Builder
  buildSnapshot() {
    // Let the native root determine the snapshot size. Its layout can change
    // while the provider is awaiting a snapshot, so fixed dimensions go stale.
    Stack({ alignContent: Alignment.TopStart }) {
      if (this.ui.getNativeContent()) {
        ContentSlot(this.ui.getNativeContent()!.content)
      }
    }
  }

  aboutToAppear(): void {
    this.ui.setCustomMarkerBuilder(() => {
      // A provider may retry its snapshot after the native marker was removed.
      if (this.ui.getNativeContent()) {
        this.buildSnapshot();
      }
    });
  }

  build() {
    // Only supplies ArkUI builder context; the native node owns visible content.
  }
}

@Builder
function buildMapMarkerHost(ui: UIBase) {
  MapMarkerBuilderHost({ ui: ui as LynxUIMapMarker })
}
