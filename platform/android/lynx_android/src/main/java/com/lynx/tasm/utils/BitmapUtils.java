// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.utils;

import android.graphics.Bitmap;
import android.util.Base64;
import java.io.ByteArrayOutputStream;
import java.io.IOException;

public class BitmapUtils {
  public static String bitmapToFormattedBase64(Bitmap bitmap, int quality) {
    if (bitmap == null) {
      return null;
    }
    Bitmap processedBitmap = bitmap;

    // set target max size to 600，to balance the quality and size.
    int targetMaxSize = 600;
    int originalWidth = bitmap.getWidth();
    int originalHeight = bitmap.getHeight();

    // Calculate the scale ratio to ensure it does not exceed the target max size.
    float scaleRatio = 1.0f;
    if (originalWidth > targetMaxSize || originalHeight > targetMaxSize) {
      float widthRatio = (float) targetMaxSize / originalWidth;
      float heightRatio = (float) targetMaxSize / originalHeight;
      scaleRatio = Math.min(widthRatio, heightRatio);
      // Create a downsampled bitmap using bilinear filtering to improve scaling quality.
      int scaledWidth = Math.round(originalWidth * scaleRatio);
      int scaledHeight = Math.round(originalHeight * scaleRatio);
      processedBitmap = Bitmap.createScaledBitmap(bitmap, scaledWidth, scaledHeight, true);
    }
    ByteArrayOutputStream outputStream = new ByteArrayOutputStream();
    // If the quality parameter is too low, raise it to ensure basic clarity.
    int adjustedQuality = Math.max(quality, 30); // Ensure quality is not lower than 30
    // Use JPEG format with the adjusted quality parameter.
    processedBitmap.compress(Bitmap.CompressFormat.JPEG, adjustedQuality, outputStream);
    byte[] bitmapBytes = outputStream.toByteArray();
    try {
      outputStream.close();
    } catch (Exception e) {
      e.printStackTrace();
    }
    // Recycle the temporary downsampled bitmap if it is different from the original bitmap.
    if (processedBitmap != bitmap) {
      processedBitmap.recycle();
    }
    return "data:image/jpeg;base64," + Base64.encodeToString(bitmapBytes, Base64.NO_WRAP);
  }

  public static String bitmapToBase64WithQuality(Bitmap bitmap, int quality) {
    return bitmapToBase64(bitmap, Bitmap.CompressFormat.JPEG, quality, Base64.NO_WRAP);
  }

  public static String bitmapToBase64(
      Bitmap bitmap, Bitmap.CompressFormat format, int quality, int flags) {
    String result = null;
    ByteArrayOutputStream baos = null;
    try {
      if (bitmap != null) {
        baos = new ByteArrayOutputStream();
        bitmap.compress(format, quality, baos);
        baos.flush();
        baos.close();
        byte[] bitmapBytes = baos.toByteArray();
        result = Base64.encodeToString(bitmapBytes, flags);
      }
    } catch (IOException e) {
      e.printStackTrace();
    } finally {
      try {
        if (baos != null) {
          baos.flush();
          baos.close();
        }
      } catch (IOException e) {
        e.printStackTrace();
      }
    }
    return result;
  }
}
