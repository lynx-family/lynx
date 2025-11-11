// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.render;

import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.RectF;
import com.lynx.tasm.behavior.shadow.text.TextMeasurer;
import com.lynx.tasm.behavior.shadow.text.TextUpdateBundle;
import java.util.Stack;

public class DisplayListApplier {
  // Operation type constants matching C++ DisplayListOpType and DisplayListSubtreePropertyOpType
  private static final int OP_BEGIN = 0;
  private static final int OP_END = 1;
  private static final int OP_FILL = 2;
  private static final int OP_DRAW_VIEW = 3;
  private static final int OP_TEXT = 6;
  private static final int OP_IMAGE = 7;
  private static final int OP_CUSTOM = 8;

  private DisplayList mDisplayList;
  private TextMeasurer mTextMeasurer;
  private Paint mPaint;
  private Stack<RectF> mBounds;

  // Separate indices for content and subtree property operations
  private int mContentOpIndex;
  private int mContentIntIndex;
  private int mContentFloatIndex;

  public DisplayListApplier(DisplayList displayList, TextMeasurer textMeasurer) {
    mDisplayList = displayList;
    mPaint = new Paint();
    mPaint.setAntiAlias(true);
    mBounds = new Stack<>();
    mTextMeasurer = textMeasurer;
    reset();
  }

  public void reset() {
    mContentOpIndex = 0;
    mContentIntIndex = 0;
    mContentFloatIndex = 0;

    mBounds.clear();
  }

  public void drawTillNextView(Canvas canvas) {
    if (mDisplayList == null) {
      return;
    }

    // Process content operations
    processContentOperations(canvas);
  }

  private void processContentOperations(Canvas canvas) {
    if (mDisplayList.ops == null) {
      return;
    }

    while (mContentOpIndex < mDisplayList.ops.length) {
      // Read operation type and parameter counts
      if (mContentIntIndex >= mDisplayList.iArgv.length) {
        break;
      }

      int op = mDisplayList.ops[mContentOpIndex++];
      int intParamCount = mDisplayList.iArgv[mContentIntIndex++];
      int floatParamCount = mDisplayList.iArgv[mContentIntIndex++];

      switch (op) {
        case OP_BEGIN:
          // Begin fragment: x, y, width, height (4 floats)
          if (floatParamCount == 4) {
            float x = nextContentFloat();
            float y = nextContentFloat();
            float width = nextContentFloat();
            float height = nextContentFloat();
            mBounds.push(new RectF(0, 0, width, height));
            canvas.save();
            canvas.translate(x, y);
            // No direct canvas operation, just consume parameters
          }
          break;

        case OP_END:
          // End fragment - no parameters
          canvas.restore();
          mBounds.pop();
          return; // End of this view's content

        case OP_FILL:
          mPaint.reset();
          if (intParamCount == 1) {
            // Fill: color (1 int)
            int color = nextContentInt();
            // This would fill the entire fragment bound
            mPaint.setColor(color);
            // Need to get bounds from the Begin operation or context
            canvas.drawRect(mBounds.peek(), mPaint);
          }
          break;

        case OP_DRAW_VIEW:
          // Draw view: view_id (1 int)
          if (intParamCount >= 1) {
            int viewId = nextContentInt();
            // This indicates we should stop processing and let the view draw itself
          }
          return;

        case OP_TEXT:
          // Text: id (1 int)
          if (intParamCount >= 1) {
            int textId = nextContentInt();
            TextUpdateBundle textLayout = (TextUpdateBundle) (mTextMeasurer.takeTextLayout(textId));
            if (textLayout != null) {
              textLayout.getTextLayout().draw(canvas);
            }
          }
          break;

        case OP_IMAGE:
          // Image: image_id (1 int)
          if (intParamCount >= 1 && floatParamCount >= 4) {
            int imageId = nextContentInt();
            // Image drawing would use the imageId to look up bitmap data
          }
          break;

        case OP_CUSTOM:
        default:
          break;
      }
    }
  }

  // Helper methods for reading content data
  private int nextContentInt() {
    if (mDisplayList.iArgv != null && mContentIntIndex < mDisplayList.iArgv.length) {
      return mDisplayList.iArgv[mContentIntIndex++];
    }
    return 0;
  }

  private float nextContentFloat() {
    if (mDisplayList.fArgv != null && mContentFloatIndex < mDisplayList.fArgv.length) {
      return mDisplayList.fArgv[mContentFloatIndex++];
    }
    return 0.0f;
  }

  public void setDisplayList(DisplayList displayList) {
    mDisplayList = displayList;
    reset();
  }
}
