// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.utils;

import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.HardwareRenderer;
import android.graphics.Matrix;
import android.graphics.Paint;
import android.graphics.PixelFormat;
import android.graphics.RecordingCanvas;
import android.graphics.Rect;
import android.graphics.RenderNode;
import android.hardware.HardwareBuffer;
import android.media.Image;
import android.media.ImageReader;
import android.os.Build;
import android.os.Looper;
import android.os.SystemClock;
import android.view.View;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.RequiresApi;
import androidx.annotation.RestrictTo;
import androidx.annotation.VisibleForTesting;
import com.lynx.tasm.LynxEnv;
import com.lynx.tasm.base.LLog;
import java.nio.ByteBuffer;

/**
 * Draws a View through an offscreen {@link HardwareRenderer} on Android 10+.
 *
 * <p>The View's display list is recorded into a RenderNode and rendered into an ImageReader, then
 * read back and drawn into the destination canvas. Compared with drawing into a software canvas,
 * the result keeps hardware-only content such as hardware bitmaps, RenderEffect, TextureView and
 * outline clipping like rounded corners ({@link View#setClipToOutline(boolean)}), which a software
 * canvas ignores. Compared with PixelCopy, it renders the current display lists without waiting
 * for a frame, covers the whole View even when part of it is outside the window, and contains only
 * the View itself. No light source is set, so elevation shadows are not drawn; Lynx UI disables
 * them.
 *
 * <p>Large Views are rendered in tiles that are stitched in the destination canvas, so the GPU
 * target and the read-back memory stay bounded while the destination holds the full View.
 *
 * <p>Enabled by {@link LynxEnv#enableOffscreenCapture(boolean)}.
 */
@RestrictTo(RestrictTo.Scope.LIBRARY_GROUP)
public final class OffscreenViewCapture {
  private static final String TAG = "OffscreenViewCapture";
  private static final long IMAGE_TIMEOUT_MS = 1000;
  // Upper bound for each side of a tile. The real GPU limit is not queried: HWUI aborts the
  // process when it is read before the RenderThread has a GPU context, and Android 10+ GPUs
  // support at least 4096.
  private static final int MAX_TILE_SIDE = 4096;
  // Pixels in one tile, 16 MiB per RGBA buffer: the GPU buffer, the read-back bitmap and, when
  // rows are padded, a staging copy.
  private static final int MAX_TILE_PIXELS = 4 * 1024 * 1024;

  private OffscreenViewCapture() {}

  /**
   * Draws {@code view} at the origin of {@code canvas}, as the View appears within its own bounds
   * on screen, if offscreen capture is enabled and supported. Must be called on the main thread.
   *
   * @return true if the View was drawn. False if it was not: the canvas is left untouched when
   *     offscreen rendering is unavailable, and may hold some tiles of the View when rendering
   *     fails partway. Either way the caller should draw the View with its existing path, which
   *     covers the whole View.
   */
  public static boolean drawIfEnabled(@NonNull View view, @NonNull Canvas canvas) {
    if (Build.VERSION.SDK_INT < Build.VERSION_CODES.Q
        || !LynxEnv.inst().isOffscreenCaptureEnabled()) {
      return false;
    }
    if (Looper.myLooper() != Looper.getMainLooper() || !view.isHardwareAccelerated()) {
      return false;
    }
    return draw(view, canvas);
  }

  @VisibleForTesting
  @RequiresApi(Build.VERSION_CODES.Q)
  static boolean draw(@NonNull View view, @NonNull Canvas canvas) {
    return draw(view, canvas, MAX_TILE_SIDE, MAX_TILE_SIDE);
  }

  /**
   * Draws {@code view} in tiles of at most {@code maxTileWidth} x {@code maxTileHeight} pixels,
   * further limited by {@link #MAX_TILE_SIDE} and {@link #MAX_TILE_PIXELS}.
   */
  @VisibleForTesting
  @RequiresApi(Build.VERSION_CODES.Q)
  static boolean draw(
      @NonNull View view, @NonNull Canvas canvas, int maxTileWidth, int maxTileHeight) {
    final int width = view.getWidth();
    final int height = view.getHeight();
    if (width <= 0 || height <= 0 || maxTileWidth <= 0 || maxTileHeight <= 0) {
      return false;
    }
    final int tileWidth = Math.min(width, Math.min(maxTileWidth, MAX_TILE_SIDE));
    final int tileHeight = Math.min(Math.min(height, maxTileHeight),
        Math.min(MAX_TILE_SIDE, Math.max(1, MAX_TILE_PIXELS / tileWidth)));
    final boolean dirty = view.isDirty();
    RenderNode node = new RenderNode(TAG);
    ImageReader reader = null;
    HardwareRenderer renderer = null;
    ReadBack readBack = new ReadBack(tileHeight);
    try {
      node.setPosition(0, 0, tileWidth, tileHeight);
      // CPU_READ_OFTEN lets the CPU copy the rendered pixels directly, several times faster than
      // copying a HARDWARE bitmap; GPU_SAMPLED_IMAGE keeps that copy as a fallback.
      reader = ImageReader.newInstance(tileWidth, tileHeight, PixelFormat.RGBA_8888, 2,
          HardwareBuffer.USAGE_GPU_COLOR_OUTPUT | HardwareBuffer.USAGE_GPU_SAMPLED_IMAGE
              | HardwareBuffer.USAGE_CPU_READ_OFTEN);
      renderer = new HardwareRenderer();
      // An opaque renderer does not clear the buffer, which leaves stale pixels in transparent
      // areas.
      renderer.setOpaque(false);
      renderer.setSurface(reader.getSurface());
      renderer.setContentRoot(node);

      // Filter only when the destination canvas scales the capture; an unfiltered 1:1 copy is
      // faster.
      Paint paint = scales(canvas) ? new Paint(Paint.FILTER_BITMAP_FLAG) : null;
      Rect source = new Rect();
      Rect destination = new Rect();
      for (int top = 0; top < height; top += tileHeight) {
        for (int left = 0; left < width; left += tileWidth) {
          final int right = Math.min(width, left + tileWidth);
          final int bottom = Math.min(height, top + tileHeight);
          RecordingCanvas recording = node.beginRecording(tileWidth, tileHeight);
          try {
            recording.clipRect(0, 0, right - left, bottom - top);
            // View#draw() leaves the View's own scroll offset to its parent. Remove it so that
            // the output matches the View's bounds on screen, then move to this tile.
            recording.translate(-left - view.getScrollX(), -top - view.getScrollY());
            // Before Android 12, ViewGroup enables Z reordering only when recording its own
            // RenderNode, so enable it here for the direct children of the View.
            recording.enableZ();
            view.draw(recording);
            recording.disableZ();
          } finally {
            node.endRecording();
          }
          Bitmap pixels = render(renderer, reader, readBack);
          if (pixels == null) {
            return false;
          }
          try {
            source.set(0, 0, right - left, bottom - top);
            destination.set(left, top, right, bottom);
            canvas.drawBitmap(pixels, source, destination, paint);
          } finally {
            readBack.release(pixels);
          }
        }
      }
      return true;
    } catch (RuntimeException e) {
      LLog.e(TAG, "Offscreen capture failed: " + e.getMessage());
      return false;
    } finally {
      if (renderer != null) {
        renderer.destroy();
      }
      if (reader != null) {
        reader.close();
      }
      readBack.recycle();
      node.discardDisplayList();
      if (dirty) {
        view.postInvalidate();
      }
    }
  }

  // getMatrix() is deprecated because a hardware canvas may not report its full matrix; the
  // capture destination is a software canvas, where it is exact.
  @SuppressWarnings("deprecation")
  private static boolean scales(@NonNull Canvas canvas) {
    float[] values = new float[9];
    canvas.getMatrix().getValues(values);
    return values[Matrix.MSCALE_X] != 1f || values[Matrix.MSCALE_Y] != 1f
        || values[Matrix.MSKEW_X] != 0f || values[Matrix.MSKEW_Y] != 0f;
  }

  /** Renders the current content root and returns its pixels, or null on failure. */
  @Nullable
  @RequiresApi(Build.VERSION_CODES.Q)
  private static Bitmap render(
      @NonNull HardwareRenderer renderer, @NonNull ImageReader reader, @NonNull ReadBack readBack) {
    // The default vsync time is the Choreographer frame time when called during a frame, which
    // would be the same for every tile; HWUI then skips all tiles after the first as already drawn
    // in that vsync and renders them one vsync later.
    int status = renderer.createRenderRequest()
                     .setVsyncTime(System.nanoTime())
                     .setWaitForPresent(true)
                     .syncAndDraw();
    if ((status
            & (HardwareRenderer.SYNC_LOST_SURFACE_REWARD_IF_FOUND
                | HardwareRenderer.SYNC_CONTEXT_IS_STOPPED))
        != 0) {
      LLog.w(TAG, "syncAndDraw failed with status " + status);
      return null;
    }
    Image image = acquireImage(reader, (status & HardwareRenderer.SYNC_FRAME_DROPPED) != 0);
    if (image == null) {
      LLog.w(TAG, "Timed out waiting for the rendered image");
      return null;
    }
    try {
      return readBack.read(image);
    } finally {
      image.close();
    }
  }

  /**
   * Returns the image of the frame just requested. HWUI renders extra frames of the content root
   * while it holds running animations or animated images, so older queued images may show the
   * previous tile.
   */
  @Nullable
  private static Image acquireImage(@NonNull ImageReader reader, boolean frameDropped) {
    if (!frameDropped) {
      // The requested frame has been presented, so it is the latest image, unless an extra
      // frame of the same content followed it.
      return reader.acquireLatestImage();
    }
    // HWUI renders the content at the next vsync instead. Everything queued now is older.
    Image stale;
    while ((stale = reader.acquireNextImage()) != null) {
      stale.close();
    }
    long deadline = SystemClock.uptimeMillis() + IMAGE_TIMEOUT_MS;
    Image image;
    while ((image = reader.acquireNextImage()) == null && SystemClock.uptimeMillis() < deadline) {
      SystemClock.sleep(1);
    }
    return image;
  }

  /** Copies rendered images into a software bitmap that is reused across tiles. */
  @RequiresApi(Build.VERSION_CODES.Q)
  private static final class ReadBack {
    private final int mHeight;
    @Nullable private Bitmap mPixels;
    @Nullable private ByteBuffer mStaging;

    ReadBack(int height) {
      mHeight = height;
    }

    /**
     * Returns a bitmap whose top-left part holds the pixels of {@code image}, or null on failure.
     * Pass the result to {@link #release(Bitmap)} after drawing it.
     */
    @Nullable
    Bitmap read(@NonNull Image image) {
      Bitmap pixels = readPlane(image);
      return pixels != null ? pixels : copyHardwareBuffer(image);
    }

    void release(@NonNull Bitmap pixels) {
      if (pixels != mPixels) {
        pixels.recycle();
      }
    }

    void recycle() {
      if (mPixels != null) {
        mPixels.recycle();
        mPixels = null;
      }
    }

    /** Copies the CPU-mapped plane into the reused bitmap, which is as wide as a padded row. */
    @Nullable
    private Bitmap readPlane(@NonNull Image image) {
      Image.Plane plane;
      try {
        plane = image.getPlanes()[0];
      } catch (RuntimeException e) {
        // The buffer cannot be mapped for the CPU.
        return null;
      }
      final int stride = plane.getRowStride();
      if (plane.getPixelStride() != 4 || stride % 4 != 0) {
        return null;
      }
      if (mPixels == null || mPixels.getWidth() * 4 != stride) {
        recycle();
        mPixels = Bitmap.createBitmap(stride / 4, mHeight, Bitmap.Config.ARGB_8888);
      }
      ByteBuffer buffer = plane.getBuffer();
      buffer.rewind();
      if (buffer.remaining() < stride * mHeight) {
        // The last row of a padded buffer is not padded. Copy into a full-size staging buffer,
        // because copyPixelsFromBuffer() needs as many bytes as the bitmap holds.
        if (mStaging == null) {
          mStaging = ByteBuffer.allocateDirect(stride * mHeight);
        }
        mStaging.clear();
        mStaging.put(buffer);
        mStaging.rewind();
        buffer = mStaging;
      }
      mPixels.copyPixelsFromBuffer(buffer);
      return mPixels;
    }

    /** Copies the image as a HARDWARE bitmap, which reads it back through the GPU. */
    @Nullable
    private static Bitmap copyHardwareBuffer(@NonNull Image image) {
      HardwareBuffer buffer = image.getHardwareBuffer();
      if (buffer == null) {
        return null;
      }
      try {
        Bitmap hardware = Bitmap.wrapHardwareBuffer(buffer, null);
        if (hardware == null) {
          return null;
        }
        // A software canvas cannot draw HARDWARE bitmaps.
        Bitmap pixels = hardware.copy(Bitmap.Config.ARGB_8888, false);
        hardware.recycle();
        return pixels;
      } finally {
        buffer.close();
      }
    }
  }
}
