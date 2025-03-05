// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { DEFAULT_CONTEXT_SIZE, IErrorParser, parseJsonStringSafely } from './base';
import { IErrorRecord } from '@/common/interface';
import { BTSErrorParser } from './btsErrorParser';
import { MTSErrorParser } from './mtsErrorParser';
import { DefaultErrorParser } from './defaultParser';

let adapterInited = false;
const parsers: IErrorParser[] = [];

function initAdapters(): void {
  if (adapterInited) {
    return;
  }
  adapterInited = true;
  parsers.push(new BTSErrorParser());
  parsers.push(new MTSErrorParser());
  // the default parser should be the last one
  parsers.push(new DefaultErrorParser());
}

function constructFallbackErrorRecord(message: string): IErrorRecord {
  return {
    message,
    contextSize: DEFAULT_CONTEXT_SIZE,
    rawErrorText: message,
    errorProps: {
      code: -1,
    },
  };
}

export async function parseErrorWrapper(rawErrorText: string): Promise<IErrorRecord | null> {
  initAdapters();
  const json = parseJsonStringSafely(rawErrorText);
  if (!json) {
    console.warn('Failed to parse error, the raw string is:', rawErrorText);
    return constructFallbackErrorRecord(rawErrorText);
  }
  for (const a of parsers) {
    let res;
    try {
      res = await a.parse(json);
    } catch (e) {
      console.warn('Exception encountered while parsing raw error:', e);
    }
    if (res) {
      return { ...res, rawErrorText };
    }
  }
  return constructFallbackErrorRecord(rawErrorText);
}

export * from './base';
