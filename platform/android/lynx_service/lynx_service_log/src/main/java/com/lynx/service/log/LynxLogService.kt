// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.service.log

import android.util.Log
import androidx.annotation.Keep
import com.lynx.base.service.ILynxBaseLogService
import com.lynx.base.service.ILynxBaseLogService.LogOutputChannelType
import com.lynx.base.log.LynxLog

@Keep
object LynxLogService : ILynxBaseLogService {
  private var logOutputChannel: LogOutputChannelType = LogOutputChannelType.Platform

  // By default, lynx logs are consumed on the platform layer.
  override fun logByPlatform(
    level: Int,
    tag: String,
    msg: String,
  ) {
    when (level) {
      LynxLog.VERBOSE -> Log.v(tag, msg)
      LynxLog.DEBUG -> Log.d(tag, msg)
      LynxLog.INFO -> Log.i(tag, msg)
      LynxLog.WARN -> Log.w(tag, msg)
      LynxLog.ERROR -> Log.e(tag, msg)
      else -> Log.i(tag, msg)
    }
  }

  override fun isLogOutputByPlatform(): Boolean = logOutputChannel == LogOutputChannelType.Platform

  override fun getDefaultWriteFunction(): Long = 0

  override fun switchLogToSystem(enableSystemLog: Boolean) {}

  override fun getLogToSystemStatus(): Boolean = false
}
