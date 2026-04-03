// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import type { DrawContext, FrameNode, UIContext } from '@kit.ArkUI';
import type { AsyncCallback } from '@ohos.base';
import type { NodeContent } from '@ohos.arkui.node';

export const initGlobalEnv: (resourceManager: Object) => void;

export const registerImageService: (instance: number[]) => void;

export const getBaseTraceBackend: () => number;

export const setTracingDirPath: (tracingDirPath: string) => void;

export const setCacheDirPath: (cacheDirPath: string) => void;

export const traceEventBegin: (traceCategory: string, name: string, args?: Record<string, string>) => void;

export const traceEventEnd: (traceCategory: string, name: string) => void;

export const traceInstant: (traceCategory: string, name: string, args?: Record<string, string>) => void;

export const parserTestBenchRecordData: (source: string) => string;

export const notifyMemoryPressure: (pressure: number) => void;

export class PaintingContext {
  static create(ref: Object, create: Function, update: Function, insert: Function, remove: Function, destroy: Function,
    layout: Function, flush: Function, layoutFinish: Function, updateContentSizeAndOffset: Function,
    updateScrollInfo: Function, removeListItemNode: Function, updateExtraData: Function, invokeUIMethod: Function,
    updateNodeReadyPatching: Function): number[];
}

export class ShadowNodeOwner {
  constructor(ref: Object, create: Function);

  findJSShadowNodeBySign(sign: number): Object | undefined;

  /**
   * Calls the layout node's measure function based on the provided parameters.
   *
   * @param {number} sign - The unique signature of the target node.
   * @param {number} width - The expected width of the node.
   * @param {number} widthMode - The mode for measuring the width. Possible values: 0 (SLMeasureModeIndefinite), 1 (SLMeasureModeDefinite), 2 (SLMeasureModeAtMost).
   * @param {number} height - The expected height of the node.
   * @param {number} heightMode - The mode for measuring the height. Possible values: 0 (SLMeasureModeIndefinite), 1 (SLMeasureModeDefinite), 2 (SLMeasureModeAtMost).
   * @param {boolean} finalMeasure - Indicates whether it is the final measurement.
   * @returns {ArrayBuffer} - An ArrayBuffer containing three float values: width, height, and baseline.
   */
  measureNativeNode(sign: number, width: number, widthMode: number, height: number, heightMode: number,
    finalMeasure: boolean): ArrayBuffer;

  /**
   * Calls the layout node's align function based on the provided parameters.
   * @param {number} sign - The unique signature of the target node.
   * @param {number} top - The top of the node
   * @param {number} left - The left of the node
   */
  alignNativeNode(sign: number, top: number, left: number): void;

  /**
   * Prefetch font resource.
   * @param {string} uri - The uri of the font resource.
   */
  prefetchFont(uri: string, callback?: (code: number, msg: string) => void): void;

  /**
   * Destroy the native instance immediately.
   */
  destroy(): void;
}

export class EmbedderPlatform {
  constructor(embedder: EmbedderEngine, paintingContext: UIOwner,
    shadowNodeOwner: ShadowNodeOwner);

  static updateRefreshRate(refreshRate: number): void;

  getUIDelegate(): number[];

  destroy(): void;
}

export interface DevtoolResult {
  result: ArrayBuffer,
  width: number,
  height: number,
}


export class LynxWhiteBoard {
  constructor();
}

export class LynxTemplateRenderer {
  constructor();

  nativeAttach(density: number): void;

  nativeReset(ui_delegate: number[], providers: (Object | undefined)[], width: number, height: number,
    isHostRenderer: boolean, perfController: PerformanceCollector, threadMode: number, groupId: string,
    useQuickjs: boolean, enableJSGroupThread: boolean, preloadJSPaths: string[], enableBytecode: boolean,
    bytecodeSourceUrl: string, enableJSRuntime: boolean, moduleManagerArgs: Object[],
    sendableModuleManagerArgs: Object[], backgroundRuntime: NativeLynxBackgroundRuntime,
    whiteBoard?: LynxWhiteBoard): void;

  nativeDetach(): void;

  loadTemplate(url: string, buffer: ArrayBuffer, processor?: string, templateData?: Object, readOnly?: boolean,
    enableRecycleTemplateBundle?: boolean, timingOption?: Object): void;

  reloadTemplate(processor?: string, data?: Object | string, readonly?: boolean, props?: Object | string,
    timingOption?: Object): void;

  loadTemplateBundle(url: string, bundle: TemplateBundle, processor?: string, templateData?: Object, readOnly?: boolean,
    enableDumpElementTree?: boolean, timingOption?: Object)

  updateViewport(width: number, widthMode: number, height: number, heightMode: number): void;

  updateScreenMetrics(width: number, height: number, scale: number): void;

  updateGlobalProps(props?: Object | string): void;

  updateMetaData(processor?: string, data?: Object | string, readonly?: boolean, props?: Object | string,
    updateMode?: number): void;

  callJSFunction(module: string, method: string, params: Array<Object>): void;

  triggerEventBus(name: string, params: Array<Object>): void;

  callJSApiCallbackWithValue(callbackId: number, params: Object): void;

  callJSIntersectionObserver(observerId: number, callbackId: number, params: Array<Object>): void;

  evaluateScript(url: string, script: string, callbackId: number): void;

  rejectDynamicComponentLoad(url: string, callbackId: string, errorCode: number, errorMsg: string): void;

  sendTouchEvent(name: string, id: number, x: number, y: number, clientX: number, clientY: number, pageX: number,
    pageY: number): void;

  sendMultiTouchEvent(name: string, params: Object): void;

  sendCustomEvent(name: string, id: number, params: Object, paramName: string): void;

  scrollByListContainer(containerSign: number, x: number, y: number): void;

  scrollToPosition(containerSign: number, position: number, offset: number, align: number, smooth: boolean): void;

  scrollStopped(containerSign: number): void;

  takeScreenShot(callback: AsyncCallback<DevtoolResult>, quality: number, maxWidth: number, maxHeight: number): void;

  getAllTimingInfo(): object;

  getInstanceId(): number;

  updateFontScale(scale: number): void;

  nativeSetEnableBytecode(enableBytecode: boolean, sourceUrl: string): void;

  getPageDataByKey(keys: string[]): Object;

  setupExtensionDelegate(ptr: number[]): void;

  onEnterForeground(): void;

  onEnterBackground(): void;

  callJSFunction(moduleName: string, method: string, params: Object[]): void;

  nativeSetPlatformConfig(config: string): void;

  nativeGetAllJsSource(): Record<string, string>;

  nativeSetSessionStorageItem(key: string, data?: Object | string): void;

  nativeGetSessionStorageItem(key: string, callback: Function): void;

  nativeSubscribeSessionStorage(key: string, callback: Function): number;

  nativeUnsubscribeSessionStorage(key: string, listenerId: number): void;

  invokeLepusCallback(id: number, entryName: string, args: Object): void;
}

export type JSMeasureFunc = (width: number, widthMode: number, height: number,
  heightMode: number) => [number, number, number];

export type JSAlignLayoutFunc = () => void;

export class ShadowNode {
  constructor(ref: Object, sign: number, tag: string);

  setMeasureFunc(measureFunc: JSMeasureFunc | null, alignFunc: JSAlignLayoutFunc | null): void;

  markDirty(): void;

  requestLayout(): void;

  /**
   * Retrieves the children of the current node.
   *
   * @returns {ArrayBuffer} - An ArrayBuffer containing the integer signature of each child.
   */
  getChildren(): ArrayBuffer;

  setExtraDataFunc(func: () => Object): void;
}

export class UIOwner {
  constructor(ref: Object, create: Function, uiContext: UIContext, createNodeContent: Function,
    startFluencyTrace: Function, stopFluencyTrace: Function, getNodeType: (string) => number,
    postDrawEndTimingFrameCallback: Function, onAvoidKeyboardCallback: Function, onResourceLoaded: Function);

  attachPageRoot(content: NativeContent): void;

  getId(): string;

  destroy(id: string): void;

  requestLayout(): void;

  keyboardStatusChanged(height: number): void;

  canConsumeTouchEvent(x: number, y: number): boolean;

  updateRootTarget(node: NativeContent): void;

  setLynxImageConfig(config?: Object): void;

  onEnterForeground(): void;

  onEnterBackground(): void;
}

export class UIBase {
  constructor(ref: Object, context: number[], node: FrameNode | null, sign: number, tag: string, update: Function,
    layout: Function, invokeUIMethod: Function, dispose: Function, focusChange: Function, focusable: Function,
    onNodeReady: Function,
    customLayout: boolean, updateExtraData: Function,
    needWindowStateChangeEvent: boolean, onEnterForeground: Function, onEnterBackground: Function);

  static getUIFromNativeContent(nativeContent: NativeContent): Object | undefined;

  static getContentSize(nativeContent: NativeContent): [number, number];

  /**
   * Retrieves the children of the current node.
   *
   * @returns {ArrayBuffer} - An ArrayBuffer containing the integer signature of each child.
   */
  getChildren(): ArrayBuffer;

  setFrameNode(node: FrameNode): void;

  sendGlobalEvent(name: string, params: Object[]): void;

  sendCustomEvent(name: string, params: Object, paramName: string): void;

  findJSShadowNodeBySign(sign: number): Object | undefined;

  findShadowNodeBySign(sign: number): number[] | undefined;

  setFocusedUI(): void;

  unsetFocusedUI(): void;

  setChildrenManagementFuncs(insertChild: Function, removeChild: Function): void;

  attachGestureToNode(node: NativeContent): void;
}

export class NativeContent {
  readonly content: NodeContent;

  constructor(content: NodeContent);
}

export class EventReporter {
  static registerJSMethods(ref: Object, onEventFunc: Function, updateGenericInfoFunc: Function,
    clearCacheFunc: Function): void;
}

export class PerformanceController {
  constructor(ref: Object, onPerformanceEventFunc: Function);

  destroy();

  setTiming(timestamp: number, timingKey: string, pipelineID: string): void;

  markTiming(timingKey: string, pipelineID: string): void;
}

export class LynxTrailHubImpl {
  static setTrailMap(trailMap: Record<string, string>): void;
}

export class TraceControllerHarmony {
  static startTracing(traceConfig: object = {}): number;

  static stopTracing(sessionId: number): boolean;
}

export class TemplateBundle {
  constructor();

  nativeParseTemplate(template: ArrayBuffer): string | undefined;

  nativeAsyncParseTemplate(template: ArrayBuffer): Promise<string> | undefined;

  nativeGetExtraInfo(): Record<string, Object>;

  nativeGetContainsElementTree(): boolean;

  nativeInitWithOption(contextPoolSize: number, enableContextAutoRefill: boolean): void;

  nativePostJsCacheGenerationTask(bytecodeSourceUrl: string, useV8: boolean): void;
}


export class LynxInfoReporterHelper {
  static nativeRegister(infoReporter: Object): void;
}

export class ExtensionModule {
  constructor(uiDelegate: number[])

  nativeSetup(): void;

  nativeOnTemplateLoad(url : stirng) : void;

  nativeGetExtensionDelegatePtr(): number[];

  nativeDestroy(): void;

  nativeRegisterService(type: string, service: Object): void;
}

export class LynxRuntimeWrapper {
  constructor();

  protected nativeCreate(providers: (Object | undefined)[], groupId: string,
    useQuickjs: boolean, enableJSGroupThread: boolean, preloadJSPaths: string[], enableBytecode: boolean,
    bytecodeSourceUrl: string, moduleManagerArgs: Object[],
    sendableModuleManagerArgs: Object[], processor?: string, data?: Object | string, readonly?: boolean,
    props?: Object | string): void;

  protected nativeEvaluateScript(url: string, sources: string): void;

  protected nativeEvaluateBundleScript(url: string, bundle: TemplateBundle, jsFile: string): void;

  protected nativeTransitionToFullRuntime(): void;

  protected nativeCallJSFunction(module: string, method: string, params: Object[]): void;

  protected nativeAddRuntimeLifecycleListener(listener: Object);

  protected nativeSetSessionStorageItem(key: string, data?: Object | string): void;

  protected nativeGetSessionStorageItem(key: string, callback: Function): void;

  protected nativeSubscribeSessionStorage(key: string, callback: Function): number;

  protected nativeUnsubscribeSessionStorage(key: string, listenerId: number): void;
}

export const registerStaticTask: (
  taskId: string,
  callback: (nativeContextPtr: bigint) => void,
) => boolean;

export const unregisterStaticTask: (taskId: string) => boolean;

export { RendererFunctionNG, FiberEventListenerOption, CommonCallbackNG } from './HarmonyRendererFunctionNG';
