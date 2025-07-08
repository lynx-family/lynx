// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import {
  LynxError,
  LynxGenericResourceFetcher,
  LynxResourceRequest,
  LynxResourceType,
  LynxStreamDelegate,
  LynxSubErrorCode
} from '@lynx/lynx';
import { AsyncCallback, BusinessError } from '@ohos.base';
import http from '@ohos.net.http';
import { CacheStrategy, ImageKnife, ImageKnifeOption } from '@ohos/imageknife';

export class ExampleGenericResourceFetcher extends LynxGenericResourceFetcher {
  fetchResource(request: LynxResourceRequest, callback: AsyncCallback<ArrayBuffer, void>): void {
    let httpRequest = http.createHttp();
    httpRequest.request(request.url, {
      expectDataType: http.HttpDataType.ARRAY_BUFFER
    }, (err: BusinessError, data: http.HttpResponse) => {
      callback(err, data?.result as ArrayBuffer)
      httpRequest.destroy();
    })
  }

  fetchResourcePath(request: LynxResourceRequest, callback: AsyncCallback<string, void>): void {
    if (request.type === LynxResourceType.LYNX_RESOURCE_TYPE_IMAGE) {
      let option = new ImageKnifeOption();
      option.loadSrc = request.url;
      option.writeCacheStrategy = CacheStrategy.File;
      let error: BusinessError | undefined = undefined;
      ImageKnife.getInstance().preLoadCache(option).then((data: string) => {
        if (data.length > 0) {
          callback(error, data);
        } else {
          error = {
            code: LynxSubErrorCode.E_RESOURCE_IMAGE_PIC_SOURCE,
            message: 'Image path is invalid',
            name: 'Image Error',
          }
          callback(error, '');
        }
      }).catch((e: string) => {
        error = {
          code: LynxSubErrorCode.E_RESOURCE_IMAGE_FROM_NETWORK_OR_OTHERS,
          message: e,
          name: 'Image Error',
        }
        callback(error, '');
      })
    } else {
      callback({
        code: LynxError.LYNX_ERROR_CODE_RESOURCE,
        message: 'unsupported type: ' + request.type,
        name: 'Resource Error',
      }, '');
    }
  }

  fetchStream(request: LynxResourceRequest, delegate: LynxStreamDelegate): void {
    // TODO(nihao.royal): support fetching stream.
    delegate.onStart(100);
    let a = new ArrayBuffer(10);
    delegate.onData(a, 0, 10);
    delegate.onEnd();
  }

  cancel(request: LynxResourceRequest): void {
    // TODO(nihao.royal)
  }
}
