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
import com.lynx.recorder.LynxDebugInfoRecorder;
import com.lynx.tasm.LynxError;
import com.lynx.tasm.LynxView;
import com.lynx.tasm.TemplateData;
import com.lynx.tasm.base.PageReloadHelper;
import com.lynx.tasm.behavior.LynxUIOwner;
import org.json.JSONObject;

public interface LynxBaseInspectorOwner {
  void setReloadHelper(PageReloadHelper reloadHelper);
  void setDebugInfoInterceptor(LynxDebugInfoRecorder debugInfoRecorder);
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

  String getDebugInfoUrl(String fileName);

  void onReceiveMessageEvent(ReadableMap event);

  void onGlobalPropsUpdated(TemplateData props);

  /**
   * Called when LynxTemplateRender triggers an update using UpdateData or UpdateMetaData
   */
  void onTemplateDataUpdated(TemplateData templateData);
  /**
   * Called when data is reset through LynxTemplateRender resetData method
   */
  void onTemplateDataReset(TemplateData templateData);

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

  /**
   * Adds a listener identified by a given name for view-level CDP events.
   *
   * This method registers a view-level CDP event listener associated with the specified name. The
   * name acts as an identifier for the listener, which can be used later to remove the listener via
   * `removeCDPEventListener` method.
   *
   * @param name The unique name identifying the event listener. This parameter must not be nil.
   * @param listener An object implements `CDPEventListener` interface that will receive
   * notifications for the view-level CDP events. This parameter must not be nil.
   *
   * @discussion Multiple listeners can be registered under different names. When a view-level CDP
   * event occurs, all registered listener interfaces will receive callbacks. All event callbacks
   * are dispatched and executed on the dedicated `cdp_event_listener` thread.
   *
   * The lifecycle of the listener object is managed by the caller. This API holds only a weak
   * reference to the listener. This means the listener will only be called as long as it is still
   * alive.
   *
   *
   * @note Example usage:
   *
   * ```
   * inspectorOwner.addCDPEventListener("test_cdp_listener", eventListener);
   * ```
   */
  void addCDPEventListener(final String name, final CDPEventListener listener);

  /**
   * Removes the CDP event listener identified by the given name.
   *
   * This method unregisters and removes the event listener associated with the specified name.
   * After this call, the listener will no longer receive event notifications.
   *
   * @param name The unique name identifying the event listener to remove. This parameter must not
   *     be nil.
   *
   * @discussion If no listener is found with the given name, this method performs no operation.
   *
   *
   * @note Example usage:
   *
   * ```
   * inspectorOwner.removeCDPEventListener("test_cdp_listener");
   * ```
   */
  void removeCDPEventListener(final String name);

  void setDebugTag(String debugTag);
}
