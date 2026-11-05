// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.render;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertSame;
import static org.junit.Assert.assertTrue;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.when;

import android.app.Application;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.HardwareRenderer;
import android.graphics.Paint;
import android.graphics.Path;
import android.graphics.PixelFormat;
import android.graphics.RecordingCanvas;
import android.graphics.RenderNode;
import android.media.Image;
import android.media.ImageReader;
import androidx.test.ext.junit.runners.AndroidJUnit4;
import androidx.test.filters.SdkSuppress;
import androidx.test.platform.app.InstrumentationRegistry;
import com.lynx.tasm.LynxEnv;
import java.io.File;
import java.io.FileOutputStream;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.util.concurrent.atomic.AtomicInteger;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

@RunWith(AndroidJUnit4.class)
public class PathResourceCacheTest {
  @Before
  public void setUp() {
    LynxEnv.inst().init((Application) InstrumentationRegistry.getInstrumentation()
                            .getTargetContext()
                            .getApplicationContext(),
        null, null, null, null);
    NativeDisplayListBuilder.ensureRegistered();
  }

  private PathResourceCache.Table prepare(
      PathResourceCache cache, NativeDisplayListBuilder list, AtomicInteger reads) {
    return cache.prepare(list.toPathKeysBuffer(), list.toItemsBuffer(), (index, key, part) -> {
      reads.incrementAndGet();
      return list.getPathBuffer(index, key, part);
    });
  }

  @Test
  public void retainsCompiledPathAcrossListsAndDrawOnlyFrames() {
    PathResourceCache cache = new PathResourceCache();
    AtomicInteger reads = new AtomicInteger();
    try (NativeDisplayListBuilder first = new NativeDisplayListBuilder();
         NativeDisplayListBuilder second = new NativeDisplayListBuilder()) {
      int path = first.addPath(30, 20, 5, 5, 5, 5, 5, 5, 5, 5);
      first.pathOp(path, Color.RED, 0, 2, 3, false);
      PathResourceCache.Table initial = prepare(cache, first, reads);
      assertNotNull(initial.get(0, 0));
      int shared = second.copyPath(first, path);
      second.pathOp(shared, Color.BLUE, 0, 10, 12, false);
      PathResourceCache.Table next = prepare(cache, second, reads);
      assertSame(initial.get(0, 0), next.get(0, 0));
      assertEquals(1, cache.getCompilationCount());
      assertEquals(3, reads.get());
      first.close();
      Bitmap bitmap = Bitmap.createBitmap(64, 64, Bitmap.Config.ARGB_8888);
      Canvas canvas = new Canvas(bitmap);
      Paint paint = new Paint();
      paint.setColor(Color.BLUE);
      for (int i = 0; i < 20; i++) next.get(0, 0).draw(canvas, paint, 10, 12);
      assertEquals(Color.BLUE, bitmap.getPixel(20, 20));
      assertEquals(Color.TRANSPARENT, bitmap.getPixel(10, 12));
      assertEquals(3, reads.get());
    }
  }

  @Test
  public void fillRuleVariantsRemainIndependent() {
    try (NativeDisplayListBuilder list = new NativeDisplayListBuilder()) {
      int path = list.addPath(new int[] {0, 1, 1, 1, 4, 0, 1, 1, 1, 4},
          new float[] {0, 0, 30, 0, 30, 30, 0, 30, 10, 10, 20, 10, 20, 20, 10, 20});
      list.pathOp(path, Color.RED, 0, 0, 0, false).pathOp(path, Color.BLUE, 1, 0, 0, false);
      PathResourceCache cache = new PathResourceCache();
      PathResourceCache.Table table = prepare(cache, list, new AtomicInteger());
      assertEquals(2, cache.getCompilationCount());
      Bitmap bitmap = Bitmap.createBitmap(32, 32, Bitmap.Config.ARGB_8888);
      Canvas canvas = new Canvas(bitmap);
      Paint paint = new Paint();
      paint.setColor(Color.RED);
      table.get(0, 0).draw(canvas, paint, 0, 0);
      assertEquals(Color.RED, bitmap.getPixel(15, 15));
      bitmap.eraseColor(Color.TRANSPARENT);
      table.get(0, 1).draw(canvas, paint, 0, 0);
      assertEquals(Color.TRANSPARENT, bitmap.getPixel(15, 15));
      assertEquals(Color.RED, bitmap.getPixel(5, 5));
      table.get(0, 0).draw(canvas, paint, 0, 0);
      assertEquals(Color.RED, bitmap.getPixel(15, 15));
    }
  }

  @Test
  public void clipPreservesPlacementAndInvalidClipStaysClosed() {
    try (NativeDisplayListBuilder list = new NativeDisplayListBuilder();
         NativeDisplayListBuilder invalid = new NativeDisplayListBuilder()) {
      int clip = list.addPath(20, 20, 5, 5, 5, 5, 5, 5, 5, 5);
      int fill = list.addPath(40, 40);
      list.begin(1, 1, 0, 0, 40, 40)
          .pathOp(clip, 0, 0, 10, 10, true)
          .pathOp(fill, Color.GREEN, 0, 0, 0, false)
          .end();
      PathResourceCache cache = new PathResourceCache();
      DisplayListApplier applier = new DisplayListApplier(list.toItemsBuffer(), null,
          mock(PlatformRendererContext.class), mock(IRendererHost.class));
      applier.setPathResources(prepare(cache, list, new AtomicInteger()));
      Bitmap bitmap = Bitmap.createBitmap(40, 40, Bitmap.Config.ARGB_8888);
      Canvas canvas = new Canvas(bitmap);
      applier.drawTillNextView(canvas);
      assertEquals(Color.GREEN, bitmap.getPixel(20, 20));
      assertEquals(Color.TRANSPARENT, bitmap.getPixel(10, 10));
      assertEquals(Color.TRANSPARENT, bitmap.getPixel(5, 5));
      applier.reset();
      bitmap.eraseColor(Color.TRANSPARENT);
      applier.drawTillNextView(canvas);
      assertEquals(Color.GREEN, bitmap.getPixel(20, 20));
      assertEquals(2, cache.getCompilationCount());

      int newFill = invalid.copyPath(list, fill);
      invalid.begin(1, 1, 0, 0, 40, 40)
          .pathOp(-1, 0, 0, 0, 0, true)
          .pathOp(newFill, Color.RED, 0, 0, 0, false)
          .end();
      applier.setBuffer(invalid.toItemsBuffer(), null);
      applier.setPathResources(prepare(cache, invalid, new AtomicInteger()));
      bitmap.eraseColor(Color.TRANSPARENT);
      applier.drawTillNextView(canvas);
      assertEquals(Color.TRANSPARENT, bitmap.getPixel(20, 20));
      assertNull(prepare(cache, invalid, new AtomicInteger()).get(-1, 0));
    }
  }

  @Test
  public void rejectsMalformedGeometryAndWrongResourceIdentity() {
    try (NativeDisplayListBuilder list = new NativeDisplayListBuilder()) {
      int path = list.addPath(new int[] {0, 1, 1, 4}, new float[] {0, 0, 20, 0, 20, 20});
      list.pathOp(path, Color.RED, 0, 0, 0, false);
      long key = list.toPathKeysBuffer().order(ByteOrder.nativeOrder()).getLong(0);
      assertNull(list.getPathBuffer(0, key + 1, 0));
      assertNull(list.getPathBuffer(99, key, 0));
      ByteBuffer source = list.getPathBuffer(0, key, 0);
      ByteBuffer malformed =
          ByteBuffer.allocateDirect(source.capacity()).order(ByteOrder.nativeOrder());
      malformed.put(source.duplicate());
      malformed.putInt(8, Integer.MAX_VALUE);
      assertNull(PathResourceCache.decode(
          malformed, list.getPathBuffer(0, key, 1), list.getPathBuffer(0, key, 2), 0));
      malformed.putInt(8, 3);
      malformed.putFloat(20, Float.NaN);
      assertNull(PathResourceCache.decode(
          malformed, list.getPathBuffer(0, key, 1), list.getPathBuffer(0, key, 2), 0));
    }
  }

  @Test
  public void transformedCurvesMatchThePlatformPathAtEachScale() {
    try (NativeDisplayListBuilder list = new NativeDisplayListBuilder()) {
      int path = list.addPath(
          new int[] {0, 2, 3, 4}, new float[] {0, 0, 0, 20, 20, 20, 30, 20, 30, 0, 0, 0});
      list.begin(1, 1, 0, 0, 64, 64).pathOp(path, Color.BLUE, 0, 3, 4, false).end();
      PathResourceCache cache = new PathResourceCache();
      AtomicInteger reads = new AtomicInteger();
      DisplayListApplier applier = new DisplayListApplier(list.toItemsBuffer(), null,
          mock(PlatformRendererContext.class), mock(IRendererHost.class));
      applier.setPathResources(prepare(cache, list, reads));
      Path reference = new Path();
      reference.moveTo(0, 0);
      reference.quadTo(0, 20, 20, 20);
      reference.cubicTo(30, 20, 30, 0, 0, 0);
      reference.close();
      Paint paint = new Paint();
      paint.setColor(Color.BLUE);
      for (int scale = 1; scale <= 3; scale++) {
        Bitmap actual = Bitmap.createBitmap(180, 180, Bitmap.Config.ARGB_8888);
        Bitmap expected = Bitmap.createBitmap(180, 180, Bitmap.Config.ARGB_8888);
        Canvas a = new Canvas(actual);
        Canvas b = new Canvas(expected);
        for (Canvas canvas : new Canvas[] {a, b}) {
          canvas.translate(50, 40);
          canvas.scale(scale, scale);
          canvas.skew(.2f, .1f);
          canvas.rotate(25);
        }
        applier.drawTillNextView(a);
        b.translate(3, 4);
        b.drawPath(reference, paint);
        for (int y = 0; y < 180; y++) {
          for (int x = 0; x < 180; x++)
            assertEquals("scale " + scale + " pixel " + x + "," + y, expected.getPixel(x, y),
                actual.getPixel(x, y));
        }
        applier.reset();
        actual.recycle();
        expected.recycle();
      }
      assertEquals(1, cache.getCompilationCount());
      assertEquals(3, reads.get());
    }
  }

  @Test
  public void emptyPrimitiveAndCommandClipsSuppressContent() {
    for (boolean primitive : new boolean[] {true, false}) {
      try (NativeDisplayListBuilder list = new NativeDisplayListBuilder()) {
        int empty = primitive ? list.addPath(0, 20) : list.addPath(new int[0], new float[0]);
        int fill = list.addPath(20, 20);
        list.begin(1, 1, 0, 0, 20, 20)
            .pathOp(empty, 0, 0, 0, 0, true)
            .pathOp(fill, Color.RED, 0, 0, 0, false)
            .end();
        PathResourceCache cache = new PathResourceCache();
        DisplayListApplier applier = new DisplayListApplier(list.toItemsBuffer(), null,
            mock(PlatformRendererContext.class), mock(IRendererHost.class));
        applier.setPathResources(prepare(cache, list, new AtomicInteger()));
        Bitmap bitmap = Bitmap.createBitmap(20, 20, Bitmap.Config.ARGB_8888);
        Canvas canvas = new Canvas(bitmap);
        applier.drawTillNextView(canvas);
        assertEquals(Color.TRANSPARENT, bitmap.getPixel(10, 10));
        canvas.drawColor(Color.GREEN);
        assertEquals("End restores the empty clip", Color.GREEN, bitmap.getPixel(10, 10));
        bitmap.recycle();
      }
    }
  }

  @Test
  public void rootNormalizationMatchesLegacyAndIsReused() {
    IRendererHost host = mock(IRendererHost.class);
    when(host.getRendererHostWidth()).thenReturn(30);
    when(host.getRendererHostHeight()).thenReturn(25);
    try (NativeDisplayListBuilder legacy = new NativeDisplayListBuilder();
         NativeDisplayListBuilder retained = new NativeDisplayListBuilder()) {
      float[] radii = {8, 6, 10, 9, 5, 8, 7, 4};
      int path = retained.addPath(40, 40, radii);
      retained.begin(1, 1, 0, 0, 40, 40)
          .pathOp(path, Color.GREEN, 0, -3, -2, false)
          .pathOp(path, Color.RED, 0, 10, 0, false)
          .pathOp(path, 0, 0, -3, -2, true)
          .drawView(2, 0, 0)
          .end();
      legacy.begin(1, 1, 0, 0, 40, 40)
          .recordBox(-3, -2, 40, 40, radii)
          .fill(Color.GREEN, 0)
          .recordBox(10, 0, 40, 40, radii)
          .fill(Color.RED, 1)
          .clipRect(-3, -2, 40, 40, radii)
          .drawView(2, 0, 0)
          .end();
      PathResourceCache cache = new PathResourceCache();
      PathResourceCache.Table table = prepare(cache, retained, new AtomicInteger());
      DisplayListApplier actual = new DisplayListApplier(
          retained.toItemsBuffer(), null, mock(PlatformRendererContext.class), host);
      actual.setPathResources(table);
      DisplayListApplier expected = new DisplayListApplier(
          legacy.toItemsBuffer(), null, mock(PlatformRendererContext.class), host);
      for (int frame = 0; frame < 20; frame++) {
        Bitmap a = Bitmap.createBitmap(64, 64, Bitmap.Config.ARGB_8888);
        Bitmap b = Bitmap.createBitmap(64, 64, Bitmap.Config.ARGB_8888);
        Canvas ac = new Canvas(a);
        Canvas bc = new Canvas(b);
        actual.drawTillNextView(ac);
        expected.drawTillNextView(bc);
        ac.drawColor(Color.BLUE);
        bc.drawColor(Color.BLUE);
        actual.drawTillNextView(ac);
        expected.drawTillNextView(bc);
        for (int y = 0; y < 64; y++) {
          for (int x = 0; x < 64; x++)
            assertEquals("pixel " + x + "," + y, b.getPixel(x, y), a.getPixel(x, y));
        }
        assertEquals(1, ac.getSaveCount());
        actual.reset();
        expected.reset();
        a.recycle();
        b.recycle();
      }
      assertEquals(1, cache.getCompilationCount());
      assertEquals(2, table.get(0, 0).getNormalizationCount());
    }
  }

  @Test
  @SdkSuppress(minSdkVersion = 29)
  public void hardwareCanvasUsesRetainedFillAndClipWithoutGeometryReads() throws Exception {
    try (NativeDisplayListBuilder list = new NativeDisplayListBuilder()) {
      int clip = list.addPath(30, 30, 8, 8, 8, 8, 8, 8, 8, 8);
      int fill = list.addPath(new int[] {0, 1, 1, 1, 5}, new float[] {0, 0, 60, 0, 60, 60, 0, 60});
      list.begin(1, 1, 0, 0, 64, 64)
          .pathOp(clip, 0, 0, 10, 10, true)
          .pathOp(fill, Color.GREEN, 0, 0, 0, false)
          .end();
      PathResourceCache cache = new PathResourceCache();
      AtomicInteger reads = new AtomicInteger();
      DisplayListApplier applier = new DisplayListApplier(list.toItemsBuffer(), null,
          mock(PlatformRendererContext.class), mock(IRendererHost.class));
      applier.setPathResources(prepare(cache, list, reads));
      RenderNode root = new RenderNode("retained-path");
      root.setPosition(0, 0, 64, 64);
      HardwareRenderer renderer = new HardwareRenderer();
      try (ImageReader reader = ImageReader.newInstance(64, 64, PixelFormat.RGBA_8888, 2)) {
        renderer.setSurface(reader.getSurface());
        renderer.setContentRoot(root);
        for (int frame = 0; frame < 20; frame++) {
          RecordingCanvas canvas = root.beginRecording(64, 64);
          assertTrue(canvas.isHardwareAccelerated());
          canvas.drawColor(Color.WHITE);
          applier.drawTillNextView(canvas);
          assertEquals(1, canvas.getSaveCount());
          root.endRecording();
          renderer.createRenderRequest().setWaitForPresent(true).syncAndDraw();
          try (Image image = reader.acquireNextImage()) {
            assertNotNull("Hardware frame must be available", image);
            Image.Plane plane = image.getPlanes()[0];
            ByteBuffer pixels = plane.getBuffer();
            int[] colors = new int[64 * 64];
            for (int y = 0; y < 64; y++) {
              for (int x = 0; x < 64; x++) {
                int offset = y * plane.getRowStride() + x * plane.getPixelStride();
                colors[y * 64 + x] =
                    Color.argb(pixels.get(offset + 3) & 255, pixels.get(offset) & 255,
                        pixels.get(offset + 1) & 255, pixels.get(offset + 2) & 255);
              }
            }
            Bitmap bitmap = Bitmap.createBitmap(colors, 64, 64, Bitmap.Config.ARGB_8888);
            assertEquals(Color.GREEN, bitmap.getPixel(20, 20));
            assertEquals(Color.WHITE, bitmap.getPixel(10, 10));
            assertEquals(Color.WHITE, bitmap.getPixel(5, 5));
            if (frame == 19) {
              File output = new File(
                  InstrumentationRegistry.getInstrumentation().getTargetContext().getFilesDir(),
                  "retained-path-hardware.png");
              try (FileOutputStream stream = new FileOutputStream(output)) {
                assertTrue(bitmap.compress(Bitmap.CompressFormat.PNG, 100, stream));
              }
            }
            bitmap.recycle();
          }
          applier.reset();
        }
        assertEquals(2, cache.getCompilationCount());
        assertEquals(6, reads.get());
      } finally {
        renderer.destroy();
        root.discardDisplayList();
      }
    }
  }
}
