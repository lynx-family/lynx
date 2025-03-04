// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm;

public enum LynxLoadOption {
  // DUMP_ELEMENT:enable to dump a TemplateBundle with element tree after first load through
  // onTemplateBundleReady in LynxViewClient
  DUMP_ELEMENT(1 << 1),
  // RECYCLE_TEMPLATE_BUNDLE:enable to provide a reusable TemplateBundle after template is decoding
  // through onTemplateBundleReady in LynxViewClient
  RECYCLE_TEMPLATE_BUNDLE(1 << 2),
  // PROCESS_LAYOUT_WITHOUT_UI_FLUSH:For calculating layout results without UI operations, used for
  // height calculation/pre-layout scenarios.
  PROCESS_LAYOUT_WITHOUT_UI_FLUSH(1 << 3);

  private int mId;

  LynxLoadOption(int id) {
    mId = id;
  }

  public int id() {
    return mId;
  }
}
