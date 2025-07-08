// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import lynx from 'liblynx.so';
import { LynxServiceCenter, LynxServiceType } from './service/LynxServiceCenter';
import { ILynxSecurityService, LynxTasmType, SecurityResult } from './service/ILynxSecurityService';

import { TemplateBundleOption } from './TemplateBundleOption';

export class TemplateBundle extends lynx.TemplateBundle {
  private extraInfo: Map<string, Object> = new Map();
  private templateSize: number = 0;
  private errMsg: string = '';

  constructor(templateSize: number) {
    super();
    this.templateSize = templateSize;
  }

  public setErrMsg(errMsg: string) {
    this.errMsg = errMsg;
  }

  public static async fromTemplate(template: ArrayBuffer): Promise<TemplateBundle> {
    let bundle: TemplateBundle = new TemplateBundle(template.byteLength);
    let securityService = LynxServiceCenter.getService<ILynxSecurityService>(LynxServiceType.Security);
    if (securityService) {
      let result: SecurityResult = securityService.verifyTASM(template, undefined, LynxTasmType.TYPE_TEMPLATE)
      if (!result.verified) {
        bundle.setErrMsg(`template verify failed, error message: ${result.errMsg}`)
        return bundle;
      }
    }
    let errMsg: string | undefined = bundle.nativeParseTemplate(template);
    if (errMsg === undefined) {
      errMsg = 'native parse template failed';
    }
    bundle.setErrMsg(errMsg);
    return bundle;
  }

  public static async fromTemplateWithOption(template: ArrayBuffer,
    option: TemplateBundleOption): Promise<TemplateBundle> {
    let bundle: TemplateBundle = await TemplateBundle.fromTemplate(template);
    bundle.initWithOption(option);
    return bundle;
  }

  private initWithOption(option: TemplateBundleOption): void {
    this.nativeInitWithOption(option.getContextPoolSize(), option.getEnableContextAutoRefill());
  }

  public getExtraInfo(): Map<string, Object> {
    if (this.extraInfo.size == 0) {
      let extraInfoRecord = this.nativeGetExtraInfo();
      if (extraInfoRecord) {
        for (const entry of Object.entries(extraInfoRecord)) {
          this.extraInfo[entry[0]] = entry[1];
        }
      }
    }
    return this.extraInfo;
  }

  public isElementBundleValid(): boolean {
    return this.nativeGetContainsElementTree();
  }

  public getTemplateSize() {
    return this.templateSize;
  }

  public postJsCacheGenerationTask(bytecodeSourceUrl: string, useV8: boolean): void {
    if (bytecodeSourceUrl.length > 0) {
      this.nativePostJsCacheGenerationTask(bytecodeSourceUrl, useV8);
    }
  }

  public getErrorMessage(): string {
    return this.errMsg;
  }
}