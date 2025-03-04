// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior;

import static org.junit.Assert.*;

import android.app.Application;
import android.content.Context;
import androidx.test.ext.junit.runners.AndroidJUnit4;
import androidx.test.platform.app.InstrumentationRegistry;
import com.lynx.tasm.LynxEnv;
import com.lynx.tasm.LynxView;
import com.lynx.tasm.LynxViewBuilder;
import com.lynx.tasm.behavior.shadow.ShadowNode;
import com.lynx.tasm.behavior.ui.LynxFlattenUI;
import com.lynx.tasm.behavior.ui.LynxUI;
import com.lynx.tasm.image.AutoSizeImage;
import com.lynx.testing.base.TestingUtils;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

@RunWith(AndroidJUnit4.class)
public class BehaviorOverrideRegistryTest {
  @Before
  public void setUp() throws Exception {
    Context context =
        InstrumentationRegistry.getInstrumentation().getTargetContext().getApplicationContext();
    LynxEnv.inst().init((Application) context, null, null, null, null);
  }

  @Test
  public void testBehaviorRegistryOverride() {
    Map<String, String> schemaMap = new HashMap<>();
    schemaMap.put("test", "true");
    LynxContext lynxContext = TestingUtils.getLynxContext();
    BehaviorBundle overrideImageBundle = new BehaviorBundle() {
      @Override
      public List<Behavior> create() {
        List<Behavior> list = new ArrayList<>();
        list.add(new Behavior("image", true, true) {
          @Override
          public LynxUI createUI(LynxContext context) {
            return null;
          }

          @Override
          public LynxFlattenUI createFlattenUI(final LynxContext context) {
            return null;
          }

          @Override
          public ShadowNode createShadowNode() {
            return new AutoSizeImage();
          }
        });
        return list;
      }
    };
    LynxView overrideLynxView = new LynxViewBuilder()
                                    .setLynxViewConfig(schemaMap)
                                    .addBehaviors(overrideImageBundle.create())
                                    .build(lynxContext);
    assertNull(overrideLynxView.getLynxContext().getLynxUIOwner().createUI("image", false));
    assertNull(overrideLynxView.getLynxContext().getLynxUIOwner().createUI("image", true));
  }
}
