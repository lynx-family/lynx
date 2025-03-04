// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm;

import com.lynx.tasm.base.TraceEvent;

public class LynxEngineBuilder extends LynxViewBuilder {
  public LynxEngineBuilder() {
    super();
    // If the runtime context is Application, there may be exceptions during the JSB invocation
    // process, so the default behavior is to pause JS execution.
    setEnablePendingJsTask(true);
  }

  /**
   * build LynxEngine for context free
   * @return LynxEngine
   */
  public ILynxEngine build() {
    TraceEvent.beginSection("LynxEngineBuilder.build");
    ILynxEngine templateRender = new LynxTemplateRender(this);
    TraceEvent.endSection("LynxEngineBuilder.build");
    return templateRender;
  }
}
