// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm;

public enum LynxLoadMode {
  // NORMAL:Load Template as usual
  // PRE_PAINTING:Pending JS Events when load template, events will be send when update, layout is
  // blocked until update
  // PRE_PAINTING_DRAW: Pending JS Events when load template, events will be
  // send when update
  NORMAL(0),
  PRE_PAINTING(1),
  PRE_PAINTING_DRAW(2);

  private int mId;

  LynxLoadMode(int id) {
    mId = id;
  }

  public int id() {
    return mId;
  }
}
