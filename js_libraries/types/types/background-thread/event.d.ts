// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

export interface EventEmitter {
  addListener(eventName: string, listener: (...args: unknown[]) => void, context?: object): void;

  removeListener(eventName: string, listener: (...args: unknown[]) => void): void;

  emit(eventName: string, data: unknown): void;

  removeAllListeners(eventName?: string): void;

  trigger(eventName: string, params: string | Record<any, any>): void;

  toggle(eventName: string, ...data: unknown[]): void;
}

export type GlobalEventEmitter = EventEmitter;

export interface BeforePublishEvent extends EventEmitter {
  add(eventName: string, callback: (...args: unknown[]) => void, context?: object): BeforePublishEvent;

  remove(eventName: string, callback: (...args: unknown[]) => void): BeforePublishEvent;
}

export interface RelativeToMargins {
  bottom?: number;
  left?: number;
  right?: number;
  top?: number;
}

export interface BoundingClientRectResult {
  bottom: number;
  height: number;
  left: number;
  right: number;
  top: number;
  width: number;
}

export interface IntersectionRectResult {
  bottom: number;
  height: number;
  left: number;
  right: number;
  top: number;
  width: number;
}

export interface RelativeRectResult {
  bottom: number;
  left: number;
  right: number;
  top: number;
}

export interface RectResult {  
  top: number;
  right: number;
  bottom: number;
  left: number;
}

export interface ObserveCallbackResult {
  isIntersecting: boolean;
  intersectionRatio: number;
  intersectionRect: RectResult;
  boundingClientRect: RectResult;
  relativeRect: RectResult;
  observerId: string;
  time: number;
}

export type ObserveCallback = (result: ObserveCallbackResult) => void;

export interface RelativeToViewportMargins {
  top?: number;
  right?: number;
  bottom?: number;
  left?: number;
}

export interface IntersectionObserver {
  relativeTo(selector: string, margins?: RelativeToMargins): IntersectionObserver;

  relativeToViewport(margins?: RelativeToViewportMargins): IntersectionObserver;

  relativeToScreen(margins?: RelativeToViewportMargins): IntersectionObserver;

  observe(selector: string, callback: ObserveCallback): void;

  disconnect(): void;

  invokeCallback(callbackId: number, data: Record<string, any>): void;
}
