// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { LynxResourceRequest } from './LynxResourceRequest';
import { AsyncCallback, BusinessError } from '@ohos.base';
import { LynxResourceProvider } from './LynxResourceProvider';
import { LynxServiceCenter, LynxServiceType } from '../tasm/service/LynxServiceCenter';
import { ILynxSecurityService, LynxTasmType, SecurityResult } from '../tasm/service/ILynxSecurityService';
import { LLog } from '../base/LLog';
import { LynxSubErrorCode } from '../tasm/gen/LynxSubErrorCode';
import { TemplateBundle } from '../tasm/TemplateBundle';

export interface TemplateProviderResult {
  binary?: ArrayBuffer;
  bundle?: TemplateBundle;
}

const TAG = 'LynxTemplateResourceFetcher';

/**
 * `LynxTemplateResourceFetcher` is defined inside LynxEngine and
 * injected from outside to implement the resource loading interface of
 * [Bundle](/guide/spec.html#lynx-bundle-or-bundle) and [Lazy Bundle](/guide/spec.html#lazy-bundle)
 * etc.
 * @apidoc
 */
export abstract class LynxTemplateResourceFetcher implements LynxResourceProvider {
  /**
   * `LynxEngine` internally calls this method to obtain the contents of Bundle
   * and Lazy Bundle. The returned result type can contain `byte[]` or `TemplateBundle`.
   *
   * @apidoc
   * @param request Request to obtain the contents of Bundle and Lazy Bundle
   * @param callback Response with the requiring content file in ArrayBuffer or TemplateBundle type.
   */
  abstract fetchTemplate(request: LynxResourceRequest,
    callback: AsyncCallback<TemplateProviderResult>);

  /**
   * fetch SSRData of lynx.
   *
   * @param request
   * @param callback response with the requiring SSR data.
   */
  abstract fetchSSRData(request: LynxResourceRequest, callback: AsyncCallback<ArrayBuffer>);

  private buildFetchAndVerifyError(code: number, message: string): BusinessError<string> {
    return { code: code, data: message, message: message } as BusinessError<string>;
  }

  private stringifyFetchError(error: ESObject): string {
    if (!error) {
      return "fetch template failed";
    }
    if (typeof error.data === 'string' && error.data.length > 0) {
      return error.data;
    }
    if (typeof error.message === 'string' && error.message.length > 0) {
      return error.message;
    }
    try {
      return JSON.stringify(error);
    } catch (e) {
      return `${error}`;
    }
  }

  private fetchTemplateAndVerify(request: LynxResourceRequest,
    callback: AsyncCallback<TemplateProviderResult, string>) {
    let fetchCallback: AsyncCallback<TemplateProviderResult> =
      (fetchError: BusinessError, fetchResult: TemplateProviderResult) => {
        try {
          let fetchAndVerifyError: BusinessError<string> =
            this.buildFetchAndVerifyError(LynxSubErrorCode.E_SUCCESS, "");
          if (fetchError) {
            let errorCode = fetchError.code > 0 ? fetchError.code : LynxSubErrorCode.E_APP_BUNDLE_LOAD_BAD_RESPONSE;
            let errorMessage = this.stringifyFetchError(fetchError as ESObject);
            fetchAndVerifyError = this.buildFetchAndVerifyError(errorCode, errorMessage);
            callback(fetchAndVerifyError, fetchResult ?? ({} as TemplateProviderResult));
            return;
          }

          if (fetchResult?.bundle) {
            // The TemplateBundle has already been verified when it was created
            // via TemplateBundle.fromTemplate, check its native validity here.
            if (!fetchResult.bundle.isValid()) {
              const bundleErrorMessage = fetchResult.bundle.getErrorMessage();
              const errorMessage = bundleErrorMessage.length > 0
                ? bundleErrorMessage
                : "invalid TemplateBundle";
              LLog.e(TAG, `fetch template failed, bundle error message: ${errorMessage}`);
              fetchAndVerifyError =
                this.buildFetchAndVerifyError(LynxSubErrorCode.E_APP_BUNDLE_LOAD_BAD_RESPONSE, errorMessage);
            }
          } else if (fetchResult?.binary && fetchResult.binary.byteLength > 0) {
            let securityService = LynxServiceCenter.getService<ILynxSecurityService>(LynxServiceType.Security);
            let securityResult: SecurityResult = { verified: true };
            if (securityService != undefined) {
              securityResult =
                securityService.verifyTASM(fetchResult.binary, request.url, LynxTasmType.TYPE_DYNAMIC_COMPONENT,
                  undefined);
            }
            if (!securityResult.verified) {
              let errorMessage = securityResult.errMsg ?? "Signature check failed for template binary";
              LLog.e(TAG, `verify template failed, errMsg: ${errorMessage}`);
              fetchAndVerifyError =
                this.buildFetchAndVerifyError(LynxSubErrorCode.E_APP_BUNDLE_VERIFY_INVALID_SIGNATURE, errorMessage);
            }
          } else {
            const errorMessage = "both bundle and binary are undefined";
            LLog.e(TAG, `fetch template failed, ${errorMessage}`);
            fetchAndVerifyError =
              this.buildFetchAndVerifyError(LynxSubErrorCode.E_APP_BUNDLE_LOAD_BAD_RESPONSE, errorMessage);
          }
          callback(fetchAndVerifyError, fetchResult ?? ({} as TemplateProviderResult));
        } catch (e) {
          let errorMessage = this.stringifyFetchError(e as ESObject);
          LLog.e(TAG, `fetch template failed, errMsg: ${errorMessage}`);
          callback(this.buildFetchAndVerifyError(LynxSubErrorCode.E_APP_BUNDLE_LOAD_BAD_RESPONSE, errorMessage),
            fetchResult ?? ({} as TemplateProviderResult));
        }
      }
    try {
      this.fetchTemplate(request, fetchCallback);
    } catch (e) {
      let errorMessage = this.stringifyFetchError(e as ESObject);
      LLog.e(TAG, `fetch template failed, errMsg: ${errorMessage}`);
      callback(this.buildFetchAndVerifyError(LynxSubErrorCode.E_APP_BUNDLE_LOAD_BAD_RESPONSE, errorMessage),
        {} as TemplateProviderResult);
    }
  }
}
