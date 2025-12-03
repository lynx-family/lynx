// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.utils;

import android.graphics.Bitmap;
import android.util.Base64;
import java.io.ByteArrayOutputStream;
import java.io.IOException;

public class BitmapUtils {
  public static String bitmapToBase64v2(Bitmap bitmap, int quality) {
    if (bitmap == null) {
      return null;
    }

    // 对bitmap进行降采样处理，提高清晰度同时控制base64字符串大小
    Bitmap processedBitmap = bitmap;

    // 增加目标最大尺寸以提高清晰度
    int targetMaxSize = 600; // 目标最大尺寸（宽度或高度）从300提高到600
    int originalWidth = bitmap.getWidth();
    int originalHeight = bitmap.getHeight();

    // 计算缩放比例
    float scaleRatio = 1.0f;
    if (originalWidth > targetMaxSize || originalHeight > targetMaxSize) {
      float widthRatio = (float) targetMaxSize / originalWidth;
      float heightRatio = (float) targetMaxSize / originalHeight;
      scaleRatio = Math.min(widthRatio, heightRatio);

      // 创建降采样后的bitmap，使用双线性过滤提高缩放质量
      int scaledWidth = Math.round(originalWidth * scaleRatio);
      int scaledHeight = Math.round(originalHeight * scaleRatio);
      processedBitmap = Bitmap.createScaledBitmap(bitmap, scaledWidth, scaledHeight, true);
    }

    ByteArrayOutputStream outputStream = new ByteArrayOutputStream();

    // 如果质量参数过低，适当提高以保证基本清晰度
    int adjustedQuality = Math.max(quality, 30); // 确保质量不低于30

    // 使用JPEG格式，调整质量参数
    processedBitmap.compress(Bitmap.CompressFormat.JPEG, adjustedQuality, outputStream);
    byte[] bitmapBytes = outputStream.toByteArray();

    try {
      outputStream.close();
    } catch (Exception e) {
      e.printStackTrace();
    }

    // 释放临时创建的bitmap资源
    if (processedBitmap != bitmap) {
      processedBitmap.recycle();
    }

    return "data:image/jpeg;base64," + Base64.encodeToString(bitmapBytes, Base64.DEFAULT);
  }

  public static String bitmapToBase64WithQuality(Bitmap bitmap, int quality) {
    return bitmapToBase64(bitmap, Bitmap.CompressFormat.JPEG, quality, Base64.DEFAULT);
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
