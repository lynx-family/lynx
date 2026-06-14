// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { toNumberValue } from './VideoOptions';

export interface LynxVideoMethodCodes {
  success: number;
  paramInvalid: number;
  operationError: number;
}

export class LynxVideoMethodErrorResponse {
  errorMsg: string;

  constructor(errorMsg: string) {
    this.errorMsg = errorMsg;
  }
}

export function parseVideoSeekPosition(params: Object | null | undefined): number | undefined {
  if (params === null || params === undefined) {
    return undefined;
  }
  const positionParam = (params as Record<string, Object>).position;
  if (positionParam === undefined || positionParam === null) {
    return undefined;
  }
  const position = toNumberValue(positionParam, Number.NaN);
  if (Number.isNaN(position) || position < 0) {
    return undefined;
  }
  return position;
}

export function respondToVideoMethod(
  callback: (code: number, res: Object) => void,
  promise: Promise<void>,
  codes: LynxVideoMethodCodes
): void {
  promise
    .then(() => {
      callback(codes.success, new Object());
    })
    .catch((error: Error) => {
      const message = error.message === undefined ? `${error}` : error.message;
      const code = methodErrorCodeForMessage(message, codes);
      callback(code, new LynxVideoMethodErrorResponse(message));
    });
}

export function methodErrorCodeForMessage(message: string, codes: LynxVideoMethodCodes): number {
  return message.indexOf('out of range') >= 0 ? codes.paramInvalid : codes.operationError;
}
