// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { CommonLynx, GlobalProps } from '../common';
import { AnimationElement } from './animation';
import { BeforePublishEvent, GlobalEventEmitter, IntersectionObserver } from './event';
import { SelectorQuery } from './nodes-ref';
import { Performance } from './performance';
import { Response } from './fetch';

export * from './fetch';

export interface LoadDynamicComponentSuccessResult {
  code: 0;
  detail?: {
    schema: string;
    errMsg: '';
    cache: boolean;
  };
}

export interface LoadDynamicComponentFailedResult {
  // E_DYNAMIC_COMPONENT_LOAD_BAD_RESPONSE = 160102,
  // E_DYNAMIC_COMPONENT_LOAD_EMPTY_FILE = 160102,
  // E_DYNAMIC_COMPONENT_LOAD_DECODE_FAIL = 160103,
  code: 160101 | 160102 | 160103;
  detail?: {
    url: string;
    errMsg: string;
  };
}

export type LoadDynamicComponentFunc = {
  (url: string, options?: Record<string, any>): Promise<LoadDynamicComponentSuccessResult | LoadDynamicComponentFailedResult>;
  (id: string | string[], url: string, options?: Record<string, any>): Promise<LoadDynamicComponentSuccessResult | LoadDynamicComponentFailedResult>;
};

//TODO(liyanbo.monster): migrate component type.
export type CreateIntersectionObserverFunc = (
  component: unknown,
  options?: {
    thresholds?: [];
    initialRatio?: number;
    observeAll?: boolean;
  }
) => IntersectionObserver;

export interface ResourcePrefetchData {
  data: {
    uri: string;
    type: 'image' | 'video';
    param?: {
      priority?: 'high' | 'medium' | 'low';
      cacheTarget?: 'disk' | 'bitmap';
      preloadKey?: string;
      size: number;
    };
  }[];
}

export interface ResourcePrefetchResult {
  code: number;
  msg: string;
  details: {
    code: number;
    msg: string;
    uri: string;
    type: 'image' | 'video';
  }[];
}

export type GetElementByIdFunc = (id: string) => AnimationElement;

/**
 * Lynx provide `background-thread lynx` public api.
 */
export interface Lynx extends CommonLynx {
  /**
   * @description Properties inside `lynx.__globalProps` is NOT managed by Lynx, so you MUST extends this interface yourself
   * @see https://www.typescriptlang.org/docs/handbook/declaration-merging.html
   */
  __globalProps: GlobalProps;
  __presetData: Record<string, unknown>; // readonly data injected by client via LynxBackgroundRuntimeOptions

  performance: Performance;

  beforePublishEvent: BeforePublishEvent;

  getElementById: GetElementByIdFunc;

  // cancelAnimationFrame(animationId: number): void;

  // requestAnimationFrame(callback: () => void): number;

  cancelResourcePrefetch(data: ResourcePrefetchData, callback: (res: ResourcePrefetchResult) => void): void;

  createIntersectionObserver: CreateIntersectionObserverFunc;

  createSelectorQuery(): SelectorQuery;

  getJSModule<Module = unknown>(name: string): Module;

  getJSModule(name: 'GlobalEventEmitter'): GlobalEventEmitter;

  setSharedData<T>(dataKey: string, dataVal: T): void;
  getSharedData<T = unknown>(dataKey: string): T;

  loadDynamicComponent: LoadDynamicComponentFunc;

  registerModule<Module extends object>(name: string, module: Module): void;

  registerSharedDataObserver<T>(callback: (data: T) => void): void;

  removeSharedDataObserver<T>(callback: (data: T) => void): void;

  reload(value: object, callback: () => void): void;

  requestResourcePrefetch(data: ResourcePrefetchData, callback: (res: ResourcePrefetchResult) => void): void;

  requireModuleAsync<T>(path: string, callback: (error: Error | null, exports?: T) => void): void;

  /**
   * @description requireModule 
   * @param path: the source's path
   * @param entryName: the resource's entry's name
   * @param options(@since 3.2): timeout is waiting time,the unit is seconds
   * @since 1.0
   */
  requireModule<T>(path: string, entryName?: string, options?: {timeout: number }): T;

  resumeExposure(): void;
  
  stopExposure(options?: { sendEvent: boolean }): void;

  setObserverFrameRate(options?: { forPageRect?: number; forExposureCheck?: number }): void;

  /**
   * @description subset of Fetch API
   * @see https://developer.mozilla.org/en-US/docs/Web/API/Fetch_API
   * @since 2.18
   */
  fetch(input: RequestInfo, init?: RequestInit): Promise<Response>;

  /**
   * @description queue microtask
   * @see https://developer.mozilla.org/en-US/docs/Web/API/Window/queueMicrotask
   * @since 3.1
   */ 
  queueMicrotask(callback: () => void): void;

}
