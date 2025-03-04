// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.explorer;

import com.lynx.tasm.behavior.Behavior;
import com.lynx.tasm.behavior.BehaviorBundle;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.tasm.behavior.shadow.ShadowNode;
import com.lynx.tasm.behavior.ui.LynxFlattenUI;
import com.lynx.tasm.behavior.ui.LynxUI;
import com.lynx.tasm.behavior.ui.image.FlattenUIImage;
import com.lynx.tasm.behavior.ui.image.InlineImageShadowNode;
import com.lynx.tasm.behavior.ui.image.UIImage;
import com.lynx.tasm.image.AutoSizeImage;
import java.util.ArrayList;
import java.util.List;

public class ImageBehavior implements BehaviorBundle {
  @Override
  public List<Behavior> create() {
    List<Behavior> bc = new ArrayList<>();
    bc.add(new Behavior("image", true, true) {
      @Override
      public LynxUI createUI(LynxContext context) {
        return new UIImage(context);
      }

      @Override
      public LynxFlattenUI createFlattenUI(final LynxContext context) {
        return new FlattenUIImage(context);
      }

      @Override
      public ShadowNode createShadowNode() {
        return new AutoSizeImage();
      }
    });

    bc.add(new Behavior("inline-image", false, false) {
      @Override
      public ShadowNode createShadowNode() {
        return new InlineImageShadowNode();
      }
    });

    return bc;
  }
}
