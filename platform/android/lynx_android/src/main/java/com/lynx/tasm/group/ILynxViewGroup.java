// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.group;

import androidx.annotation.Nullable;
import com.lynx.jsbridge.LynxModule;
import com.lynx.tasm.LynxView;
import com.lynx.tasm.TemplateBundle;
import com.lynx.tasm.TemplateData;
import com.lynx.tasm.resourceprovider.LynxResourceCallback;

/**
 * Interface provider for accessing LynxViewGroup prop getter;
 */
public interface ILynxViewGroup extends ILynxViewConfigProvider {
  int generateNextLynxViewID();
  void addLynxView(int LynxViewId, LynxView view);
  void removeLynxView(int LynxViewId);
  LynxView getLynxViewById(int LynxViewId);
  void registerModule(String name, Class<? extends LynxModule> module, Object param);
  String getUrl();
  TemplateData getGlobalProps();

  /**
   * Get the associated TemplateBundle with LynxViewGroup.
   * If templateBundle is not ready yet. It will block current thread
   * and waiting for the result.
   *
   * @return The Associated TemplateBundle
   */
  TemplateBundle getTemplateBundle();

  @Nullable TemplateBundle getTemplateBundleNonBlocking();

  /**
   * - If the TemplateBundle is ready, immediately invoke the callback (in-place)
   * - If the TemplateBundle is being fetched, trigger the callback once the fetch completes
   * - If the TemplateBundle failed to fetch, pass the failure error to the callback
   */
  void fetchTemplateBundle(LynxResourceCallback<TemplateBundle> callback);

  /**
   * Check if the bundle in LynxViewGroup is ready yet.
   *
   * @return TemplateBundle ready or not.
   */
  boolean isTemplateBundleReady();
}
