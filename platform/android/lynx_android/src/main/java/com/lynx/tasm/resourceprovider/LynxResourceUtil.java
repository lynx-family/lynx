// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.resourceprovider;

import android.content.ContentResolver;
import android.content.Context;
import android.net.Uri;
import androidx.annotation.Nullable;
import com.lynx.tasm.utils.StringUtils;
import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.InputStream;

/**
 * @apidoc
 * @brief Utility methods for Lynx resources.
 */
public final class LynxResourceUtil {
  private static final String FILE_SCHEME_PREFIX = ContentResolver.SCHEME_FILE + "://";

  private LynxResourceUtil() {}

  /**
   * @apidoc
   * @brief Read file content from a file uri.
   *
   * @param context Android context used to resolve relative file paths from app files dir.
   * @param uri File uri string whose scheme must be `file`.
   * @return Bytes read from the uri, or null when the input is invalid or read fails.
   */
  @Nullable
  public static byte[] readFileFromFileUri(@Nullable Context context, @Nullable String uri) {
    File file = resolveFileUri(context, uri);
    return readFile(file);
  }

  @Nullable
  private static byte[] readFile(@Nullable File file) {
    if (file == null) {
      return null;
    }

    InputStream inputStream = null;
    try {
      inputStream = new FileInputStream(file);
      return StringUtils.streamToBytes(inputStream);
    } catch (Exception e) {
      return null;
    } finally {
      if (inputStream != null) {
        try {
          inputStream.close();
        } catch (IOException ignored) {
        }
      }
    }
  }

  /**
   * @apidoc
   * @brief Read file content from a file uri.
   *
   * @param context Android context used to resolve relative file paths from app files dir.
   * @param uri File uri whose scheme must be `file`.
   * @return Bytes read from the uri, or null when the input is invalid or read fails.
   */
  @Nullable
  public static byte[] readFileFromFileUri(@Nullable Context context, @Nullable Uri uri) {
    File file = resolveFileUri(context, uri);
    return readFile(file);
  }

  /**
   * @apidoc
   * @brief Read file content from an Android ContentProvider uri.
   *
   * @param context Android context used to access ContentResolver.
   * @param uri ContentProvider uri whose scheme must be `content`.
   * @return Bytes read from the uri, or null when the input is invalid or read fails.
   */
  @Nullable
  public static byte[] readFileFromContentProvider(@Nullable Context context, @Nullable Uri uri) {
    if (context == null || uri == null || !ContentResolver.SCHEME_CONTENT.equals(uri.getScheme())) {
      return null;
    }

    InputStream inputStream = null;
    try {
      inputStream = context.getContentResolver().openInputStream(uri);
      return StringUtils.streamToBytes(inputStream);
    } catch (Exception e) {
      return null;
    } finally {
      if (inputStream != null) {
        try {
          inputStream.close();
        } catch (IOException ignored) {
        }
      }
    }
  }

  /**
   * @apidoc
   * @brief Read file content from an Android ContentProvider uri string.
   *
   * @param context Android context used to access ContentResolver.
   * @param uri ContentProvider uri string whose scheme must be `content`.
   * @return Bytes read from the uri, or null when the input is invalid or read fails.
   */
  @Nullable
  public static byte[] readFileFromContentProvider(
      @Nullable Context context, @Nullable String uri) {
    if (uri == null) {
      return null;
    }
    return readFileFromContentProvider(context, Uri.parse(uri));
  }

  @Nullable
  private static File resolveFileUri(@Nullable Context context, @Nullable String uri) {
    if (context == null || uri == null || !uri.startsWith(FILE_SCHEME_PREFIX)
        || uri.length() <= FILE_SCHEME_PREFIX.length()) {
      return null;
    }

    String rawPath = uri.substring(FILE_SCHEME_PREFIX.length());
    String path = rawPath.startsWith("/") ? Uri.parse(uri).getPath() : Uri.decode(rawPath);
    return resolveFilePath(context, path);
  }

  @Nullable
  private static File resolveFileUri(@Nullable Context context, @Nullable Uri uri) {
    if (context == null || uri == null || !ContentResolver.SCHEME_FILE.equals(uri.getScheme())) {
      return null;
    }

    String path = uri.getPath();
    if (path == null || path.length() == 0) {
      String uriString = uri.toString();
      if (!uriString.startsWith(FILE_SCHEME_PREFIX)
          || uriString.length() <= FILE_SCHEME_PREFIX.length()) {
        return null;
      }
      path = Uri.decode(uriString.substring(FILE_SCHEME_PREFIX.length()));
    }
    return resolveFilePath(context, path);
  }

  @Nullable
  private static File resolveFilePath(@Nullable Context context, @Nullable String path) {
    if (path == null || path.length() == 0) {
      return null;
    }
    if (path.startsWith("/")) {
      return new File(path);
    }
    if (context == null) {
      return null;
    }
    return new File(context.getFilesDir(), path);
  }
}
