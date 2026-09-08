// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.render;

import com.lynx.tasm.behavior.LynxContext;
import com.lynx.tasm.behavior.shadow.text.TextMeasurer;
import com.lynx.tasm.behavior.ui.LynxBaseUI;
import com.lynx.tasm.behavior.ui.image.LynxImageManager;
import com.lynx.tasm.service.ILynxTextService.Page;
import java.nio.ByteBuffer;

/** Services required by a fragment layer to acquire and apply a DisplayList. */
public interface LayerRenderContext {
  LynxContext getLynxContext();

  TextMeasurer getTextMeasurer();

  ByteBuffer getDisplayListItemsBuffer(int id);

  ByteBuffer getDisplayListDataBuffer(int id);

  LynxBaseUI getFragmentLayer(int sign);

  LynxImageManager getImage(int imageKey);

  Page getTextBundle(int textKey);
}
