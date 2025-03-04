// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.ref;

class Wrap {
  private Object obj;

  public Wrap(Object obj) {
    this.obj = obj;
  }

  <T> T get() {
    return (T) obj;
  }
}
