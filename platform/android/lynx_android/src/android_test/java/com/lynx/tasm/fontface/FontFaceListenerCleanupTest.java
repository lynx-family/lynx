// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.fontface;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertSame;
import static org.junit.Assert.assertTrue;

import android.graphics.Typeface;
import android.os.Handler;
import android.os.Looper;
import android.util.Pair;
import androidx.test.ext.junit.runners.AndroidJUnit4;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.tasm.behavior.shadow.text.TypefaceCache;
import java.lang.reflect.Field;
import java.lang.reflect.Method;
import java.util.Map;
import org.junit.Test;
import org.junit.runner.RunWith;

@RunWith(AndroidJUnit4.class)
public class FontFaceListenerCleanupTest {
  private static class Listener implements TypefaceCache.TypefaceListener {
    boolean cancelled;

    @Override
    public void onTypefaceUpdate(Typeface typeface, int style) {}

    @Override
    public boolean isCancelled() {
      return cancelled;
    }
  }

  @Test
  public void removesAllRegistrationsOnlyForReleasedListener() {
    FontFaceGroup group = new FontFaceGroup();
    Listener released = new Listener();
    Listener live = new Listener();
    group.addListener(new Pair<>(released, 0));
    group.addListener(new Pair<>(live, 0));
    group.addListener(new Pair<>(released, 1));
    group.removeListener(released);
    group.removeListener(released);
    assertEquals(1, group.getListeners().size());
    assertSame(live, group.getListeners().get(0).first);
  }

  @SuppressWarnings("unchecked")
  @Test
  public void managerRemovesListenerAndRejectsLateRegistration() throws Exception {
    FontFaceManager manager = new FontFaceManager();
    Field field = FontFaceManager.class.getDeclaredField("mLoadingFontFaceBySrc");
    field.setAccessible(true);
    Map<String, FontFaceGroup> groups = (Map<String, FontFaceGroup>) field.get(manager);
    FontFaceGroup group = new FontFaceGroup();
    Listener released = new Listener();
    Listener live = new Listener();
    group.addListener(new Pair<>(released, 0));
    group.addListener(new Pair<>(live, 0));
    groups.put("first-src", group);
    groups.put("second-src", group);

    released.cancelled = true;
    manager.removeTypefaceListener(released);
    manager.removeTypefaceListener(released);
    assertEquals(1, group.getListeners().size());
    assertSame(live, group.getListeners().get(0).first);
    assertEquals(2, groups.size());

    // Simulate an IO task that starts after its sign was released. No font
    // or context is needed: cancellation must precede either lookup.
    Method load = FontFaceManager.class.getDeclaredMethod("findOrLoadFontFace", LynxContext.class,
        FontFace.class, int.class, TypefaceCache.TypefaceListener.class, Handler.class);
    load.setAccessible(true);
    load.invoke(manager, null, null, 0, released, new Handler(Looper.getMainLooper()));
    assertEquals(1, group.getListeners().size());
    assertTrue(released.isCancelled());
  }
}
