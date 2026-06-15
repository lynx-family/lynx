// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.service.devtool

import android.content.Context
import androidx.annotation.Keep
import com.lynx.devtool.LynxDevtoolEnv
import com.lynx.devtool.LynxGlobalDebugBridge
import com.lynx.devtool.LynxInspectorOwner
import com.lynx.devtool.logbox.LynxLogBoxWrapper
import com.lynx.devtoolwrapper.DevToolSettings
import com.lynx.devtoolwrapper.LynxBaseInspectorController
import com.lynx.devtoolwrapper.ILynxLogBox
import com.lynx.devtoolwrapper.LynxDevtool
import com.lynx.devtool.module.LynxDevToolSetModule
import com.lynx.devtool.module.LynxTrailModule
import com.lynx.devtool.module.LynxWebSocketModule
import com.lynx.devtoolwrapper.LynxDevtoolCardListener
import com.lynx.jsbridge.LynxModule
import com.lynx.tasm.INativeLibraryLoader
import com.lynx.tasm.LynxView
import com.lynx.tasm.base.LLog
import com.lynx.tasm.service.ILynxDevToolService
import org.json.JSONObject

private const val TAG = "LynxDevToolService"

@Keep
class LynxDevToolService : ILynxDevToolService {
    companion object {
        @JvmStatic
        val INSTANCE: ILynxDevToolService by lazy {
            LynxDevToolService()
        }

        operator fun invoke(): ILynxDevToolService = INSTANCE
    }

    override fun createInspectorOwner(view: LynxView?, debuggable: Boolean): LynxBaseInspectorController? {
        try {
            return LynxInspectorOwner(view, debuggable)
        } catch (e: ClassNotFoundException) {
            LLog.e(TAG, "createInspectorOwner failed, ${e.message}")
            return null
        } catch (e: NoClassDefFoundError) {
            LLog.e(TAG, "createInspectorOwner failed, ${e.message}")
            return null
        }
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

    override fun getDevToolSetModuleClass(): Class<out LynxModule>? {
        try {
            return LynxDevToolSetModule::class.java
        } catch (e: ClassNotFoundException) {
            LLog.e(TAG, "getDevToolSetModuleClass failed, ${e.message}")
            return null
        } catch (e: NoClassDefFoundError) {
            LLog.e(TAG, "getDevToolSetModuleClass failed, ${e.message}")
            return null
        }
    }

    override fun getLynxWebSocketModuleClass(): Class<out LynxModule>? {
        try {
            return LynxWebSocketModule::class.java
        } catch (e: ClassNotFoundException) {
            LLog.e(TAG, "getLynxWebSocketModuleClass failed, ${e.message}")
            return null
        } catch (e: NoClassDefFoundError) {
            LLog.e(TAG, "getLynxWebSocketModuleClass failed, ${e.message}")
            return null
        }
    }

    override fun getLynxTrailModule(): Class<out LynxModule>? {
        try {
            return LynxTrailModule::class.java
        } catch (e: ClassNotFoundException) {
            LLog.e(TAG, "getLynxTrailModule failed, ${e.message}")
            return null
        } catch (e: NoClassDefFoundError) {
            LLog.e(TAG, "getLynxTrailModule failed, ${e.message}")
            return null
        }
    }

    override fun globalDebugBridgeShouldPrepareRemoteDebug(url: String): Boolean {
        try {
            return LynxGlobalDebugBridge.getInstance().shouldPrepareRemoteDebug(url)
        } catch (e: ClassNotFoundException) {
            LLog.e(TAG, "globalDebugBridgeShouldPrepareRemoteDebug failed, ${e.message}")
            return false
        } catch (e: NoClassDefFoundError) {
            LLog.e(TAG, "globalDebugBridgeShouldPrepareRemoteDebug failed, ${e.message}")
            return false
        }
    }

    override fun globalDebugBridgePrepareRemoteDebug(scheme: String): Boolean {
        try {
            return LynxGlobalDebugBridge.getInstance().prepareRemoteDebug(scheme)
        } catch (e: ClassNotFoundException) {
            LLog.e(TAG, "globalDebugBridgePrepareRemoteDebug failed, ${e.message}")
            return false
        } catch (e: NoClassDefFoundError) {
            LLog.e(TAG, "globalDebugBridgePrepareRemoteDebug failed, ${e.message}")
            return false
        }
    }

    override fun globalDebugBridgeRegisterCardListener(listener: LynxDevtoolCardListener?) {
        try {
            LynxGlobalDebugBridge.getInstance().registerCardListener(listener)
        } catch (e: ClassNotFoundException) {
            LLog.e(TAG, "globalDebugBridgeRegisterCardListener failed, ${e.message}")
        } catch (e: NoClassDefFoundError) {
            LLog.e(TAG, "globalDebugBridgeRegisterCardListener failed, ${e.message}")
        }
    }

    override fun globalDebugBridgeSetAppInfo(ctx: Context?, appInfo: MutableMap<String, String>?) {
        try {
            LynxGlobalDebugBridge.getInstance().setAppInfo(ctx, appInfo)
        } catch (e: ClassNotFoundException) {
            LLog.e(TAG, "globalDebugBridgeSetAppInfo failed, ${e.message}")
        } catch (e: NoClassDefFoundError) {
            LLog.e(TAG, "globalDebugBridgeSetAppInfo failed, ${e.message}")
        }
    }

    override fun globalDebugBridgeOnPerfMetricsEvent(
        eventName: String?,
        data: JSONObject,
        instanceId: Int
    ) {
        try {
            LynxGlobalDebugBridge.getInstance().onPerfMetricsEvent(eventName, data, instanceId)
        } catch (e: ClassNotFoundException) {
            LLog.e(TAG, "globalDebugBridgeOnPerfMetricsEvent failed, ${e.message}")
        } catch (e: NoClassDefFoundError) {
            LLog.e(TAG, "globalDebugBridgeOnPerfMetricsEvent failed, ${e.message}")
        }
    }

    override fun devtoolEnvSetDevToolLibraryLoader(loader: INativeLibraryLoader?) {
        try {
            LynxDevtoolEnv.inst().setDevToolLibraryLoader(loader)
        } catch (e: ClassNotFoundException) {
            LLog.e(TAG, "devtoolEnvSetDevToolLibraryLoader failed, ${e.message}")
        } catch (e: NoClassDefFoundError) {
            LLog.e(TAG, "devtoolEnvSetDevToolLibraryLoader failed, ${e.message}")
        }
    }

    override fun devtoolEnvInit(ctx: Context) {
        try {
            LynxDevtoolEnv.inst().init(ctx)
        } catch (e: ClassNotFoundException) {
            LLog.e(TAG, "devtoolEnvInit failed, ${e.message}")
        } catch (e: NoClassDefFoundError) {
            LLog.e(TAG, "devtoolEnvInit failed, ${e.message}")
        }
    }

    override fun isDevtoolAttached(): Boolean {
        try {
            return LynxDevtoolEnv.inst().isAttached();
        } catch (e: ClassNotFoundException) {
            LLog.e(TAG, "isDevtoolAttached failed, ${e.message}")
            return false
        } catch (e: NoClassDefFoundError) {
            LLog.e(TAG, "isDevtoolAttached failed, ${e.message}")
            return false
        }
        return false;
    }

    override fun enableAllSessions() {
        try {
            LynxDevtoolEnv.inst().enableAllSessions();
        } catch (e: ClassNotFoundException) {
            LLog.e(TAG, "enableAllSessions failed, ${e.message}")
        } catch (e: NoClassDefFoundError) {
            LLog.e(TAG, "enableAllSessions failed, ${e.message}")
        }
    }

    @Deprecated("Use DevToolSettings.inst().bootstrap().isLynxDebugEnabled() instead")
    override fun getLynxDebugPresetValue(): Boolean {
        return DevToolSettings.inst().bootstrap().isLynxDebugEnabled()
    }

    @Deprecated("Use DevToolSettings.inst().bootstrap().setLynxDebugEnabled(value) instead")
    override fun setLynxDebugPresetValue(value: Boolean) {
        DevToolSettings.inst().bootstrap().setLynxDebugEnabled(value)
    }

    @Deprecated("Use DevToolSettings.inst().bootstrap().isLogBoxEnabled() instead")
    override fun getLogBoxPresetValue(): Boolean {
        return DevToolSettings.inst().bootstrap().isLogBoxEnabled()
    }

    @Deprecated("Use DevToolSettings.inst().bootstrap().setLogBoxEnabled(value) instead")
    override fun setLogBoxPresetValue(value: Boolean) {
        DevToolSettings.inst().bootstrap().setLogBoxEnabled(value)
    }

    @Deprecated("Use DevToolSettings.inst().bootstrap().shouldLoadQJSBridge() instead")
    override fun getLoadQJSBridge(): Boolean {
        return DevToolSettings.inst().bootstrap().shouldLoadQJSBridge()
    }

    @Deprecated("Use DevToolSettings.inst().bootstrap().setLoadQJSBridge(value) instead")
    override fun setLoadQJSBridge(value: Boolean) {
        DevToolSettings.inst().bootstrap().setLoadQJSBridge(value)
    }

    @Deprecated("Use DevToolSettings.inst().bootstrap().shouldLoadV8Bridge() instead")
    override fun getLoadV8Bridge(): Boolean {
        return DevToolSettings.inst().bootstrap().shouldLoadV8Bridge()
    }

    @Deprecated("Use DevToolSettings.inst().bootstrap().setLoadV8Bridge(value) instead")
    override fun setLoadV8Bridge(value: Boolean) {
        DevToolSettings.inst().bootstrap().setLoadV8Bridge(value)
    }
}
