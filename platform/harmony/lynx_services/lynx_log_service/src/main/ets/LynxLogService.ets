// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import hilog from '@ohos.hilog';
import { ILynxBaseLogService, LynxBaseLogLevel, LogOutputChannelType } from '@lynx/lynx_base';

const DOMAIN = 0xff00;

export class LynxLogService implements ILynxBaseLogService {
  private static format: string = '%{public}s';
  static instance = new LynxLogService();
  static logOutputChannel: LogOutputChannelType = LogOutputChannelType.Platform

  private LynxLogService() {
  }

  logByPlatform(level: number, tag: string, message: string): void {
    switch (level) {
      case LynxBaseLogLevel.VERBOSE:
      case LynxBaseLogLevel.DEBUG:
        hilog.debug(DOMAIN, tag, LynxLogService.format, message);
        break;
      case LynxBaseLogLevel.INFO:
        hilog.info(DOMAIN, tag, LynxLogService.format, message);
        break;
      case LynxBaseLogLevel.WARN:
        hilog.warn(DOMAIN, tag, LynxLogService.format, message);
        break;
      case LynxBaseLogLevel.ERROR:
        hilog.error(DOMAIN, tag, LynxLogService.format, message);
        break;
      default:
        break;
    }
  }

  isLogOutputByPlatform(): boolean {
    return LynxLogService.logOutputChannel == LogOutputChannelType.Platform;
  }

  getDefaultWriteFunction(): number {
    return 0;
  }
}