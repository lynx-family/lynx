// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.explorer.shell;

import androidx.annotation.Keep;
import com.lynx.explorer.utils.QueryMapUtils;
import com.lynx.tasm.LynxView;
import java.util.Map;

@Keep
public interface TemplateLoader {
  void load(LynxView view, String url, QueryMapUtils queryMap);

  Map<String, Object> parseData(String url);
}
