// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import hilog from '@ohos.hilog';
import BuildProfile from '../../../../BuildProfile';
import lynx from 'liblynx.so';
import { LynxServiceCenter, LynxServiceType } from '../tasm/service/LynxServiceCenter';
import { ILynxLogService } from '../tasm/service/ILynxLogService';
const DOMAIN = 0xff00;

export enum LogLevel {
  VERBOSE = 0,
  DEBUG = 1,
  INFO = 2,
  WARN = 3,
  ERROR = 4,
}

export class LLog {
  private static format: string = '%{public}s';
  private static mMinLogLevel: number = BuildProfile.DEBUG ? LogLevel.DEBUG : LogLevel.INFO;
  private static mService: ILynxLogService | undefined;
  private static mUseSysLog: boolean = false;

  static useSysLog(useSysLog: boolean): void {
    LLog.mUseSysLog = useSysLog;
    lynx.nativeUseSysLog(LLog.mUseSysLog);
  }

  static initLynxLog(isPrintLogToAllChannel: boolean): void {
    if (LLog.mService) {
      if (LLog.mService.isLogOutputByPlatform()) {
        LLog.useSysLog(true);
      }
      lynx.nativeInitLynxLogWriteFunction(LLog.mService.getDefaultWriteFunction());
    }
    lynx.nativeInitLynxLog(isPrintLogToAllChannel);
  }

  static setMinimumLoggingLevel(level: LogLevel): void {
    let logLevelName: Array<string> = ["DEBUG", "INFO", "WARN", "ERROR"];
    if (LLog.mMinLogLevel < level) {
      LLog.mMinLogLevel = level;
      lynx.nativeSetMinLogLevel(level);
      hilog.warn(DOMAIN, "lynx", LLog.format, `Reset minimum log level as ${logLevelName[LLog.mMinLogLevel]}`);
    } else {
      hilog.warn(DOMAIN, "lynx", LLog.format,
        `Please set a log level higher than ${logLevelName[LLog.mMinLogLevel]}} to filter lynx logs!`);
    }
  }

  static v(tag: string, message: string): void {
    LLog.internalLog(LogLevel.VERBOSE, tag, message);
  }

  static d(tag: string, message: string): void {
    LLog.internalLog(LogLevel.DEBUG, tag, message);
  }

  static i(tag: string, message: string): void {
    LLog.internalLog(LogLevel.INFO, tag, message);
  }

  static w(tag: string, message: string): void {
    LLog.internalLog(LogLevel.WARN, tag, message);
  }

  static e(tag: string, message: string): void {
    LLog.internalLog(LogLevel.ERROR, tag, message);
  }

  static r(tag: string, message: string): void {
    LLog.internalLog(LogLevel.ERROR, tag, message);
  }

  private static internalLog(level: LogLevel, tag: string, message: string): void {
    if (level >= LLog.mMinLogLevel) {
      if (LLog.mUseSysLog && LLog.mService) {
        LLog.mService.logByPlatform(level, tag, message);
      }
      lynx.nativeInternalLog(level, tag, message);
    }
  }
}