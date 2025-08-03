// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.group;

import com.lynx.tasm.TemplateData;

/**
 * Interface provider for accessing LynxViewGroup prop getter;
 */
public interface ILynxViewGroup extends ILynxViewConfigProvider {
  String getUrl();

  TemplateData getGlobalProps();
}
