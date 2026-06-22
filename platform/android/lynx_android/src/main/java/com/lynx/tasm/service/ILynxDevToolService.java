// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.service;

import android.content.Context;
import androidx.annotation.Keep;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import com.lynx.devtoolwrapper.ILynxLogBox;
import com.lynx.devtoolwrapper.LynxBaseInspectorController;
import com.lynx.devtoolwrapper.LynxDevtool;
import com.lynx.devtoolwrapper.LynxDevtoolCardListener;
import com.lynx.tasm.INativeLibraryLoader;
import com.lynx.tasm.LynxView;
import java.util.Map;
import org.json.JSONObject;

@Keep
public interface ILynxDevToolService extends IServiceProvider {
  /**
   * Get service class, DO NOT OVERRIDE THIS METHOD
   */
  @NonNull
  default Class<? extends IServiceProvider> getServiceClass() {
    return ILynxDevToolService.class;
  }
  LynxBaseInspectorController createInspectorOwner(@Nullable LynxView view, boolean debuggable);
  ILynxLogBox createLogBox(@NonNull LynxDevtool devtool);
  Class<? extends com.lynx.jsbridge.LynxModule> getDevToolSetModuleClass();
  Class<? extends com.lynx.jsbridge.LynxModule> getLynxWebSocketModuleClass();

  Class<? extends com.lynx.jsbridge.LynxModule> getLynxTrailModule();

  Boolean globalDebugBridgeShouldPrepareRemoteDebug(String url);
  Boolean globalDebugBridgePrepareRemoteDebug(String scheme);
  void globalDebugBridgeRegisterCardListener(LynxDevtoolCardListener listener);
  void globalDebugBridgeSetAppInfo(Context ctx, Map<String, String> appInfo);
  void globalDebugBridgeOnPerfMetricsEvent(
      String eventName, @NonNull JSONObject data, int instanceId);

  void devtoolEnvSetDevToolLibraryLoader(INativeLibraryLoader loader);
  void devtoolEnvInit(Context ctx);

  boolean isDevtoolAttached();

  /**
   * Enable all debug sessions
   *
   * Note:
   * - This method should only be used when full-session debugging is allowed, otherwise it should
   * generally not be used
   * - When enabled, it will allow debugging of all sessions, which may pose security risks
   * - Only use in development, testing, or internal debugging scenarios
   */
  void enableAllSessions();

  // Deprecated compatibility accessors for DevTool bootstrap settings.
  //
  // Note:
  // - These methods can be called before `LynxEnv` initialization.
  // - Calling `setLynxDebugPresetValue(true)` does not initialize `LynxDevToolEnv`. If `LynxEnv`
  // has already been initialized and you need to initialize `LynxDevToolEnv`, please call
  // `LynxEnv.inst().enableLynxDebug(true)`.
  @Deprecated boolean getLynxDebugPresetValue();
  @Deprecated void setLynxDebugPresetValue(boolean value);
  @Deprecated boolean getLogBoxPresetValue();
  @Deprecated void setLogBoxPresetValue(boolean value);
  @Deprecated boolean getLoadQJSBridge();
  @Deprecated void setLoadQJSBridge(boolean value);
  @Deprecated boolean getLoadV8Bridge();
  @Deprecated void setLoadV8Bridge(boolean value);
}
