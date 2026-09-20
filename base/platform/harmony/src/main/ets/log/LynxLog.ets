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

// Internal INFO sublevels. NOTE: 0 means original INFO; future sublevels must be nonzero.
enum LynxInfoLogLevel { MONITOR = 5, OBSERVE = 10, INFO = 15 }

export class LynxLog {
  private static mInfoLogLevel: number = BuildProfile.DEBUG ? LynxInfoLogLevel.MONITOR : LynxInfoLogLevel.INFO;
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
    lynxbase.nativeInitLynxLog(isPrintLogToAllChannel, LynxLog);
  }

  static setMinimumLoggingLevel(level: LynxBaseLogLevel): void {
    LynxLog.updateLogLevels(level);
    lynxbase.nativeSetMinLogLevel(level);
    hilog.warn(DOMAIN, "lynxbase", LynxLog.format, `Reset minimum log level as ${LynxLog.mMinLogLevel}`);
  }

  private static updateLogLevels(level: LynxBaseLogLevel): void {
    const infoLevel = level >>> 16;
    if (level >= 0 && (level & 0xffff) == LynxBaseLogLevel.INFO && infoLevel != 0) {
      LynxLog.mMinLogLevel = LynxBaseLogLevel.INFO;
      LynxLog.mInfoLogLevel = Math.max(LynxInfoLogLevel.MONITOR, Math.min(LynxInfoLogLevel.INFO, infoLevel));
      return;
    }
    LynxLog.mMinLogLevel = Math.max(LynxBaseLogLevel.VERBOSE, Math.min(LynxBaseLogLevel.ERROR, level));
    LynxLog.mInfoLogLevel = LynxLog.mMinLogLevel < LynxBaseLogLevel.INFO
      ? LynxInfoLogLevel.MONITOR : LynxInfoLogLevel.INFO;
  }

  static v(tag: string, message: string): void {
    LynxLog.internalLynxLog(LynxBaseLogLevel.VERBOSE, tag, message);
  }

  static d(tag: string, message: string): void {
    LynxLog.internalLynxLog(LynxBaseLogLevel.DEBUG, tag, message);
  }

  // TODO: Preserve M/O identity if output consumers need it in the future.
  static m(tag: string, message: string): void {
    if (LynxLog.mInfoLogLevel <= LynxInfoLogLevel.MONITOR) { LynxLog.i(tag, message); }
  }
  static o(tag: string, message: string): void {
    if (LynxLog.mInfoLogLevel <= LynxInfoLogLevel.OBSERVE) { LynxLog.i(tag, message); }
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
