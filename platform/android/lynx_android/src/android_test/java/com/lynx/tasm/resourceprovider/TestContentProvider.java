// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.resourceprovider;

import android.content.ContentProvider;
import android.content.ContentValues;
import android.database.Cursor;
import android.net.Uri;
import android.os.ParcelFileDescriptor;
import java.io.FileNotFoundException;
import java.io.IOException;

public class TestContentProvider extends ContentProvider {
  private static volatile byte[] sContent;

  public static void setContent(byte[] content) {
    sContent = content == null ? null : content.clone();
  }

  @Override
  public boolean onCreate() {
    return true;
  }

  @Override
  public Cursor query(
      Uri uri, String[] projection, String selection, String[] selectionArgs, String sortOrder) {
    return null;
  }

  @Override
  public String getType(Uri uri) {
    return null;
  }

  @Override
  public Uri insert(Uri uri, ContentValues values) {
    return null;
  }

  @Override
  public int delete(Uri uri, String selection, String[] selectionArgs) {
    return 0;
  }

  @Override
  public int update(Uri uri, ContentValues values, String selection, String[] selectionArgs) {
    return 0;
  }

  @Override
  public ParcelFileDescriptor openFile(Uri uri, String mode) throws FileNotFoundException {
    if (sContent == null) {
      throw new FileNotFoundException(uri.toString());
    }

    try {
      ParcelFileDescriptor[] pipe = ParcelFileDescriptor.createPipe();
      byte[] content = sContent.clone();
      ParcelFileDescriptor writeSide = pipe[1];
      new Thread(new Runnable() {
        @Override
        public void run() {
          ParcelFileDescriptor.AutoCloseOutputStream outputStream =
              new ParcelFileDescriptor.AutoCloseOutputStream(writeSide);
          try {
            outputStream.write(content);
          } catch (IOException ignored) {
          } finally {
            try {
              outputStream.close();
            } catch (IOException ignored) {
            }
          }
        }
      }).start();
      return pipe[0];
    } catch (IOException e) {
      throw new FileNotFoundException(e.getMessage());
    }
  }
}
