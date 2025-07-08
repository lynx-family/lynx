// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

export class LynxHttpRequest {
  public httpMethod: string;
  public url: string;
  public originUrl: string;
  public httpBody: ArrayBuffer;
  public httpHeaders: Record<string, string>;
  public customConfig: Record<string, Object>;

  constructor(
    httpMethod: string,
    url: string,
    originUrl: string,
    httpBody: ArrayBuffer,
    httpHeaders: Record<string, string>,
    customConfig: Record<string, Object>,
  ) {
    this.httpMethod = httpMethod;
    this.url = url;
    this.originUrl = originUrl;
    this.httpBody = httpBody;
    this.httpHeaders = httpHeaders;
    this.customConfig = customConfig;
  }
}