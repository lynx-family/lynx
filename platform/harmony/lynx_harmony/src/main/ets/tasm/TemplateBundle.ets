// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import lynx from 'liblynx.so';
import { LynxServiceCenter, LynxServiceType } from './service/LynxServiceCenter';
import { ILynxSecurityService, LynxTasmType, SecurityResult } from './service/ILynxSecurityService';

import { TemplateBundleOption } from './TemplateBundleOption';

/**
 * `TemplateBundle` is the output product of the PreDecode capability
 * provided by the Lynx SDK. Client developers can parse the Lynx App Bundle product
 * in advance to obtain the `TemplateBundle` object and consume the App Bundle product.
 * @apidoc
 */
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

  private static createTemplateBundle(template: ArrayBuffer): TemplateBundle {
    let bundle: TemplateBundle = new TemplateBundle(template.byteLength);
    let securityService = LynxServiceCenter.getService<ILynxSecurityService>(LynxServiceType.Security);
    if (securityService) {
      let result: SecurityResult = securityService.verifyTASM(template, undefined, LynxTasmType.TYPE_TEMPLATE)
      if (!result.verified) {
        bundle.setErrMsg(`template verify failed, error message: ${result.errMsg}`)
      }
    }
    return bundle;
  }

  /**
   * Input Lynx template binary content and return the parsed `TemplateBundle` object.
   * @apidoc
   * @param template Template binary content.
   * @returns The `TemplateBundle` object.
   */
  public static fromTemplate(template: ArrayBuffer): TemplateBundle {
    let bundle: TemplateBundle = TemplateBundle.createTemplateBundle(template);
    if (bundle.getErrorMessage() != '') {
      return bundle;
    }

    let errMsg: string | undefined = bundle.nativeParseTemplate(template);
    if (errMsg === undefined) {
      errMsg = 'native parse template failed';
    }
    bundle.setErrMsg(errMsg);
    return bundle;
  }

  /**
   * Creates a TemplateBundle from a template binary data with options.
   *
   * @apidoc
   * @param template The template binary content.
   * @param option The TemplateBundleOption object.
   * @returns The new TemplateBundle object.
   */
  public static fromTemplateWithOption(template: ArrayBuffer,
    option: TemplateBundleOption): TemplateBundle {
    let bundle: TemplateBundle = TemplateBundle.fromTemplate(template);
    bundle.initWithOption(option);
    return bundle;
  }

  /**
   * Asynchronously creates a TemplateBundle from a template binary data. This method avoids blocking the main thread.
   *
   * @apidoc
   * @param template ArrayBuffer, a buffer of template binary data.
   * @return A promise that resolves with a new TemplateBundle object.
   * @note Due to ArkTS being restricted to run on the app’s main thread, we provide an asynchronous 
   * TemplateBundle creation API to reduce blocking time from main-thread I/O operations. On other platforms, 
   * developers can decide whether to run on a background thread without requiring a special asynchronous API.
   */
  public static async fromTemplateAsync(template: ArrayBuffer): Promise<TemplateBundle> {
    let bundle: TemplateBundle = TemplateBundle.createTemplateBundle(template);
    if (bundle.getErrorMessage() != '') {
      return bundle;
    }

    let errMsg: string | undefined = await bundle.nativeAsyncParseTemplate(template);
    if (errMsg === undefined) {
      errMsg = "native parse template bundle failed";
    }
    bundle.setErrMsg(errMsg);
    return bundle;
  }

  /**
   * Asynchronously creates a TemplateBundle from a template binary data with options. This method avoids blocking the main thread.
   *
   * @apidoc
   * @param template The template binary content.
   * @param option The TemplateBundleOption object.
   * @returns A promise that resolves with the new TemplateBundle object.
   * @note Due to ArkTS being restricted to run on the app’s main thread, we provide an asynchronous 
   * TemplateBundle creation API to reduce blocking time from main-thread I/O operations. On other platforms, 
   * developers can decide whether to run on a background thread without requiring a special asynchronous API.
   */
  public static async fromTemplateAsyncWithOption(template: ArrayBuffer,
    option: TemplateBundleOption): Promise<TemplateBundle> {
    let bundle: TemplateBundle = await TemplateBundle.fromTemplateAsync(template);
    bundle.initWithOption(option);
    return bundle;
  }

  private initWithOption(option: TemplateBundleOption): void {
    this.nativeInitWithOption(option.getContextPoolSize(), option.getEnableContextAutoRefill());
  }

  /**
   * Read the content of the `extraInfo` field configured in the `pageConfig` of the
   * front-end template.
   * @apidoc
   * @returns When the front-end does not configure extraInfo or is called on an empty TemplateBundle object,
   * it returns null.
   */
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

  /**
   * Whether the TemplateBundle contains a Valid ElementBundle.
   * @apidoc
   * @returns True if valid, else false.
   */
  public isElementBundleValid(): boolean {
    return this.nativeGetContainsElementTree();
  }

  /**
   * Get the size of current template.
   * @apidoc
   * @returns Template size.
   */
  public getTemplateSize(): number {
    return this.templateSize;
  }

  /**
   * Start a sub-thread task to generate the `js code cache` of the current template.
   * @apidoc
   * @param bytecodeSourceUrl The url of the current template.
   * @param useV8 Whether to generate V8 Code Cache, otherwise generate QuickJS Code Cache.
   */
  public postJsCacheGenerationTask(bytecodeSourceUrl: string, useV8: boolean): void {
    if (bytecodeSourceUrl.length > 0) {
      this.nativePostJsCacheGenerationTask(bytecodeSourceUrl, useV8);
    }
  }

  /**
   * When `TemplateBundle` is an invalid object, use this method to
   * obtain the exception information that occurred during template parsing.
   * @apidoc
   * @returns The exception information during template parsing.
   */
  public getErrorMessage(): string {
    return this.errMsg;
  }
}
