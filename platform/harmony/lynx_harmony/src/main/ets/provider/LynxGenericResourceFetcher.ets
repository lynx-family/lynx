// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { LynxResourceRequest } from './LynxResourceRequest';
import { LynxResourceProvider } from './LynxResourceProvider';
import { LynxStreamDelegate } from './LynxStreamDelegate';
import { AsyncCallback } from '@ohos.base';

/**
 * Generic Resource Provider for Lynx.
 */
export abstract class LynxGenericResourceFetcher implements LynxResourceProvider {
  /**
   * fetch resource with contents.
   *
   * @param request
   * @param callback containing contents of the requiring resource
   */
  abstract fetchResource(request: LynxResourceRequest, callback: AsyncCallback<ArrayBuffer>);

  /**
   * fetch resource with resource path
   *
   * @param request
   * @param callback with path on the disk of the requiring resource
   */
  abstract fetchResourcePath(request: LynxResourceRequest, callback: AsyncCallback<string>);

  /**
   * fetch resource with stream.
   *
   * @optional user may impl this method if stream loading is necessary.
   * @param request
   * @param delegate streaming of the requiring resource;
   */
  fetchStream(request: LynxResourceRequest, delegate: LynxStreamDelegate) {
  }

  /**
   * cancel the request of the requiring resource.
   *
   * @param request
   */
  abstract cancel(request: LynxResourceRequest);
}