// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { LynxLog, LynxBaseLogLevel } from '@lynx/lynx_base';
import { LynxEnv } from '../tasm/LynxEnv';

export enum LogLevel {
  VERBOSE = 0,
  DEBUG = 1,
  INFO = 2,
  WARN = 3,
  ERROR = 4,
}

export class LLog {
  private static logLevelToLynxBaseLogLevel(level: LogLevel): LynxBaseLogLevel {
    switch (level) {
      case LogLevel.VERBOSE:
        return LynxBaseLogLevel.VERBOSE;
      case LogLevel.DEBUG:
        return LynxBaseLogLevel.DEBUG;
      case LogLevel.INFO:
        return LynxBaseLogLevel.INFO;
      case LogLevel.WARN:
        return LynxBaseLogLevel.WARN;
      case LogLevel.ERROR:
        return LynxBaseLogLevel.ERROR;
      default:
        return LynxBaseLogLevel.VERBOSE;
    }
  }

  static useSysLog(useSysLog: boolean): void {
    LynxLog.useSysLog(useSysLog);
  }

  static initLynxLog(): void {
    LynxLog.initLynxLog(LynxEnv.isDevtoolEnabled());
  }

  static setMinimumLoggingLevel(level: LogLevel): void {
    LynxLog.setMinimumLoggingLevel(LLog.logLevelToLynxBaseLogLevel(level));
  }

  static v(tag: string, message: string): void {
    LynxLog.v(tag, message);
  }

  static d(tag: string, message: string): void {
    LynxLog.d(tag, message);
  }

  static i(tag: string, message: string): void {
    LynxLog.i(tag, message);
  }

  static w(tag: string, message: string): void {
    LynxLog.w(tag, message);
  }

  static e(tag: string, message: string): void {
    LynxLog.e(tag, message);
  }

  static r(tag: string, message: string): void {
    LynxLog.r(tag, message);
  }
}
