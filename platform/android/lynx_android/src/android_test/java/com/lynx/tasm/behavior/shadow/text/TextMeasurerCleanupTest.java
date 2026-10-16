// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.shadow.text;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNotSame;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertSame;
import static org.junit.Assert.assertTrue;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import android.text.SpannableStringBuilder;
import android.util.SparseArray;
import androidx.test.ext.junit.runners.AndroidJUnit4;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.tasm.behavior.LynxUIOwner;
import com.lynx.tasm.behavior.shadow.TextLayout;
import com.lynx.tasm.behavior.shadow.TextMeasurerProvider;
import com.lynx.testing.base.TestingUtils;
import java.lang.reflect.Field;
import java.lang.reflect.Method;
import org.junit.Test;
import org.junit.runner.RunWith;

@RunWith(AndroidJUnit4.class)
public class TextMeasurerCleanupTest {
  @SuppressWarnings("unchecked")
  private SparseArray<Object> cache(TextMeasurer measurer, String name) throws Exception {
    Field field = TextMeasurer.class.getDeclaredField(name);
    field.setAccessible(true);
    return (SparseArray<Object>) field.get(measurer);
  }

  private TypefaceCache.TypefaceListener listener(TextMeasurer measurer, int sign)
      throws Exception {
    Method method = TextMeasurer.class.getDeclaredMethod("getTypefaceListener", int.class);
    method.setAccessible(true);
    return (TypefaceCache.TypefaceListener) method.invoke(measurer, sign);
  }

  @Test
  public void releaseIsPerSignIdempotentAndAllowsRebuilding() throws Exception {
    TextMeasurer measurer = new TextMeasurer(mock(LynxContext.class));
    Object firstBundle = new Object();
    Object secondBundle = new Object();
    Object firstLayout = new Object();
    Object secondLayout = new Object();
    SparseArray<Object> bundles = cache(measurer, "mAttributedTextBundles");
    SparseArray<Object> layouts = cache(measurer, "mExtraDatas");
    bundles.put(11, firstBundle);
    bundles.put(22, secondBundle);
    layouts.put(11, firstLayout);
    layouts.put(22, secondLayout);
    TypefaceCache.TypefaceListener firstListener = listener(measurer, 11);
    TypefaceCache.TypefaceListener secondListener = listener(measurer, 22);
    assertSame(firstListener, listener(measurer, 11));

    measurer.releaseLayoutObject(11);
    measurer.releaseLayoutObject(11);
    assertNull(bundles.get(11));
    assertNull(measurer.takeTextLayout(11));
    assertTrue(firstListener.isCancelled());
    assertSame(secondBundle, bundles.get(22));
    assertSame(secondLayout, measurer.takeTextLayout(22));
    assertFalse(secondListener.isCancelled());

    TypefaceCache.TypefaceListener rebuiltListener = listener(measurer, 11);
    assertNotSame(firstListener, rebuiltListener);
    assertFalse(rebuiltListener.isCancelled());
    bundles.put(11, firstBundle);
    layouts.put(11, firstLayout);
    measurer.removeLayoutObjects();
    measurer.removeLayoutObjects();
    assertTrue(rebuiltListener.isCancelled());
    assertTrue(secondListener.isCancelled());
    assertEquals(0, bundles.size());
    assertEquals(0, layouts.size());
  }

  @Test
  public void realAndroidTextMeasurementChangesWithFontSizeAndReleasesResources() throws Exception {
    TextMeasurer measurer = new TextMeasurer(TestingUtils.getLynxContext());
    SparseArray<Object> bundles = cache(measurer, "mAttributedTextBundles");
    SparseArray<Object> layouts = cache(measurer, "mExtraDatas");
    float[] first = null;
    for (float fontSize : new float[] {16, 32}) {
      TextAttributes attributes = new TextAttributes();
      attributes.setFontSize(fontSize);
      bundles.put(11,
          new AttributedTextBundle(
              new SpannableStringBuilder("Lynx detached measurement"), attributes));
      // Use the actual Android TextRenderer / StaticLayout backend, not a provider mock.
      float[] size = measurer.measureText(11, 4096, 2, 0, 0, new float[0]);
      assertTrue(size.length >= 3);
      assertTrue(size[0] > 0);
      assertTrue(size[1] > 0);
      assertTrue(size[2] > 0);
      if (first == null) {
        first = size;
      } else {
        assertTrue(size[0] > first[0]);
        assertTrue(size[1] > first[1]);
      }
      measurer.releaseLayoutObject(11);
      assertEquals(0, bundles.size());
      assertEquals(0, layouts.size());
      assertEquals(0, cache(measurer, "mTypefaceListeners").size());
    }
  }

  @Test
  public void textLayoutPreservesNullAndShortMeasurementFailuresForNative() {
    TextMeasurerProvider provider = mock(TextMeasurerProvider.class);
    TextLayout layout = new TextLayout(provider);
    float[] inlineSizes = new float[0];
    when(provider.measureText(11, 100, 2, 80, 1, inlineSizes)).thenReturn(null);
    assertNull(layout.measureText(11, 100, 2, 80, 1, inlineSizes));

    float[] shortResult = new float[] {0, 0};
    when(provider.measureText(11, 100, 2, 80, 1, inlineSizes)).thenReturn(shortResult);
    assertSame(shortResult, layout.measureText(11, 100, 2, 80, 1, inlineSizes));

    float[] size = new float[] {35, 20, 15};
    when(provider.measureText(11, 100, 2, 80, 1, inlineSizes)).thenReturn(size);
    assertSame(size, layout.measureText(11, 100, 2, 80, 1, inlineSizes));

    // Zero is also a legitimate measurement, not a failure sentinel. Native
    // detached measurement checks successful preparation separately.
    float[] emptySize = new float[] {0, 0, 0};
    when(provider.measureText(11, 100, 2, 80, 1, inlineSizes)).thenReturn(emptySize);
    assertSame(emptySize, layout.measureText(11, 100, 2, 80, 1, inlineSizes));
  }

  @Test
  public void textLayoutForwardsDestroyToBothProviderConstructors() {
    TextMeasurerProvider provider = mock(TextMeasurerProvider.class);
    TextLayout direct = new TextLayout(provider);
    direct.destroyText(11);
    direct.destroyText(11);
    verify(provider, times(2)).destroyText(11);

    LynxUIOwner owner = mock(LynxUIOwner.class);
    TextLayout legacy = new TextLayout(owner);
    legacy.destroyText(22);
    verify(owner).destroyText(22);
  }
}
