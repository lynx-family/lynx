// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.devtool;

import android.content.SharedPreferences;
import androidx.annotation.Nullable;
import java.util.HashMap;
import java.util.Map;
import java.util.Set;

class MockSharedPreferences implements SharedPreferences {
  private final Map<String, Object> preferences = new HashMap<>();
  private final Editor editor = new MockEditor();

  @Override
  public Editor edit() {
    return editor;
  }

  @Override
  public void registerOnSharedPreferenceChangeListener(OnSharedPreferenceChangeListener listener) {
    // TODO when need
  }

  @Override
  public void unregisterOnSharedPreferenceChangeListener(
      OnSharedPreferenceChangeListener listener) {
    // TODO when need
  }

  @Override
  public Map<String, ?> getAll() {
    return preferences;
  }

  @Override
  public String getString(String key, String defValue) {
    Object value = preferences.get(key);
    return value != null ? (String) value : defValue;
  }

  @Nullable
  @Override
  public Set<String> getStringSet(String key, @Nullable Set<String> defValues) {
    // TODO when need
    return null;
  }

  @Override
  public int getInt(String key, int defValue) {
    Object object = preferences.get(key);
    if (object == null) {
      return defValue;
    }
    return (int) object;
  }

  @Override
  public long getLong(String key, long defValue) {
    Object object = preferences.get(key);
    if (object == null) {
      return defValue;
    }
    return (long) object;
  }

  @Override
  public float getFloat(String key, float defValue) {
    Object object = preferences.get(key);
    if (object == null) {
      return defValue;
    }
    return (float) object;
  }

  @Override
  public boolean getBoolean(String key, boolean defValue) {
    Object object = preferences.get(key);
    if (object == null) {
      return defValue;
    }
    return (boolean) object;
  }

  @Override
  public boolean contains(String key) {
    return preferences.containsKey(key);
  }

  class MockEditor implements SharedPreferences.Editor {
    @Override
    public SharedPreferences.Editor putString(String key, String value) {
      preferences.put(key, value);
      return this;
    }

    @Override
    public SharedPreferences.Editor putStringSet(String key, @Nullable Set<String> values) {
      // TODO when need
      return null;
    }

    @Override
    public SharedPreferences.Editor putInt(String key, int value) {
      preferences.put(key, value);
      return this;
    }

    @Override
    public SharedPreferences.Editor putLong(String key, long value) {
      preferences.put(key, value);
      return this;
    }

    @Override
    public SharedPreferences.Editor putFloat(String key, float value) {
      preferences.put(key, value);
      return this;
    }

    @Override
    public SharedPreferences.Editor putBoolean(String key, boolean value) {
      preferences.put(key, value);
      return this;
    }

    @Override
    public SharedPreferences.Editor remove(String key) {
      preferences.remove(key);
      return this;
    }

    @Override
    public SharedPreferences.Editor clear() {
      preferences.clear();
      return this;
    }

    @Override
    public boolean commit() {
      return true;
    }

    @Override
    public void apply() {
      commit();
    }
  }
}
