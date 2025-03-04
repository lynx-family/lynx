// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.testing.base;

import android.content.Context;
import android.util.DisplayMetrics;
import androidx.test.platform.app.InstrumentationRegistry;
import com.lynx.tasm.behavior.Behavior;
import com.lynx.tasm.behavior.BehaviorRegistry;
import com.lynx.tasm.behavior.BuiltInBehavior;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.tasm.behavior.LynxUIOwner;
import com.lynx.tasm.behavior.ui.UIBody;
import com.lynx.tasm.utils.DisplayMetricsHolder;
import java.util.ArrayList;
import java.util.List;

public class TestingUtils {
  public static LynxContext getLynxContext() {
    Context context =
        InstrumentationRegistry.getInstrumentation().getTargetContext().getApplicationContext();
    DisplayMetricsHolder.updateOrInitDisplayMetrics(context, 3.0f);
    DisplayMetrics displayMetrics = new DisplayMetrics();
    displayMetrics.widthPixels = 1080;
    displayMetrics.heightPixels = 1920;
    displayMetrics.density = 3.0f;
    return new TestingLynxContext(context, displayMetrics);
  }

  public static UIBody getUIBody(LynxContext lynxContext) {
    return new UIBody(lynxContext, new UIBody.UIBodyView(lynxContext));
  }

  public interface BehaviorRegisterCallback {
    default void RegisterBehavior(List<Behavior> behaviors) {}
  }

  public static LynxUIOwner getLynxUIOwner(
      LynxContext lynxContext, UIBody.UIBodyView uiBodyView, BehaviorRegisterCallback callback) {
    List<Behavior> behaviors = new ArrayList<Behavior>();
    BuiltInBehavior builtInBehavior = new BuiltInBehavior();
    behaviors.addAll(builtInBehavior.create());
    if (callback != null) {
      callback.RegisterBehavior(behaviors);
    }
    return new LynxUIOwner(lynxContext, new BehaviorRegistry(behaviors), uiBodyView);
  }
}
