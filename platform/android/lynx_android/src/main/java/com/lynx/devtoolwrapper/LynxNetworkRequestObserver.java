// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.devtoolwrapper;

import androidx.annotation.RestrictTo;
import com.lynx.react.bridge.JavaOnlyMap;

/** Internal bridge from the Android Fetch implementation to its owning DevTool instance. */
@RestrictTo(RestrictTo.Scope.LIBRARY_GROUP)
public interface LynxNetworkRequestObserver {
  boolean isEnabled();

  String requestWillBeSent(String url, String method, JavaOnlyMap headers, byte[] body);

  void responseReceived(
      String requestId, String url, int status, String statusText, JavaOnlyMap headers);

  void dataReceived(String requestId, byte[] data);

  void loadingFinished(String requestId);

  void loadingFailed(String requestId, String errorText, boolean canceled);
}
