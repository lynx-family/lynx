// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.provider;

import androidx.annotation.NonNull;
import com.lynx.tasm.behavior.LynxContext;

/**
 * Created by zhangxing on 2019/4/30
 */
public abstract class AbsTemplateProvider {
  public interface Callback {
    void onSuccess(byte[] template);

    void onFailed(String msg);
  }

  public abstract void loadTemplate(@NonNull String url, Callback callback);

  public void loadTemplate(@NonNull String url, Callback callback, LynxContext context) {
    loadTemplate(url, callback);
  }
}
