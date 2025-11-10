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

export interface TemplateProviderResult {
  binary?: ArrayBuffer;
  // TODO(nihao.royal): support returning TemplateBundle
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
   * @param callback Response with the requiring content file in ArrayBuffer type.
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

  private fetchTemplateAndVerify(request: LynxResourceRequest,
    callback: AsyncCallback<TemplateProviderResult, string>) {
    let fetchCallback: AsyncCallback<TemplateProviderResult> =
      (fetchError: BusinessError, fetchResult: TemplateProviderResult) => {
        let fetchAndVerifyError: BusinessError<string> =
          { code: LynxSubErrorCode.E_SUCCESS, data: "" } as BusinessError<string>;
        if (fetchResult.binary) {
          let securityService = LynxServiceCenter.getService<ILynxSecurityService>(LynxServiceType.Security);
          let securityResult: SecurityResult = { verified: true };
          if (securityService != undefined) {
            securityResult =
              securityService.verifyTASM(fetchResult.binary, request.url, LynxTasmType.TYPE_DYNAMIC_COMPONENT,
                undefined);
          }
          if (!securityResult.verified) {
            LLog.e(TAG, `verify template failed, errMsg: ${securityResult.errMsg}`);
            fetchAndVerifyError.code = LynxSubErrorCode.E_APP_BUNDLE_VERIFY_INVALID_SIGNATURE;
            fetchAndVerifyError.data = securityResult.errMsg;
          }
        } else {
          LLog.e(TAG, "fetch template failed, the binary is undefined");
          fetchAndVerifyError.code = LynxSubErrorCode.E_APP_BUNDLE_LOAD_BAD_RESPONSE;
          fetchAndVerifyError.data = "fetch binary is undefined";
        }
        callback(fetchAndVerifyError, fetchResult);
      }
    this.fetchTemplate(request, fetchCallback);
  }
}
