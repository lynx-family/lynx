// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import lynx from 'liblynx.so';
import { LynxError } from './LynxError';
import { LynxErrorLevel } from './gen/LynxSubErrorCode';
import { LynxViewClient } from './LynxViewClient';
import { LynxExtraTiming } from './performance/timing/LynxExtraTiming';
import { LynxResourceProvider } from '../provider/LynxResourceProvider';
import { LynxView, ThreadStrategyForRendering } from './LynxView';
import { LynxBackgroundRuntimeOptions } from '../jsbridge/LynxBackgroundRuntimeOptions';
import { LynxEnv } from './LynxEnv';
import { TemplateBundle } from './TemplateBundle';
import { LynxDevTool } from '../devtool_wrapper/LynxDevTool';
import { PerformanceController } from './performance/PerformanceController';
import { convertModuleArgs, LynxModuleManager, SendableLynxModuleManager } from '../jsbridge/LynxModuleManager';
import { LynxLepusModule } from '../lepus/LynxLepusModule';
import { LynxBackgroundRuntime } from '../Index';

const TAG = 'LynxTemplateRenderer';


export class LynxTemplateRenderer extends lynx.LynxTemplateRenderer {
  private clients: LynxViewClient[];
  private url?: string;
  private perfController?: PerformanceController;
  private providers: (LynxResourceProvider | undefined)[];
  private isHostRenderer: boolean;
  private enableJSRuntime: boolean;
  private readonly threadMode: ThreadStrategyForRendering;
  private backgroundRuntimeOptions: LynxBackgroundRuntimeOptions;
  private lynxView: LynxView;
  private lynxDevTool?: LynxDevTool;
  private moduleManager: LynxModuleManager;
  private sendableModuleManager: SendableLynxModuleManager;
  private destroyed = false;
  private displayInfo = LynxEnv.getDisplayInfo().clone();

  constructor(clients: LynxViewClient[], providers: (LynxResourceProvider | undefined)[],
    isHostRenderer: boolean,
    threadMode: ThreadStrategyForRendering,
    backgroundRuntimeOptions: LynxBackgroundRuntimeOptions,
    lynxView: LynxView,
    enableJSRuntime: boolean,
    moduleManager: LynxModuleManager,
    sendableModuleManager: SendableLynxModuleManager) {
    super();
    this.clients = clients;
    this.providers = providers;
    this.isHostRenderer = isHostRenderer;
    this.enableJSRuntime = enableJSRuntime;
    this.threadMode = threadMode;
    this.backgroundRuntimeOptions = new LynxBackgroundRuntimeOptions(backgroundRuntimeOptions);
    this.nativeAttach(LynxEnv.getDensity());
    this.lynxView = lynxView;
    this.moduleManager = moduleManager;
    this.sendableModuleManager = sendableModuleManager;
  }

  reset(uiDelegate: number[], perfController: PerformanceController) {
    this.perfController = perfController;
    this.nativeReset(uiDelegate, this.providers, this.displayInfo.screenWidth, this.displayInfo.screenHeight,
      this.isHostRenderer, this.perfController.native, this.threadMode,
      this.backgroundRuntimeOptions.getGroupID(),
      this.backgroundRuntimeOptions.useQuickJSEngine(),
      this.backgroundRuntimeOptions.enableJSGroupThread(),
      this.backgroundRuntimeOptions.getPreloadJSPath(),
      this.backgroundRuntimeOptions.enableBytecode, this.backgroundRuntimeOptions.bytecodeSourceUrl,
      this.enableJSRuntime,
      convertModuleArgs(this.moduleManager),
      convertModuleArgs(this.sendableModuleManager), this.lynxView.backgroundRuntime);
    // release backgroundRuntime
    this.lynxView.backgroundRuntime = undefined;
  }

  aboutToDisappear() {
    this.nativeDetach();
    this.destroyed = true;
    this.clients.forEach(client => client.onDestroy());
    this.lynxDevTool?.destroy(this.lynxView);
  }

  // Called by native, EmbedderEngineClient
  onLoaded(url: string) {
    this.url = url;
    this.clients.forEach(client => client.onLoadSuccess(url));
    this.lynxDevTool?.onLoaded(this.lynxView);
    this.lynxView?.logBox?.onLoaded(url);
  }

  onReloadTemplate() {
    this.lynxView.loadTemplate();
  }

  onRuntimeReady() {
    this.clients.forEach(client => client.onRuntimeReady());
  }

  onDataUpdated() {
    this.clients.forEach(client => client.onDataUpdated());
  }

  onPageChanged(isFirstScreen: boolean) {
    if (isFirstScreen) {
      this.clients.forEach(client => client.onFirstScreen());
    } else {
      this.clients.forEach(client => client.onPageUpdate());
    }
  }

  onFirstLoadPerfReady(perf: Record<number, number>, perfTiming: Record<number, string>) {
  }

  onUpdatePerfReady(perf: Record<number, number>, perfTiming: Record<number, string>) {
  }

  onErrorOccurred(level: string, errorCode: number, message: string, suggestion: string,
    customInfo: Record<string, string>, logboxOnly: boolean) {
    let err = new LynxError(LynxErrorLevel[level], errorCode, message, suggestion, LynxError.NATIVE_ERROR, customInfo);
    err.setTemplateUrl(this.url);
    this.lynxView.logBox?.reportError(err);
    if (logboxOnly) {
      return;
    }
    this.clients.forEach(client => {
      client.onReceivedError(err);
    });
  }

  createDevTool(embedderProxy: number[]) {
    this.lynxDevTool = new LynxDevTool(embedderProxy);
  }

  getDevTool() {
    return this.lynxDevTool;
  }

  //TODO(zhengyuwei.123): Optimize this implementation to use ModuleFactory instead of moduleManager.
  initNativeSetModule(moduleManager: number[]) {
    if (this.lynxDevTool) {
      this.lynxDevTool.initDevToolSetModule(moduleManager);
    }
  }

  onThemeUpdatedByJs(theme: Record<string, string>) {
  }

  createTemplateBundle(): TemplateBundle {
    return new TemplateBundle(0);
  }

  onTemplateBundleReady(bundle: TemplateBundle, errMsg: string) {
    bundle.setErrMsg(errMsg);

    this.clients.forEach(client => {
      client.onTemplateBundleReady(bundle);
    })
  }

  setTiming(timestamp: number, timingKey: string, pipelineID: string = '') {
    this.perfController?.setTiming(timestamp, timingKey, pipelineID);
  }

  setExtraTiming(extraTiming: LynxExtraTiming) {
    this.perfController?.setExtraTiming(extraTiming);
  }

  initScreenMetrics() {
    super.updateScreenMetrics(this.displayInfo.screenWidth, this.displayInfo.screenHeight, LynxEnv.getDensity());
  }

  override updateScreenMetrics(width: number, height: number) {
    if (width != this.displayInfo.screenWidth || height != this.displayInfo.screenHeight) {
      this.displayInfo.updateDisplayInfo(width, height);
      super.updateScreenMetrics(width, height, LynxEnv.getDensity());
    }
  }

  setEnableBytecode(enable: boolean, sourceUrl: string) {
    if (enable === this.backgroundRuntimeOptions.enableBytecode &&
      sourceUrl === this.backgroundRuntimeOptions.bytecodeSourceUrl) {
      return;
    }
    this.backgroundRuntimeOptions.enableBytecode = enable;
    this.backgroundRuntimeOptions.bytecodeSourceUrl = sourceUrl;
    this.nativeSetEnableBytecode(enable, sourceUrl);
  }

  getAllJsSource(): Record<string, string> {
    return this.nativeGetAllJsSource();
  }

  setPlatformConfig(config: string) {
    this.nativeSetPlatformConfig(config);
  }

  triggerLepusMethod(name: string, args: Record<string, Object>): Object | undefined {
    return LynxLepusModule.triggerLepusBridge(name, args, this, this.moduleManager);
  }

  triggerLepusMethodAsync(name: string, args: Record<string, Object>) {
    LynxLepusModule.triggerLepusBridgeAsync(name, args, this, this.moduleManager);
  }

  override invokeLepusCallback(id: number, entryName: string, args: Object) {
    if (this.destroyed) {
      return;
    }
    super.invokeLepusCallback(id, entryName, args);
  }
}
