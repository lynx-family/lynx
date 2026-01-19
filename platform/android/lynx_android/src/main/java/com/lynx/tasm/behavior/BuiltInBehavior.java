// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior;

import com.lynx.tasm.behavior.shadow.ShadowNode;
import com.lynx.tasm.behavior.shadow.text.*;
import com.lynx.tasm.behavior.ui.LynxFlattenUI;
import com.lynx.tasm.behavior.ui.LynxUI;
import com.lynx.tasm.behavior.ui.frame.FrameShadowNode;
import com.lynx.tasm.behavior.ui.frame.UIFrame;
import com.lynx.tasm.behavior.ui.list.UIList;
import com.lynx.tasm.behavior.ui.list.UIListItem;
import com.lynx.tasm.behavior.ui.list.container.UIListContainer;
import com.lynx.tasm.behavior.ui.scroll.UIBounceView;
import com.lynx.tasm.behavior.ui.scroll.UIScrollView;
import com.lynx.tasm.behavior.ui.text.FlattenUIText;
import com.lynx.tasm.behavior.ui.text.UIText;
import com.lynx.tasm.behavior.ui.view.UIComponent;
import com.lynx.tasm.behavior.ui.view.UIView;
import java.util.ArrayList;
import java.util.List;

public class BuiltInBehavior implements BehaviorBundle {
  @Override
  public List<Behavior> create() {
    List<Behavior> bc = new ArrayList<>();
    bc.add(new Behavior("view", true, true) {
      @Override
      public LynxUI createUIWithParams(LynxContext context, Object params) {
        return new UIView(context, params);
      }

      @Override
      public LynxFlattenUI createFlattenUIWithParams(final LynxContext context, Object params) {
        return new LynxFlattenUI(context, params);
      }
    });
    bc.add(new Behavior("text", true, true) {
      @Override
      public LynxUI createUIWithParams(LynxContext context, Object params) {
        return new UIText(context, params);
      }

      @Override
      public LynxFlattenUI createFlattenUIWithParams(final LynxContext context, Object params) {
        return new FlattenUIText(context, params);
      }

      @Override
      public ShadowNode createShadowNode() {
        return new TextShadowNode();
      }
    });
    bc.add(new Behavior("raw-text", false, true) {
      @Override
      public ShadowNode createShadowNode() {
        return new RawTextShadowNode();
      }
    });
    bc.add(new Behavior("inline-text", false, true) {
      @Override
      public ShadowNode createShadowNode() {
        return new InlineTextShadowNode();
      }
    });

    bc.add(new Behavior("inline-truncation", false, true) {
      @Override
      public ShadowNode createShadowNode() {
        return new InlineTruncationShadowNode();
      }
    });
    bc.add(new Behavior("scroll-view", false, true) {
      @Override
      public LynxUI createUIWithParams(LynxContext context, Object params) {
        return new UIScrollView(context, params);
      }
    });
    bc.add(new Behavior("bounce-view", false, true) {
      @Override
      public LynxUI createUIWithParams(LynxContext context, Object params) {
        return new UIBounceView(context, params);
      }
    });
    bc.add(new Behavior("component", true, true) {
      @Override
      public LynxUI createUIWithParams(LynxContext context, Object params) {
        return new UIComponent(context, params);
      }

      @Override
      public LynxFlattenUI createFlattenUIWithParams(final LynxContext context, Object params) {
        return new LynxFlattenUI(context, params);
      }
    });
    bc.add(new Behavior("list", false, true) {
      @Override
      public LynxUI createUIWithParams(LynxContext context, Object params) {
        return new UIList(context, params);
      }
    });
    bc.add(new Behavior("list-item", false, true) {
      @Override
      public LynxUI createUIWithParams(LynxContext context, Object params) {
        return new UIListItem(context, params);
      }
    });
    bc.add(new Behavior("list-container", false, true) {
      @Override
      public LynxUI createUIWithParams(LynxContext context, Object params) {
        return new UIListContainer(context, params);
      }
    });
    bc.add(new Behavior("frame", false, true) {
      @Override
      public LynxUI createUIWithParams(LynxContext context, Object params) {
        return new UIFrame(context, params);
      }
      @Override
      public ShadowNode createShadowNode() {
        return new FrameShadowNode();
      }
    });
    return bc;
  }
}
