// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.core;

import android.text.TextUtils;
import androidx.annotation.NonNull;
import com.lynx.tasm.LynxEnv;
import com.lynx.tasm.base.CalledByNative;
import com.lynx.tasm.base.LLog;
import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.InputStream;

public class ResourceLoader {
  private static final String CORE_JS = "assets://lynx_core.js";
  private static final String CORE_DEBUG_JS = "lynx_core_dev.js";
  private static final String FILE_SCHEME = "file://";
  private static final String ASSETS_SCHEME = "assets://";
  private static final String TAG = "ResourceLoader";
  // For compatibility with iOS, on iOS the path of assets://lynx_core.js and assets://[others].js
  // is different
  private static final String LYNX_ASSETS_SCHEME = "lynx_assets://";

  private byte[] toByteArray(@NonNull InputStream in) throws IOException {
    ByteArrayOutputStream out = new ByteArrayOutputStream();
    byte[] buffer = new byte[1024 * 4];
    int n = 0;
    while ((n = in.read(buffer)) != -1) {
      out.write(buffer, 0, n);
    }
    return out.toByteArray();
  }

  @CalledByNative
  private byte[] loadJSSource(String name) {
    /* 1. if the file name is assets://lynx_core.js
     *   i. if devtool is enabled, try to load assets://lynx_core_dev.js
     * 2. if the file name is start with "file://", use FileInputStream to get the file content.
     * 3. if the file name is start with "assets://", use
     * LynxEnv.inst().getAppContext().getResources().getAssets().open() to get the
     * file content
     * 4. if the file name is start with "lynx_assets://", the same as above. (the reason for this
     * is compatibility with iOS resource loader, the paths of assets://lynx_core.js and
     * assets://other_file.js are different in iOS)
     * */
    if (TextUtils.isEmpty(name)) {
      LLog.w(TAG, "loadJSSource failed with empty name");
      return null;
    }
    LLog.i(TAG, "loadJSSource with name " + name);
    InputStream inputStream = null;
    try {
      if (CORE_JS.equals(name)) {
        //  get DebugResource from DevTool
        if (LynxEnv.inst().isDevtoolEnabled()) {
          try {
            inputStream =
                LynxEnv.inst().getAppContext().getResources().getAssets().open(CORE_DEBUG_JS);
          } catch (IOException e) {
            e.printStackTrace();
          }
          if (inputStream != null) {
            nativeConfigLynxResourceSetting();
          }
        }
      }
      if (inputStream == null) {
        if (name.length() > FILE_SCHEME.length() && name.startsWith(FILE_SCHEME)) {
          String path = name.substring(FILE_SCHEME.length());
          File file;
          // Absolute path starting with /, relative path starting with ./
          if (path.startsWith("/")) {
            file = new File(path);
          } else {
            file = new File(LynxEnv.inst().getAppContext().getFilesDir(), path);
          }
          inputStream = new FileInputStream(file);
        } else if (name.length() > ASSETS_SCHEME.length() && name.startsWith(ASSETS_SCHEME)) {
          inputStream = LynxEnv.inst().getAppContext().getResources().getAssets().open(
              name.substring(ASSETS_SCHEME.length()));
        } else if (name.startsWith(LYNX_ASSETS_SCHEME)) {
          return loadLynxJSAsset(name);
        }
      }
      if (null != inputStream) {
        return toByteArray(inputStream);
      }
    } catch (IOException e) {
      LLog.e(TAG, "loadJSSource " + name + "with error message " + e.getMessage());
    } finally {
      if (inputStream != null) {
        try {
          inputStream.close();
        } catch (IOException e) {
          // ignore
        }
      }
    }
    LLog.e(TAG, "loadJSSource failed, can not load " + name);
    return null;
  }

  public byte[] loadLynxJSAsset(String name) {
    String assetName = name.substring(LYNX_ASSETS_SCHEME.length());
    InputStream inputStream = null;
    try {
      // If devtool is enabled, try to load [filename]_dev.js first.
      // If the file is not available, try to load [filename].js.
      if (LynxEnv.inst().isDevtoolEnabled()) {
        try {
          String[] assetSplitByDot = assetName.split("\\.");
          if (assetSplitByDot.length == 2) {
            // devAssetName = [filename]_dev.js
            String devAssetName = assetSplitByDot[0] + "_dev"
                + "." + assetSplitByDot[1];
            inputStream =
                LynxEnv.inst().getAppContext().getResources().getAssets().open(devAssetName);
          }
        } catch (IOException e) {
          // loading [devAssetName].js failed will try to load assetName
        }
      }
      // in prod or in dev but [filename]_dev.js unavailable
      if (null == inputStream) {
        inputStream = LynxEnv.inst().getAppContext().getResources().getAssets().open(assetName);
      }
      if (null != inputStream) {
        return toByteArray(inputStream);
      }
    } catch (IOException e) {
      LLog.e(TAG, "loadLynxJSAsset " + name + "with error message " + e.getMessage());
    } finally {
      if (inputStream != null) {
        try {
          inputStream.close();
        } catch (IOException e) {
          LLog.e(TAG, "loadLynxJSAsset inputStream close error: " + e.getMessage());
        }
      }
    }
    LLog.e(TAG, "loadLynxJSAsset failed, can not load " + name);
    return null;
  }

  private native void nativeConfigLynxResourceSetting();
}
