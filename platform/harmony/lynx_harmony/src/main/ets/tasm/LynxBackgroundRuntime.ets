// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import lynx from 'liblynx.so';
import { LynxError } from './LynxError';
import { LynxErrorLevel } from './gen/LynxSubErrorCode';
import { LynxResourceProvider } from '../provider/LynxResourceProvider';
import { LynxBackgroundRuntimeOptions } from '../jsbridge/LynxBackgroundRuntimeOptions';
import { LynxDevTool } from '../devtool_wrapper/LynxDevTool';
import { convertModuleArgs, LynxModuleManager, SendableLynxModuleManager } from '../jsbridge/LynxModuleManager';
import { LynxGenericResourceFetcher } from '../provider/LynxGenericResourceFetcher';
import { LynxMediaResourceFetcher } from '../provider/LynxMediaResourceFetcher';
import { LynxTemplateResourceFetcher } from '../provider/LynxTemplateResourceFetcher';
import { ModuleClassWrapper, SendableModuleClassWrapper } from '../jsbridge/LynxModule';
import { LLog, LynxEnv, MetaData, TemplateBundle } from '../Index';
import { RuntimeLifecycleListener } from '../jsbridge/RuntimeLifecycleListener';
import { TemplateData } from './MetaData';
import { SessionStorageCallback } from './SessionStorage';

const TAG = 'LynxBackgroundRuntime';
const INVALID_SESSION_STORAGE_LISTENER_ID = -1;

export abstract class LynxBackgroundRuntimeClient {
  public onReceivedError(error: LynxError) : void {}
  public onModuleMethodInvoked(module: string, method: string, error_code: number) : void {}
  public onEvaluateJavaScriptEnd(url: string) : void {}
}

export class LynxBackgroundRuntime extends lynx.LynxRuntimeWrapper {
  private clients: Set<LynxBackgroundRuntimeClient> = new Set<LynxBackgroundRuntimeClient>();
  private providers: (LynxResourceProvider | undefined)[];
  private backgroundRuntimeOptions: LynxBackgroundRuntimeOptions;
  private lynxDevTool?: LynxDevTool;
  private moduleManager: LynxModuleManager = new LynxModuleManager(undefined);
  private sendableModuleManager: SendableLynxModuleManager = new SendableLynxModuleManager();
  private lastScriptUrl?: string;

  constructor(backgroundRuntimeOptions: LynxBackgroundRuntimeOptions, genericResourceFetcher : LynxGenericResourceFetcher | undefined,
    mediaResourceFetcher: LynxMediaResourceFetcher | undefined,
    templateResourceFetcher: LynxTemplateResourceFetcher | undefined,  modules: Map<string, ModuleClassWrapper> | undefined,
    sendableModules: Map<string, SendableModuleClassWrapper> | undefined, data: MetaData | undefined) {
    super();
    this.providers = [genericResourceFetcher, mediaResourceFetcher, templateResourceFetcher];
    this.backgroundRuntimeOptions = new LynxBackgroundRuntimeOptions(backgroundRuntimeOptions);
    modules?.forEach((v, k) => {
      this.moduleManager.registerModule(k, v);
    });
    sendableModules?.forEach((v, k) => {
      this.sendableModuleManager.registerModule(k, v);
    });
    this.nativeCreate(this.providers,  this.backgroundRuntimeOptions.getGroupID(),
      this.backgroundRuntimeOptions.useQuickJSEngine(),
      this.backgroundRuntimeOptions.enableJSGroupThread(),
      this.backgroundRuntimeOptions.getPreloadJSPath(),
      this.backgroundRuntimeOptions.enableBytecode, this.backgroundRuntimeOptions.bytecodeSourceUrl,  convertModuleArgs(this.moduleManager),
      convertModuleArgs(this.sendableModuleManager), data?.templateData?.processor, data?.templateData?.data, data?.templateData?.readOnly,
      data?.globalProps?.data);
  }

  addLifecycleClient(client: LynxBackgroundRuntimeClient) {
    this.clients.add(client);
  }
  
  removeLifecycleClient(client: LynxBackgroundRuntimeClient) {
    this.clients.delete(client);
  }

  // client callback
  onErrorOccurred(level: string, errorCode: number, message: string, suggestion: string,
    customInfo: Record<string, string>, logboxOnly: boolean) {
    let err = new LynxError(LynxErrorLevel[level], errorCode, message, suggestion, LynxError.NATIVE_ERROR, customInfo);
    err.setTemplateUrl(this.lastScriptUrl);
    // TODO: add logBox
    if (logboxOnly) {
      LLog.i(TAG, 'onErrorOccurred, error:' + message);
      return;
    }
    this.clients.forEach(client => {
      client.onReceivedError(err);
    });
  }

  onModuleMethodInvoked(module: string, method: string, error_code: number) {
    this.clients.forEach(client => {
      client.onModuleMethodInvoked(module, method, error_code);
    });
  }

  onEvaluateJavaScriptEnd(url: string) {
    this.clients.forEach(client => {
      client.onEvaluateJavaScriptEnd(url);
    });
  }
  
  // devtool
  createDevTool(embedderProxy: number[]) {
    this.lynxDevTool = new LynxDevTool(embedderProxy);
  }

  getDevTool() {
    return this.lynxDevTool;
  }

  // background runtime methods
  evaluateJavaScript(url: string, sources: string) : void {
    this.lastScriptUrl = url;
    this.nativeEvaluateScript(url, sources);
  }
  
  evaluateBundleScript(url: string, bundle: TemplateBundle, jsFileName: string) {
    this.lastScriptUrl = url;
    this.nativeEvaluateBundleScript(url, bundle, jsFileName);
  }
  
  callJSFunction(module: string, method: string, params: Object[]) {
    this.nativeCallJSFunction(module, method, params);
  }

  sendGlobalEvent(name: string, params: object[]) {
    this.callJSFunction('GlobalEventEmitter', 'emit', [name, params]);
  }
  
  transitionToFullRuntime(): void {
    this.nativeTransitionToFullRuntime();
  }

  setSessionStorageItem(key: string, value: TemplateData): void {
    if (!key || !value) {
      return;
    }
    // TODO(nihao.royal): call native C++ sessionStorage setter after NAPI is ready.
  }

  getSessionStorageItem(key: string, callback: SessionStorageCallback): void {
    if (!key || !callback) {
      return;
    }
    // TODO(nihao.royal): call native C++ sessionStorage getter after NAPI is ready.
  }

  subscribeSessionStorage(key: string, callback: SessionStorageCallback): number {
    if (!key || !callback) {
      return INVALID_SESSION_STORAGE_LISTENER_ID;
    }
    // TODO(nihao.royal): call native C++ sessionStorage subscribe after NAPI is ready.
    return INVALID_SESSION_STORAGE_LISTENER_ID;
  }

  unsubscribeSessionStorage(key: string, listenerId: number): void {
    if (!key || listenerId < 0) {
      return;
    }
    // TODO(nihao.royal): call native C++ sessionStorage unsubscribe after NAPI is ready.
  }

  addRuntimeLifecycleListener(listener: RuntimeLifecycleListener) {
    this.nativeAddRuntimeLifecycleListener(listener);
  }

  getLastScriptUrl(): string | undefined {
    return this.lastScriptUrl;
  }
  
  get moduleFactory(): LynxModuleManager {
    return this.moduleManager;
  }
  
  get runtimeOptions(): LynxBackgroundRuntimeOptions {
    return this.backgroundRuntimeOptions;
  }
  
  get genericResourceFetcher() : LynxGenericResourceFetcher {
    return this.providers[0] as LynxGenericResourceFetcher;
  }

  get mediaResourceFetcher() : LynxMediaResourceFetcher {
    return this.providers[1] as LynxMediaResourceFetcher;
  }

  get templateResourceFetcher() : LynxTemplateResourceFetcher {
    return this.providers[2] as LynxTemplateResourceFetcher;
  }
  
}
