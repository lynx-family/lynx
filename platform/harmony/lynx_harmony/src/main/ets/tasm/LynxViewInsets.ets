// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

export enum LynxInsetType {
  STATUS_BARS = 'statusBars', NAVIGATION_BARS = 'navigationBars', CAPTION_BAR = 'captionBar',
  SYSTEM_BARS = 'systemBars', SYSTEM_OVERLAYS = 'systemOverlays', IME = 'ime',
  DISPLAY_CUTOUT = 'displayCutout', WATERFALL = 'waterfall', ROUNDED_CORNERS = 'roundedCorners',
  SYSTEM_GESTURES = 'systemGestures', MANDATORY_SYSTEM_GESTURES = 'mandatorySystemGestures',
  TAPPABLE_ELEMENT = 'tappableElement'
}

export enum LynxInsetGeometryType { EDGES = 'edges', RECTS = 'rects', CORNERS = 'corners' }

export interface LynxInsetEdges {
  readonly type: LynxInsetGeometryType.EDGES;
  readonly left: number;
  readonly top: number;
  readonly right: number;
  readonly bottom: number;
}

export interface LynxInsetRect {
  readonly x: number;
  readonly y: number;
  readonly width: number;
  readonly height: number;
}

export interface LynxInsetRects {
  readonly type: LynxInsetGeometryType.RECTS;
  readonly rects: LynxInsetRect[];
}

export interface LynxInsetCorner {
  readonly centerX: number;
  readonly centerY: number;
  readonly radius: number;
}

export interface LynxInsetCorners {
  readonly type: LynxInsetGeometryType.CORNERS;
  readonly topLeft?: LynxInsetCorner;
  readonly topRight?: LynxInsetCorner;
  readonly bottomRight?: LynxInsetCorner;
  readonly bottomLeft?: LynxInsetCorner;
}

export type LynxInsetGeometry = LynxInsetEdges | LynxInsetRects | LynxInsetCorners;

export interface LynxInsetState {
  readonly geometry: LynxInsetGeometry;
  readonly visible?: boolean;
}

const TYPES: LynxInsetType[] = [LynxInsetType.STATUS_BARS, LynxInsetType.NAVIGATION_BARS,
  LynxInsetType.CAPTION_BAR, LynxInsetType.SYSTEM_BARS, LynxInsetType.SYSTEM_OVERLAYS,
  LynxInsetType.IME, LynxInsetType.DISPLAY_CUTOUT, LynxInsetType.WATERFALL,
  LynxInsetType.ROUNDED_CORNERS, LynxInsetType.SYSTEM_GESTURES,
  LynxInsetType.MANDATORY_SYSTEM_GESTURES, LynxInsetType.TAPPABLE_ELEMENT];

function coordinate(value: number): number {
  if (typeof value !== 'number' || !Number.isFinite(value)) {
    throw new Error('Inset coordinates must be finite');
  }
  return value;
}

function length(value: number): number {
  if (coordinate(value) < 0) throw new Error('Inset lengths must be nonnegative');
  return value;
}

function copyEdges(value: LynxInsetEdges): LynxInsetEdges {
  if (value.type !== LynxInsetGeometryType.EDGES) throw new Error('Expected edge geometry');
  return { type: LynxInsetGeometryType.EDGES, left: length(value.left), top: length(value.top),
    right: length(value.right), bottom: length(value.bottom) };
}

function copyCorner(value?: LynxInsetCorner): LynxInsetCorner | undefined {
  if (value === undefined) return undefined;
  return { centerX: coordinate(value.centerX), centerY: coordinate(value.centerY),
    radius: length(value.radius) };
}

function copyState(type: LynxInsetType, state: LynxInsetState): LynxInsetState {
  const value = state.geometry;
  let geometry: LynxInsetGeometry;
  if (value.type === LynxInsetGeometryType.EDGES) {
    if (type === LynxInsetType.ROUNDED_CORNERS) throw new Error('Expected corner geometry');
    geometry = copyEdges(value as LynxInsetEdges);
  } else if (value.type === LynxInsetGeometryType.RECTS) {
    if (type !== LynxInsetType.DISPLAY_CUTOUT) throw new Error('Rectangles require displayCutout');
    const rects = (value as LynxInsetRects).rects;
    if (!Array.isArray(rects)) throw new Error('Expected rectangle list');
    geometry = { type: LynxInsetGeometryType.RECTS, rects: rects.map((rect: LynxInsetRect): LynxInsetRect => {
      return { x: coordinate(rect.x), y: coordinate(rect.y),
        width: length(rect.width), height: length(rect.height) };
    }) };
  } else if (value.type === LynxInsetGeometryType.CORNERS) {
    if (type !== LynxInsetType.ROUNDED_CORNERS) throw new Error('Corners require roundedCorners');
    const corners = value as LynxInsetCorners;
    geometry = { type: LynxInsetGeometryType.CORNERS, topLeft: copyCorner(corners.topLeft),
      topRight: copyCorner(corners.topRight), bottomRight: copyCorner(corners.bottomRight),
      bottomLeft: copyCorner(corners.bottomLeft) };
  } else {
    throw new Error('Unsupported inset geometry');
  }
  if (state.visible !== undefined && typeof state.visible !== 'boolean') {
    throw new Error('Inset visibility must be boolean');
  }
  return { geometry: geometry, visible: state.visible };
}

/** Category update in screen coordinates, in vp. Missing/null categories do not update. */
export class LynxViewInsets {
  private readonly categories: Map<LynxInsetType, LynxInsetState>;

  constructor(categories: Map<LynxInsetType, LynxInsetState | null> = new Map()) {
    this.categories = new Map();
    categories.forEach((state: LynxInsetState | null, type: LynxInsetType): void => {
      if (!TYPES.includes(type)) throw new Error('Unsupported inset category');
      if (state !== null && state !== undefined) this.categories.set(type, copyState(type, state));
    });
  }

  isEmpty(): boolean { return this.categories.size === 0; }

  /** Returns a detached category value. */
  get(type: LynxInsetType): LynxInsetState | undefined {
    const value = this.categories.get(type);
    return value === undefined ? undefined : copyState(type, value);
  }

  merge(update: LynxViewInsets): LynxViewInsets {
    const next: Map<LynxInsetType, LynxInsetState | null> = new Map();
    this.categories.forEach((state: LynxInsetState, type: LynxInsetType): void => { next.set(type, state); });
    update.categories.forEach((state: LynxInsetState, type: LynxInsetType): void => { next.set(type, state); });
    return new LynxViewInsets(next);
  }

  copy(): LynxViewInsets { return new LynxViewInsets().merge(this); }

  /** @internal Serializes a detached complete snapshot for native transport. */
  toNative(): Record<string, Object> {
    const result: Record<string, Object> = {};
    this.categories.forEach((state: LynxInsetState, type: LynxInsetType): void => {
      const copied = copyState(type, state);
      const value: Record<string, Object> = {};
      if (copied.geometry.type === LynxInsetGeometryType.CORNERS) {
        const corners = copied.geometry as LynxInsetCorners;
        const geometry: Record<string, Object> = {};
        geometry['type'] = LynxInsetGeometryType.CORNERS;
        if (corners.topLeft !== undefined) geometry['topLeft'] = corners.topLeft;
        if (corners.topRight !== undefined) geometry['topRight'] = corners.topRight;
        if (corners.bottomRight !== undefined) geometry['bottomRight'] = corners.bottomRight;
        if (corners.bottomLeft !== undefined) geometry['bottomLeft'] = corners.bottomLeft;
        value['geometry'] = geometry;
      } else {
        value['geometry'] = copied.geometry;
      }
      if (copied.visible !== undefined) value['visible'] = copied.visible;
      result[type] = value;
    });
    return result;
  }
}
