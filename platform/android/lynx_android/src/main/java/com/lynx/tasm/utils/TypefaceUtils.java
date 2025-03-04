// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.utils;

import android.content.Context;
import android.graphics.Typeface;
import android.os.Process;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.OutputStream;

public class TypefaceUtils {
  /**
   * @param context
   * @return
   * @see androidx.core.graphics.TypefaceCompatBaseImpl
   */
  public static Typeface createFromBytes(Context context, byte[] bytes) {
    final File tmpFile = getTempFile(context);
    if (tmpFile == null) {
      return null;
    }
    try {
      if (!copyToFile(tmpFile, bytes)) {
        return null;
      }
      return Typeface.createFromFile(tmpFile.getPath());
    } catch (RuntimeException e) {
      // This was thrown from Typeface.createFromFile when a Typeface could not be loaded.
      // such as due to an invalid ttf or unreadable file. We don't want to throw that
      // exception anymore.
      return null;
    } finally {
      tmpFile.delete();
    }
  }

  private static boolean copyToFile(File tmpFile, byte[] bytes) {
    OutputStream outputStream = null;
    try {
      outputStream = new FileOutputStream(tmpFile);
      outputStream.write(bytes);
      outputStream.flush();
      return true;
    } catch (Exception e) {
      return false;
    } finally {
      try {
        if (outputStream != null) {
          outputStream.close();
        }
      } catch (Exception e) {
      }
    }
  }

  private static File getTempFile(Context context) {
    final String CACHE_FILE_PREFIX = ".lynx-font";

    final String prefix = CACHE_FILE_PREFIX + Process.myPid() + "-" + Process.myTid() + "-";
    for (int i = 0; i < 100; ++i) {
      final File file = new File(context.getCacheDir(), prefix + i);
      try {
        if (file.createNewFile()) {
          return file;
        }
      } catch (IOException e) {
        // ignore. Try next file.
      }
    }
    return null;
  }
}
