// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { IErrorParser, DEFAULT_CONTEXT_SIZE, parseJsonStringSafely, E_CODE_MTS, MAX_STACK_FRAME_LEN } from './base';
import { IErrorProps, IErrorRecord } from '@/common/interface';
import parseStack from '@/utils/stackParser';
import { getBridge } from '@/jsbridge';
import { map, IResourceProvider } from '@/utils/mapper';
import StackFrame from '@/common/stackFrame';

interface IMTJSErrorInfo {
  message: string;
  stack: string;
  debugInfoUrl: string;
}

class MTSResourceProvider implements IResourceProvider {
  debugInfo: any;

  constructor(debugInfo: any) {
    this.debugInfo = debugInfo;
  }

  async getResource(name: string): Promise<string> {
    if (!this.debugInfo) {
      return '';
    }
    if (this.debugInfo.lepusNG_debug_info) {
      return this.debugInfo.lepusNG_debug_info.function_source ?? null;
    } else if (this.debugInfo.lepus_debug_info) {
      // compatible with legacy format for debug info
      return this.debugInfo.lepus_debug_info.function_source ?? null;
    }
    return '';
  }
}

export class MTSErrorParser implements IErrorParser {
  async parse(rawError: any): Promise<IErrorRecord | null> {
    const errorProps: IErrorProps = {
      code: rawError.error_code,
      level: rawError.level,
    };
    const error = rawError.error;
    const code = errorProps.code ?? 0;
    const highCode = Math.floor(code / 100);
    if (highCode !== E_CODE_MTS) {
      return null;
    }
    const json = parseJsonStringSafely(error);
    let originalErrorInfo: string = error;
    if (json && json.rawError && json.rawError.cause && json.rawError.cause.cause) {
      originalErrorInfo = json.rawError.cause.cause;
    }
    const errorInfo = this.extractErrorInfo(originalErrorInfo);
    errorProps.stack = errorInfo.stack;
    // get the origin stack frames
    let rawFrames = parseStack(errorInfo.stack);
    if (rawFrames.length > MAX_STACK_FRAME_LEN) {
      rawFrames = rawFrames.slice(0, MAX_STACK_FRAME_LEN);
    }
    const errorRecord: IErrorRecord = {
      contextSize: DEFAULT_CONTEXT_SIZE,
      message: errorInfo.message,
      stackFrames: rawFrames,
      errorProps,
    };
    if (!errorInfo.debugInfoUrl) {
      return errorRecord;
    }
    const debugInfo = await getBridge().queryResource(errorInfo.debugInfoUrl);
    const debugInfoJson = parseJsonStringSafely(debugInfo);
    if (!debugInfoJson) {
      console.log('Failed to parse main thread js error caused by invalid debug info');
      return errorRecord;
    }
    rawFrames = this.getStackFramesInProduction(rawFrames, debugInfoJson);
    const parsedFrames = await map(rawFrames, DEFAULT_CONTEXT_SIZE, new MTSResourceProvider(debugInfoJson));
    errorRecord.stackFrames = parsedFrames;
    return errorRecord;
  }

  extractErrorInfo(str: string): IMTJSErrorInfo {
    const [error, debugUrl] = str.split('template_debug_url:');
    const [message, stack] = error.split('backtrace:');
    return {
      message,
      stack,
      debugInfoUrl: debugUrl,
    };
  }

  getStackFramesInProduction(frames: StackFrame[], debugInfoJson: any): StackFrame[] {
    let debugInfo = null;
    if (debugInfoJson.lepusNG_debug_info) {
      debugInfo = debugInfoJson.lepusNG_debug_info;
    } else if (debugInfoJson.lepus_debug_info) {
      // compatible with legacy format for debug info
      debugInfo = debugInfoJson.lepus_debug_info;
    }
    const functionInfoList = debugInfo ? debugInfo.function_info : null;
    const parsedFrames = frames.map((frame) => {
      if (!frame.fileName) {
        frame.fileName = 'main-thread.js';
      }
      if (!functionInfoList) {
        return frame;
      }
      const function_id = frame.lineNumber ?? -1;
      const pc_index = frame.columnNumber ?? -1;
      const fInfo = functionInfoList.find((info: any) => info.function_id === function_id);
      if (!fInfo || pc_index === -1) {
        return frame;
      }
      if (fInfo.line_col && fInfo.line_col.length > pc_index) {
        const pos = fInfo.line_col[pc_index];
        frame.lineNumber = pos.line ?? frame.lineNumber;
        frame.columnNumber = pos.column ?? frame.columnNumber;
      } else if (fInfo.line_col_info && fInfo.line_col_info.line_col && fInfo.line_col_info.line_col.length > pc_index) {
        // compatible with legacy format for debug info
        const pos = fInfo.line_col_info.line_col.at(pc_index);
        const line = pos.line ?? -1;
        const column = pos.column ?? -1;
        const shift = 16;
        if (line === 0 && column > 1 << shift) {
          frame.lineNumber = (column >> shift) & 0xffff;
          frame.columnNumber = column & 0xffff;
        }
      }
      return frame;
    });
    return parsedFrames;
  }
}
