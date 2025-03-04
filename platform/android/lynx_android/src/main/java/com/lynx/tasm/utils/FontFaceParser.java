// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.utils;

import androidx.annotation.NonNull;
import com.lynx.tasm.LynxError;
import com.lynx.tasm.LynxSubErrorCode;
import com.lynx.tasm.base.LLog;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.tasm.fontface.FontFace;
import java.util.Map;

public class FontFaceParser {
  private final static String TAG = "FontFaceParser";
  private final static String SRC_FORMAT_ERROR = "Src format error";

  public static String trim(String s) {
    if (s == null) {
      return null;
    }
    int start = 0;
    /*  trim space and quotes in both ends

        multiple font-family might be written as :'family 1', 'family 2';
        it will be split to |'family 1'| and | 'family 2'|
        we need to remove space and quotes to get the correct family name
    */
    while (start < s.length()
        && (s.charAt(start) == ' ' || s.charAt(start) == '"' || s.charAt(start) == '\'')) {
      start++;
    }
    if (start >= s.length()) {
      return null;
    }
    int end = s.length() - 1;
    while (end > start && (s.charAt(end) == ' ' || s.charAt(end) == '"' || s.charAt(end) == '\'')) {
      end--;
    }
    return s.substring(start, end + 1);
  }

  public static FontFace parse(LynxContext context, String fontFamily) {
    fontFamily = trim(fontFamily);
    if (fontFamily == null) {
      reportSrcFormatError(context, "Font-family is empty", "");
      return null;
    }

    Map map = context.getFontFaces(fontFamily);
    if (map == null) {
      return null;
    }
    FontFace fontFace = new FontFace();
    fontFace.setFontFamily(fontFamily);
    String resolvedSrc = "";
    for (Object value : map.values()) {
      if (!(value instanceof String)) {
        reportSrcFormatError(context, "Src is not string", value.toString());
        continue;
      }
      final String src = (String) value;
      int index = 0;
      final int length = src.length();
      /*
      example src:
       url('http://developer.mozilla.org/VeraSeBd1.ttf'),
       url("http://developer.mozilla.org/VeraSeBd2.ttf"),
       local("Monosspace"),
       local('Monosspace2'),
       url("http://developer.mozilla.org/VeraSeBd3.ttf"),
       url(http://developer.mozilla.org/VeraSeBd4.ttf),
       url(data:application/x-font-woff;charset=utf-8;base64,d09GRgABAAAAAHwwAB....MAA==)
      we will get:
       1.http://developer.mozilla.org/VeraSeBd1.ttf
       2.http://developer.mozilla.org/VeraSeBd2.ttf
       3.Monosspace
       4.Monosspace2
       5.http://developer.mozilla.org/VeraSeBd3.ttf
       6.http://developer.mozilla.org/VeraSeBd4.ttf
       7.data:application/x-font-woff;charset=utf-8;base64,d09GRgABAAAAAHwwAB....MAA==
       */
      String urlIdentify = "url("; // maybe start with url("  or url('
      String localIdentify = "local(";
      boolean isLegal = true;
      while (index < length) {
        final int urlStart = src.indexOf(urlIdentify, index);
        final int localStart = src.indexOf(localIdentify, index);
        if (urlStart == -1 && localStart == -1) {
          break;
        }
        if (urlStart != -1 && localStart != -1) {
          // both exist url(  and local(
          if (urlStart < localStart) {
            final int end = src.indexOf(")", urlStart);
            if (end != -1) {
              index = end + 2; // 2-> ),
              String url = src.substring(urlStart + urlIdentify.length(), end);
              url = trimSrc(url);
              if (url == null) {
                // illegal
                isLegal = false;
                break;
              }
              fontFace.addUrl(url);
              resolvedSrc = url;
            } else {
              // illegal
              isLegal = false;
              break;
            }
          } else {
            final int end = src.indexOf(")", localStart);
            if (end != -1) {
              index = end + 2; // 2-> ),
              String local = src.substring(localStart + localIdentify.length(), end);
              local = trimSrc(local);
              if (local == null) {
                // illegal
                isLegal = false;
                break;
              }
              fontFace.addLocal(local);
              resolvedSrc = local;
            } else {
              // illegal
              isLegal = false;
              break;
            }
          }
        } else if (urlStart != -1) {
          // only exist url(
          final int end = src.indexOf(")", urlStart);
          if (end != -1) {
            index = end + 2; // 2-> ),
            String url = src.substring(urlStart + urlIdentify.length(), end);
            url = trimSrc(url);
            if (url == null) {
              // illegal
              isLegal = false;
              break;
            }
            fontFace.addUrl(url);
            resolvedSrc = url;
          } else {
            // illegal
            isLegal = false;
            break;
          }
        } else {
          // only exist local(
          final int end = src.indexOf(")", localStart);
          if (end != -1) {
            index = end + 2; // 2-> ),
            String local = src.substring(localStart + localIdentify.length(), end);
            local = trimSrc(local);
            if (local == null) {
              // illegal
              isLegal = false;
              break;
            }
            fontFace.addLocal(local);
            resolvedSrc = local;
          } else {
            // illegal
            isLegal = false;
            break;
          }
        }
      }
      if (!isLegal) {
        reportSrcFormatError(context, SRC_FORMAT_ERROR, src);
      }
      if (resolvedSrc.endsWith(".woff") || resolvedSrc.endsWith(".woff2")) {
        reportFontFileFormatError(context, "The woff file format is not supported on Android", src);
      }
    }
    return fontFace;
  }

  /**
   * remove prefix or suffix  " and '
   *
   * @param str
   * @return
   */
  private static String trimSrc(String str) {
    if (str == null) {
      return null;
    }
    int start = 0;
    int end = str.length();
    if (str.startsWith("'") || str.startsWith("\"")) {
      start++;
    }
    if (str.endsWith("'") || str.endsWith("\"")) {
      end--;
    }
    if (start > end) {
      return null;
    }
    return str.substring(start, end);
  }

  private static void reportSrcFormatError(
      LynxContext context, @NonNull String errorMsg, @NonNull String src) {
    LynxError error = new LynxError(LynxSubErrorCode.E_RESOURCE_FONT_SRC_FORMAT_ERROR, errorMsg);
    context.reportResourceError(src, "font", error);
    LLog.e(TAG, errorMsg + ",src:" + src);
  }

  private static void reportFontFileFormatError(
      LynxContext context, @NonNull String errorMsg, @NonNull String src) {
    LynxError error =
        new LynxError(LynxSubErrorCode.E_RESOURCE_FONT_FILE_FORMAT_NOT_SUPPORTED, errorMsg);
    context.reportResourceError(src, "font", error);
    LLog.e(TAG, errorMsg + ",src:" + src);
  }
}
