// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { LynxResourceRequest } from './LynxResourceRequest';
import { AsyncCallback } from '@ohos.base';
import { LynxResourceProvider } from './LynxResourceProvider';

export interface TemplateProviderResult {
  binary?: ArrayBuffer;
  // TODO(nihao.royal): support returning TemplateBundle
}

export abstract class LynxTemplateResourceFetcher implements LynxResourceProvider {
  /**
   * fetch template resource of lynx & dynamic component etc.
   *
   * @param request
   * @param callback response with the requiring content file: ArrayBuffer or TemplateBundle
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
}