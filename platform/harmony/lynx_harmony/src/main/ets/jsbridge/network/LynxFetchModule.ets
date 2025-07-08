// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { LynxServiceCenter, LynxServiceType } from '../../tasm/service/LynxServiceCenter';
import { ILynxHttpService } from '../../tasm/service/ILynxHttpService';
import { LynxModule } from '../LynxModule';
import { LynxHttpRequest } from './LynxHttpRequest';
import { LynxHttpResponse } from './LynxHttpResponse';

const TAG = 'LynxFetchModule'

interface RequestType {
  method: string;
  url: string;
  origin: string;
  body: ArrayBuffer;
  headers: Record<string, string>;
}

interface ResponseType {
  url: string;
  status: number;
  statusText: string;
  headers: Record<string, string>;
  body: ArrayBuffer;
}

type ResponseCallBack = (resp: ResponseType) => void;

export class LynxFetchModule extends LynxModule {
  fetch(request: RequestType, callback: ResponseCallBack) {
    const httpRequest = new LynxHttpRequest(
      request.method,
      request.url,
      request.origin,
      request.body,
      request.headers,
      {},
    );
    LynxServiceCenter.getService<ILynxHttpService>(LynxServiceType.Http)?.request(httpRequest,
      (resp: LynxHttpResponse) => {
        callback({
          body: resp.httpBody,
          status: resp.statusCode,
          statusText: resp.statusText,
          headers: resp.httpHeaders,
          url: request.url,
        });
      });
  }
}
