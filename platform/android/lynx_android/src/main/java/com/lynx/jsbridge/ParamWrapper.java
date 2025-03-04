// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.jsbridge;

public class ParamWrapper {
  private String mName;
  private Class<? extends LynxModule> mModuleClass;
  private Object mParam;

  public String getName() {
    return mName;
  }

  public void setName(String name) {
    this.mName = name;
  }

  public Class<? extends LynxModule> getModuleClass() {
    return mModuleClass;
  }

  public void setModuleClass(Class<? extends LynxModule> moduleClass) {
    this.mModuleClass = moduleClass;
  }

  public Object getParam() {
    return mParam;
  }

  public void setParam(Object param) {
    this.mParam = param;
  }

  @Override
  public String toString() {
    return "[" + mModuleClass.getSimpleName() + " - " + mName + "]";
  }
}
