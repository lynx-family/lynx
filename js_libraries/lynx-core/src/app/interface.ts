// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { Lynx, NativeLynxProxy } from '../lynx';
import { AMDFactory } from '../common';
import { LynxSetTimeout } from '@lynx-js/types';
import { BaseApp } from '.';
import { LynxFeature } from '../common';
import { IdentifierType } from '../modules/selectorQuery';
import { PipelineOptions } from '../modules/performance/performance';
import { NativeModule } from '../modules/nativeModules';
import {
  BaseError,
  LynxErrorLevel,
  sourceMapReleaseObj,
} from '../modules/report';
import { TraceOption } from '@lynx-js/types/types/common/performance';

/**
 * The enum values should be sync with `lynx_env.h`.
 */
export const enum EnvKey {}

export interface LynxKernelInject {
  setSourceMapRelease(error: Error): void;
  __sourcemap__release__?: string;
  define(moduleName: string, factory: AMDFactory): void;
  require(moduleName: string): unknown;
}

export type BundleInitReturnObj = {
  init: (opt: { tt: LynxKernelInject }) => void;
  buildVersion?: string;
};

/**
 * App is the interface that both TTApp and ReactApp should implements
 * it different from AppProxy above that these props is used by js code
 */

export interface App {
  lynx: Lynx;
  __sourcemap__release__?: string;
  nativeApp: NativeApp;
  onError?(message: string): void;
  getDynamicComponentExports(url: string): object;
}

export interface loadCardParams {
  initData?: object;
  updateData?: object;
  entryName?: string;
  initConfig?: object;
  cardType: 'tt' | 'react' | 'standalone';
  appGUID?: string;
  bundleSupportLoadScript?: boolean;
  rlynxVersion?: number;
  processorName?: string;
  cacheData?: Array<TemplateData>;
  isReload?: boolean;
  srcName?: string;
  pageConfigSubset?: Record<string, any>;
}

export interface AppProxyParams<NativeAppProxy> {
  nativeApp: NativeAppProxy;
  params: loadCardParams;
  lynx: NativeLynxProxy;
}

/**
 *
 */
export interface CompileSwitch {
  [key: string]: boolean;
}

/**
 *
 */
export interface lynxProps {
  _switches?: CompileSwitch;
  __globalProps?: object;
  [prop: string]: any;
}

export interface requireParamObj {
  dynamicComponentEntry: string;
  [propName: string]: any;
}

export interface TemplateData {
  data: object;
  processorName: string;
}

export const enum ComponentLifecycleState {
  created = 'created',
  attached = 'attached',
  ready = 'ready',
  detached = 'detached',
  moved = 'moved',
}

export type LifeEvent =
  | ComponentLifecycleState
  | 'propertiesChanged'
  | 'datasetChanged'
  | 'instanceChanged';

/**
 * NativeApp is the interface that native app implements
 * implementations are in `js_app.cc`
 */
export interface NativeApp {
  /**
   * call
   *
   * @param sourceURL
   * @param entryName
   */
  loadScript: (sourceURL: string, entryName?: string) => BundleInitReturnObj;

  loadScriptAsync(
    sourceURL: string,
    callback: (message: string | null, exports: BundleInitReturnObj) => void
  ): void;

  readScript: (
    sourceURL: string,
    params?: { dynamicComponentEntry: string }
  ) => string;

  setCard(app: BaseApp): void;

  setTimeout: LynxSetTimeout;

  setInterval: LynxSetTimeout;

  clearTimeout(timeoutId: number): void;

  clearInterval(intervalId: number): void;

  reportException(
    error: BaseError,
    errorOptions: {
      filename: string;
      release: string;
      slot: string;
      buildVersion: string;
      versionCode: string;
      errorCode?: number;
      errorLevel?: LynxErrorLevel;
    }
  ): void;

  triggerLepusGlobalEvent(eventName: string, params: Record<any, any>): void;

  triggerWorkletFunction(
    componentId: string,
    moduleName: string,
    funcName: string,
    params?: unknown,
    callback?: (res: unknown) => void
  ): void;

  triggerComponentEvent(id: string, params: Record<any, any>): void;

  selectComponent(
    componentId: string,
    idSelector: string,
    single: boolean,
    callback?: () => void
  ): void;

  onPiperInvoked(moduleName: string, methodName: string): void;

  /**
   * Get a string external env.
   *
   * For boolean env, please use {@link BaseApp.prototype.getBoolEnv}
   *
   * @param key The {@link EnvKey}, should be placed in `lynx_env.h`
   */
  getEnv: (key: EnvKey) => string | undefined;

  getPathInfo: (
    type: IdentifierType,
    identifier: string,
    component_id: string,
    first_only: boolean,
    callback: Function,
    root_unique_id?: number
  ) => void;

  getFields: (
    type: IdentifierType,
    identifier: string,
    component_id: string,
    first_only: boolean,
    fields: Array<string>,
    callback: Function,
    root_unique_id?: number
  ) => void;

  setNativeProps: (
    type: IdentifierType,
    identifier: string,
    component_id: string,
    first_only: boolean,
    native_props: object,
    root_unique_id?: number
  ) => void;

  invokeUIMethod: (
    type: IdentifierType,
    identifier: string,
    component_id: string,
    method: string,
    params: object,
    callback: Function,
    root_unique_id?: number
  ) => void;

  callLepusMethod(
    methodName: string,
    args: unknown,
    callback?: () => void,
    groupId?: string,
    stackTraces?: string
  ): void;

  createJSObjectDestructionObserver(cb: () => void): unknown;

  nativeModuleProxy: NativeModule;

  id: string;

  /**
   * Support from Lynx 2.8
   * @param timing_flag
   * @param key
   */
  markTiming: (timing_flag: string, key: string) => void;

  /**
   * Support from Lynx 3.0
   */
  profileStart: (traceName: string, option?: TraceOption) => void;

  /**
   * Support from Lynx 3.0
   */
  profileEnd: () => void;

  /**
   * Support from Lynx 3.0
   */
  profileMark: (traceName: string, option?: TraceOption) => void;

  /**
   * Support from Lynx 3.0
   */
  profileFlowId: () => number;

  /**
   * Support from Lynx 2.18
   */
  isProfileRecording: () => boolean;

  /**
   * Support from Lynx 2.12
   * @param feature
   */
  featureCount: (feature: LynxFeature) => void;

  /**
   * Support from Lynx 2.15
   * Only use for internal.
   *
   */
  pauseGcSuppressionMode: () => void;
  resumeGcSuppressionMode: () => void;

  /**
   * SessionStorage.
   * Support from Lynx 2.16
   */
  setSessionStorageItem: (key: string, value: any) => void;
  getSessionStorageItem: (key: string, callback: (value: any) => void) => any;
  subscribeSessionStorage: (
    key: string,
    listenerId: number,
    callback: (value: any) => void
  ) => any;
  unsubscribeSessionStorage: (key: string, listenerId: number) => void;

  // Timing related
  generatePipelineOptions: () => PipelineOptions;
  onPipelineStart: (pipeline_id: string) => void;
  markPipelineTiming: (pipeline_id: string, timing_key: string) => void;
  bindPipelineIdWithTimingFlag: (
    pipeline_id: string,
    timing_flag: string
  ) => void;

  __SetSourceMapRelease: (baseError: sourceMapReleaseObj) => void;
  __GetSourceMapRelease: (url: string) => string | undefined;

  __pageUrl: string;

  /**
   * @since 3.0
   */
  requestAnimationFrame: (cb?: (timeStamp?: number) => void) => number;

  /**
   * @since 3.0
   */
  cancelAnimationFrame: (requestID?: number) => void;

  /**
   * @since 3.0
   * add custom info for error.
   */
  __addReporterCustomInfo: (info: Record<string, string>) => void;
}
