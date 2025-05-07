// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
import { DEFAULT_CONTEXT_SIZE, parseJsonStringSafely, E_CODE_MTS, MAX_STACK_FRAME_LEN } from './base';
import { IErrorProps, IErrorRecord, IErrorParser, IResourceProvider } from '@lynx-dev/logbox-types';
import type { StackFrame } from '@lynx-dev/logbox-types';

interface IMTJSErrorInfo {
  message: string;
  stack: string;
  debugInfoUrl: string;
}

class MTSResourceProvider implements IResourceProvider {
  debugInfo: any;

  constructor(debugInfo: any) {
    this.debugInfo = debugInfo;
    if (!debugInfo) {
      console.log('MTSResourceProvider: init without debug info');
    }
  }

  async getResource(name: string): Promise<string> {
    if (!this.debugInfo) {
      return '';
    }
    if (this.debugInfo.lepusNG_debug_info) {
      return this.debugInfo.lepusNG_debug_info.function_source ?? null;
    } else if (this.debugInfo.lepus_debug_info) {
      // compatible with legacy format for debug info
      const functionInfoList = this.debugInfo.lepus_debug_info.function_info;
      if (functionInfoList && functionInfoList.length > 0) {
        return functionInfoList[0].function_source ?? null;
      }
      console.log('MTSResourceProvider: invalid function info in debug info');
    }
    console.log('MTSResourceProvider: cannot find function source in debug info');
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
    let rawFrames = window.logBoxCore.parseStack(errorInfo.stack);
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
    const debugInfo = await window.logBoxCore.queryResource(errorInfo.debugInfoUrl);
    const debugInfoJson = parseJsonStringSafely(debugInfo);
    if (!debugInfoJson) {
      console.log('Failed to parse main thread js error caused by invalid debug info');
      return errorRecord;
    }
    // fill in the call info for the error
    if (errorRecord.message.includes('not a function') && rawFrames.length > 0) {
      const rawFrame = rawFrames[0];
      const callerInfo = this.getCallerInfo(rawFrame.lineNumber, rawFrame.columnNumber, debugInfoJson);
      if (callerInfo) {
        errorRecord.message = errorRecord.message + ':' + callerInfo;
      }
    }
    rawFrames = this.getStackFramesInProduction(rawFrames, debugInfoJson);
    const parsedFrames = await window.logBoxCore.map(rawFrames, DEFAULT_CONTEXT_SIZE, new MTSResourceProvider(debugInfoJson));
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

  getCallerInfo(functionId: number | null, pcIndex: number | null, debugInfoJson: any): string {
    if (!functionId || !pcIndex) {
      console.log('Failed to get caller info caused by invalid function id or pc index');
      return '';
    }
    let debugInfo = null;
    if (debugInfoJson.lepusNG_debug_info) {
      debugInfo = debugInfoJson.lepusNG_debug_info;
    } else if (debugInfoJson.lepus_debug_info) {
      // compatible with legacy format for debug info
      debugInfo = debugInfoJson.lepus_debug_info;
    }
    const functionInfoList = debugInfo ? debugInfo.function_info : null;
    if (functionInfoList) {
      const functionInfo = functionInfoList.find((info: any) => info.function_id == functionId);
      if (functionInfo && functionInfo.pc2caller_info && pcIndex in functionInfo.pc2caller_info) {
        return functionInfo.pc2caller_info[pcIndex];
      }
      console.log('Cannot find field pc2caller_info in debug info');
    }
    console.log('Failed to get caller info');
    return '';
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
      const functionId = frame.lineNumber ?? -1;
      const pcIndex = frame.columnNumber ?? -1;
      const fInfo = functionInfoList.find((info: any) => info.function_id === functionId);
      if (!fInfo || pcIndex === -1) {
        return frame;
      }
      if (fInfo.line_col && fInfo.line_col.length > pcIndex) {
        const pos = fInfo.line_col[pcIndex];
        frame.lineNumber = pos.line ?? frame.lineNumber;
        frame.columnNumber = pos.column ?? frame.columnNumber;
      } else if (fInfo.line_col_info && fInfo.line_col_info.line_col && fInfo.line_col_info.line_col.length > pcIndex) {
        // compatible with legacy format for debug info
        const pos = fInfo.line_col_info.line_col[pcIndex];
        const line = pos.line ?? -1;
        const column = pos.column ?? -1;
        const shift = 16;
        if (line === 0 && column > 1 << shift) {
          frame.lineNumber = (column >> shift) & 0xffff;
          frame.columnNumber = column & 0xffff;
        } else if (line > 0 && column > 0) {
          frame.lineNumber = line;
          frame.columnNumber = column;
        }
      }
      return frame;
    });
    return parsedFrames;
  }
}
