// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.jsbridge;

import android.content.Context;
import androidx.annotation.Keep;
import com.lynx.react.bridge.JavaOnlyArray;

@Keep
public abstract class LynxModule {
  /**
   * LynxMethod verification interface, used to verify whether the LynxMethod is allowed to be
   * called
   */
  public interface AuthValidator {
    /**
     *
     * @param moduleName Module Name called in JS.
     * @param methodName Method Name called in JS.
     * @param methodParams Parameters of this call，Do not make any changes to the parameters!
     * @return
     * true: This verification passes, LynxMethod can be called
     * false: This verification fails, LynxMethod cannot be called, and a JS error will be
     * generated.
     */
    boolean verify(String moduleName, String methodName, JavaOnlyArray methodParams);
  }

  /**
   * In CommonLynxModule, there is a unique corresponding LynxContext.However, in SharedLynxModule,
   * mContext is ApplicationContext.
   */
  protected Context mContext;
  protected Object mParam;
  protected Object mExtraData;

  @Keep
  public LynxModule(Context context) {
    this(context, null);
  }

  @Keep
  public LynxModule(Context context, Object param) {
    mContext = context;
    mParam = param;
  }

  public void setExtraData(Object data) {
    mExtraData = data;
  }

  @Keep
  public void destroy() {}
}
