// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.group;

import com.lynx.tasm.ILynxLogicExecutor;
import com.lynx.tasm.IUIRendererCreator;
import com.lynx.tasm.LynxBackgroundRuntimeOptions;
import com.lynx.tasm.LynxBooleanOption;
import com.lynx.tasm.ThreadStrategyForRendering;
import com.lynx.tasm.behavior.BehaviorRegistry;
import com.lynx.tasm.resourceprovider.generic.LynxGenericResourceFetcher;
import com.lynx.tasm.resourceprovider.media.LynxMediaResourceFetcher;
import com.lynx.tasm.resourceprovider.template.LynxTemplateResourceFetcher;
import java.util.HashMap;
import java.util.Map;

/**
 * Interface that defines the behaviours of LynxViewBuilder & LynxViewGroup;
 */
public interface ILynxViewConfigProvider {
  boolean hasPresetMeasureSpec();

  /**
   * @return current config BehaviorRegistry
   */
  BehaviorRegistry getBehaviorRegistry();

  /**
   * @return whether AutoExpose enabled.
   */
  boolean isEnableAutoExpose();

  /**
   * Returns the densityOverride;
   */
  Float getDensity();

  /**
   * @return current threadStrategy config
   */
  ThreadStrategyForRendering getThreadStrategy();

  /**
   * @return whether layoutSafePoint is enabled;
   */
  boolean isEnableLayoutSafepoint();

  /**
   * @return whether unifiedPipeline enabled;
   */
  boolean isEnableUnifiedPipeline();

  /**
   * @return ContextData passed In.
   */
  HashMap getContextData();

  /**
   * Returns the associated lynxRuntimeOptions
   */
  LynxBackgroundRuntimeOptions getLynxRuntimeOptions();

  /**
   * @return ScreenWidth set
   */
  int getScreenWidth();

  /**
   * @return ScreenHeight set
   */
  int getScreenHeight();

  /**
   * @return ForceDarkAllow set
   */
  boolean getForceDarkAllowed();

  /**
   * @return whether MultiAsyncThread enabled;
   */
  boolean isEnableMultiAsyncThread();

  /**
   * @return whether SyncFlush enabled;
   */
  boolean isEnableSyncFlush();

  /**
   * @return whether AutoConcurrency enabled;
   */
  @Deprecated boolean isEnableAutoConcurrency();

  /**
   * @return whether VsyncAlignedMessageLoop enabled;
   */
  boolean isEnableVSyncAlignedMessageLoop();

  /**
   * @return whether PendingJSTask enabled;
   */
  boolean isEnablePendingJsTask();

  /**
   * @return whether AsyncHydration enabled;
   */
  boolean isEnableAsyncHydration();

  /**
   * @return whether JSRuntime enabled;
   */
  boolean isEnableJSRuntime();

  /**
   * @return whether AirStrictMode enabled;
   */
  boolean isEnableAirStrictMode();

  /**
   * @return whether debuggable enabled;
   */
  boolean isDebuggable();

  /**
   * @return presetWidthMeasureSpec set;
   */
  int getPresetWidthMeasureSpec();

  /**
   * @return presetHeightMeasureSpec set;
   */
  int getPresetHeightMeasureSpec();

  /**
   * @return fontScale set;
   */
  float getFontScale();

  /**
   * @return whether preUpdateData enabled;
   */
  boolean isEnablePreUpdateData();

  IUIRendererCreator getUIRendererCreator();

  int getEmbeddedMode();

  boolean isEnableMTSModule();

  /* resource fetcher related */

  /**
   * Returns whether genericResourceFetcher is enabled.
   */
  LynxBooleanOption isEnableGenericResourceFetcher();

  /**
   * @return GenericResourceFetcher set
   */
  LynxGenericResourceFetcher getLynxGenericResourceFetcher();

  /**
   * @return MediaResourceFetcher set
   */
  LynxMediaResourceFetcher getLynxMediaResourceFetcher();

  /**
   * @return TemplateResourceFetcher set
   */
  LynxTemplateResourceFetcher getLynxTemplateResourceFetcher();

  /**
   * @return LogicExecutor which execute event callback and other logic.
   */
  ILynxLogicExecutor getLogicExecutor();
}
