// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import common from '@ohos.app.ability.common';
import deviceInfo from '@ohos.deviceInfo';

import lynx, { EmbedderPlatform } from 'liblynx.so';
import BuildProfile from '../../../../BuildProfile';
import { LLog } from '../base/LLog';
import { TraceEventDef } from '../base/TraceEventDef';
import { LynxBaseEnv, LynxBaseServiceCenter, LynxBaseServiceType } from '@lynx/lynx_base';
import { LynxBaseTraceService } from '../base/LynxBaseTraceService';
import { LynxEventReporter } from './eventreport/LynxEventReporter';
import { LynxInfoReporterHelper } from './eventreport/LynxInfoReporterHelper';
import { DisplayInfo } from './DisplayInfo';
import { LynxTrailHub } from './LynxTrailHub';
import { LynxDebugger } from '../devtool_wrapper/LynxDebugger';
import { LynxServiceCenter, LynxServiceType } from '../tasm/service/LynxServiceCenter';
import { ILynxDevToolService } from '../tasm/service/ILynxDevToolService';
import { ILynxImageService } from './service/ILynxImageService';
import { MemoryPressureCallback, MemoryPressureCallbackDispatcher } from '../base/memory/MemoryPressureCallbackDispatcher';
import { MemoryPressureLevel } from '../base/memory/MemoryPressureLevel';
import { DevToolLifecycle } from '../devtool_wrapper/DevToolLifecycle';
import { DevToolSettings } from '../devtool_wrapper/DevToolSettings';

const TAG = 'LynxEnv';

class LynxEnvironment {
  private hasInitLynxDebug: boolean = false;
  private performanceEnable: boolean = false;
  private appInfo?: Map<string, string>;
  private tracingDirPath: string = '';
  private defaultDensityEnabled = false;
  private displayInfo = new DisplayInfo();
  private initialized = false;

  public initialize(context: common.Context) {
    if (this.initialized) {
      return;
    }
    DevToolSettings.initialize();
    this.initLynxBaseEnv();
    if (!this.performanceEnable) {
      this.setTracingDirPath(context.filesDir);
    }
    const imageService = LynxServiceCenter.getService<ILynxImageService>(LynxServiceType.Image);
    if (imageService) {
      lynx.registerImageService(imageService.initNativeService(context.resourceManager));
    } else {
      LLog.e(TAG, 'LynxImageService not found. Register via LynxServiceCenter.registerService before using.');
    }

    this.tryInitLynxDebug();
    lynx.initGlobalEnv(context.resourceManager);
    this.registerNativeMemoryPressureCallback();
    LynxInfoReporterHelper.initialize();
    LynxEventReporter.initialize();
    LynxTrailHub.getInstance().init(context.getApplicationContext());
    LynxEnvironment.setCacheDirPath(context.cacheDir);
    LLog.i(TAG, 'LynxEnv initialize with version ' + BuildProfile.HAR_VERSION);
    this.initialized = true;
  }

  setAppInfo(appInfo: Map<string, string>) {
    this.updateAppInfo(appInfo);
    this.appInfo = appInfo;
    LynxDebugger.setAppInfo(Array.from(appInfo.keys()), Array.from(appInfo.values()));
  }

  private initLynxBaseEnv() {
    LynxBaseServiceCenter.registerService(LynxBaseServiceType.Trace, LynxBaseTraceService.instance);
    LynxBaseEnv.initialize(this.isDevtoolEnabled());
  }

  private setDefaultAppInfo() {
    this.setAppInfo(
      new Map([
        ['network', ''],
        ['deviceModel', ''],
        ['osVersion', 'HarmonyOS'],
        ['sdkVersion', '1.4.0']
      ])
    );
  }

  /** @deprecated */
  public enableLynxDebug(enabled: boolean) {
    if (!DevToolLifecycle.isAttached()) {
      return;
    }
    if (enabled) {
      DevToolLifecycle.onEnabled();
    } else {
      DevToolLifecycle.onDisabled();
    }
  }

  /** @deprecated */
  public isLynxDebugEnabled() {
    return DevToolLifecycle.isEnabled();
  }

  /**
   * Attempt to initialize Lynx debug functionality
   *
   * @description This method performs deferred initialization after DevTool module async loading completes.
   * Checks if DevTool module import has finished, then executes initialization if ready.
   * Uses hasInitLynxDebug flag to prevent duplicate initialization.
   *
   * @remarks Designed to be called multiple times safely, with internal state checking.
   * Solves timing issues between DevTool module async loading and initialization.
   *
   * - LynxEnv.initialize(): Called during application initialization
   * - LynxDevToolService: Called after DevTool module loading completes
   *
   */
  public tryInitLynxDebug() {
    if (this.hasInitLynxDebug) {
      return;
    }
    let isDevToolModuleLoaded =
      LynxServiceCenter.getService<ILynxDevToolService>(LynxServiceType.DevTool)?.isDevToolModuleLoaded();
    if (isDevToolModuleLoaded) {
      LLog.i(TAG, "isDevToolModuleLoaded is true");
      this.hasInitLynxDebug = true;
      DevToolLifecycle.onAttached();
      DevToolLifecycle.onEnabled();
      // assume dynamic libraries are ready since modules are imported
      DevToolLifecycle.onInitialized();
      if (this.appInfo) {
        LynxDebugger.setAppInfo(Array.from(this.appInfo.keys()), Array.from(this.appInfo.values()));
      } else {
        this.setDefaultAppInfo();
      }
    }
  }

  public isDevtoolComponentAttach() {
    return DevToolLifecycle.isAttached();
  }

  public isDevtoolEnabled() {
    if (!DevToolLifecycle.isEnabled()) {
      return false;
    }
    return DevToolSettings.isDevToolEnabled();
  }

  public enableDevtool(enabled: boolean) {
    if (!DevToolLifecycle.isEnabled()) {
      return;
    }
    DevToolSettings.setDevToolEnabled(enabled);
  }

  public quickjsDebugEnabled() {
    if (!DevToolLifecycle.isEnabled()) {
      return false;
    }
    return DevToolSettings.isQuickjsDebugEnabled();
  }

  public setQuickjsDebugEnabled(enabled: boolean) {
    if (!DevToolLifecycle.isEnabled()) {
      return;
    }
    DevToolSettings.setQuickjsDebugEnabled(enabled);
  }

  public setTracingDirPath(tracingDirPath: string) {
    this.tracingDirPath = tracingDirPath;
    lynx.setTracingDirPath(tracingDirPath);
  }

  public getTracingDirPath(): string {
    return this.tracingDirPath;
  }

  public static setCacheDirPath(cacheDirPath: string) {
    lynx.setCacheDirPath(cacheDirPath);
  }

  public isDebugModeEnabled(): boolean {
    return DevToolLifecycle.isAttached();
  }

  public setLogBoxEnabled(enabled: boolean) {
    DevToolSettings.setLogBoxEnabled(enabled);
  }

  public isLogBoxEnabled() {
    return DevToolSettings.isLogBoxEnabled();
  }

  private updateAppInfo(appInfo: Map<string, string>) {
    appInfo.set('osType', 'HarmonyOS');
    appInfo.set('sdkVersion', LynxEnv.getLynxVersion());
    const productModel = deviceInfo.productModel;
    appInfo.set('deviceModel', productModel);
    let displayVersion = deviceInfo.displayVersion;
    const index = displayVersion.indexOf(productModel);
    if (index !== -1) {
      displayVersion = displayVersion.slice(index + productModel.length);
    }
    appInfo.set('osVersion', displayVersion);
    if (!appInfo.has('App')) {
      appInfo.set('App', 'LynxPlayground');
    }
  }

  public parserTestBenchRecordData(source: string): string {
    return lynx.parserTestBenchRecordData(source);
  }

  /**
   * Dispatches a memory pressure signal to registered callbacks.
   */
  public trimMemory(pressure: MemoryPressureLevel) {
    if (this.isDebugModeEnabled()) {
      lynx.traceEventBegin('lynx', TraceEventDef.LYNX_ENV_TRIM_MEMORY);
    }
    MemoryPressureCallbackDispatcher.getInstance().notifyMemoryPressure(pressure);
    if (this.isDebugModeEnabled()) {
      lynx.traceEventEnd('lynx', TraceEventDef.LYNX_ENV_TRIM_MEMORY);
    }
  }

  public getLynxVersion(): string {
    return BuildProfile.HAR_VERSION
  }

  private nativeOnMemoryPressure(pressure: number) {
    lynx.notifyMemoryPressure(pressure);
  }

  private registerNativeMemoryPressureCallback() {
    const handler: MemoryPressureCallback = (pressure) => {
      this.nativeOnMemoryPressure(pressure);
    };
    MemoryPressureCallbackDispatcher.getInstance().addCallback(handler);
  }

  // Be called by Benchmark or performance action before initialize
  public setPerformanceEnable(isOpen: boolean): void {
    this.performanceEnable = isOpen;
  }

  public setDefaultDensityEnabled(enabled: boolean) {
    this.defaultDensityEnabled = enabled;
  }

  public getDisplayInfo() {
    return this.displayInfo;
  }

  public getDensity(): number {
    return this.defaultDensityEnabled ? vp2px(1) : this.displayInfo.densityPixels;
  }

  public updateDisplayInfo(width: number, height: number, densityPixels?: number, densityDPI?: number,
    refreshRate?: number) {
    LLog.i(TAG, `updateDisplayInfo, w: ${width}, h: ${height}, density: ${densityPixels}`);
    this.displayInfo.updateDisplayInfo(width, height, densityPixels, densityDPI, refreshRate);
    if (refreshRate) {
      EmbedderPlatform.updateRefreshRate(refreshRate);
    }
  }
}

export const LynxEnv = new LynxEnvironment();
