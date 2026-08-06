// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.explorer;

import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.util.Log;
import androidx.appcompat.app.AppCompatActivity;
import com.lynx.explorer.routing.RequestedRuntime;
import com.lynx.explorer.routing.RouteCoordinator;
import com.lynx.explorer.routing.RouteResult;
import com.lynx.explorer.routing.RouteSource;

public class DebugBridgeActivity extends AppCompatActivity {
  private static final String TAG = "DebugBridgeActivity";

  @Override
  protected void onCreate(Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);

    Uri data = getIntent().getData();

    // Handle lynx://open?url= scheme for runtime page switching.
    if (data != null && "lynx".equals(data.getScheme()) && "open".equals(data.getHost())) {
      String targetUrl = data.getQueryParameter("url");
      if (targetUrl != null && !targetUrl.isEmpty()) {
        Log.d(TAG, "Opening URL via lynx://open: " + targetUrl);
        RouteResult result = RouteCoordinator.open(
            this, data.toString(), RequestedRuntime.AUTOMATIC, RouteSource.DEBUG_BRIDGE);
        if (!result.getAccepted()) {
          Log.e(TAG, result.getCode() + ": " + result.getMessage());
        }
        finish();
        return;
      }
    }

    finish();
  }
}
