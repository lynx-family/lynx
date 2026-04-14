// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { LynxTemplateRenderer } from './LynxTemplateRenderer';
import { LynxUpdateMeta, LynxUpdateMode, MetaData, TemplateData } from './MetaData';
import { LynxScreenMetrics } from './behavior/LynxScreenMetrics';
import { ShadowNodeOwner } from './behavior/ShadowNodeOwner';
import { UIOwner } from './behavior/UIOwner';
import { TraceCategory, TraceEvent } from '../base/TraceEvent';
import { TraceEventDef } from '../base/TraceEventDef';
import { LynxExtraTiming } from './performance/timing/LynxExtraTiming';
import { LynxView } from './LynxView';
import { LynxReportEventPropValue } from './eventreport/ILynxEventReportObserver';
import { LynxEventReporter } from './eventreport/LynxEventReporter';
import {
  LLog,
  LynxEnv,
  LynxGenericResourceFetcher,
  LynxMediaResourceFetcher,
  LynxResourceLoadInfo,
  LynxViewClient,
  TemplateBundle
} from '../Index';
import { componentUtils } from '@kit.ArkUI';
import { IContext } from './IContext';
import { LynxExtensionModule } from '../extension/LynxExtensionModule';
import { TimingOption } from './performance/timing/TimingOption';
import { TimingConstants } from './performance/timing/TimingConstants';
import { LynxBaseInspectorOwner } from '../devtool_wrapper/LynxBaseInspectorOwner'
import { NativeContent } from '../Index';
import { LynxImageConfig } from './image/LynxImageConfig';
import { SessionStorageCallback } from './SessionStorage';

const TAG = 'LynxContext'
const INVALID_SESSION_STORAGE_LISTENER_ID = -1;

/**
 * LynxContext is a LynxView's core context, managing templates, data, and events. It connects the
 * native platform to the Lynx engine, acting like a WebView for rendering templates.
 * @apidoc
 */
export class LynxContext implements IContext {
  private renderer?: LynxTemplateRenderer;
  private shadowNodeOwner?: ShadowNodeOwner;
  private genericResourceFetcher?: LynxGenericResourceFetcher;
  private mediaResourceFetcher?: LynxMediaResourceFetcher;
  private uiOwner?: UIOwner;
  private lynxView?: LynxView;
  private contextData?: Record<string, Object>;
  private extensionModule?: LynxExtensionModule;

  constructor() {
    this._screenMetrics = new LynxScreenMetrics();
  }

  private _screenMetrics: LynxScreenMetrics;

  public get screenMetrics(): LynxScreenMetrics {
    return this._screenMetrics;
  }

  findJSShadowNodeBySign(sign: number): Object | undefined {
    return this.shadowNodeOwner?.findJSShadowNodeBySign(sign);
  }

  public prefetchFont(uri: string, callback?: (code: number, msg: string) => void) {
    this.shadowNodeOwner?.prefetchFont(uri, callback);
  }

  public setShadowNodeOwner(nodeOwner?: ShadowNodeOwner) {
    this.shadowNodeOwner = nodeOwner;
  }

  public setUIOwner(uiOwner?: UIOwner) {
    this.uiOwner = uiOwner;
  }

  public measureNativeNode(sign: number, width: number, widthMode: number, height: number, heightMode: number,
    finalMeasure: boolean) {
    TraceEvent.beginSection(TraceCategory.Lynx, TraceEventDef.CONTEXT_MEASURE_NATIVE);
    const measureResult =
      this.shadowNodeOwner?.measureNativeNode(sign, width, widthMode, height, heightMode, finalMeasure);
    TraceEvent.endSection(TraceCategory.Lynx, TraceEventDef.CONTEXT_MEASURE_NATIVE);
    return measureResult;
  }

  public alignNativeNode(sign: number, left: number, top: number) {
    TraceEvent.beginSection(TraceCategory.Lynx, TraceEventDef.CONTEXT_ALIGN_NATIVE);
    this.shadowNodeOwner?.alignNativeNode(sign, left, top);
    TraceEvent.endSection(TraceCategory.Lynx, TraceEventDef.CONTEXT_ALIGN_NATIVE);
  }

  setRenderer(renderer?: LynxTemplateRenderer) {
    this.renderer = renderer;
  }

  setLynxView(lynxView?: LynxView) {
    this.lynxView = lynxView;
  }

  /**
   * Send global events to the front end through the client,
   * and the front end can listen to the event through `GlobalEventEmitter`.
   * @apidoc
   * @param name Global event name.
   * @param params Parameters carried by global events.
   */
  sendGlobalEvent(name: string, params: Object[]) {
    this.lynxView?.sendGlobalEvent(name, params);
  }

  public callJSFunction(moduleName: string, method: string, params: Object[]) {
    this.renderer?.callJSFunction(moduleName, method, params);
  }

  /**
   * Set a value to session storage.
   * @apidoc
   * @param key Session storage key.
   * @param value Session storage value wrapped by TemplateData.
   */
  public setSessionStorageItem(key: string, value: TemplateData): void {
    if (!key || !value) {
      return;
    }
    this.renderer?.setSessionStorageItem(key, value);
  }

  /**
   * Get a value from session storage asynchronously.
   * @apidoc
   * @param key Session storage key.
   * @param callback Callback invoked with the current value.
   */
  public getSessionStorageItem(key: string, callback: SessionStorageCallback): void {
    if (!key || !callback) {
      return;
    }
    this.renderer?.getSessionStorageItem(key, callback);
  }

  /**
   * Subscribe to session storage updates for the specified key.
   * @apidoc
   * @param key Session storage key.
   * @param callback Callback invoked when the key value changes.
   * @returns Listener id for unsubscribe, returns -1 if subscribe fails.
   */
  public subscribeSessionStorage(key: string, callback: SessionStorageCallback): number {
    if (!key || !callback || !this.renderer) {
      return INVALID_SESSION_STORAGE_LISTENER_ID;
    }
    return this.renderer.subscribeSessionStorage(key, callback);
  }

  /**
   * Unsubscribe a session storage listener by key and listener id.
   * @apidoc
   * @param key Session storage key.
   * @param listenerId Listener id returned by subscribeSessionStorage.
   */
  public unsubscribeSessionStorage(key: string, listenerId: number): void {
    if (!key || listenerId < 0) {
      return;
    }
    this.renderer?.unsubscribeSessionStorage(key, listenerId);
  }

  sendCustomEvent(name: string, id: number, params: Object, paramName?: string) {
    this.renderer?.sendCustomEvent(name, id, params, paramName ? paramName : 'params');
  }

  /**
   * Using [LynxUpdateMeta](/api/lynx-native-api/lynx-update-meta) to update `LynxView`,
   * it is the main entrance for the client to update template data.
   * @apidoc
   * @param data Lynx template update metadata, developers use this data structure to specify the optional data content of the update template.
   */
  updateMetaData(data: LynxUpdateMeta): void;

  updateMetaData(data: MetaData): void;

  updateMetaData(data: LynxUpdateMeta | MetaData): void {
    const updateMode = (data as LynxUpdateMeta).updateMode ?? LynxUpdateMode.UPDATE;
    if (this.lynxView?.templateLoaded) {
      if (updateMode === LynxUpdateMode.RELOAD) {
        this.reload(data);
        return;
      }

      this.renderer?.updateMetaData(data.templateData?.processor, data.templateData?.data, data.templateData?.readOnly,
        data.globalProps?.data, updateMode);
    } else {
      this.lynxView?.metaData.merge(data);
    }
  }

  scrollByListContainer(containerSign: number, x: number, y: number) {
    this.renderer?.scrollByListContainer(containerSign, x, y);
  }

  scrollToPosition(containerSign: number, position: number, offset: number, align: number, smooth: boolean) {
    this.renderer?.scrollToPosition(containerSign, position, offset, align, smooth);
  }

  scrollStopped(containerSign: number) {
    this.renderer?.scrollStopped(containerSign);
  }

  /**
   * The client updates the `LynxView` view size.
   * @apidoc
   * @param width Current `LynxView` width.
   * @param widthMode The measure mode for width. 0: Indefinite, 1: Definite, 2: AtMost.
   * @param height Current `LynxView` height.
   * @param heightMode The measure mode for height. 0: Indefinite, 1: Definite, 2: AtMost.
   */
  public updateViewport(width: number, widthMode: number, height: number, heightMode: number) {
    this.renderer?.updateViewport(width, widthMode, height, heightMode);
  }

  public onEnterForeground() {
    LLog.i(TAG, 'onEnterForeground, url: ' + this.lynxView?.url);
    this.lynxView?.onEnterForeground();
  }

  public onEnterBackground() {
    LLog.i(TAG, 'onEnterBackground, url: ' + this.lynxView?.url);
    this.lynxView?.onEnterBackground();
  }

  public updateScreenMetrics(width: number, height: number) {
    this.renderer?.updateScreenMetrics(width, height);
  }

  updateCustomData(data: Record<string, Object>): void {
    Object.entries(data).forEach((value) => {
      let behavior = this.uiOwner?.behaviors.get(value[0]);
      if (behavior) {
        behavior.customData = value[1];
      }
    });
  }

  public loadTemplate(url?: string, bundle?: TemplateBundle, buffer?: ArrayBuffer, metaData?: MetaData) {
    TraceEvent.beginSection(TraceCategory.Lynx, TraceEventDef.CONTEXT_RE_INIT_TEMPLATE);
    if (!url && (bundle || buffer)) {
      LLog.e(TAG, 'loadTemplate invalid url');
      return;
    }
    if (!this.lynxView) {
      LLog.e(TAG, 'loadTemplate invalid lynxView');
      return;
    }
    if (url) {
      this.lynxView.url = url;
      this.lynxView.templateBundle = bundle;
      this.lynxView.buffer = buffer;
    }
    if (metaData) {
      this.lynxView.metaData = metaData;
    }
    LLog.i(TAG, 'loadTemplate width url: ' + this.lynxView.url);
    this.lynxView.ensureTemplateRendererAndLoadTemplate();
    TraceEvent.endSection(TraceCategory.Lynx, TraceEventDef.CONTEXT_RE_INIT_TEMPLATE);
  }

  public reload(data: MetaData) {
    TraceEvent.beginSection(TraceCategory.Lynx, TraceEventDef.CONTEXT_RELOAD);
    let timingOption: TimingOption = new TimingOption(TimingConstants.RELOAD_BUNDLE_FROM_NATIVE);
    let loadBundleStartTimestamp = Date.now();
    timingOption.setTiming(TimingConstants.PIPELINE_START, loadBundleStartTimestamp);
    timingOption.setTiming(TimingConstants.RELOAD_BUNDLE_START, loadBundleStartTimestamp);
    timingOption.setTiming(TimingConstants.FFI_START, Date.now());
    this.renderer?.reloadTemplate(data.templateData?.processor,
      data.templateData?.data,
      data.templateData?.readOnly,
      data.globalProps?.data,
      timingOption.toRecord());
    TraceEvent.endSection(TraceCategory.Lynx, TraceEventDef.CONTEXT_RELOAD);
  }

  /**
   * Warning: Please use the `getAllTimingInfo` API with caution.
   *
   * This API is very performance-unfriendly as it blocks while fetching all Timing information from the asynchronous Timing thread.
   *
   * Generally, business logic does not require using this API to obtain Timing information.
   * LynxSDK will support providing Timing information to the business through Client callbacks in future updates.
   */
  public getAllTimingInfo() {
    TraceEvent.beginSection(TraceCategory.Lynx, TraceEventDef.CONTEXT_GET_ALL_TIMING_INFO);
    const timingInfo = this.renderer?.getAllTimingInfo();
    TraceEvent.endSection(TraceCategory.Lynx, TraceEventDef.CONTEXT_GET_ALL_TIMING_INFO);
    return timingInfo;
  }

  /**
   * Supplement key performance data before loading the Lynx page.
   * @apidoc
   * @param extraTiming Used to supplement key performance data before loading the Lynx page.
   */
  public setExtraTiming(extraTiming: LynxExtraTiming) {
    this.renderer?.setExtraTiming(extraTiming);
  }

  /**
   * Put parameters for reporting events, overriding old values if the parameters already exist.
   * @param params  common parameters for report events, these values will be merged with params of LynxEventReporter.onEvent.
   */
  public putParamsForReportingEvents(params: ReadonlyMap<string, LynxReportEventPropValue>) {
    if (this.renderer) {
      LynxEventReporter.putExtraParams(params, this.renderer.getInstanceId());
    }
  }

  /**
   * Changing the font scaling ratio in client settings will automatically change the text size.
   * @apidoc
   * @param scale Font scaling.
   */
  updateFontScale(scale: number) {
    this.renderer?.updateFontScale(scale);
  }

  public setContextData(data?: Record<string, Object>): void {
    this.contextData = data;
  }

  public getContextData(): Record<string, Object> | undefined {
    return this.contextData;
  }

  public setMediaResourceFetcher(mediaResourceFetcher?: LynxMediaResourceFetcher) {
    this.mediaResourceFetcher = mediaResourceFetcher;
  }

  public setGenericResourceFetcher(genericResourceFetcher?: LynxGenericResourceFetcher) {
    this.genericResourceFetcher = genericResourceFetcher;
  }


  public getGenericResourceFetcher(): LynxGenericResourceFetcher | undefined {
    return this.genericResourceFetcher;
  }

  public getMediaResourceFetcher(): LynxMediaResourceFetcher | undefined {
    return this.mediaResourceFetcher;
  }

  public getDensity(): number {
    return LynxEnv.getDensity();
  }

  public getRectangleById(id: string) {
    return componentUtils.getRectangleById(this.uiOwner?.getId() + id);
  }

  public canConsumeTouchEvent(x: number, y: number): boolean {
    if (this.uiOwner) {
      return this.uiOwner.canConsumeTouchEvent(x, y);
    }
    return false;
  }

  public updateRootTarget(content: NativeContent) {
    if (this.uiOwner) {
      this.uiOwner.updateRootTarget(content);
    }
  }

  /**
   * Get page data by an array of keys.
   *
   * Caution: This method is time-consuming.
   *
   */
  public getPageDataByKey(keys: string[]) {
    return this.renderer?.getPageDataByKey(keys);
  }

  public setExtensionModule(module?: LynxExtensionModule) {
    this.extensionModule = module;
  }

  public getExtensionModule(): LynxExtensionModule | undefined {
    return this.extensionModule;
  }

  public getBaseInspectorOwner(): LynxBaseInspectorOwner | null {
    if (this.renderer) {
      const devTool = this.renderer.getDevTool();
      if (devTool) {
        return devTool.getBaseInspectorOwner();
      }
    }
    return null;
  }

  public getLynxImageConfig(): LynxImageConfig | undefined {
    return this.lynxView?.lynxImageConfig;
  }

  public getLynxViewClient(): LynxViewClient[] | undefined {
    return this.lynxView?.clients;
  }

  aboutToDisappear() {
    this.setLynxView(undefined);
    this.setRenderer(undefined);
    this.setUIOwner(undefined);
    this.setShadowNodeOwner(undefined);
    this.setGenericResourceFetcher(undefined);
    this.setMediaResourceFetcher(undefined);
    this.setExtensionModule(undefined);
  }
}
