// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.service;

import android.content.Context;
import android.view.ViewGroup;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import com.lynx.devtoolwrapper.LynxBaseInspectorOwnerNG;
import com.lynx.devtoolwrapper.LynxBaseLogBoxProxy;
import com.lynx.devtoolwrapper.LynxDevtool;
import com.lynx.devtoolwrapper.LynxDevtoolCardListener;
import com.lynx.tasm.INativeLibraryLoader;
import com.lynx.tasm.LynxView;
import java.util.Map;
import java.util.Set;
import org.json.JSONObject;

public interface ILynxDevToolService extends IServiceProvider {
  /**
   * Get service class, DO NOT OVERRIDE THIS METHOD
   */
  @NonNull
  default Class<? extends IServiceProvider> getServiceClass() {
    return ILynxDevToolService.class;
  }
  LynxBaseInspectorOwnerNG createInspectorOwner(@Nullable LynxView view);
  LynxBaseLogBoxProxy createLogBoxProxy(@NonNull LynxDevtool devtool);
  Class<? extends com.lynx.jsbridge.LynxModule> getDevToolSetModuleClass();
  Class<? extends com.lynx.jsbridge.LynxModule> getDevToolWebSocketModuleClass();

  Class<? extends com.lynx.jsbridge.LynxModule> getLynxTrailModule();

  Boolean globalDebugBridgeShouldPrepareRemoteDebug(String url);
  Boolean globalDebugBridgePrepareRemoteDebug(String scheme);
  void globalDebugBridgeSetContext(Context ctx);
  void globalDebugBridgeRegisterCardListener(LynxDevtoolCardListener listener);
  void globalDebugBridgeSetAppInfo(Context ctx, Map<String, String> appInfo);
  void globalDebugBridgeOnPerfMetricsEvent(
      String eventName, @NonNull JSONObject data, int instanceId);
  void globalDebugBridgeStartRecord();

  void devtoolEnvSetDevToolLibraryLoader(INativeLibraryLoader loader);
  void setDevtoolEnv(String key, Object value);
  void setDevtoolGroupEnv(String groupKey, Set<String> newGroupValues);
  boolean getDevtoolBooleanEnv(String key, Boolean defaultValue);
  Integer getDevtoolIntEnv(String key, Integer defaultValue);
  Set<String> getDevtoolGroupEnv(String key);
  void devtoolEnvInit(Context ctx);

  boolean isDevtoolAttached();
}
