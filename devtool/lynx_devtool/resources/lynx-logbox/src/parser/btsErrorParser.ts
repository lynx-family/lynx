// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { IErrorParser, DEFAULT_CONTEXT_SIZE, parseJsonStringSafely, E_CODE_BTS, MAX_STACK_FRAME_LEN } from './base';
import { IErrorProps, IErrorRecord } from '@/common/interface';
import parseStack from '@/utils/stackParser';
import { map } from '@/utils/mapper';
import { getBridge } from '@/jsbridge';

function resourceNameMapping(name: string): string {
  if (name.indexOf('lynx_core.js') !== -1) {
    return 'core.js';
  }
  const lastSlashIndex: number = name.lastIndexOf('/');
  return name.substring(lastSlashIndex + 1);
}

export class BTSErrorParser implements IErrorParser {
  async parse(rawError: any): Promise<IErrorRecord | null> {
    const errorProps: IErrorProps = {
      code: rawError.error_code,
      level: rawError.level,
    };
    const error = rawError.error;
    let code = errorProps.code ?? 0;
    code = Math.floor(code / 100);
    if (code !== E_CODE_BTS) {
      return null;
    }
    const json = parseJsonStringSafely(error);
    if (!json || !json.rawError || !json.rawError.stack) {
      return null;
    }
    errorProps.stack = json.rawError.stack;
    let rawFrames = parseStack(json.rawError);
    if (rawFrames.length > MAX_STACK_FRAME_LEN) {
      rawFrames = rawFrames.slice(0, MAX_STACK_FRAME_LEN);
    }
    // get the real stack line and col number
    const parsedFrames = await map(rawFrames, DEFAULT_CONTEXT_SIZE, {
      async getResource(name: string): Promise<string> {
        const resName = resourceNameMapping(name);
        const res = await getBridge().queryResource(resName);
        return res ?? '';
      },
    });
    return {
      contextSize: DEFAULT_CONTEXT_SIZE,
      message: json.rawError.message,
      stackFrames: parsedFrames,
      errorProps,
    };
  }
}
