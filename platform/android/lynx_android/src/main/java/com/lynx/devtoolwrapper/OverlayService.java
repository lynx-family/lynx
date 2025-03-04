// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.devtoolwrapper;

import android.app.Dialog;
import java.util.ArrayList;

public interface OverlayService {
  public ArrayList<Dialog> getGlobalOverlayNGView();

  public ArrayList<Integer> getAllVisibleOverlaySign();
}
