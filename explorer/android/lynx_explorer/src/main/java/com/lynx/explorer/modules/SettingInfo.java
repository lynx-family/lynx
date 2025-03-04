// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.explorer.modules;

import android.os.Bundle;
import androidx.annotation.Nullable;

public class SettingInfo {
  private static final String STRATEGY = "strategy";
  private static final String ENABLE_PRESET_SIZE = "enablePresetSize";
  private static final String ENABLE_DEBUG_MENU = "enableDebugMenu";
  private static final String ENABLE_RENDER_NODE = "enableRenderNode";
  public int strategy;
  public boolean enablePresetSize;
  public boolean enableDebugMenu;
  public boolean enableRenderNode;

  public static class Builder {
    private int strategy;
    private boolean enablePresetSize;
    private boolean enableDebugMenu;
    private boolean enableRenderNode;
    private boolean enableDevToolDebug;

    public Builder setStrategy(int strategy) {
      this.strategy = strategy;
      return this;
    }

    public Builder setEnablePresetSize(boolean enablePresetSize) {
      this.enablePresetSize = enablePresetSize;
      return this;
    }

    public Builder setEnableDebugMenu(boolean enableDebugMenu) {
      this.enableDebugMenu = enableDebugMenu;
      return this;
    }

    public Builder setEnableRenderNode(boolean enableRenderNode) {
      this.enableRenderNode = enableRenderNode;
      return this;
    }

    public Builder setEnableDevtoolDebug(boolean enableDevToolDebug) {
      this.enableDevToolDebug = enableDevToolDebug;
      return this;
    }

    public SettingInfo build() {
      return new SettingInfo(strategy, enablePresetSize, enableDebugMenu, enableRenderNode);
    }
  }

  public Bundle convertToBundle() {
    Bundle bundle = new Bundle();
    bundle.putInt(STRATEGY, strategy);
    bundle.putBoolean(ENABLE_PRESET_SIZE, enablePresetSize);
    bundle.putBoolean(ENABLE_DEBUG_MENU, enableDebugMenu);
    bundle.putBoolean(ENABLE_RENDER_NODE, enableRenderNode);
    return bundle;
  }

  @Nullable
  public static SettingInfo convertFromBundle(Bundle bundle) {
    return bundle == null ? new Builder().build()
                          : new Builder()
                                .setStrategy(bundle.getInt(STRATEGY))
                                .setEnablePresetSize(bundle.getBoolean(ENABLE_PRESET_SIZE))
                                .setEnableDebugMenu(bundle.getBoolean(ENABLE_DEBUG_MENU))
                                .setEnableRenderNode(bundle.getBoolean(ENABLE_RENDER_NODE))
                                .build();
  }

  private SettingInfo(
      int strategy, boolean enablePresetSize, boolean enableDebugMenu, boolean enableRenderNode) {
    this.strategy = strategy;
    this.enablePresetSize = enablePresetSize;
    this.enableDebugMenu = enableDebugMenu;
    this.enableRenderNode = enableRenderNode;
  }
}
