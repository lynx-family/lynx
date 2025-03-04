// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.testbench;

public class TestBenchEnv {
  private static TestBenchEnv instance;
  public String mUriKey = "uri";
  public String mTestBenchUrlPrefix = "file://testbench?";

  public static synchronized TestBenchEnv getInstance() {
    if (instance == null) {
      instance = new TestBenchEnv();
    }
    return instance;
  }
}
