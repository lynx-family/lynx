// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm;

import androidx.annotation.RestrictTo;

/**
 * base interface to receive `LynxError` for `LynxTemplateRender` and `LynxBackgroundRuntime`
 */
@RestrictTo(RestrictTo.Scope.LIBRARY)
public interface ILynxErrorReceiver {
  void onErrorOccurred(LynxError error);
}
