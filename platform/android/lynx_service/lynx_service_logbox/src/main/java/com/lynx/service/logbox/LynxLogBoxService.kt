// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.service.logbox

import androidx.annotation.Keep
import com.google.auto.service.AutoService
import com.lynx.tasm.service.IServiceProvider
import com.lynx.devtool.logbox.LynxLogBoxWrapper
import com.lynx.devtoolwrapper.ILynxLogBox
import com.lynx.devtoolwrapper.LynxDevtool
import com.lynx.tasm.base.LLog
import com.lynx.tasm.service.ILynxLogBoxService

private const val TAG = "LynxLogBoxService"

@Keep
@AutoService(IServiceProvider::class)
class LynxLogBoxService : ILynxLogBoxService {
    companion object {
        @JvmStatic
        val INSTANCE: ILynxLogBoxService by lazy {
            LynxLogBoxService()
        }

        operator fun invoke(): ILynxLogBoxService = INSTANCE
    }

    override fun createLogBox(devtool: LynxDevtool): ILynxLogBox? {
        try {
            return LynxLogBoxWrapper(devtool)
        } catch (e: ClassNotFoundException) {
            LLog.e(TAG, "createLogBox failed, ${e.message}")
            return null
        } catch (e: NoClassDefFoundError) {
            LLog.e(TAG, "createLogBox failed, ${e.message}")
            return null
        }
    }
}
