// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.devtoolwrapper;

import android.view.InputEvent;
import androidx.annotation.NonNull;
import androidx.annotation.RestrictTo;
import com.lynx.react.bridge.ReadableMap;
import com.lynx.recorder.LynxDebugInfoRecorder;
import com.lynx.tasm.LynxError;
import com.lynx.tasm.LynxView;
import com.lynx.tasm.TemplateData;
import com.lynx.tasm.base.PageReloadHelper;
import com.lynx.tasm.behavior.LynxUIOwner;
import org.json.JSONObject;

@RestrictTo(RestrictTo.Scope.LIBRARY_GROUP)
public interface LynxBaseInspectorController {
  void setReloadHelper(PageReloadHelper reloadHelper);
  void setDebugInfoInterceptor(LynxDebugInfoRecorder debugInfoRecorder);
  void setDevToolDelegate(IDevToolDelegate devToolDelegate);

  void onTemplateAssemblerCreated(long ptr);
  long onBackgroundRuntimeCreated(String groupName);
  void attach(LynxView view);
  void attachLynxUIOwnerToAgent(LynxUIOwner uiOwner);
  void attachToDebugBridge(String url);
  void destroy();

  void reload(boolean ignoreCache, String templateBin, boolean fromTemplateFragments,
      int templateSize, String reloadUrl);

  void continueCasting();
  void pauseCasting();

  void onRootViewInputEvent(InputEvent ev);

  void updateScreenMetrics(int width, int height, float density);

  void endTestbench(String filePath);

  void onPageUpdate();

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

  void showErrorMessageOnConsole(final LynxError error);
  void showMessageOnConsole(final String message, int level);
}
