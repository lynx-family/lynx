// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { LynxHttpRequest } from '../../jsbridge/network/LynxHttpRequest';
import { LynxHttpResponse } from '../../jsbridge/network/LynxHttpResponse';
import { HttpStreamingDelegate } from '../../jsbridge/network/HttpStreamingDelegate';
import { IServiceProvider } from './IServiceProvider';

export interface ILynxHttpService extends IServiceProvider {
  request(request: LynxHttpRequest, callback: (resp: LynxHttpResponse) => void): void;
  requestStreaming(request: LynxHttpRequest, callback: (resp: LynxHttpResponse) => void,
    delegate: HttpStreamingDelegate): void;
}
