// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.ui.image;

import android.content.Context;
import android.view.View;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.tasm.behavior.LynxProp;
import com.lynx.tasm.behavior.PropsConstants;
import com.lynx.tasm.behavior.ui.LynxUI;

public abstract class AbsUIImage<T extends View> extends LynxUI<T> {
  /**
   * Use {@link AbsUIImage#AbsUIImage(LynxContext)} instead.
   * @param context
   */
  @Deprecated
  public AbsUIImage(Context context) {
    super(context);
  }

  public AbsUIImage(LynxContext context) {
    super(context);
  }

  @LynxProp(name = PropsConstants.SRC) public abstract void setSource(String sources);

  @LynxProp(name = PropsConstants.PLACEHOLDER)
  public abstract void setPlaceholder(String placeholder);

  @LynxProp(name = PropsConstants.MODE) public abstract void setObjectFit(String objectFit);

  @LynxProp(name = PropsConstants.BLUR_RADIUS) public abstract void setBlurRadius(String objectFit);

  @LynxProp(name = PropsConstants.REPEAT, defaultBoolean = false)
  public abstract void setRepeat(boolean repeat);

  @LynxProp(name = PropsConstants.COVER_START, defaultBoolean = false)
  public abstract void setCoverStart(boolean coverStart);

  @LynxProp(name = PropsConstants.CAP_INSETS) public abstract void setCapInsets(String insets);

  @LynxProp(name = PropsConstants.CAP_INSETS_BACKUP)
  public void setCapInsetsBackUp(String insets) {}

  @LynxProp(name = PropsConstants.CAP_INSETS_SCALE)
  public void setCapInsetsScale(String scale) {}

  @LynxProp(name = PropsConstants.LOOP_COUNT) public abstract void setLoopCount(int count);

  @LynxProp(name = PropsConstants.PRE_FETCH_WIDTH)
  public abstract void setPreFetchWidth(String width);

  @LynxProp(name = PropsConstants.PRE_FETCH_HEIGHT)
  public abstract void setPreFetchHeight(String height);

  @LynxProp(name = PropsConstants.DISABLE_DEFAULT_PLACEHOLDER, defaultBoolean = false)
  public void setDisableDefaultPlaceholder(boolean disable) {}

  @LynxProp(name = PropsConstants.AUTO_SIZE)
  public void setAutoSize(boolean autoSize) {}

  @LynxProp(name = PropsConstants.DISABLE_DEFAULT_RESIZE)
  public void setDisableDefaultResize(boolean disable) {}
}
