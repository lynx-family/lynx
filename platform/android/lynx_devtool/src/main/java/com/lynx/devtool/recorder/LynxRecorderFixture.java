// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.devtool.recorder;

import android.util.Base64;
import com.lynx.devtool.RecorderController;
import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.nio.charset.StandardCharsets;
import java.util.HashMap;
import java.util.HashSet;
import java.util.zip.ZipEntry;
import java.util.zip.ZipInputStream;
import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

/** An extracted fixture, retained while either its replay view or JS context uses its assets. */
public final class LynxRecorderFixture {
  private static final long MAX_BYTES = 128L * 1024 * 1024;
  private static final long MAX_TEMPLATE_SOURCE_BYTES = 16L * 1024 * 1024;
  private final File mDirectory;
  private int mReferences = 1;
  final JSONArray actions;
  final JSONObject sharedData;
  final JSONObject config;
  final JSONArray components;

  private LynxRecorderFixture(File directory, JSONObject result) throws IOException, JSONException {
    mDirectory = directory;
    actions = normalizeActions(directory, result.getJSONArray("actions"));
    sharedData = result.getJSONObject("sharedData");
    File configFile = new File(directory, "config.json");
    config = configFile.isFile() ? new JSONObject(readText(configFile)) : new JSONObject();
    File componentsFile = new File(directory, "component_list.json");
    components =
        componentsFile.isFile() ? new JSONArray(readText(componentsFile)) : new JSONArray();
  }

  static LynxRecorderFixture open(InputStream input, File cache) throws IOException, JSONException {
    File directory = File.createTempFile("lynx-fixture-", "", cache);
    if (!directory.delete() || !directory.mkdir())
      throw new IOException("Cannot create fixture directory");
    try {
      extract(input, directory);
      byte[] bytes = RecorderController.nativeEvaluateFixture(directory.getAbsolutePath());
      if (bytes == null)
        throw new IOException("Fixture evaluation returned no data");
      JSONObject result = new JSONObject(new String(bytes, StandardCharsets.UTF_8));
      if (result.has("error"))
        throw new IOException(result.getString("error"));
      return new LynxRecorderFixture(directory, result);
    } catch (IOException | JSONException | RuntimeException e) {
      delete(directory);
      throw e;
    } catch (LinkageError e) {
      delete(directory);
      throw new IOException("Fixture replay native bridge unavailable", e);
    } catch (OutOfMemoryError e) {
      delete(directory);
      throw e;
    }
  }

  synchronized LynxRecorderFixture retain() {
    if (mReferences == 0)
      throw new IllegalStateException("Fixture already released");
    ++mReferences;
    return this;
  }

  synchronized void release() {
    if (mReferences > 0 && --mReferences == 0)
      delete(mDirectory);
  }

  String directory() {
    return mDirectory.getAbsolutePath();
  }

  // ZipInputStream writes every entry as a regular file/directory, never as a symbolic link.
  static void extract(InputStream input, File directory) throws IOException {
    long total = 0;
    HashSet<String> entries = new HashSet<>();
    byte[] buffer = new byte[8192];
    try (ZipInputStream zip = new ZipInputStream(input)) {
      ZipEntry entry;
      while ((entry = zip.getNextEntry()) != null) {
        String name = entry.getName();
        if (entry.isDirectory())
          name = name.substring(0, name.length() - 1);
        File target = resolve(directory, name);
        if (!entries.add(name) || entries.size() > 65535)
          throw new IOException("Invalid fixture entries");
        if (entry.isDirectory()) {
          if (zip.read() != -1)
            throw new IOException("Fixture directory contains data");
          if (!target.isDirectory() && !target.mkdirs())
            throw new IOException("Cannot create fixture directory");
          continue;
        }
        File parent = target.getParentFile();
        if (!parent.isDirectory() && !parent.mkdirs())
          throw new IOException("Cannot create fixture parent");
        try (FileOutputStream output = new FileOutputStream(target)) {
          int count;
          while ((count = zip.read(buffer)) != -1) {
            total += count;
            if (total > MAX_BYTES)
              throw new IOException("Fixture exceeds size limit");
            output.write(buffer, 0, count);
          }
        }
      }
    }
    if (!new File(directory, "fixture.js").isFile())
      throw new IOException("Missing fixture.js");
  }

  static File resolve(File root, String path) throws IOException {
    if (path.indexOf('\\') >= 0 || path.indexOf(':') >= 0)
      throw new IOException("Invalid fixture path");
    for (String part : path.split("/", -1)) {
      if (part.isEmpty() || part.equals(".") || part.equals(".."))
        throw new IOException("Invalid fixture path");
    }
    File file = new File(root, path).getCanonicalFile();
    if (!file.getPath().startsWith(root.getCanonicalPath() + File.separator))
      throw new IOException("Invalid fixture path");
    return file;
  }

  static JSONArray normalizeActions(File directory, JSONArray actions)
      throws IOException, JSONException {
    JSONArray result = new JSONArray();
    HashMap<String, String> templates = new HashMap<>();
    long templateBytes = 0;
    for (int i = 0; i < actions.length(); ++i) {
      JSONObject action = actions.getJSONObject(i);
      String name = action.getString("functionName");
      JSONObject params = action.getJSONObject("params");
      if (name.equals("loadTemplate")) {
        String path = params.getString("templateAsset");
        if (path.startsWith("assets/"))
          path = path.substring("assets/".length());
        File asset = resolve(new File(directory, "assets"), path);
        String source = templates.get(asset.getPath());
        if (source == null) {
          byte[] template = readBytes(asset, (MAX_TEMPLATE_SOURCE_BYTES - templateBytes) / 4 * 3);
          if (template.length == 0)
            throw new IOException("Empty fixture template");
          source = Base64.encodeToString(template, Base64.NO_WRAP);
          templateBytes += source.length();
          templates.put(asset.getPath(), source);
        }
        params.put("source", source);
      }
      JSONObject record = new JSONObject();
      record.put("Function Name", name);
      record.put("Record Time", 0);
      record.put("RecordMillisecond", action.getLong("delayMs"));
      record.put("Params", params);
      result.put(record);
    }
    return result;
  }

  private static String readText(File file) throws IOException {
    return new String(readBytes(file, 16L * 1024 * 1024), StandardCharsets.UTF_8);
  }

  private static byte[] readBytes(File file, long limit) throws IOException {
    if (file.length() > limit)
      throw new IOException("Fixture file exceeds size limit");
    try (InputStream input = new FileInputStream(file);
         ByteArrayOutputStream output = new ByteArrayOutputStream()) {
      byte[] buffer = new byte[8192];
      int count;
      while ((count = input.read(buffer)) != -1) {
        if ((long) output.size() + count > limit)
          throw new IOException("Fixture file exceeds size limit");
        output.write(buffer, 0, count);
      }
      return output.toByteArray();
    }
  }

  private static void delete(File file) {
    File[] children = file.listFiles();
    if (children != null)
      for (File child : children) delete(child);
    file.delete();
  }
}
