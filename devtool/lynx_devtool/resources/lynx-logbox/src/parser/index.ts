// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
import { IErrorRecord, IErrorParser } from '@lynx-dev/logbox-types';
import { getBridge } from '@/jsbridge';

const UNKNOWN_NAMESPACE = '__UNKNOWN__';

const parsers: Record<string, IErrorParser> = {};

export function registerErrorParser(errNamespace: string, parser: IErrorParser): void {
  parsers[errNamespace] = parser;
}

function constructFallbackErrorRecord(message: string): IErrorRecord {
  return {
    message,
    contextSize: 3,
    rawErrorText: message,
    errorProps: {
      code: -1,
    },
  };
}

class FallbackErrorParser implements IErrorParser {
  async parse(rawError: any): Promise<IErrorRecord | null> {
    return constructFallbackErrorRecord(rawError);
  }
}

export async function parseErrorWrapper(rawData: any): Promise<IErrorRecord | null> {
  const errNamespace = rawData.namespace ?? UNKNOWN_NAMESPACE;
  if (errNamespace !== UNKNOWN_NAMESPACE && !parsers[errNamespace]) {
    await getBridge().loadErrorParser(errNamespace);
  }
  let parser = parsers[errNamespace];
  if (!parser) {
    parser = new FallbackErrorParser();
    parsers[errNamespace] = parser;
  }
  try {
    let res = await parser.parse(rawData.log);
    if (res) {
      return { ...res, rawErrorText: rawData.log };
    }
  } catch (e) {
    console.warn('Exception encountered while parsing raw error:', e);
  }
  return constructFallbackErrorRecord(rawData.log);
}
