// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { IErrorParser, parseJsonStringSafely } from './base';
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

export async function parseErrorWrapper(rawErrorText: string): Promise<IErrorRecord | null> {
  initAdapters();
  const json = parseJsonStringSafely(rawErrorText);
  if (!json) {
    console.warn('Failed to parse error, the raw string is:', rawErrorText);
    return null;
  }
  for (const a of parsers) {
    const res = await a.parse(json);
    if (res) {
      return { ...res, rawErrorText };
    }
  }
  return null;
}

export * from './base';
