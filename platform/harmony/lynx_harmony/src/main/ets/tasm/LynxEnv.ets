// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import common from '@ohos.app.ability.common';
import deviceInfo from '@ohos.deviceInfo';

import lynx, { EmbedderPlatform } from 'liblynx.so';
import BuildProfile from '../../../../BuildProfile';
import { LLog } from '../base/LLog';
import { LynxEventReporter } from './eventreport/LynxEventReporter';
import { LynxEnvKey } from './LynxEnvKey';
import { LynxInfoReporterHelper } from './eventreport/LynxInfoReporterHelper';
import { DisplayInfo } from './DisplayInfo';
import { LynxTrailHub } from './LynxTrailHub';
import { LynxDebugger } from '../devtool_wrapper/LynxDebugger';
import { LynxServiceCenter, LynxServiceType } from '../tasm/service/LynxServiceCenter';
import { ILynxDevToolService } from '../tasm/service/ILynxDevToolService';

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
    LLog.initLynxLog(this.isLynxDebugEnabled());
    if (!this.performanceEnable) {
      this.setTracingDirPath(context.filesDir);
    }
    this.tryInitLynxDebug();
    lynx.initGlobalEnv(context.resourceManager);
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

  public enableLynxDebug(enabled: boolean) {
    LynxDebugger.setSwitch(LynxEnvKey.KEY_LYNX_DEBUG_ENABLED, enabled);
  }

  public isLynxDebugEnabled() {
    return this.isDevtoolComponentAttach() && LynxDebugger.getSwitch(LynxEnvKey.KEY_LYNX_DEBUG_ENABLED);
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
      this.enableLynxDebug(true);
      LynxDebugger.setSwitch(LynxEnvKey.KEY_DEVTOOL_COMPONENT_ATTACH, true);
      if (this.appInfo) {
        LynxDebugger.setAppInfo(Array.from(this.appInfo.keys()), Array.from(this.appInfo.values()));
      } else {
        this.setDefaultAppInfo();
      }
    }
  }

  public isDevtoolComponentAttach() {
    return LynxDebugger.getSwitch(LynxEnvKey.KEY_DEVTOOL_COMPONENT_ATTACH);
  }

  public isDevtoolEnabled() {
    if (!this.isLynxDebugEnabled()) {
      return false;
    }
    let devtoolEnabled = LynxDebugger.getSwitch(LynxEnvKey.SP_KEY_ENABLE_DEVTOOL);
    return devtoolEnabled;
  }

  public enableDevtool(enabled: boolean) {
    if (!this.isLynxDebugEnabled()) {
      return;
    }
    LynxDebugger.setSwitch(LynxEnvKey.SP_KEY_ENABLE_DEVTOOL, enabled);
  }

  public quickjsDebugEnabled() {
    if (!this.isLynxDebugEnabled()) {
      return false;
    }
    let quickjsEnabled = LynxDebugger.getSwitch(LynxEnvKey.SP_KEY_ENABLE_QUICKJS_DEBUG);
    return quickjsEnabled;
  }

  public setQuickjsDebugEnabled(enabled: boolean) {
    if (!this.isLynxDebugEnabled()) {
      return;
    }
    LynxDebugger.setSwitch(LynxEnvKey.SP_KEY_ENABLE_QUICKJS_DEBUG, enabled);
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
    return this.isDevtoolComponentAttach();
  }

  public setLogBoxEnabled(enabled: boolean) {
    LynxDebugger.setSwitch(LynxEnvKey.SP_KEY_ENABLE_LOGBOX, enabled);
  }

  public isLogBoxEnabled() {
    return LynxDebugger.getSwitch(LynxEnvKey.SP_KEY_ENABLE_LOGBOX);
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

  public getLynxVersion(): string {
    return BuildProfile.HAR_VERSION
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
    this.displayInfo.updateDisplayInfo(width, height, densityPixels, densityDPI, refreshRate);
    if (refreshRate) {
      EmbedderPlatform.updateRefreshRate(refreshRate);
    }
  }
}

export const LynxEnv = new LynxEnvironment();
