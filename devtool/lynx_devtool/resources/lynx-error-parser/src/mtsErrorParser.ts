// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
import { DEFAULT_CONTEXT_SIZE, parseJsonStringSafely, E_CODE_MTS, MAX_STACK_FRAME_LEN } from './base';
import type { StackFrame, IErrorProps, IErrorRecord, IErrorParser, IResourceProvider } from '@lynx-dev/logbox-types';

interface IMTJSErrorInfo {
  message: string;
  stack: string;
  debugInfoUrl: string;
}

interface MTSParsedFrames {
  frames: StackFrame[];
  resourceProvider: MTSResourceProvider;
  callerInfo: string;
}

const COMPACT_DEBUG_INFO_VERSION = 2;

const ROOT_DEBUG_INFO_KEYS = {
  function_source: 'fs',
  end_line_num: 'el',
  function_number: 'fn',
  function_info: 'fi',
} as const;

const FUNCTION_DEBUG_INFO_KEYS = {
  function_id: 'id',
  function_name: 'n',
  file_name: 'f',
  line_number: 'ln',
  column_number: 'cn',
  pc2line_len: 'pll',
  pc2line_buf: 'plb',
  pc2caller_info: 'pci',
  function_source_len: 'fsl',
  function_source_offset: 'fso',
  function_source: 'fs',
} as const;

const LINE_COL_DEBUG_INFO_KEYS = {
  line: 'l',
  column: 'c',
} as const;

function getDebugInfoValue(debugInfo: any, key: string, compactKey: string): any {
  return debugInfo?.[compactKey] ?? debugInfo?.[key];
}

function normalizeDebugInfoKeys(debugInfo: any, keyMap: Record<string, string>): any {
  if (!debugInfo || typeof debugInfo !== 'object') {
    return debugInfo;
  }
  const normalizedDebugInfo = { ...debugInfo };
  for (const key of Object.keys(keyMap)) {
    const compactKey = keyMap[key];
    const value = getDebugInfoValue(debugInfo, key, compactKey);
    if (value !== undefined) {
      normalizedDebugInfo[key] = value;
    }
  }
  return normalizedDebugInfo;
}

function normalizeFunctionDebugInfo(functionInfo: any): any {
  const normalizedFunctionInfo = normalizeDebugInfoKeys(functionInfo, FUNCTION_DEBUG_INFO_KEYS);
  if (!normalizedFunctionInfo || typeof normalizedFunctionInfo !== 'object') {
    console.error('Failed to normalize function debug info caused by invalid function info');
    return null;
  }

  const lineCol = getDebugInfoValue(functionInfo, 'line_col', 'lc');
  if (Array.isArray(lineCol)) {
    normalizedFunctionInfo.line_col = lineCol.map((position: any) => normalizeDebugInfoKeys(position, LINE_COL_DEBUG_INFO_KEYS));
  }

  return normalizedFunctionInfo;
}

function normalizeLepusNGDebugInfoEntry(debugInfoEntry: any): any {
  const version = Number.isInteger(debugInfoEntry?.v) ? debugInfoEntry.v : null;
  if (version !== null && version > COMPACT_DEBUG_INFO_VERSION) {
    console.error(`Unsupported debug-info schema version: ${version}`);
    return null;
  }

  const normalizedDebugInfoEntry = normalizeDebugInfoKeys(debugInfoEntry, ROOT_DEBUG_INFO_KEYS);
  if (!normalizedDebugInfoEntry || typeof normalizedDebugInfoEntry !== 'object') {
    console.error('Failed to normalize debug info caused by invalid debug info entry');
    return null;
  }

  const functionInfoList = normalizedDebugInfoEntry.function_info;
  if (Array.isArray(functionInfoList)) {
    normalizedDebugInfoEntry.function_info = functionInfoList.map((functionInfo: any) => normalizeFunctionDebugInfo(functionInfo));
  }
  return normalizedDebugInfoEntry;
}

class MTSResourceProvider implements IResourceProvider {
  resourceMap: Map<string, any>;

  constructor() {
    this.resourceMap = new Map();
  }
  setResource(resource: any, fileName: string) {
    if (this.resourceMap.has(fileName) && !this.resourceMap.get(fileName)) {
      return;
    }
    this.resourceMap.set(fileName, resource);
  }

  async getResource(name: string): Promise<string> {
    return this.resourceMap.get(name) ?? '';
  }
}

export class MTSErrorParser implements IErrorParser {
  debugInfo: Map<string, any>;

  constructor() {
    this.debugInfo = new Map();
  }

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
    if (errorInfo.debugInfoUrl) {
      const debugInfo = await window.logBoxCore.queryResource(errorInfo.debugInfoUrl);
      this.debugInfo.set('main-thread.js', debugInfo);
    }
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

    const { frames, resourceProvider, callerInfo } = await this.getStackFramesInProduction(rawFrames);

    // fill in the call info for the error
    if (errorRecord.message.includes('not a function') && callerInfo) {
      errorRecord.message = errorRecord.message + ':' + callerInfo;
    }

    const parsedFrames = await window.logBoxCore.map(frames, DEFAULT_CONTEXT_SIZE, resourceProvider);
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

  getCallerInfo(functionId: number | null, pcIndex: number | null, functionInfoList: any): string {
    if (functionId == null || pcIndex == null) {
      console.error('Failed to get caller info caused by invalid function id or pc index');
      return '';
    }
    if (functionInfoList) {
      const functionInfo = functionInfoList.find((info: any) => info.function_id == functionId);
      const callerInfo = functionInfo?.pc2caller_info;
      if (callerInfo && pcIndex in callerInfo) {
        return callerInfo[pcIndex];
      }
      console.error('Cannot find field pc2caller_info in debug info');
    }
    console.error('Failed to get caller info');
    return '';
  }

  async getStackFramesInProduction(frames: StackFrame[]): Promise<MTSParsedFrames> {
    let resourceProvider = new MTSResourceProvider();
    let callerInfo = '';
    const parsedFrames: StackFrame[] = [];
    for (let index = 0; index < frames.length; index++) {
      const frame = frames[index];
      if (!frame.fileName) {
        frame.fileName = 'main-thread.js';
      }

      let debugInfo = null;
      if (this.debugInfo.has(frame.fileName)) {
        debugInfo = this.debugInfo.get(frame.fileName);
      } else {
        debugInfo = await window.logBoxCore.queryResource(frame.fileName);
        this.debugInfo.set(frame.fileName, debugInfo);
      }
      const debugInfoJson = parseJsonStringSafely(debugInfo);
      if (!debugInfoJson) {
        console.error('Failed to parse main thread js error caused by invalid debug info');
        parsedFrames.push(frame);
        continue;
      }

      const { functionInfoList, functionSource } = this.getFunctionInfo(debugInfoJson, frame.fileName);
      resourceProvider.setResource(functionSource ?? '', frame.fileName);

      if (!functionInfoList) {
        parsedFrames.push(frame);
        continue;
      }

      if (index === 0) {
        callerInfo = this.getCallerInfo(frame.lineNumber, frame.columnNumber, functionInfoList);
      }

      const functionId = frame.lineNumber ?? -1;
      const pcIndex = frame.columnNumber ?? -1;
      const fInfo = functionInfoList.find((info: any) => info.function_id === functionId);
      if (!fInfo || pcIndex === -1) {
        parsedFrames.push(frame);
        continue;
      }
      const lineCol = fInfo.line_col;
      if (lineCol && lineCol.length > pcIndex) {
        const pos = lineCol[pcIndex];
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
      parsedFrames.push(frame);
      continue;
    }
    return Promise.resolve({
      frames: parsedFrames,
      resourceProvider,
      callerInfo,
    });
  }

  getFunctionInfo(debugInfoJson: any, fileName: string): any {
    let debugInfoEntry = null;
    for (const key in debugInfoJson) {
      if (fileName.includes(key)) {
        debugInfoEntry = debugInfoJson[key];
        break;
      }
    }
    if (debugInfoEntry === null) {
      if (debugInfoJson.lepusNG_debug_info) {
        debugInfoEntry = debugInfoJson.lepusNG_debug_info;
      } else if (debugInfoJson.lepus_debug_info) {
        // compatible with legacy format for debug info
        debugInfoEntry = debugInfoJson.lepus_debug_info;
      }
    }

    debugInfoEntry = normalizeLepusNGDebugInfoEntry(debugInfoEntry);
    let functionInfoList = null;
    let functionSource = null;
    if (debugInfoEntry) {
      functionInfoList = debugInfoEntry.function_info;
      if (debugInfoEntry.function_source) {
        functionSource = debugInfoEntry.function_source;
      } else if (functionInfoList && functionInfoList.length > 0) {
        functionSource = functionInfoList[0].function_source;
      }
    }

    if (!functionSource) {
      console.log('getFunctionInfo: cannot find function source in debug info');
    }
    if (!functionInfoList) {
      console.log('getFunctionInfo: cannot find function info in debug info');
    }

    return { functionInfoList, functionSource };
  }
}
