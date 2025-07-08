// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

export class LynxHttpResponse {
  public url: string = '';
  public statusCode: number = 0;
  public statusText: string = 'OK';
  public httpBody: ArrayBuffer = new ArrayBuffer(0);
  public httpHeaders: Record<string, string> = {};
  public customInfo: Record<string, Object> = {};
}