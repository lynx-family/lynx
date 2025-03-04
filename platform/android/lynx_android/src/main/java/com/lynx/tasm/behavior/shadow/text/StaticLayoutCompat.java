// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.shadow.text;

import android.text.Layout;
import android.text.StaticLayout;
import android.text.TextDirectionHeuristic;
import android.text.TextDirectionHeuristics;
import android.text.TextPaint;
import android.text.TextUtils;
import android.util.Log;
import java.lang.reflect.Constructor;

public class StaticLayoutCompat {
  @Deprecated
  public static StaticLayout get(CharSequence source, int bufstart, int bufend, TextPaint paint,
      int outerwidth, Layout.Alignment align, float spacingmult, float spacingadd,
      boolean includepad, TextUtils.TruncateAt ellipsize, int maxLines) {
    return get(source, bufstart, bufend, paint, outerwidth, align, spacingmult, spacingadd,
        includepad, ellipsize, maxLines, TextDirectionHeuristics.FIRSTSTRONG_LTR);
  }

  public static StaticLayout get(CharSequence source, int bufstart, int bufend, TextPaint paint,
      int outerwidth, Layout.Alignment align, float spacingmult, float spacingadd,
      boolean includepad, TextUtils.TruncateAt ellipsize, int maxLines,
      TextDirectionHeuristic directionHeuristic) {
    if (maxLines != TextAttributes.NOT_SET
        || directionHeuristic != TextDirectionHeuristics.FIRSTSTRONG_LTR) {
      try {
        /*
          @hide
          public StaticLayout(CharSequence source, int bufstart, int bufend,
                           TextPaint paint, int outerwidth,
                           Alignment align, TextDirectionHeuristic textDir,
                           float spacingmult, float spacingadd,
                           boolean includepad,
                           TextUtils.TruncateAt ellipsize, int ellipsizedWidth, int maxLines)
         */
        Constructor<StaticLayout> constructor = StaticLayout.class.getConstructor(
            CharSequence.class, int.class, int.class, TextPaint.class, int.class,
            Layout.Alignment.class, TextDirectionHeuristic.class, float.class, float.class,
            boolean.class, TextUtils.TruncateAt.class, int.class, int.class);

        return constructor.newInstance(source, bufstart, bufend, paint, outerwidth, align,
            directionHeuristic, spacingmult, spacingadd, includepad, ellipsize, outerwidth,
            maxLines == TextAttributes.NOT_SET ? Integer.MAX_VALUE : maxLines);
      } catch (Throwable e) {
        Log.e("Lynx", "create StaticLayout failed!", e);
        return new StaticLayout(source, bufstart, bufend, paint, outerwidth, align, spacingmult,
            spacingadd, includepad);
      }
    } else {
      return new StaticLayout(
          source, bufstart, bufend, paint, outerwidth, align, spacingmult, spacingadd, includepad);
    }
  }
}
