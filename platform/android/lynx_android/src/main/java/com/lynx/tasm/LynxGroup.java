// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm;

import androidx.annotation.Nullable;
import com.lynx.tasm.base.LLog;
import java.util.HashMap;
import java.util.Map;

/**
 * Caution: destroy() should be called to release native memory when using the whiteboard.
 */
public class LynxGroup {
  private boolean mEnableV8;
  // FIXME(heshan): ensure thread-safe.
  private static int sNextID = 0;

  public static String SINGNLE_GROUP = "-1";

  private String mGroupName;

  /**
   * The `mID` property should ideally be set to `SINGLE_GROUP`
   * Other values are considered experimental features and should be used with caution.
   *
   * Note:
   * - The `id` parameter in any constructor directly sets this `mID` property.
   * - The `id` parameter should ideally be set to `SINGLE_GROUP`.
   * - Setting `mID` to other values is experimental and should be used with caution.
   */
  private String mID;

  @Nullable private String[] mPreloadJSPaths;

  private boolean mEnableJSGroupThread;

  private LynxWhiteBoard mWhiteBoard;

  private Map<String, Object> mConfig;

  private LynxGroup(LynxGroupBuilder builder) {
    this.mGroupName = builder.mGroupName;
    this.mID = builder.mID != null ? builder.mID : generateID();
    this.mPreloadJSPaths = builder.mPreloadJSPaths;
    this.mEnableJSGroupThread = builder.mEnableJSGroupThread;
    this.mEnableV8 = builder.mEnableV8;
    this.mConfig = builder.mConfig;

    if (builder.mEnableWhiteBoard) {
      this.mWhiteBoard = new LynxWhiteBoard();
    }

    LLog.i("LynxGroup", "LynxGroup init with name " + mGroupName + ", id: " + mID);
  }

  protected LynxGroup(String name, String id, @Nullable String[] preloadJSPaths,
      boolean enableJSGroupThread, boolean enableWhiteBoard) {
    this(new LynxGroup.LynxGroupBuilder()
             .setGroupName(name)
             .setID(id)
             .setPreloadJSPaths(preloadJSPaths)
             .setEnableJSGroupThread(enableJSGroupThread)
             .setEnableWhiteBoard(enableWhiteBoard));
  }

  static String generateID() {
    String id = String.format("%d", sNextID);
    sNextID++;
    return id;
  }

  public String getID() {
    return mID;
  }

  @Nullable
  public String[] getPreloadJSPaths() {
    return mPreloadJSPaths;
  }

  public boolean enableJSGroupThread() {
    return mEnableJSGroupThread;
  }

  public boolean enableV8() {
    return mEnableV8;
  }

  public String getStringConfig(String key) {
    if (mConfig == null) {
      return null;
    }
    Object value = mConfig.get(key);
    if (value != null && value instanceof String) {
      return (String) value;
    }
    return null;
  }

  public boolean getBoolConfig(String key) {
    if (mConfig == null) {
      return false;
    }
    Object value = mConfig.get(key);
    if (value != null && value instanceof Boolean) {
      return ((Boolean) value).booleanValue();
    }

    return false;
  }

  /**
   * It's necessary to invoke the destroy method to avoid memory leaks when using the whiteboard.
   */
  public void destroy() {
    if (mWhiteBoard != null) {
      mWhiteBoard.destroy();
    }
  }

  public long getWhiteBoardPtr() {
    return mWhiteBoard != null ? mWhiteBoard.getPtr() : 0;
  }

  public static class LynxGroupBuilder {
    protected String mGroupName;
    protected String mID;
    protected String[] mPreloadJSPaths;
    protected boolean mEnableV8;
    protected boolean mEnableJSGroupThread;
    protected boolean mEnableWhiteBoard;
    protected Map<String, Object> mConfig;

    public LynxGroupBuilder() {}

    public LynxGroupBuilder setGroupName(String groupName) {
      this.mGroupName = groupName;
      return this;
    }

    /**
     * Sets the mID property for `LynxGroup`.
     * Please refer to the mID property documentation int the `LynxGroup` class for usage
     * guidelines.
     *
     * @param id The id to set for the `mID` property.
     */
    public LynxGroupBuilder setID(String id) {
      this.mID = id;
      return this;
    }

    public LynxGroupBuilder setPreloadJSPaths(String[] preloadJSPaths) {
      this.mPreloadJSPaths = preloadJSPaths;
      return this;
    }

    public LynxGroupBuilder setEnableJSGroupThread(boolean enableJSGroupThread) {
      this.mEnableJSGroupThread = enableJSGroupThread;
      return this;
    }

    public LynxGroupBuilder setEnableWhiteBoard(boolean enableWhiteBoard) {
      this.mEnableWhiteBoard = enableWhiteBoard;
      return this;
    }

    public LynxGroupBuilder setEnableV8(boolean enableV8) {
      this.mEnableV8 = enableV8;
      return this;
    }

    public LynxGroupBuilder setStringConfig(String key, String value) {
      if (mConfig == null) {
        mConfig = new HashMap<>();
      }
      mConfig.put(key, value);
      return this;
    }

    public LynxGroupBuilder setBoolConfig(String key, boolean value) {
      if (mConfig == null) {
        mConfig = new HashMap<>();
      }
      mConfig.put(key, Boolean.valueOf(value));
      return this;
    }

    public LynxGroup build() {
      return new LynxGroup(this);
    }
  }
}
