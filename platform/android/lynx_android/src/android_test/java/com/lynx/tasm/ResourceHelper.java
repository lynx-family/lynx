// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm;

import android.content.Context;
import android.content.res.AssetManager;
import android.util.Log;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.InputStream;

public final class ResourceHelper {
  /**
   * load byte[] from assets.
   * @param context
   * @param name
   * @return
   */
  public static byte[] loadByteArray(@NonNull Context context, @NonNull String name) {
    final AssetManager assets = context.getAssets();
    byte[] bytes = null;
    if (assets != null) {
      try {
        InputStream stream = assets.open(name);
        bytes = new byte[stream.available()];
        stream.read(bytes);
      } catch (IOException e) {
        Log.e("ResourceHelper", "loadByteArray with error: " + e.getMessage());
      }
    }
    return bytes;
  }

  /**
   * load String from assets
   * @param context
   * @param name
   * @return
   */
  public static String loadString(@NonNull Context context, @NonNull String name) {
    final AssetManager assets = context.getAssets();

    if (assets == null) {
      return null;
    }

    try {
      return loadString(assets.open(name));
    } catch (IOException e) {
      Log.e("ResourceHelper", "loadString with error: " + e.getMessage());
      return null;
    }
  }

  private static String loadString(@Nullable InputStream inputStream) {
    if (inputStream == null) {
      return null;
    }

    try {
      ByteArrayOutputStream result = new ByteArrayOutputStream();
      byte[] buffer = new byte[4096];
      int length;
      while ((length = inputStream.read(buffer)) > 0) {
        result.write(buffer, 0, length);
      }
      return result.toString();
    } catch (IOException e) {
      Log.e("ResourceHelper", "loadString from stream with error: " + e.getMessage());
      return null;
    }
  }
}
