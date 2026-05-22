// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

export enum LynxVideoObjectFit {
  Contain = 'contain',
  Cover = 'cover',
  Fill = 'fill',
}

export enum LynxVideoMethodMode {
  Queue = 'queue',
  Direct = 'direct',
  Latest = 'latest',
}

export interface LynxVideoAttributes {
  src: string;
  loop: boolean;
  volume: number;
  muted: boolean;
  speed: number;
  objectFit: LynxVideoObjectFit;
  mode: LynxVideoMethodMode;
  timeUpdateInterval: number;
}

export interface LynxVideoAttributeUpdate {
  attributes: LynxVideoAttributes;
  srcChanged: boolean;
}

const DEFAULT_VOLUME = 1.0;
const DEFAULT_SPEED = 1.0;
const DEFAULT_TIME_UPDATE_INTERVAL = 0.33;
const MIN_SPEED = 0.1;
const MAX_SPEED = 2.0;

export function createDefaultLynxVideoAttributes(): LynxVideoAttributes {
  return {
    src: '',
    loop: false,
    volume: DEFAULT_VOLUME,
    muted: false,
    speed: DEFAULT_SPEED,
    objectFit: LynxVideoObjectFit.Contain,
    mode: LynxVideoMethodMode.Queue,
    timeUpdateInterval: DEFAULT_TIME_UPDATE_INTERVAL,
  };
}

export function cloneLynxVideoAttributes(attributes: LynxVideoAttributes): LynxVideoAttributes {
  return {
    src: attributes.src,
    loop: attributes.loop,
    volume: attributes.volume,
    muted: attributes.muted,
    speed: attributes.speed,
    objectFit: attributes.objectFit,
    mode: attributes.mode,
    timeUpdateInterval: attributes.timeUpdateInterval,
  };
}

export function updateLynxVideoAttributes(
  current: LynxVideoAttributes,
  props: Record<string, Object>
): LynxVideoAttributeUpdate {
  const next = cloneLynxVideoAttributes(current);
  let srcChanged = false;

  if (props.src !== undefined) {
    const src = toStringValue(props.src, '');
    srcChanged = src !== next.src;
    next.src = src;
  }
  if (props.loop !== undefined) {
    next.loop = toBooleanValue(props.loop, next.loop);
  }
  if (props.volume !== undefined) {
    next.volume = clamp(toNumberValue(props.volume, next.volume), 0.0, 1.0);
  }
  if (props.muted !== undefined) {
    next.muted = toBooleanValue(props.muted, next.muted);
  }
  if (props.speed !== undefined) {
    next.speed = clamp(toNumberValue(props.speed, next.speed), MIN_SPEED, MAX_SPEED);
  }
  if (props['object-fit'] !== undefined) {
    next.objectFit = toObjectFit(props['object-fit'], next.objectFit);
  }
  if (props.mode !== undefined) {
    next.mode = toMethodMode(props.mode, next.mode);
  }
  if (props['timeupdate-interval'] !== undefined) {
    const interval = toNumberValue(props['timeupdate-interval'], next.timeUpdateInterval);
    if (interval > 0) {
      next.timeUpdateInterval = interval;
    }
  }

  return {
    attributes: next,
    srcChanged,
  };
}

export function isOnlineVideoSrc(src: string): boolean {
  return src.startsWith('https://') || src.startsWith('http://');
}

export function shouldMuteVideo(attributes: LynxVideoAttributes): boolean {
  // ArkUI Video has no per-instance volume control; volume=0 degrades to muted.
  return attributes.muted || attributes.volume <= 0;
}

export function toStringValue(value: Object, fallback: string): string {
  if (typeof value === 'string') {
    return value as string;
  }
  return fallback;
}

export function toBooleanValue(value: Object, fallback: boolean): boolean {
  if (typeof value === 'boolean') {
    return value as boolean;
  }
  if (typeof value === 'string') {
    const stringValue = (value as string).toLowerCase();
    if (stringValue === 'true') {
      return true;
    }
    if (stringValue === 'false') {
      return false;
    }
  }
  return fallback;
}

export function toNumberValue(value: Object, fallback: number): number {
  if (typeof value === 'number') {
    return value as number;
  }
  if (typeof value === 'string') {
    const parsed = Number.parseFloat(value as string);
    if (!Number.isNaN(parsed)) {
      return parsed;
    }
  }
  return fallback;
}

export function toObjectFit(value: Object, fallback: LynxVideoObjectFit): LynxVideoObjectFit {
  if (typeof value !== 'string') {
    return fallback;
  }
  switch (value as string) {
    case LynxVideoObjectFit.Cover:
      return LynxVideoObjectFit.Cover;
    case LynxVideoObjectFit.Fill:
      return LynxVideoObjectFit.Fill;
    case LynxVideoObjectFit.Contain:
      return LynxVideoObjectFit.Contain;
    default:
      return fallback;
  }
}

export function toMethodMode(value: Object, fallback: LynxVideoMethodMode): LynxVideoMethodMode {
  if (typeof value !== 'string') {
    return fallback;
  }
  switch (value as string) {
    case LynxVideoMethodMode.Direct:
      return LynxVideoMethodMode.Direct;
    case LynxVideoMethodMode.Latest:
      return LynxVideoMethodMode.Latest;
    case LynxVideoMethodMode.Queue:
      return LynxVideoMethodMode.Queue;
    default:
      return fallback;
  }
}

function clamp(value: number, min: number, max: number): number {
  return Math.min(Math.max(value, min), max);
}
