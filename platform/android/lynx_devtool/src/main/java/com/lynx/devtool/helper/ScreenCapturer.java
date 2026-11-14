// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.devtool.helper;

import android.graphics.Bitmap;
import android.graphics.Matrix;
import android.os.HandlerThread;
import android.util.Base64;
import android.util.DisplayMetrics;
import android.view.Choreographer;
import android.view.View;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import com.lynx.devtool.framecapture.FrameCapturer;
import com.lynx.devtoolwrapper.IDevToolDelegate;
import com.lynx.devtoolwrapper.ScreenshotBitmapHandler;
import com.lynx.tasm.base.LLog;
import com.lynx.tasm.base.TraceEvent;
import com.lynx.tasm.base.trace.TraceEventDef;
import com.lynx.tasm.utils.BitmapUtils;
import com.lynx.tasm.utils.DisplayMetricsHolder;
import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.util.HashMap;
import java.util.Map;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.ThreadFactory;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.concurrent.atomic.AtomicLong;

public class ScreenCapturer extends FrameCapturer {
  private static class ScreenRequest {
    private int mMaxWidth;
    private int mMaxHeight;
    private int mQuality;
    private String mScreenshotMode;
    private String mFormat;
  }

  public static class ScreenMetadata {
    public float mOffsetTop;
    public float mPageScaleFactor;
    public float mDeviceWidth;
    public float mDeviceHeight;
    public float mScrollOffsetX;
    public float mScrollOffsetY;
    public float mTimestamp;
  }

  public interface ScreenshotListener {
    void onNewScreenshotBitmapData(String screenData, long timeCost);
  }

  private static final String TAG = "ScreenCapturer";
  private static final String FORMAT_JPEG = "jpeg";
  private static final String FORMAT_PNG = "png";
  private static final ScreenCapturer mInstance = new ScreenCapturer();

  private static final int CARD_PREVIEW_QUALITY = 100;
  private static final int CARD_PREVIEW_MAX_WIDTH = 400;
  private static final int CARD_PREVIEW_MAX_HEIGHT = 800;

  private IDevToolDelegate mDevToolDelegate = null;

  // Delay for 1500ms to allow time for rendering remote resources
  public static final int SCREEN_SHOT_PREVIEW_DELAY_TIME = 1500;

  private final ScreenRequest mScreenRequest;
  private final ScreenMetadata mScreenMetadata;
  private boolean mIsEnabled;
  private ScreenshotListener mScreenshotListener;
  // mAckReceived is true that represents we can capture screen now.
  // when we send screen data to remote, we need reset its value(i.e. false)
  private final AtomicBoolean mAckReceived;
  // mIsDirty represents current frame whether need capture or not
  // when devtool has captured screen, we need reset its value(i.e. false)
  protected AtomicBoolean mIsDirty;
  private Choreographer.FrameCallback mResetDirtyCallBack;

  private volatile long mScreenshotOnMainThreadTime = 0;
  private long mScreenshotStartTime = 0;

  // Trace-only frame numbering, joining the main-thread capture with its background work.
  private static final String TRACE_CATEGORY = "lynx";
  private final AtomicLong mTraceFrameSeq = new AtomicLong();
  private volatile long mTraceFrameId;
  // Trace-only: the frame last handed to the frontend, which its ack refers to.
  private volatile long mTraceSentFrameId;
  // Trace-only: what asked for the latest capture (start, resume, frame or ack), and how it ended.
  private volatile String mTraceTrigger = "start";
  private volatile String mTraceCaptureResult;

  // use this executorService to encode and send screen data
  private final ExecutorService executorService =
      Executors.newCachedThreadPool(new ThreadFactory() {
        @Override
        public Thread newThread(@NonNull Runnable runnable) {
          return new Thread(runnable, TAG);
        }
      });

  public static ScreenCapturer getInstance() {
    return mInstance;
  }

  private ScreenCapturer() {
    super();
    mScreenRequest = new ScreenRequest();
    mScreenMetadata = new ScreenMetadata();
    mIsEnabled = false;
    mScreenshotListener = null;
    mAckReceived = new AtomicBoolean(false);
    mIsDirty = new AtomicBoolean(false);
    mFrameChangeListener = new FrameChangeListener() {
      @Override
      public void onFrameChanged() {
        triggerNextCapture("frame");
      }
    };
    mResetDirtyCallBack = new Choreographer.FrameCallback() {
      @Override
      public void doFrame(long frameTimeNanos) {
        mIsDirty.set(true);
        Choreographer.getInstance().postFrameCallback(mResetDirtyCallBack);
      }
    };
  }

  public void setDevToolDelegate(IDevToolDelegate devToolDelegate) {
    mDevToolDelegate = devToolDelegate;
  }

  public void startCapture(int maxWidth, int maxHeight, int quality, String screenShotMode,
      String format, ScreenshotListener listener) {
    mScreenRequest.mMaxWidth = maxWidth;
    mScreenRequest.mMaxHeight = maxHeight;
    mScreenRequest.mQuality = quality;
    mScreenRequest.mScreenshotMode = screenShotMode;
    mScreenRequest.mFormat = normalizeFormat(format);
    mScreenshotListener = listener;
    mIsEnabled = true;
    addResetDirtyStatusCallBack();
    startFrameViewTrace();
    // manually trigger first screenshot
    triggerNextCapture("start");
  }

  private void addResetDirtyStatusCallBack() {
    Choreographer.getInstance().postFrameCallback(mResetDirtyCallBack);
  }

  public void stopCapture(View targetView) {
    if (targetView != mView.get()) {
      return;
    }
    stopFrameViewTrace();
    mIsEnabled = false;
    mView.clear();
  }

  public void triggerNextCapture() {
    triggerNextCapture("resume");
  }

  private void triggerNextCapture(String trigger) {
    if (!mIsEnabled) {
      return;
    }

    if (mScreenshotBitmapDataCache == null || (mAckReceived.get() && mIsDirty.get())) {
      mTraceTrigger = trigger;
      screenshot();
    }
  }

  public void onAckReceived() {
    if (TraceEvent.enableTrace()) {
      TraceEvent.instant(
          TRACE_CATEGORY, TraceEventDef.SCREENCAST_ACK, traceArgs("frame_id", mTraceSentFrameId));
    }
    mAckReceived.set(true);
    triggerNextCapture("ack");
  }

  public ScreenMetadata getScreenMetadata() {
    return mScreenMetadata;
  }

  @Override
  protected boolean isEnabled() {
    return mIsEnabled;
  }

  protected String getScreenshotDataFromBitmap(Bitmap bitmap, boolean isPreview) {
    return getScreenshotDataFromBitmap(bitmap, isPreview, 0, 0);
  }

  // frameId and queueUs only label ScreenCast trace slices.
  private String getScreenshotDataFromBitmap(
      Bitmap bitmap, boolean isPreview, long frameId, long queueUs) {
    int quality = isPreview ? CARD_PREVIEW_QUALITY : mScreenRequest.mQuality;
    int maxWidth = isPreview ? CARD_PREVIEW_MAX_WIDTH : mScreenRequest.mMaxWidth;
    int maxHeight = isPreview ? CARD_PREVIEW_MAX_HEIGHT : mScreenRequest.mMaxHeight;
    String format = isPreview ? FORMAT_JPEG : mScreenRequest.mFormat;
    Bitmap.CompressFormat compressFormat =
        FORMAT_PNG.equals(format) ? Bitmap.CompressFormat.PNG : Bitmap.CompressFormat.JPEG;
    if (isPreview) {
      return BitmapUtils.bitmapToBase64(
          scaleImage(bitmap, maxWidth, maxHeight), compressFormat, quality, Base64.NO_WRAP);
    }
    return encodeScreencastFrame(
        bitmap, compressFormat, quality, maxWidth, maxHeight, frameId, queueUs);
  }

  // Same output as BitmapUtils.bitmapToBase64 after scaleImage, with compression and Base64
  // timed separately.
  private String encodeScreencastFrame(Bitmap bitmap, Bitmap.CompressFormat compressFormat,
      int quality, int maxWidth, int maxHeight, long frameId, long queueUs) {
    Bitmap scaledBitmap = null;
    traceBegin(
        TraceEventDef.SCREENCAST_SCALE, traceArgs("frame_id", frameId, "worker_queue_us", queueUs));
    try {
      scaledBitmap = scaleImage(bitmap, maxWidth, maxHeight);
    } finally {
      TraceEvent.endSection(TraceEventDef.SCREENCAST_SCALE);
    }
    if (scaledBitmap == null) {
      return null;
    }
    TraceEvent.beginSection(TraceEventDef.SCREENCAST_ENCODE);
    try {
      byte[] encoded;
      TraceEvent.beginSection(TraceEventDef.SCREENCAST_ENCODE_COMPRESS);
      try (ByteArrayOutputStream output = new ByteArrayOutputStream()) {
        scaledBitmap.compress(compressFormat, quality, output);
        encoded = output.toByteArray();
      } finally {
        TraceEvent.endSection(TraceEventDef.SCREENCAST_ENCODE_COMPRESS);
      }
      String result;
      traceBegin(TraceEventDef.SCREENCAST_ENCODE_BASE64, traceArgs("input_bytes", encoded.length));
      try {
        result = Base64.encodeToString(encoded, Base64.NO_WRAP);
      } finally {
        TraceEvent.endSection(TraceEventDef.SCREENCAST_ENCODE_BASE64);
      }
      return result;
    } catch (IOException e) {
      e.printStackTrace();
      return null;
    } finally {
      TraceEvent.endSection(TraceEventDef.SCREENCAST_ENCODE);
    }
  }

  @Nullable
  private static Map<String, String> traceArgs(Object... keyValues) {
    if (!TraceEvent.enableTrace()) {
      return null;
    }
    Map<String, String> args = new HashMap<>();
    for (int i = 0; i + 1 < keyValues.length; i += 2) {
      args.put(String.valueOf(keyValues[i]), String.valueOf(keyValues[i + 1]));
    }
    return args;
  }

  private static void traceBegin(String name, @Nullable Map<String, String> args) {
    if (args == null) {
      TraceEvent.beginSection(name);
    } else {
      TraceEvent.beginSection(name, args);
    }
  }

  private static void traceEnd(String name, @Nullable Map<String, String> args) {
    if (args == null) {
      TraceEvent.endSection(name);
    } else {
      TraceEvent.endSection(TRACE_CATEGORY, name, args);
    }
  }

  private String normalizeFormat(String format) {
    return FORMAT_PNG.equalsIgnoreCase(format) ? FORMAT_PNG : FORMAT_JPEG;
  }

  public void onScreenshotBitmapReady(Bitmap bitmap) {
    // Called synchronously from takeScreenshot on the main thread, inside ScreenCast.Capture.
    mTraceCaptureResult = bitmap == null ? "no_bitmap" : "ok";
    final long frameId = mTraceFrameId;
    final long submittedNs = System.nanoTime();
    executorService.submit(new Runnable() {
      @Override
      public void run() {
        if (bitmap != null) {
          long queueUs = (System.nanoTime() - submittedNs) / 1000;
          String screenshotData = getScreenshotDataFromBitmap(bitmap, false, frameId, queueUs);
          if (screenshotData == null) {
            return;
          }

          boolean unchanged = false;
          traceBegin(TraceEventDef.SCREENCAST_DIFF, traceArgs("frame_id", frameId));
          try {
            unchanged = screenshotData.equals(mScreenshotBitmapDataCache);
          } finally {
            traceEnd(TraceEventDef.SCREENCAST_DIFF, traceArgs("unchanged", unchanged ? 1 : 0));
          }
          if (unchanged) {
            return;
          }
          mScreenshotBitmapDataCache = screenshotData;
          mTraceSentFrameId = frameId;
          onNewScreenshotBitmapData(mScreenshotBitmapDataCache);
        }
      }
    });
  }

  @Override
  protected void onNewScreenshotBitmapData(String data) {
    mAckReceived.set(false);
    if (mScreenshotListener != null) {
      if (mScreenshotOnMainThreadTime == 0) {
        LLog.e(TAG, "onNewScreenshotBitmapData: screenshotMainTime == 0");
      }
      mScreenshotListener.onNewScreenshotBitmapData(data, mScreenshotOnMainThreadTime);
    }
  }

  @Override
  protected void onScreenshotActionStart() {
    mIsDirty.set(false);
    mScreenshotStartTime = System.nanoTime();
    mScreenshotOnMainThreadTime = 0;
  }

  @Override
  protected void onScreenshotActionEnd() {
    if (mScreenshotStartTime == 0) {
      LLog.e(TAG, "onScreenshotActionEnd: lastTime == 0");
      return;
    }
    mScreenshotOnMainThreadTime = System.nanoTime() - mScreenshotStartTime;
  }

  @Override
  protected void screenshot(final View view, ScreenshotBitmapHandler handler) {
    mTraceFrameId = mTraceFrameSeq.incrementAndGet();
    // Replaced by onScreenshotBitmapReady when the renderer hands back a bitmap.
    mTraceCaptureResult = mDevToolDelegate == null ? "no_delegate" : "no_renderer";
    traceBegin(TraceEventDef.SCREENCAST_CAPTURE,
        traceArgs("frame_id", mTraceFrameId, "mode", mScreenRequest.mScreenshotMode, "trigger",
            mTraceTrigger, "ui_queue_us", mUiQueueDelayNs / 1000));
    try {
      if (mDevToolDelegate != null) {
        mDevToolDelegate.takeScreenshot(handler, mScreenRequest.mScreenshotMode);
      }
      onScreenshotActionEnd();
    } finally {
      traceEnd(TraceEventDef.SCREENCAST_CAPTURE, traceArgs("result", mTraceCaptureResult));
    }
  }

  public void submit(Runnable task) {
    executorService.submit(task);
  }

  private float getScale(int originalWidth, int originalHeight, int maxWidth, int maxHeight) {
    float scale = 0;
    float scalingWidth = 1;
    float scalingHeight = 1;
    try {
      if (maxWidth != 0 && maxHeight != 0
          && ((originalWidth) > maxWidth || (originalHeight) > maxHeight)) {
        scalingWidth = maxWidth / (float) (originalWidth);
        scalingHeight = maxHeight / (float) (originalHeight);
      }
      scale = Math.min(scalingWidth, scalingHeight);
    } catch (Throwable e) {
      e.printStackTrace();
    }
    return scale;
  }

  private Bitmap scaleImage(Bitmap bitmap, int maxWidth, int maxHeight) {
    if (bitmap == null) {
      LLog.e(TAG, "scaleImage: bitmap is null");
      return null;
    }
    int originalWidth = bitmap.getWidth();
    int originalHeight = bitmap.getHeight();
    float scale = getScale(originalWidth, originalHeight, maxWidth, maxHeight);
    DisplayMetrics dm = DisplayMetricsHolder.getScreenDisplayMetrics();
    mScreenMetadata.mDeviceWidth = originalWidth / dm.density;
    mScreenMetadata.mDeviceHeight = originalHeight / dm.density;
    mScreenMetadata.mPageScaleFactor = 1;
    try {
      Matrix matrix = new Matrix();
      matrix.postScale(scale, scale);
      bitmap = Bitmap.createBitmap(bitmap, 0, 0, originalWidth, originalHeight, matrix, true);
    } catch (Throwable e) {
      e.printStackTrace();
    }
    return bitmap;
  }
}
