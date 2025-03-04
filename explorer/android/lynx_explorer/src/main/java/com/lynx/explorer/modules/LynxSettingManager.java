// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.explorer.modules;

import com.lynx.tasm.fontface.FontFaceManager;
import com.lynx.tasm.rendernode.compat.RenderNodeCompat;
import com.lynx.tasm.theme.LynxTheme;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.concurrent.atomic.AtomicInteger;

public class LynxSettingManager {
  private static class LinkedTheme extends LynxTheme {
    public LinkedTheme next;
  }

  private AtomicBoolean mEnablePresetSize = new AtomicBoolean();

  private AtomicInteger mStrategy = new AtomicInteger();
  private final LynxTheme mThemes = new LinkedTheme() {
    {
      final String lans[] = new String[] {"zh-TW", "zh", "en", "ar", "jp"};
      final String brights[] = new String[] {"day", "night", "other"};

      LinkedTheme dummy = new LinkedTheme();
      dummy.next = this;
      LinkedTheme curr = dummy;
      for (String lan : lans) {
        for (String br : brights) {
          curr.next.update("language", lan);
          curr.next.update("brightness", br);
          curr.next.next = new LinkedTheme();
          curr = curr.next;
        }
      }
      curr.next = this;
    }
  };

  private static final LynxSettingManager sInstance = new LynxSettingManager();

  public static LynxSettingManager getInstance() {
    return sInstance;
  }

  public SettingInfo getSettingInfo() {
    return new SettingInfo.Builder()
        .setStrategy(getThreadStrategy())
        .setEnablePresetSize(isPresetSizeEnabled())
        .setEnableRenderNode(isRenderNodeEnabled())
        .build();
  }

  public LynxTheme changeTheme(LynxTheme theme) {
    if (theme != null && theme instanceof LinkedTheme) {
      return ((LinkedTheme) theme).next;
    }
    return mThemes;
  }

  void setThreadStrategy(int strategy) {
    mStrategy.set(strategy);
  }

  public void setEnablePresetSize(boolean enablePresetSize) {
    mEnablePresetSize.set(enablePresetSize);
  }

  void enableRenderNode(boolean enableRenderNode) {
    RenderNodeCompat.enable(enableRenderNode);
  }

  private int getThreadStrategy() {
    return mStrategy.get();
  }

  private boolean isPresetSizeEnabled() {
    return mEnablePresetSize.get();
  }

  private boolean isRenderNodeEnabled() {
    return RenderNodeCompat.supportRenderNode();
  }
}
