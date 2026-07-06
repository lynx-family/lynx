// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.resourceprovider;

import static org.junit.Assert.assertArrayEquals;
import static org.junit.Assert.assertNull;

import android.content.Context;
import android.net.Uri;
import androidx.test.platform.app.InstrumentationRegistry;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import org.junit.Test;

public class LynxResourceUtilTest {
  private static final Uri CONTENT_URI =
      Uri.parse("content://com.lynx.test.resourceprovider/resource");

  @Test
  public void readFileFromContentProviderReturnsBytesForContentUri() {
    byte[] expected = "Hello Lynx ContentProvider".getBytes(StandardCharsets.UTF_8);
    TestContentProvider.setContent(expected);

    byte[] actual = LynxResourceUtil.readFileFromContentProvider(getContext(), CONTENT_URI);

    assertArrayEquals(expected, actual);
  }

  @Test
  public void readFileFromContentProviderReturnsBytesForContentUriString() {
    byte[] expected = "Hello Lynx ContentProvider String".getBytes(StandardCharsets.UTF_8);
    TestContentProvider.setContent(expected);

    byte[] actual =
        LynxResourceUtil.readFileFromContentProvider(getContext(), CONTENT_URI.toString());

    assertArrayEquals(expected, actual);
  }

  @Test
  public void readFileFromContentProviderReturnsNullForInvalidInput() {
    assertNull(LynxResourceUtil.readFileFromContentProvider(null, CONTENT_URI));
    assertNull(LynxResourceUtil.readFileFromContentProvider(getContext(), (Uri) null));
    assertNull(LynxResourceUtil.readFileFromContentProvider(getContext(), (String) null));
    assertNull(
        LynxResourceUtil.readFileFromContentProvider(getContext(), Uri.parse("file:///tmp/a")));
  }

  @Test
  public void readFileFromFileUriReturnsBytesForAbsolutePath() throws IOException {
    byte[] expected = "Hello Lynx file absolute path".getBytes(StandardCharsets.UTF_8);
    File file = File.createTempFile("lynx-resource-util", ".txt", getContext().getCacheDir());
    writeBytes(file, expected);

    try {
      byte[] actual =
          LynxResourceUtil.readFileFromFileUri(getContext(), "file://" + file.getAbsolutePath());

      assertArrayEquals(expected, actual);
      assertArrayEquals(expected,
          LynxResourceUtil.readFileFromFileUri(
              getContext(), Uri.parse("file://" + file.getAbsolutePath())));
    } finally {
      file.delete();
    }
  }

  @Test
  public void readFileFromFileUriReturnsBytesForEncodedAbsolutePath() throws IOException {
    byte[] expected = "Hello Lynx encoded file path".getBytes(StandardCharsets.UTF_8);
    File file = new File(getContext().getCacheDir(), "lynx resource util 中文.txt");
    writeBytes(file, expected);
    Uri uri = Uri.fromFile(file);

    try {
      assertArrayEquals(expected, LynxResourceUtil.readFileFromFileUri(getContext(), uri));
      assertArrayEquals(
          expected, LynxResourceUtil.readFileFromFileUri(getContext(), uri.toString()));
    } finally {
      file.delete();
    }
  }

  @Test
  public void readFileFromFileUriReturnsBytesForRelativeFilesDirPath() throws IOException {
    byte[] expected = "Hello Lynx file relative path".getBytes(StandardCharsets.UTF_8);
    File file = new File(getContext().getFilesDir(), "lynx_resource_util_relative.txt");
    writeBytes(file, expected);

    try {
      byte[] actual =
          LynxResourceUtil.readFileFromFileUri(getContext(), "file://" + file.getName());

      assertArrayEquals(expected, actual);
    } finally {
      file.delete();
    }
  }

  @Test
  public void readFileFromFileUriReturnsNullForInvalidInput() {
    assertNull(LynxResourceUtil.readFileFromFileUri(null, "file:///tmp/a"));
    assertNull(LynxResourceUtil.readFileFromFileUri(getContext(), (String) null));
    assertNull(LynxResourceUtil.readFileFromFileUri(getContext(), "file://"));
    assertNull(LynxResourceUtil.readFileFromFileUri(
        getContext(), "content://com.lynx.test.resourceprovider/resource"));
  }

  private Context getContext() {
    return InstrumentationRegistry.getInstrumentation().getContext();
  }

  private void writeBytes(File file, byte[] bytes) throws IOException {
    FileOutputStream outputStream = new FileOutputStream(file);
    try {
      outputStream.write(bytes);
    } finally {
      outputStream.close();
    }
  }
}
