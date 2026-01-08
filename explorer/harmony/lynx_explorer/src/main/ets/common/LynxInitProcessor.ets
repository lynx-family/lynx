// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { common } from '@kit.AbilityKit';
import { LLog, LynxEnv, LynxServiceCenter, LynxServiceType } from '@lynx/lynx';
import { LynxBaseServiceCenter, LynxBaseServiceType } from '@lynx/lynx_base';
import { LynxDevToolService } from '@lynx/lynx_devtool_service';
import { LynxLogService } from '@lynx/lynx_log_service';
import { LynxHttpService } from '@lynx/lynx_http_service';

export class LynxInitProcessor {
  private context: common.UIAbilityContext;

  constructor(context: common.UIAbilityContext) {
    this.context = context;
  }

  public initLynxEnv() {
    LLog.useSysLog(true);
    LynxEnv.initialize(this.context);
    let options = new Map<string, string>();
    options.set('App', 'LynxExplorer');
    options.set('AppVersion', '0.0.1');
    LynxEnv.setAppInfo(options);
  }

  public initLynxService(): void {
    this.initLogService();
    this.initDevToolService();
    this.initHttpService();
  }

  private initDevToolService(): void {
    LynxServiceCenter.registerService(LynxServiceType.DevTool, LynxDevToolService.instance);
  }

  private initLogService(): void {
    LynxBaseServiceCenter.registerService(LynxBaseServiceType.Log, LynxLogService.instance);
  }
  
  private initHttpService(): void {
    LynxServiceCenter.registerService(LynxServiceType.Http, LynxHttpService.instance);
  }
}
