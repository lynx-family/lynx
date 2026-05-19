// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { LynxServiceCenter, LynxServiceType } from '../../tasm/service/LynxServiceCenter';
import { ILynxHttpService } from '../../tasm/service/ILynxHttpService';
import { LynxModule } from '../LynxModule';
import { LynxHttpRequest } from './LynxHttpRequest';
import { LynxHttpResponse } from './LynxHttpResponse';
import { HttpStreamingDelegate } from './HttpStreamingDelegate';

const streamingEventNamePrefix = 'LynxFetchModuleStreamingEvent';
const deprecatedStreamingFlag = 'useStreaming';
const standardStreamingFlag = 'enableFetchAPIStandardStreaming';
let streamingCounter = 0;

interface RequestType {
  method: string;
  url: string;
  origin: string;
  body: ArrayBuffer;
  headers: Record<string, string>;
  lynxExtension?: Record<string, Object>;
}

interface ResponseType {
  url: string;
  status: number;
  statusText: string;
  headers: Record<string, string>;
  body: ArrayBuffer;
  lynxExtension?: Record<string, Object>;
}

type ResponseCallBack = (resp: ResponseType) => void;

export class LynxFetchModule extends LynxModule {
  private buildResponse(request: RequestType, resp: LynxHttpResponse, body: ArrayBuffer,
    lynxExtension: Record<string, Object>): ResponseType {
    return {
      body,
      status: resp.statusCode,
      statusText: resp.statusText,
      headers: resp.httpHeaders,
      url: request.url,
      lynxExtension,
    };
  }

  fetch(request: RequestType, callback: ResponseCallBack) {
    const customConfig = request.lynxExtension ?? {};
    const httpRequest = new LynxHttpRequest(
      request.method,
      request.url,
      request.origin,
      request.body,
      request.headers,
      customConfig,
    );
    const httpService = LynxServiceCenter.getService<ILynxHttpService>(LynxServiceType.Http);
    const deprecatedUseStreaming = (customConfig[deprecatedStreamingFlag] as boolean) ?? false;
    const enableFetchAPIStandardStreaming = (customConfig[standardStreamingFlag] as boolean) ?? false;
    if (!deprecatedUseStreaming && !enableFetchAPIStandardStreaming) {
      httpService?.request(httpRequest, (resp: LynxHttpResponse) => {
        callback(this.buildResponse(request, resp, resp.httpBody, resp.customInfo));
      });
      return;
    }

    const streamingId = streamingEventNamePrefix + streamingCounter++;
    const delegate = new HttpStreamingDelegate(streamingId, this.context);
    httpService?.requestStreaming(httpRequest, (resp: LynxHttpResponse) => {
      const customInfo = resp.customInfo ?? {};
      customInfo['streamingId'] = streamingId;
      callback(this.buildResponse(request, resp, resp.httpBody, customInfo));
    }, delegate);
  }
}
