// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.devtoolwrapper;

import android.view.InputEvent;
import androidx.annotation.NonNull;
import androidx.annotation.RestrictTo;
import androidx.core.util.Consumer;
import com.lynx.react.bridge.Callback;
import com.lynx.react.bridge.ReadableMap;
import com.lynx.tasm.LynxError;
import com.lynx.tasm.LynxView;
import com.lynx.tasm.TemplateData;
import com.lynx.tasm.base.PageReloadHelper;
import com.lynx.tasm.behavior.LynxUIOwner;
import org.json.JSONObject;

public interface LynxBaseInspectorOwner {
  void setReloadHelper(PageReloadHelper reloadHelper);
  void onTemplateAssemblerCreated(long ptr);

  long onBackgroundRuntimeCreated(String groupName);
  void reload(boolean ignoreCache);
  void reload(
      boolean ignoreCache, String templateBin, boolean fromTemplateFragments, int templateSize);
  void navigate(String url);
  void stopCasting();
  void continueCasting();
  void pauseCasting();
  void sendResponse(String response);
  void savePostURL(@NonNull final String postUrl);
  void onRootViewInputEvent(InputEvent ev);
  void destroy();
  void attach(LynxView view);
  void sendConsoleMessage(String text, int level, long timestamp);
  void updateScreenMetrics(int width, int height, float density);

  void attachToDebugBridge(String url);
  @Deprecated String getGroupID();

  void sendFileByAgent(String type, String file);
  void endTestbench(String filePath);

  void onPageUpdate();

  void attachLynxUIOwnerToAgent(LynxUIOwner uiOwner);

  void setLynxInspectorConsoleDelegate(Object delegate);

  void getConsoleObject(String objectId, boolean needStringify, Callback callback);

  void onPerfMetricsEvent(String eventName, @NonNull JSONObject data);

  String getDebugInfoUrl();

  void onReceiveMessageEvent(ReadableMap event);

  void onGlobalPropsUpdated(TemplateData props);

  void setDevToolDelegate(IDevToolDelegate devToolDelegate);
  @RestrictTo(RestrictTo.Scope.LIBRARY) void showErrorMessageOnConsole(final LynxError error);
  @RestrictTo(RestrictTo.Scope.LIBRARY) void showMessageOnConsole(final String message, int level);

  /**
   * Invokes a CDP method from the SDK.
   *
   * This method replaces the previous `invokeCDPFromSDK:` method. Unlike the old method,
   * the new method does not limit the use of the main thread. Therefore, it can be called
   * from any thread.
   *
   * @discussion This method accepts a CDP command message and a callback block to handle the
   * result. The result of the CDP command will be returned asynchronously through the callback
   * block.
   *
   * <b>Note:</b> This is a breaking change introduced in version 3.0
   *
   * @param cdpMsg The CDP command method to be sent. This parameter must not be null.
   * @param callback A callback to be called when the CDP command result is available.
   * The final execution thread of this callback depends on the last thread that processes
   * the CDP protocol, which could be a TASM thread, UI thread, devtool thread, etc.
   *
   * @since 3.0
   *
   * @note Example usage:
   *
   * ```
   * inspectorOwner.invokeCDPFromSDK(jsonString, new CDPResultCallback() {
   *     @Override
   *     public void onResult(String result) {
   *         // Handle result
   *     }
   * });
   * ```
   */
  void invokeCDPFromSDK(final String cdpMsg, final CDPResultCallback callback);
}
