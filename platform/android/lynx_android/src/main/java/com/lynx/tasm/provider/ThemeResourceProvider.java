// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.provider;

import com.lynx.tasm.LynxView;
import com.lynx.tasm.theme.LynxTheme;

public interface ThemeResourceProvider {
  String translateResourceForTheme(String resId, LynxTheme theme, String themeKey, LynxView view);
}
