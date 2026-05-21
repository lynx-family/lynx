// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.ui.scroll;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertTrue;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.drawable.ColorDrawable;
import android.util.Base64;
import android.view.View;
import androidx.test.core.app.ApplicationProvider;
import androidx.test.ext.junit.runners.AndroidJUnit4;
import com.lynx.react.bridge.Callback;
import com.lynx.react.bridge.JavaOnlyMap;
import com.lynx.tasm.behavior.LynxUIMethodConstants;
import com.lynx.tasm.service.ILynxSystemInvokeService;
import com.lynx.tasm.service.LynxServiceCenter;
import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

@RunWith(AndroidJUnit4.class)
public class ScrollContentScreenshotHelperTest {
  private Context mContext;

  @Before
  public void setUp() {
    mContext = ApplicationProvider.getApplicationContext();
    LynxServiceCenter.inst().unregisterService(ILynxSystemInvokeService.class);
  }

  @Test
  public void contentViewPathCapturesFullContentAndRestoresScroll() throws Exception {
    View viewport = layoutView(new View(mContext), 100, 80);
    viewport.setBackground(new ColorDrawable(Color.WHITE));
    ColorView contentView = layoutView(new ColorView(mContext, Color.RED), 160, 140);
    contentView.layout(8, 12, 168, 152);
    JavaOnlyMap params = new JavaOnlyMap();
    params.putString("format", "png");
    params.putDouble("scale", 1.0);
    RecordingCallback callback = new RecordingCallback();
    RecordingController controller = new RecordingController(11, 22);

    ScrollContentScreenshotHelper.takeContentScreenshot(
        viewport, contentView, 160, 140, null, params, callback, controller);

    JavaOnlyMap result = callback.awaitSuccess();
    assertEquals(160, result.getInt("width"));
    assertEquals(140, result.getInt("height"));
    assertPngData(result);
    assertEquals(1, contentView.drawCount);
    assertEquals(11, controller.scrollX);
    assertEquals(22, controller.scrollY);
    assertEquals("(0,0);(11,22);", controller.scrollCalls());
  }

  @Test
  public void nullContentViewUsesSegmentedViewportPath() throws Exception {
    ColorView viewport = layoutView(new ColorView(mContext, Color.BLUE), 50, 40);
    JavaOnlyMap params = new JavaOnlyMap();
    params.putString("format", "jpeg");
    params.putDouble("scale", 1.0);
    RecordingCallback callback = new RecordingCallback();
    RecordingController controller = new RecordingController(7, 9);

    ScrollContentScreenshotHelper.takeContentScreenshot(
        viewport, null, 110, 90, null, params, callback, controller);

    JavaOnlyMap result = callback.awaitSuccess();
    assertEquals(110, result.getInt("width"));
    assertEquals(90, result.getInt("height"));
    assertTrue(result.getString("data").startsWith("data:image/jpeg;base64,"));
    assertEquals(9, viewport.drawCount);
    assertEquals(7, controller.scrollX);
    assertEquals(9, controller.scrollY);
    assertEquals("(0,0);(50,0);(60,0);(0,40);(50,40);(60,40);(0,50);(50,50);(60,50);"
            + "(7,9);",
        controller.scrollCalls());
  }

  @Test
  public void invalidScaleReturnsParamInvalid() {
    View viewport = layoutView(new View(mContext), 100, 80);
    JavaOnlyMap params = new JavaOnlyMap();
    params.putDouble("scale", 0.0);
    RecordingCallback callback = new RecordingCallback();

    ScrollContentScreenshotHelper.takeContentScreenshot(
        viewport, 100, 80, null, params, callback, new RecordingController(0, 0));

    Object[] args = callback.await();
    assertEquals(LynxUIMethodConstants.PARAM_INVALID, args[0]);
    assertEquals("scale must be greater than 0", args[1]);
  }

  @Test
  public void oversizedBitmapReturnsOperationErrorBeforeAllocation() {
    View viewport = layoutView(new View(mContext), 100, 80);
    RecordingCallback callback = new RecordingCallback();

    ScrollContentScreenshotHelper.takeContentScreenshot(
        viewport, Integer.MAX_VALUE, 80, null, null, callback, new RecordingController(0, 0));

    Object[] args = callback.await();
    assertEquals(LynxUIMethodConstants.OPERATION_ERROR, args[0]);
    assertEquals("screenshot memory exceeds limit", args[1]);
  }

  @Test
  public void invalidViewReturnsNoUiForNode() {
    RecordingCallback callback = new RecordingCallback();

    ScrollContentScreenshotHelper.takeContentScreenshot(
        null, 100, 80, null, null, callback, new RecordingController(0, 0));

    Object[] args = callback.await();
    assertEquals(LynxUIMethodConstants.NO_UI_FOR_NODE, args[0]);
  }

  private static <T extends View> T layoutView(T view, int width, int height) {
    view.measure(View.MeasureSpec.makeMeasureSpec(width, View.MeasureSpec.EXACTLY),
        View.MeasureSpec.makeMeasureSpec(height, View.MeasureSpec.EXACTLY));
    view.layout(0, 0, width, height);
    return view;
  }

  private static void assertPngData(JavaOnlyMap result) {
    String data = result.getString("data");
    assertTrue(data.startsWith("data:image/png;base64,"));
    byte[] bytes = Base64.decode(data.substring("data:image/png;base64,".length()), Base64.NO_WRAP);
    Bitmap bitmap = BitmapFactory.decodeByteArray(bytes, 0, bytes.length);
    assertNotNull(bitmap);
    assertEquals(result.getInt("width"), bitmap.getWidth());
    assertEquals(result.getInt("height"), bitmap.getHeight());
    bitmap.recycle();
  }

  private static class ColorView extends View {
    private final int mColor;
    int drawCount;

    ColorView(Context context, int color) {
      super(context);
      mColor = color;
      setWillNotDraw(false);
    }

    @Override
    protected void onDraw(Canvas canvas) {
      drawCount++;
      canvas.drawColor(mColor);
    }
  }

  private static class RecordingController implements ScrollContentScreenshotController {
    private final List<int[]> mScrollCalls = new ArrayList<>();
    int scrollX;
    int scrollY;

    RecordingController(int scrollX, int scrollY) {
      this.scrollX = scrollX;
      this.scrollY = scrollY;
    }

    @Override
    public int getScrollX() {
      return scrollX;
    }

    @Override
    public int getScrollY() {
      return scrollY;
    }

    @Override
    public void scrollTo(int x, int y) {
      scrollX = x;
      scrollY = y;
      mScrollCalls.add(new int[] {x, y});
    }

    String scrollCalls() {
      StringBuilder builder = new StringBuilder();
      for (int[] call : mScrollCalls) {
        builder.append('(').append(call[0]).append(',').append(call[1]).append(");");
      }
      return builder.toString();
    }
  }

  private static class RecordingCallback implements Callback {
    private final CountDownLatch mLatch = new CountDownLatch(1);
    private Object[] mArgs;

    @Override
    public void invoke(Object... args) {
      mArgs = args;
      mLatch.countDown();
    }

    Object[] await() {
      try {
        assertTrue(mLatch.await(3, TimeUnit.SECONDS));
      } catch (InterruptedException e) {
        Thread.currentThread().interrupt();
        throw new AssertionError(e);
      }
      return mArgs;
    }

    JavaOnlyMap awaitSuccess() {
      Object[] args = await();
      assertEquals(LynxUIMethodConstants.SUCCESS, args[0]);
      return (JavaOnlyMap) args[1];
    }
  }
}
