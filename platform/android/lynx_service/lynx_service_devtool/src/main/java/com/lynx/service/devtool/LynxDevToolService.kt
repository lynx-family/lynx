// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.service.devtool

import android.content.Context
import android.view.ViewGroup
import androidx.annotation.Keep
import com.lynx.devtool.LynxDevtoolEnv
import com.lynx.devtool.LynxGlobalDebugBridge
import com.lynx.devtool.LynxInspectorOwner
import com.lynx.devtool.logbox.LynxLogBoxProxy
import com.lynx.devtoolwrapper.LynxBaseInspectorOwnerNG
import com.lynx.devtoolwrapper.LynxBaseLogBoxProxy
import com.lynx.devtoolwrapper.LynxDevtool
import com.lynx.tasm.LynxView
import com.lynx.tasm.service.ILynxDevToolService
import com.lynx.devtool.module.LynxDevToolSetModule
import com.lynx.devtool.module.LynxTrailModule
import com.lynx.devtool.module.LynxWebSocketModule
import com.lynx.devtoolwrapper.LynxDevtoolCardListener
import com.lynx.jsbridge.LynxModule
import com.lynx.tasm.INativeLibraryLoader
import com.lynx.tasm.base.LLog
import org.json.JSONObject

@Keep
object LynxDevToolService : ILynxDevToolService {
    private const val TAG = "LynxDevToolService"

    override fun createInspectorOwner(view: LynxView?): LynxBaseInspectorOwnerNG? {
        try {
            return LynxInspectorOwner(view)
        } catch (e: ClassNotFoundException) {
            LLog.e(TAG, "createInspectorOwner failed, ${e.message}")
            return null
        } catch (e: NoClassDefFoundError) {
            LLog.e(TAG, "createInspectorOwner failed, ${e.message}")
            return null
        }
    }

    override fun createLogBoxProxy(devtool: LynxDevtool): LynxBaseLogBoxProxy? {
        try {
            return LynxLogBoxProxy(devtool)
        } catch (e: ClassNotFoundException) {
            LLog.e(TAG, "createLogBoxProxy failed, ${e.message}")
            return null
        } catch (e: NoClassDefFoundError) {
            LLog.e(TAG, "createLogBoxProxy failed, ${e.message}")
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

    override fun getDevToolWebSocketModuleClass(): Class<out LynxModule>? {
        try {
            return LynxWebSocketModule::class.java
        } catch (e: ClassNotFoundException) {
            LLog.e(TAG, "getDevToolWebSocketModuleClass failed, ${e.message}")
            return null
        } catch (e: NoClassDefFoundError) {
            LLog.e(TAG, "getDevToolWebSocketModuleClass failed, ${e.message}")
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

    override fun globalDebugBridgeSetContext(ctx: Context?) {
        try {
            LynxGlobalDebugBridge.getInstance().setContext(ctx)
        } catch (e: ClassNotFoundException) {
            LLog.e(TAG, "globalDebugBridgeSetContext failed, ${e.message}")
        } catch (e: NoClassDefFoundError) {
            LLog.e(TAG, "globalDebugBridgeSetContext failed, ${e.message}")
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

    override fun globalDebugBridgeStartRecord() {
        try {
            LynxGlobalDebugBridge.getInstance().startRecord()
        } catch (e: ClassNotFoundException) {
            LLog.e(TAG, "globalDebugBridgeStartRecord failed, ${e.message}")
        } catch (e: NoClassDefFoundError) {
            LLog.e(TAG, "globalDebugBridgeStartRecord failed, ${e.message}")
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

    override fun setDevtoolEnv(key: String?, value: Any?) {
        try {
            LynxDevtoolEnv.inst().setDevtoolEnv(key, value)
        } catch (e: ClassNotFoundException) {
            LLog.e(TAG, "setDevtoolEnv failed, ${e.message}")
        } catch (e: NoClassDefFoundError) {
            LLog.e(TAG, "setDevtoolEnv failed, ${e.message}")
        }
    }

    override fun setDevtoolGroupEnv(groupKey: String?, newGroupValues: MutableSet<String>?) {
        try {
            LynxDevtoolEnv.inst().setDevtoolEnv(groupKey, newGroupValues)
        } catch (e: ClassNotFoundException) {
            LLog.e(TAG, "setDevtoolGroupEnv failed, ${e.message}")
        } catch (e: NoClassDefFoundError) {
            LLog.e(TAG, "setDevtoolGroupEnv failed, ${e.message}")
        }
    }

    override fun getDevtoolBooleanEnv(key: String?, defaultValue: Boolean?): Boolean {
        try {
            return LynxDevtoolEnv.inst().getDevtoolEnv(key, defaultValue)
        } catch (e: ClassNotFoundException) {
            LLog.e(TAG, "getDevtoolBooleanEnv failed, ${e.message}")
            return defaultValue ?: false
        } catch (e: NoClassDefFoundError) {
            LLog.e(TAG, "getDevtoolBooleanEnv failed, ${e.message}")
            return defaultValue ?: false
        }
    }

    override fun getDevtoolIntEnv(key: String?, defaultValue: Int?): Int {
        try {
            return LynxDevtoolEnv.inst().getDevtoolEnv(key, defaultValue)
        } catch (e: ClassNotFoundException) {
            LLog.e(TAG, "getDevtoolIntEnv failed, ${e.message}")
            return defaultValue ?: 0
        } catch (e: NoClassDefFoundError) {
            LLog.e(TAG, "getDevtoolIntEnv failed, ${e.message}")
            return defaultValue ?: 0
        }
    }

    override fun getDevtoolGroupEnv(key: String?): MutableSet<String> {
        try {
            return LynxDevtoolEnv.inst().getDevtoolEnv(key)
        } catch (e: ClassNotFoundException) {
            LLog.e(TAG, "getDevtoolGroupEnv failed, ${e.message}")
            return mutableSetOf()
        } catch (e: NoClassDefFoundError) {
            LLog.e(TAG, "getDevtoolGroupEnv failed, ${e.message}")
            return mutableSetOf()
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
}