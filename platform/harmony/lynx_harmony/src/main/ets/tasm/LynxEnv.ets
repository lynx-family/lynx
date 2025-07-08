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
  private logBoxEnabled: boolean = false;
  private hasInitDevToolEnv: boolean = false;
  private performanceEnable: boolean = false;
  private tracingDirPath: string = '';
  private defaultDensityEnabled = false;
  private displayInfo = new DisplayInfo();
  private initialized = false;

  public initialize(context: common.Context) {
    if (this.initialized) {
      return;
    }
    LLog.initLynxLog();
    this.initLynxDebug();
    if (this.isLynxDebugEnabled() && !this.performanceEnable) {
      this.initDevToolEnv();
      this.setTracingDirPath(context.filesDir);
    }
    lynx.initGlobalEnv(context.resourceManager);
    LynxInfoReporterHelper.initialize();
    LynxEventReporter.initialize();
    LynxTrailHub.getInstance().init(context.getApplicationContext());
    LynxEnvironment.setCacheDirPath(context.cacheDir);
    LLog.i(TAG, 'LynxEnv initialize with version ' + BuildProfile.HAR_VERSION);
    this.initialized = true;
  }

  setAppInfo(appInfo: Map<string, string>) {
    if (!this.isLynxDebugEnabled() && !this.performanceEnable) {
      return;
    }
    this.updateAppInfo(appInfo);
    LynxDebugger.setAppInfo(Array.from(appInfo.keys()), Array.from(appInfo.values()))
  }

  private initLynxDebug() {
    let isDevToolModuleLoaded =
      LynxServiceCenter.getService<ILynxDevToolService>(LynxServiceType.DevTool)?.isDevToolModuleLoaded();
    if (isDevToolModuleLoaded) {
      LLog.i(TAG, "isDevToolModuleLoaded is true");
      this.enableLynxDebug(true);
      LynxDebugger.setSwitch(LynxEnvKey.KEY_DEVTOOL_COMPONENT_ATTACH, true);
    }
  }

  private initDevToolEnv() {
    if (this.hasInitDevToolEnv) {
      return;
    }
    this.hasInitDevToolEnv = true;
    this.setDefaultAppInfo();
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
    if (enabled) {
      this.initDevToolEnv();
    }
  }

  public isLynxDebugEnabled() {
    return this.isDevtoolComponentAttach() && LynxDebugger.getSwitch(LynxEnvKey.KEY_LYNX_DEBUG_ENABLED);
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

  public getDefaultDensityEnabled() {
    return this.defaultDensityEnabled;
  }

  public getDisplayInfo() {
    return this.displayInfo;
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
