// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { LynxResourceRequest, LynxTemplateResourceFetcher, TemplateProviderResult } from '@lynx/lynx';
import { AsyncCallback, BusinessError } from '@ohos.base';
import http from '@ohos.net.http';
import resourceManager from '@ohos.resourceManager';

export class ExampleTemplateResourceFetcher extends LynxTemplateResourceFetcher {
  fetchTemplate(request: LynxResourceRequest,
    callback: AsyncCallback<TemplateProviderResult, void>) {
    if (request.url.startsWith('http')) {
      let httpRequest = http.createHttp();
      httpRequest.request(
        request.url, {
        expectDataType: http.HttpDataType.ARRAY_BUFFER,
      }, (err: BusinessError, data: http.HttpResponse) => {
        callback(err, {
          binary: data?.result as ArrayBuffer
        });
        httpRequest.destroy();
      });
    } else {
      // local file
      const context: Context = getContext(this);
      const resourceMgr: resourceManager.ResourceManager = context.resourceManager;
      resourceMgr.getRawFileContent(request.url, (err: BusinessError, data: Uint8Array) => {
        callback(err, {
          binary: data?.buffer as ArrayBuffer
        })
      });
    }
  }

  fetchSSRData(request: LynxResourceRequest, callback: AsyncCallback<ArrayBuffer, void>) {
    let httpRequest = http.createHttp();
    httpRequest.request(request.url, {
      expectDataType: http.HttpDataType.ARRAY_BUFFER
    }, (err: BusinessError, data: http.HttpResponse) => {
      callback(err, data?.result as ArrayBuffer)
      httpRequest.destroy();
    })
  }
}