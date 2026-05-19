// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { HttpStreamingDelegate, ILynxHttpService, LynxHttpRequest, LynxHttpResponse } from '@lynx/lynx';
import http from '@ohos.net.http';
import { BusinessError } from '@kit.BasicServicesKit';

export class LynxHttpService implements ILynxHttpService {
  static SDK_ERROR_STATUS_CODE = 499;
  static instance = new LynxHttpService();

  private constructor() {
  }
  
  private methodFromName(methodName: string) {
    if (methodName == 'GET') {
      return http.RequestMethod.GET;
    } else if (methodName == 'POST') {
      return http.RequestMethod.POST;
    }
    return null;
  }

  request(request: LynxHttpRequest, callback: (resp: LynxHttpResponse) => void): void {
    let httpRequest = http.createHttp();
    httpRequest.request(request.url, {
      method: this.methodFromName(request.httpMethod.toUpperCase()),
      header: request.httpHeaders,
      extraData: request.httpBody,
      expectDataType: http.HttpDataType.ARRAY_BUFFER,
    }, (err: BusinessError, resp: http.HttpResponse) => {
      const httpResponse = new LynxHttpResponse();
      httpResponse.url = request.url;
      
      if (!err) {
        httpResponse.statusCode = resp.responseCode;
        const body = resp.result;
        if (body instanceof ArrayBuffer) {
          httpResponse.httpBody = body;
        } else {
          httpResponse.statusCode = LynxHttpService.SDK_ERROR_STATUS_CODE;
          httpResponse.statusText = 'Http body not arraybuffer';
        }
        httpResponse.httpHeaders = resp.header as Record<string, string>;
      } else {
        httpResponse.statusText = err.message;
      }
      callback(httpResponse);
      
      httpRequest.destroy();
    });
  }

  requestStreaming(request: LynxHttpRequest, callback: (resp: LynxHttpResponse) => void,
    delegate: HttpStreamingDelegate): void {
    let httpRequest = http.createHttp();
    let responseHeaders: Record<string, string> = {};
    let enableSSEStreaming = false;
    let hasReceivedHeaders = false;
    httpRequest.on('dataReceive', (data: ArrayBuffer) => {
      if (enableSSEStreaming) {
        delegate.onDataSSE(data);
      } else if (!hasReceivedHeaders) {
        delegate.onDataPending(data);
      } else {
        delegate.onData(data);
      }
    });
    httpRequest.once('headersReceive', (headers: Object) => {
      responseHeaders = headers as Record<string, string>;
      enableSSEStreaming = this.isServerSentEvent(responseHeaders);
      hasReceivedHeaders = true;

      const httpResponse = new LynxHttpResponse();
      httpResponse.url = request.url;
      httpResponse.httpHeaders = responseHeaders;
      httpResponse.statusCode = 200;
      callback(httpResponse);

      delegate.flushPendingData(enableSSEStreaming);
    });
    httpRequest.requestInStream(request.url, {
      method: this.methodFromName(request.httpMethod.toUpperCase()),
      header: request.httpHeaders,
      extraData: request.httpBody,
      expectDataType: http.HttpDataType.ARRAY_BUFFER,
    }, (err: BusinessError, statusCode: number) => {
      if (!err) {
        if (enableSSEStreaming) {
          delegate.flushSSEBuffer();
        }
        delegate.flushPendingData(enableSSEStreaming);
        delegate.onEnd();
      } else {
        if (!hasReceivedHeaders) {
          const httpResponse = new LynxHttpResponse();
          httpResponse.url = request.url;
          httpResponse.httpHeaders = responseHeaders;
          httpResponse.statusCode = LynxHttpService.SDK_ERROR_STATUS_CODE;
          httpResponse.statusText = err.message;
          callback(httpResponse);
        }
        delegate.onError(err.message);
      }

      httpRequest.off('dataReceive');
      httpRequest.destroy();
    });
  }

  private isServerSentEvent(headers: Record<string, string>): boolean {
    for (const key of Object.keys(headers)) {
      if (key.toLowerCase() == 'content-type') {
        return headers[key].toLowerCase().indexOf('text/event-stream') >= 0;
      }
    }
    return false;
  }

}
