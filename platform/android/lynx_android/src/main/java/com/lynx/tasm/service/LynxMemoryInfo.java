// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.service;

import android.text.TextUtils;
import androidx.annotation.Nullable;
import androidx.annotation.StringDef;
import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;

/**
 * Data model for reporting lynx memory usage
 */
public class LynxMemoryInfo {
  // Lynx Memory Type enumeration
  @Retention(RetentionPolicy.SOURCE)
  @StringDef({TYPE_IMAGE})
  public @interface LynxMemoryType {}
  public static final String TYPE_IMAGE = "image";

  // Timing metrics section
  // Duration between start fetching bitmap and complete fetching bitmap
  private long fetchDuration = 0L;
  // The timestamp when complete fetching bitmap
  private long finishTimeStamp = 0L;
  // Duration between start fetching bitmap and start reporting memory usage
  private long completeDuration = 0L;
  // The timestamp when start fetching bitmap
  private long startTimeStamp = 0L;

  // Generic info section
  // SessionID of current LynxView, generated by LynxContext
  private String sessionId = "";
  // Current rendering phase, for example:load/update
  private String phase = "";
  // TemplateUrl of current LynxView, data reserved by LynxContext
  private String templateURL = "";

  // Element info section
  // Type of current element, for example: image
  private String type = "";
  // Memory cost of current view in Bytes, cast to Float type to unify with iOS
  private float memoryCost = 0.0f;
  // Width metric of current view
  private long viewWidth = 0L;
  // Height metric of current view
  private long viewHeight = 0L;
  // Memory address of current element object allocated, in Hex format
  private String address = "";
  // Memory address of the parent view object allocated, in Hex format
  private String parentAddress = "";

  // Image info section
  // Bitmap width
  private long width = 0L;
  // Bitmap height
  private long height = 0L;
  // Bitmap config, for example: ARGB_8888
  private String config = "";
  // Resource Schema of current image
  private String resourceURL = "";
  // Whether successfully loading current image
  private int isSuccess = 0;

  private int isFlattenAnim = 0;

  public @Nullable long getFetchDuration() {
    return fetchDuration;
  }

  public @Nullable long getFinishTimeStamp() {
    return finishTimeStamp;
  }

  public @Nullable long getCompleteDuration() {
    return completeDuration;
  }

  public @Nullable long getStartTimeStamp() {
    return startTimeStamp;
  }

  public @Nullable String getSessionId() {
    return sessionId;
  }

  public @Nullable String getPhase() {
    return phase;
  }

  public @Nullable String getTemplateURL() {
    return templateURL;
  }

  @LynxMemoryType
  public @Nullable String getType() {
    return type;
  }

  public float getMemoryCost() {
    return memoryCost;
  }

  public long getViewWidth() {
    return viewWidth;
  }

  public long getViewHeight() {
    return viewHeight;
  }

  public @Nullable String getAddress() {
    return address;
  }

  public @Nullable String getParentAddress() {
    return parentAddress;
  }

  public long getWidth() {
    return width;
  }

  public long getHeight() {
    return height;
  }

  public @Nullable String getConfig() {
    return config;
  }

  public @Nullable String getResourceURL() {
    return resourceURL;
  }

  public int getIsSuccess() {
    return isSuccess;
  }

  public int getIsFlattenAnim() {
    return isFlattenAnim;
  }

  private LynxMemoryInfo(Builder builder) {
    this.fetchDuration = builder.fetchDuration;
    this.finishTimeStamp = builder.finishTimeStamp;
    this.completeDuration = builder.completeDuration;
    this.startTimeStamp = builder.startTimeStamp;

    if (builder.sessionId != null) {
      this.sessionId = builder.sessionId;
    }
    if (builder.phase != null) {
      this.phase = builder.phase;
    }
    if (builder.templateURL != null) {
      this.templateURL = builder.templateURL;
    }

    if (builder.type != null) {
      this.type = builder.type;
    }
    this.memoryCost = builder.memoryCost;
    this.viewHeight = builder.viewHeight;
    this.viewWidth = builder.viewWidth;
    if (builder.address != null) {
      this.address = builder.address;
    }
    if (builder.parentAddress != null) {
      this.parentAddress = builder.parentAddress;
    }

    this.width = builder.width;
    this.height = builder.height;
    if (builder.config != null) {
      this.config = builder.config;
    }
    if (builder.resourceURL != null) {
      this.resourceURL = builder.resourceURL;
    }
    this.isSuccess = builder.isSuccess;
    this.isFlattenAnim = builder.isFlattenAnim;
  }

  public static class Builder {
    // Timing metrics section
    // Duration between start fetching bitmap and complete fetching bitmap
    private long fetchDuration = 0L;
    // The timestamp when complete fetching bitmap
    private long finishTimeStamp = 0L;
    // Duration between start fetching bitmap and start reporting memory usage
    private long completeDuration = 0L;
    // The timestamp when start fetching bitmap
    private long startTimeStamp = 0L;

    // Generic info section
    // SessionID of current LynxView, generated by LynxContext
    private String sessionId = "";
    // Current rendering phase, for example:load/update
    private String phase = "";
    // TemplateUrl of current LynxView, data reserved by LynxContext
    private String templateURL = "";

    // Element info section
    // Type of current element, for example: image
    private String type = "";
    // Memory cost of current view in Bytes, cast to Float type to unify with iOS
    private float memoryCost = 0.0f;
    // Width metric of current view
    private long viewWidth = -1;
    // Height metric of current view
    private long viewHeight = -1;
    // Memory address of current element object allocated, in Hex format
    private String address = "";
    // Memory address of the parent view object allocated, in Hex format
    private String parentAddress = "";

    // Image info section
    // Bitmap width
    private long width = -1;
    // Bitmap height
    private long height = -1;
    // Bitmap config, for example: ARGB_8888
    private String config = "";
    // Resource Schema of current image
    private String resourceURL = "";
    // Whether successfully loading current image
    private int isSuccess = 1;
    // Whether use animatable image in flatten ui node
    private int isFlattenAnim = 0;

    public Builder fetchDuration(long fetchDuration) {
      this.fetchDuration = fetchDuration;
      return this;
    }

    public Builder finishTimeStamp(long finishTimeStamp) {
      this.finishTimeStamp = finishTimeStamp;
      return this;
    }

    public Builder completeDuration(long completeDuration) {
      this.completeDuration = completeDuration;
      return this;
    }

    public Builder startTimeStamp(long startTimeStamp) {
      this.startTimeStamp = startTimeStamp;
      return this;
    }

    public Builder sessionId(String sessionId) {
      this.sessionId = sessionId;
      return this;
    }

    public Builder phase(String phase) {
      this.phase = phase;
      return this;
    }

    public Builder templateURL(String templateURL) {
      if (TextUtils.isEmpty(templateURL)) {
        this.templateURL = "";
      } else {
        this.templateURL = templateURL;
      }
      return this;
    }

    public Builder type(@LynxMemoryType String type) {
      this.type = type;
      return this;
    }

    public Builder memoryCost(float memoryCost) {
      this.memoryCost = memoryCost;
      return this;
    }

    public Builder viewWidth(long viewWidth) {
      this.viewWidth = viewWidth;
      return this;
    }

    public Builder viewHeight(long viewHeight) {
      this.viewHeight = viewHeight;
      return this;
    }

    public Builder address(String address) {
      this.address = address;
      return this;
    }

    public Builder parentAddress(String parentAddress) {
      this.parentAddress = parentAddress;
      return this;
    }

    public Builder width(long width) {
      this.width = width;
      return this;
    }

    public Builder height(long height) {
      this.height = height;
      return this;
    }

    public Builder config(String config) {
      this.config = config;
      return this;
    }

    public Builder resourceURL(String resourceURL) {
      this.resourceURL = resourceURL;
      return this;
    }

    public Builder isSuccess(int isSuccess) {
      this.isSuccess = isSuccess;
      return this;
    }

    public Builder isFlattenAnim(int isFlattenAnim) {
      this.isFlattenAnim = isFlattenAnim;
      return this;
    }

    public LynxMemoryInfo build() {
      return new LynxMemoryInfo(this);
    }
  }
}
