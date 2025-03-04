// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.service.log

import android.util.Log
import androidx.annotation.Keep
import com.lynx.tasm.service.ILynxLogService
import com.lynx.tasm.service.ILynxLogService.LogOutputChannelType
import com.lynx.tasm.base.LLog

@Keep
object LynxLogService : ILynxLogService {
  private var logOutputChannel: LogOutputChannelType = LogOutputChannelType.Platform

  // By default, lynx logs are consumed on the platform layer.
  override fun logByPlatform(
    level: Int,
    tag: String,
    msg: String,
  ) {
    when (level) {
      LLog.VERBOSE -> Log.v(tag, msg)
      LLog.DEBUG -> Log.d(tag, msg)
      LLog.INFO -> Log.i(tag, msg)
      LLog.WARN -> Log.w(tag, msg)
      LLog.ERROR -> Log.e(tag, msg)
      else -> Log.i(tag, msg)
    }
  }

  override fun isLogOutputByPlatform(): Boolean = logOutputChannel == LogOutputChannelType.Platform

  override fun getDefaultWriteFunction(): Long = 0

  override fun switchLogToSystem(enableSystemLog: Boolean) {}

  override fun getLogToSystemStatus(): Boolean = false
}
