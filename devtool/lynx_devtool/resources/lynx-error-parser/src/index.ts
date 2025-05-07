import type { IErrorParser, IErrorRecord } from '@lynx-dev/logbox-types';
import { DEFAULT_CONTEXT_SIZE, parseJsonStringSafely } from './base';
import { BTSErrorParser } from './btsErrorParser';
import { MTSErrorParser } from './mtsErrorParser';
import { DefaultErrorParser } from './defaultParser';

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

class LynxErrorParser implements IErrorParser {
  private childParsers: IErrorParser[] = [];

  constructor() {
    this.childParsers.push(new BTSErrorParser());
    this.childParsers.push(new MTSErrorParser());
    // the default parser should be the last one
    this.childParsers.push(new DefaultErrorParser());
  }

  async parse(rawData: string): Promise<IErrorRecord | null> {
    const json = parseJsonStringSafely(rawData);
    if (!json) {
      console.warn('Failed to parse error, the raw data is:', rawData);
      return constructFallbackErrorRecord(rawData);
    }
    for (const p of this.childParsers) {
      let res;
      try {
        res = await p.parse(json);
      } catch (e) {
        console.warn('Exception encountered while parsing raw error:', e);
      }
      if (res) {
        return { ...res, rawErrorText: rawData };
      }
    }
    return constructFallbackErrorRecord(rawData);
  }
}

if (typeof window !== 'undefined' && window.logBoxCore) {
  window.logBoxCore.registerErrorParser('lynx', new LynxErrorParser());
}
