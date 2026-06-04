// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import com.lynx.tasm.utils.UIThreadUtils;
import java.lang.ref.WeakReference;

public final class LynxElement {
  private final WeakReference<LynxTemplateRender> mTemplateRender;
  private final int mSign;

  /**
   * @apidoc
   *
   * @brief Callback used by LynxElement APIs.
   */
  public interface Callback<T> {
    void onResult(@Nullable T result);
  }

  LynxElement(@Nullable LynxTemplateRender templateRender, int sign) {
    mTemplateRender = new WeakReference<>(templateRender);
    mSign = sign;
  }

  /**
   * @apidoc
   *
   * @brief Serializes this LynxElement tree to a JSON string.
   * @param callback Receives the JSON string, or null if the element is unavailable.
   */
  public void toJSONString(@NonNull final Callback<String> callback) {
    if (callback == null) {
      return;
    }
    LynxTemplateRender templateRender = mTemplateRender.get();
    if (templateRender == null || mSign == 0) {
      UIThreadUtils.runOnUiThread(() -> callback.onResult(null));
      return;
    }
    templateRender.lynxElementToJSONString(mSign, callback);
  }
}
