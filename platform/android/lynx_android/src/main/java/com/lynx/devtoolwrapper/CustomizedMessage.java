// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.devtoolwrapper;

public class CustomizedMessage {
  private String mType;
  private String mData;
  private int mMark;

  public CustomizedMessage(String type, String data) {
    this(type, data, -1);
  }

  @Deprecated
  public CustomizedMessage(String type, String data, int mark) {
    this.mType = type;
    this.mData = data;
    this.mMark = mark;
  }

  public String getType() {
    return mType;
  }

  public void setType(String type) {
    this.mType = type;
  }

  public String getData() {
    return mData;
  }

  public void setData(String data) {
    this.mData = data;
  }

  public int getMark() {
    return mMark;
  }

  public void setMark(int mark) {
    this.mMark = mark;
  }
}
