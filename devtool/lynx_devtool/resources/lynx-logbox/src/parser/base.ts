// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { IErrorRecord } from '@/common/interface';

export const E_CODE_BTS = 2;
export const E_CODE_MTS = 11;

export const DEFAULT_CONTEXT_SIZE = 3;
export const MAX_STACK_FRAME_LEN = 15;

export interface IErrorParser {
  parse(errorWrapper: any): Promise<IErrorRecord | null>;
}

export function parseJsonStringSafely(str: string): any {
  if (!str) {
    return null;
  }
  try {
    const json = JSON.parse(str);
    return json;
  } catch (e) {
    console.log('failed to parse string to json object, error:', e);
    return null;
  }
}
