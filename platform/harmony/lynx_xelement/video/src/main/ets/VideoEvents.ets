// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

export interface LynxVideoFirstFrameEvent {
  duration: number;
}

export interface LynxVideoTimeUpdateEvent {
  current: number;
  duration: number;
}

export interface LynxVideoErrorEvent {
  errorCode: number;
  errorMsg: string;
}

export interface LynxVideoBufferingEvent {
  buffering: number;
}

export function createFirstFrameEvent(duration: number): LynxVideoFirstFrameEvent {
  return {
    duration: normalizeSeconds(duration),
  };
}

export function createTimeUpdateEvent(current: number, duration: number): LynxVideoTimeUpdateEvent {
  return {
    current: normalizeSeconds(current),
    duration: normalizeSeconds(duration),
  };
}

export function createErrorEvent(errorCode: number, errorMsg: string): LynxVideoErrorEvent {
  return {
    errorCode,
    errorMsg,
  };
}

export function createBufferingEvent(buffering: number): LynxVideoBufferingEvent {
  return {
    buffering: normalizeSeconds(buffering),
  };
}

export function shouldEmitEnded(loop: boolean): boolean {
  return !loop;
}

export class LynxVideoTimeUpdateThrottle {
  private lastDispatchTimeMs: number = -1;

  reset(): void {
    this.lastDispatchTimeMs = -1;
  }

  shouldDispatch(nowMs: number, intervalSeconds: number): boolean {
    if (this.lastDispatchTimeMs < 0) {
      this.lastDispatchTimeMs = nowMs;
      return true;
    }
    if (nowMs - this.lastDispatchTimeMs >= Math.max(0, intervalSeconds) * 1000) {
      this.lastDispatchTimeMs = nowMs;
      return true;
    }
    return false;
  }
}

function normalizeSeconds(value: number): number {
  if (Number.isNaN(value) || value < 0) {
    return 0;
  }
  return value;
}
