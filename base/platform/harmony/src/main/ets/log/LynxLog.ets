// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import hilog from '@ohos.hilog';
import BuildProfile from '../../../../BuildProfile';
import lynxbase from 'liblynxbase.so';
import { LynxBaseServiceCenter, LynxBaseServiceType } from '../service/LynxBaseServiceCenter';
import { ILynxBaseLogService } from '../service/ILynxBaseLogService';

const DOMAIN = 0xff00;

export enum LynxBaseLogLevel {
  VERBOSE = 0,
  DEBUG = 1,
  INFO = 2,
  WARN = 3,
  ERROR = 4,
}

export class LynxLog {
  private static format: string = '%{public}s';
  private static mMinLogLevel: number = BuildProfile.DEBUG ? LynxBaseLogLevel.DEBUG : LynxBaseLogLevel.INFO;
  private static mService: ILynxBaseLogService | undefined;
  private static mUseSysLog: boolean = false;

  static useSysLog(useSysLog: boolean): void {
    LynxLog.mUseSysLog = useSysLog;
    lynxbase.nativeUseSysLog(LynxLog.mUseSysLog);
  }

  static initLynxLog(isPrintLogToAllChannel: boolean): void {
    LynxLog.mService = LynxBaseServiceCenter.getService<ILynxBaseLogService>(LynxBaseServiceType.Log);
    if (LynxLog.mService) {
      if (LynxLog.mService.isLogOutputByPlatform()) {
        LynxLog.useSysLog(true);
      }
      lynxbase.nativeInitLynxLogWriteFunction(LynxLog.mService.getDefaultWriteFunction());
    }
    lynxbase.nativeInitLynxLog(isPrintLogToAllChannel);
  }

  static setMinimumLoggingLevel(level: LynxBaseLogLevel): void {
    let logLevelName: Array<string> = ["DEBUG", "INFO", "WARN", "ERROR"];
    if (LynxLog.mMinLogLevel < level) {
      LynxLog.mMinLogLevel = level;
      lynxbase.nativeSetMinLogLevel(level);
      hilog.warn(DOMAIN, "lynxbase", LynxLog.format, `Reset minimum log level as ${logLevelName[LynxLog.mMinLogLevel]}`);
    } else {
      hilog.warn(DOMAIN, "lynxbase", LynxLog.format,
        `Please set a log level higher than ${logLevelName[LynxLog.mMinLogLevel]}} to filter lynx logs!`);
    }
  }

  static v(tag: string, message: string): void {
    LynxLog.internalLynxLog(LynxBaseLogLevel.VERBOSE, tag, message);
  }

  static d(tag: string, message: string): void {
    LynxLog.internalLynxLog(LynxBaseLogLevel.DEBUG, tag, message);
  }

  static i(tag: string, message: string): void {
    LynxLog.internalLynxLog(LynxBaseLogLevel.INFO, tag, message);
  }

  static w(tag: string, message: string): void {
    LynxLog.internalLynxLog(LynxBaseLogLevel.WARN, tag, message);
  }

  static e(tag: string, message: string): void {
    LynxLog.internalLynxLog(LynxBaseLogLevel.ERROR, tag, message);
  }

  static r(tag: string, message: string): void {
    LynxLog.internalLynxLog(LynxBaseLogLevel.ERROR, tag, message);
  }

  private static internalLynxLog(level: LynxBaseLogLevel, tag: string, message: string): void {
    if (level >= LynxLog.mMinLogLevel) {
      if (LynxLog.mUseSysLog && LynxLog.mService) {
        LynxLog.mService.logByPlatform(level, tag, message);
      }
      lynxbase.nativeInternalLog(level, tag, message);
    }
  }
}
