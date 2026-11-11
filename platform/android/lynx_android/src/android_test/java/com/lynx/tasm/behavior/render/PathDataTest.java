// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.render;

import static org.junit.Assert.assertArrayEquals;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertNotSame;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertSame;
import static org.junit.Assert.assertTrue;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.anyInt;
import static org.mockito.ArgumentMatchers.anyLong;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.clearInvocations;
import static org.mockito.Mockito.doAnswer;
import static org.mockito.Mockito.doCallRealMethod;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import android.app.Application;
import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Matrix;
import android.graphics.Paint;
import android.graphics.Path;
import android.graphics.PathMeasure;
import android.graphics.RectF;
import android.util.LruCache;
import android.util.SparseArray;
import android.view.View;
import androidx.core.graphics.PathParser;
import androidx.test.annotation.UiThreadTest;
import androidx.test.ext.junit.runners.AndroidJUnit4;
import androidx.test.platform.app.InstrumentationRegistry;
import com.lynx.tasm.LynxEnv;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.lang.ref.WeakReference;
import java.lang.reflect.Field;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.concurrent.ConcurrentHashMap;
import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

@RunWith(AndroidJUnit4.class)
public class PathDataTest {
  private static final int SIGN = 812;
  private static final int CLIP = NativeDisplayListBuilder.PATH_CLIP;
  private static final int OWNER = NativeDisplayListBuilder.PATH_OWNER_CLIP;
  private final ArrayList<NativeDisplayListBuilder> mLists = new ArrayList<>();
  private Context mApp;
  private PlatformRendererContext mContext;
  private IRendererHost mHost;
  private Renderer mRenderer;
  private NativeDisplayListBuilder mPathContext;
  private long mPathContextPtr;

  @Before
  public void setUp() {
    mApp = InstrumentationRegistry.getInstrumentation().getTargetContext().getApplicationContext();
    LynxEnv.inst().init((Application) mApp, null, null, null, null);
    NativeDisplayListBuilder.ensureRegistered();
    mContext = mock(PlatformRendererContext.class);
    setField(mContext, "mPaths", new LruCache<>(256));
    doCallRealMethod().when(mContext).getOrBuildPath(anyLong(), anyInt(), any());
    mHost = mock(IRendererHost.class);
    InstrumentationRegistry.getInstrumentation().runOnMainSync(() -> {
      mPathContext = new NativeDisplayListBuilder();
      mPathContextPtr = mPathContext.createRendererContext(mContext, SIGN + 1);
      View view = new View(mApp);
      view.layout(0, 0, 100, 100);
      when(mHost.getView()).thenReturn(view);
    });
  }

  @After
  public void tearDown() {
    InstrumentationRegistry.getInstrumentation().runOnMainSync(() -> {
      if (mRenderer != null)
        mRenderer.onDestroy();
      setField(mContext, "mNativePtr", 0L);
      for (int i = mLists.size() - 1; i >= 0; i--) mLists.get(i).close();
      mLists.clear();
      mPathContext.close();
    });
  }

  private NativeDisplayListBuilder list() {
    NativeDisplayListBuilder list = new NativeDisplayListBuilder().usePathContext(mPathContext);
    mLists.add(list);
    return list;
  }

  private static Object field(Object object, String name) {
    try {
      Class<?> owner = object instanceof PlatformRendererContext ? PlatformRendererContext.class
                                                                 : object.getClass();
      Field field = owner.getDeclaredField(name);
      field.setAccessible(true);
      return field.get(object);
    } catch (ReflectiveOperationException error) {
      throw new AssertionError(error);
    }
  }

  private static void setField(Object object, String name, Object value) {
    try {
      Field field = PlatformRendererContext.class.getDeclaredField(name);
      field.setAccessible(true);
      field.set(object, value);
    } catch (ReflectiveOperationException error) {
      throw new AssertionError(error);
    }
  }

  private void prepareContextForDestroy() {
    setField(mContext, "mViewHolder", new HashMap<>());
    setField(mContext, "mImageUIMethodTargets", new HashMap<>());
    setField(mContext, "mImageUIMethodSignsByKey", new HashMap<>());
    setField(mContext, "mExtraDatas", new ConcurrentHashMap<>());
    setField(mContext, "mRootView", new WeakReference<>(null));
    doCallRealMethod().when(mContext).destroy();
  }

  private DisplayListApplier applier(NativeDisplayListBuilder list) {
    doAnswer(call -> list.getPathObject(call.getArgument(1), call.getArgument(2)))
        .when(mContext)
        .getPath(eq(SIGN), anyInt(), anyInt());
    DisplayListApplier applier =
        new DisplayListApplier(list.toItemsBuffer(), list.toDataBuffer(), mContext, mHost);
    applier.setOwnerSign(SIGN);
    return applier;
  }

  @SuppressWarnings("unchecked")
  private LruCache<Long, Object> pathEntries() {
    return (LruCache<Long, Object>) field(mContext, "mPaths");
  }

  private Object onlyPathEntry() {
    assertEquals(1, pathEntries().size());
    return pathEntries().snapshot().values().iterator().next();
  }

  private static Bitmap draw(Path path) {
    Bitmap bitmap = Bitmap.createBitmap(100, 100, Bitmap.Config.ARGB_8888);
    Paint paint = new Paint();
    paint.setColor(Color.RED);
    Canvas canvas = new Canvas(bitmap);
    canvas.clipPath(path);
    canvas.drawRect(0, 0, 100, 100, paint);
    return bitmap;
  }

  private static RectF bounds(Path path) {
    RectF bounds = new RectF();
    path.computeBounds(bounds, true);
    return bounds;
  }

  private void saveBitmap(String name, Bitmap bitmap) {
    try (FileOutputStream output = new FileOutputStream(new File(mApp.getCacheDir(), name))) {
      assertTrue(bitmap.compress(Bitmap.CompressFormat.PNG, 100, output));
    } catch (IOException error) {
      throw new AssertionError(error);
    }
  }

  private NativeDisplayListBuilder rendererState() {
    mRenderer = new Renderer(mContext, SIGN);
    mRenderer.setRenderHost(mHost);
    mRenderer.setLynxFrame(false, 0, 0, 100, 100, 0, 0);
    NativeDisplayListBuilder state = list();
    setField(mContext, "mNativePtr", state.createRendererContext(mContext, SIGN));
    doCallRealMethod().when(mContext).getDisplayListItemsBuffer(SIGN);
    doCallRealMethod().when(mContext).getDisplayListDataBuffer(SIGN);
    doCallRealMethod().when(mContext).getPath(eq(SIGN), anyInt(), anyInt());
    doCallRealMethod().when(mContext).getOwnerClipPath(eq(SIGN), anyInt());
    doAnswer(call -> {
      ByteBuffer properties = call.getArgument(1);
      properties.order(ByteOrder.nativeOrder());
      mRenderer.applySubtreeProperties(properties, call.getArgument(2));
      return null;
    })
        .when(mContext)
        .updatePlatformRendererSubtreeProperties(eq(SIGN), any(), anyInt());
    return state;
  }

  private Bitmap drawNativeHost() {
    Bitmap bitmap = Bitmap.createBitmap(100, 100, Bitmap.Config.ARGB_8888);
    Canvas canvas = new Canvas(bitmap);
    int scrollX = mHost.getRendererHostScrollX();
    int scrollY = mHost.getRendererHostScrollY();
    int count = canvas.getSaveCount();
    canvas.translate(-scrollX, -scrollY);
    mRenderer.onDraw(canvas);
    mRenderer.beforeDrawHost(canvas);
    View child = new View(mApp);
    child.layout(0, 0, 100, 100);
    child.setBackgroundColor(Color.GREEN);
    canvas.save();
    canvas.translate(scrollX, scrollY);
    child.draw(canvas);
    canvas.restore();
    mRenderer.afterDispatchDraw(canvas);
    canvas.translate(scrollX, scrollY);
    assertEquals(count, canvas.getSaveCount());
    return bitmap;
  }

  @Test
  @UiThreadTest
  public void primitivesAndRequiredVariantReuse() {
    NativeDisplayListBuilder rect = list().rectPath(CLIP, 20, 30, 0, 0, 0);
    assertEquals(0, pathEntries().size());
    Path path = rect.getPathObject(0, 0);
    assertEquals(1, pathEntries().size());
    Object entry = onlyPathEntry();
    assertSame(path, field(entry, "mWinding"));
    assertNull(field(entry, "mEvenOdd"));
    assertEquals(new RectF(0, 0, 20, 30), bounds(path));
    assertSame(path, rect.getPathObject(0, 0));
    assertNull(rect.getPathObject(-1, 0));
    assertNull(rect.getPathObject(0, 2));
    NativeDisplayListBuilder rounded =
        list().rectPath(CLIP, 20, 30, 0, 0, 0, 10, 10, 2, 2, 3, 3, 4, 4);
    Path round = rounded.getPathObject(0, 0);
    assertEquals(new RectF(0, 0, 20, 30), bounds(round));
    Bitmap bitmap = draw(round);
    assertEquals(Color.TRANSPARENT, bitmap.getPixel(0, 0));
    assertEquals(Color.RED, bitmap.getPixel(10, 15));
    assertEquals(Color.TRANSPARENT,
        draw(list().rectPath(CLIP, 0, 20, 0, 0, 0).getPathObject(0, 0)).getPixel(0, 10));
  }

  @Test
  @UiThreadTest
  public void packedCommandsPreserveCurvesAndCloseContinuation() {
    Path path = list().commandPath(CLIP, 0, 0, 0).getPathObject(0, 0);
    assertEquals(new RectF(0, 0, 50, 40), bounds(path));
    PathMeasure measure = new PathMeasure(path, false);
    assertTrue(measure.isClosed());
    assertTrue(measure.getLength() > 90);
    assertTrue(measure.nextContour());
    assertFalse(measure.isClosed());
    assertEquals(Math.sqrt(50), measure.getLength(), 0.01);
    float[] point = new float[2];
    assertTrue(measure.getPosTan(measure.getLength(), point, null));
    assertArrayEquals(new float[] {5, 5}, point, 0.01f);
    assertTrue(measure.nextContour());
    assertEquals(10, measure.getLength(), 0.01);
    assertFalse(measure.nextContour());
    assertEquals(Color.RED, draw(path).getPixel(10, 15));
  }

  @Test
  @UiThreadTest
  public void svgCompactArcFlagsRelativeCommandsAndScale() {
    Path path =
        list().svgPath(CLIP, "m1e1 1e1 a5 5 0 01 10 0 l0 10 h-10z", 2, 0, 0, 0).getPathObject(0, 0);
    assertNotNull(path);
    RectF rect = bounds(path);
    assertEquals(20, rect.left, 0.01);
    assertEquals(10, rect.top, 0.01);
    assertEquals(40, rect.right, 0.01);
    assertEquals(40, rect.bottom, 0.01);
    assertEquals(Color.RED, draw(path).getPixel(30, 30));
    assertEquals(Color.TRANSPARENT, draw(path).getPixel(10, 10));
    Path zeroArc =
        list().svgPath(CLIP, "M0 0A0 5 0 0 1 20 0L20 20L0 20Z", 1, 0, 0, 0).getPathObject(0, 0);
    assertNotNull(zeroArc);
    assertEquals(Color.RED, draw(zeroArc).getPixel(15, 5));
    assertNotNull(
        list().svgPath(CLIP, "m1e2 3 A10 20 45 01-2.5 .3z", 1, 0, 0, 0).getPathObject(0, 0));
    Path afterClose = list()
                          .svgPath(CLIP, "M10 10L20 10L20 20Z m25 0 l5 0 l0 5z", 1, 0, 0, 0)
                          .getPathObject(0, 0);
    assertEquals(new RectF(10, 10, 40, 20), bounds(afterClose));
    assertEquals(Color.RED, draw(afterClose).getPixel(38, 11));
    for (String data : new String[] {"M0 0L20 20S-20 20 0 0", "M0 0L20 20T-20 20"}) {
      Path legacy = PathParser.createPathFromPathData(data);
      Path converted = list().svgPath(CLIP, data, 1, 0, 0, 0).getPathObject(0, 0);
      assertNotNull(converted);
      assertEquals(bounds(legacy), bounds(converted));
    }
  }

  @Test
  @UiThreadTest
  public void svgArcGeometryMatchesLegacyAcrossFlagsRotationAndScale() {
    for (int large = 0; large <= 1; large++) {
      for (int sweep = 0; sweep <= 1; sweep++) {
        for (int rotation : new int[] {0, 37, 90}) {
          for (float scale : new float[] {1, 1.5f}) {
            String data = "M10 20A20 12 " + rotation + " " + large + " " + sweep + " 45 40L10 45Z";
            Path expected = PathParser.createPathFromPathData(data);
            Matrix matrix = new Matrix();
            matrix.setScale(scale, scale);
            expected.transform(matrix);
            Path actual = list().svgPath(CLIP, data, scale, 0, 0, 0).getPathObject(0, 0);
            assertNotNull(data, actual);
            PathMeasure expectedMeasure = new PathMeasure(expected, false);
            PathMeasure actualMeasure = new PathMeasure(actual, false);
            assertEquals(data, expectedMeasure.getLength(), actualMeasure.getLength(), 0.02);
            float[] expectedPoint = new float[2], actualPoint = new float[2];
            for (int i = 0; i <= 64; i++) {
              assertTrue(expectedMeasure.getPosTan(
                  expectedMeasure.getLength() * i / 64, expectedPoint, null));
              assertTrue(
                  actualMeasure.getPosTan(actualMeasure.getLength() * i / 64, actualPoint, null));
              assertArrayEquals(data, expectedPoint, actualPoint, 0.02f);
            }
            Bitmap expectedClip = draw(expected), actualClip = draw(actual);
            int different = 0;
            for (int y = 0; y < 100; y++) {
              for (int x = 0; x < 100; x++) {
                if (expectedClip.getPixel(x, y) != actualClip.getPixel(x, y))
                  different++;
              }
            }
            assertTrue(data + ": " + different + " clip pixels differ", different <= 8);
          }
        }
      }
    }
  }

  @Test
  @UiThreadTest
  public void displayListBuffersStayDirectAndZeroPathSkipsPathLookup() {
    NativeDisplayListBuilder state = rendererState();
    list()
        .begin(SIGN, 0, 0, 0, 100, 100)
        .recordBox(0, 0, 100, 100)
        .fill(Color.RED, 0)
        .linearGradient(new int[] {Color.RED, Color.RED}, new float[] {0, 1}, 0, 0, 0, 0, 90)
        .end()
        .installIntoRenderer(state);
    ByteBuffer itemsWithoutPaths = mContext.getDisplayListItemsBuffer(SIGN);
    ByteBuffer dataWithoutPaths = mContext.getDisplayListDataBuffer(SIGN);
    assertNotNull(itemsWithoutPaths);
    assertTrue(itemsWithoutPaths.isDirect());
    assertTrue(itemsWithoutPaths.isReadOnly());
    assertNotNull(dataWithoutPaths);
    assertTrue(dataWithoutPaths.isDirect());
    assertTrue(dataWithoutPaths.isReadOnly());
    clearInvocations(mContext);
    DisplayListApplier noPathApplier =
        new DisplayListApplier(itemsWithoutPaths, dataWithoutPaths, mContext, mHost);
    noPathApplier.setOwnerSign(SIGN);
    noPathApplier.drawTillNextView(
        new Canvas(Bitmap.createBitmap(100, 100, Bitmap.Config.ARGB_8888)));
    verify(mContext, never()).getPath(anyInt(), anyInt(), anyInt());

    list()
        .begin(SIGN, 0, 0, 0, 100, 100)
        .rectPath(CLIP, 30, 30, 1, 40, 40)
        .recordBox(0, 0, 100, 100)
        .fill(Color.BLUE, 0)
        .end()
        .installIntoRenderer(state);
    ByteBuffer itemsWithPaths = mContext.getDisplayListItemsBuffer(SIGN);
    ByteBuffer dataWithPaths = mContext.getDisplayListDataBuffer(SIGN);
    assertNotNull(itemsWithPaths);
    assertTrue(itemsWithPaths.isDirect());
    assertTrue(itemsWithPaths.isReadOnly());
    DisplayListApplier pathApplier =
        new DisplayListApplier(itemsWithPaths, dataWithPaths, mContext, mHost);
    pathApplier.setOwnerSign(SIGN);
    Bitmap frame1 = Bitmap.createBitmap(100, 100, Bitmap.Config.ARGB_8888);
    pathApplier.drawTillNextView(new Canvas(frame1));
    assertEquals(Color.BLUE, frame1.getPixel(45, 45));
    assertEquals(Color.TRANSPARENT, frame1.getPixel(15, 15));
    verify(mContext, times(1)).getPath(SIGN, 0, 1);

    // Subsequent redraws reuse DisplayListApplier's resolved cache without querying context again.
    pathApplier.reset();
    Bitmap frame2 = Bitmap.createBitmap(100, 100, Bitmap.Config.ARGB_8888);
    pathApplier.drawTillNextView(new Canvas(frame2));
    assertEquals(Color.BLUE, frame2.getPixel(45, 45));
    verify(mContext, times(1)).getPath(SIGN, 0, 1);
  }

  @Test
  @UiThreadTest
  public void fillRuleVariantsNeverMutateExistingPaths() {
    NativeDisplayListBuilder list =
        list().svgPath(CLIP, "M0 0H40V40H0Z M10 10H30V30H10Z", 1, 0, 0, 0);
    Path winding = list.getPathObject(0, 0);
    Path evenOdd = list.getPathObject(0, 1);
    assertNotSame(winding, evenOdd);
    assertSame(winding, list.getPathObject(0, 0));
    assertSame(evenOdd, list.getPathObject(0, 1));
    assertEquals(Path.FillType.WINDING, winding.getFillType());
    assertEquals(Path.FillType.EVEN_ODD, evenOdd.getFillType());
    assertEquals(Color.RED, draw(winding).getPixel(20, 20));
    assertEquals(Color.TRANSPARENT, draw(evenOdd).getPixel(20, 20));
    assertEquals(Color.RED, draw(winding).getPixel(20, 20));
  }

  @Test
  @UiThreadTest
  public void scopedClipPlacementAndSiblingRestoration() {
    NativeDisplayListBuilder list = list()
                                        .begin(0, 0, 0, 0, 100, 100)
                                        .begin(1, 0, 5, 7, 100, 100)
                                        .rectPath(CLIP, 10, 12, 0, 3, 4)
                                        .recordBox(0, 0, 100, 100)
                                        .fill(Color.RED, 0)
                                        .end()
                                        .recordBox(50, 50, 10, 10)
                                        .fill(Color.BLUE, 1)
                                        .end();
    Bitmap bitmap = Bitmap.createBitmap(100, 100, Bitmap.Config.ARGB_8888);
    Canvas canvas = new Canvas(bitmap);
    int count = canvas.getSaveCount();
    applier(list).drawTillNextView(canvas);
    assertEquals(count, canvas.getSaveCount());
    assertEquals(Color.RED, bitmap.getPixel(10, 15));
    assertEquals(Color.TRANSPARENT, bitmap.getPixel(5, 5));
    assertEquals(Color.BLUE, bitmap.getPixel(55, 55));
    saveBitmap("path-data-scoped.png", bitmap);
  }

  @Test
  @UiThreadTest
  public void invalidSvgClipsFailClosedAndCacheFailure() {
    for (String data : new String[] {"M0 0L20 0L20", "L10 10", "M1e100 0", "M3e38 0l3e38 0",
             "M0 0 A1 1 0 2 1 10 10", "M0 0A1 1 0", "M0 0A1 1 0 0", "M0 0Z 1"}) {
      NativeDisplayListBuilder list = list()
                                          .begin(0, 0, 0, 0, 100, 100)
                                          .svgPath(CLIP, data, 1, 0, 0, 0)
                                          .recordBox(0, 0, 100, 100)
                                          .fill(Color.RED, 0)
                                          .end();
      assertNull(data, list.getPathObject(0, 0));
      assertNull(data, list.getPathObject(0, 0));
      Bitmap bitmap = Bitmap.createBitmap(100, 100, Bitmap.Config.ARGB_8888);
      applier(list).drawTillNextView(new Canvas(bitmap));
      assertEquals(data, Color.TRANSPARENT, bitmap.getPixel(50, 50));
    }
    assertNull(list().svgPath(CLIP, "M3e38 0", 2, 0, 0, 0).getPathObject(0, 0));
    assertNull(list().svgPath(CLIP, "M0 0H10V10Z", 0, 0, 0, 0).getPathObject(0, 0));
    assertNull(list().svgPath(CLIP, "M0 0H10V10Z", -1, 0, 0, 0).getPathObject(0, 0));
    assertNull(list().svgPath(CLIP, "M0 0H10V10Z", Float.NaN, 0, 0, 0).getPathObject(0, 0));
  }

  @Test
  @UiThreadTest
  public void decodePathRejectsMalformedBinaryPayloads() {
    assertNull(PlatformRendererContext.decodePath(null));
    assertNull(PlatformRendererContext.decodePath(new byte[0]));
    assertNull(PlatformRendererContext.decodePath(new byte[3]));
    ByteBuffer badKind = ByteBuffer.allocate(20).order(ByteOrder.nativeOrder());
    badKind.putInt(99);
    assertNull(PlatformRendererContext.decodePath(badKind.array()));
    ByteBuffer shortRect = ByteBuffer.allocate(16).order(ByteOrder.nativeOrder());
    shortRect.putInt(0);
    assertNull(PlatformRendererContext.decodePath(shortRect.array()));
    ByteBuffer shortRRect = ByteBuffer.allocate(20).order(ByteOrder.nativeOrder());
    shortRRect.putInt(1);
    assertNull(PlatformRendererContext.decodePath(shortRRect.array()));
    ByteBuffer badVerb = ByteBuffer.allocate(16).order(ByteOrder.nativeOrder());
    badVerb.putInt(2).putInt(0).putInt(1).putInt(77);
    assertNull(PlatformRendererContext.decodePath(badVerb.array()));
    ByteBuffer trailingPoints = ByteBuffer.allocate(20).order(ByteOrder.nativeOrder());
    trailingPoints.putInt(2).putInt(1).putInt(0).putFloat(1f).putFloat(2f);
    assertNull(PlatformRendererContext.decodePath(trailingPoints.array()));
  }

  @Test
  @UiThreadTest
  public void nonAndroidSourceOnlyPathFailsClosedSafely() {
    NativeDisplayListBuilder list = list()
                                        .begin(0, 0, 0, 0, 100, 100)
                                        .sourceOnlyRectPath(CLIP, 40, 40, 0, 0, 0)
                                        .recordBox(0, 0, 100, 100)
                                        .fill(Color.RED, 0)
                                        .end();
    assertNull(list.getPathObject(0, 0));
    Bitmap bitmap = Bitmap.createBitmap(100, 100, Bitmap.Config.ARGB_8888);
    applier(list).drawTillNextView(new Canvas(bitmap));
    assertEquals(Color.TRANSPARENT, bitmap.getPixel(20, 20));
  }

  @Test
  @UiThreadTest
  public void bindingsAndJavaPathsOutliveTheirSourceOwners() {
    NativeDisplayListBuilder source = list().rectPath(CLIP, 20, 20, 0, 0, 0);
    Path original = source.getPathObject(0, 0);
    NativeDisplayListBuilder retained = list()
                                            .begin(0, 0, 0, 0, 100, 100)
                                            .reusePath(source, 0, CLIP, 0, 10, 10)
                                            .recordBox(0, 0, 100, 100)
                                            .fill(Color.RED, 0)
                                            .end();
    Path reused = retained.getPathObject(0, 0);
    assertSame(original, reused);
    source.close();
    assertEquals(1, pathEntries().size());
    Bitmap bitmap = Bitmap.createBitmap(100, 100, Bitmap.Config.ARGB_8888);
    applier(retained).drawTillNextView(new Canvas(bitmap));
    assertEquals(Color.RED, bitmap.getPixel(15, 15));
    assertEquals(Color.TRANSPARENT, bitmap.getPixel(5, 5));
    assertEquals(Color.TRANSPARENT, bitmap.getPixel(35, 15));
    retained.close();
    assertEquals(Color.RED, draw(reused).getPixel(10, 10));
  }

  @Test
  @UiThreadTest
  public void malformedConversionIsCachedForBothVariants() {
    NativeDisplayListBuilder source = list().svgPath(CLIP, "M0 0A", 1, 0, 0, 0);
    long key = source.getPathKey(0);
    assertTrue(key != 0);
    assertNull(source.getPathObject(0, 0));
    Object entry = onlyPathEntry();
    assertNull(source.getPathObject(0, 0));
    assertSame(entry, onlyPathEntry());
    assertNull(source.getPathObject(0, 1));
    assertNull(field(entry, "mWinding"));
    assertNull(field(entry, "mEvenOdd"));
    PlatformRendererContext.PathBytesLoader throwingLoader =
        new PlatformRendererContext.PathBytesLoader() {
          @Override
          byte[] load() {
            throw new AssertionError("A cached failure must not read source data again.");
          }
        };
    assertNull(mContext.getOrBuildPath(key, 0, throwingLoader));
    assertNull(mContext.getOrBuildPath(key, 1, throwingLoader));
  }

  @Test
  @UiThreadTest
  public void coldPathsLoadOnceAndWarmVariantsDoNotReadAgain() {
    NativeDisplayListBuilder valid = list().svgPath(CLIP, "M0 0H20V20H0Z", 1, 0, 0, 0);
    NativeDisplayListBuilder invalid = list().svgPath(CLIP, "M0 0A", 1, 0, 0, 0);
    long validKey = valid.getPathKey(0);
    long invalidKey = invalid.getPathKey(0);
    assertEquals(0, pathEntries().size());
    int[] reads = {0};
    Path evenOdd =
        mContext.getOrBuildPath(validKey, 1, new PlatformRendererContext.PathBytesLoader() {
          @Override
          byte[] load() {
            reads[0]++;
            return valid.getPathBytes(0);
          }
        });
    assertEquals(1, reads[0]);
    assertNotNull(evenOdd);
    assertEquals(Path.FillType.EVEN_ODD, evenOdd.getFillType());
    assertNull(
        mContext.getOrBuildPath(invalidKey, 0, new PlatformRendererContext.PathBytesLoader() {
          @Override
          byte[] load() {
            reads[0]++;
            return invalid.getPathBytes(0);
          }
        }));
    assertEquals(2, reads[0]);
    PlatformRendererContext.PathBytesLoader warmLoader =
        new PlatformRendererContext.PathBytesLoader() {
          @Override
          byte[] load() {
            throw new AssertionError("Warm paths and failures must not export source data.");
          }
        };
    assertSame(evenOdd, mContext.getOrBuildPath(validKey, 1, warmLoader));
    Path winding = mContext.getOrBuildPath(validKey, 0, warmLoader);
    assertNotSame(evenOdd, winding);
    assertEquals(Path.FillType.WINDING, winding.getFillType());
    assertEquals(Path.FillType.EVEN_ODD, evenOdd.getFillType());
    assertEquals(bounds(evenOdd), bounds(winding));
    assertNull(mContext.getOrBuildPath(invalidKey, 1, warmLoader));
    assertEquals(2, reads[0]);
  }

  @Test
  @UiThreadTest
  public void builtPathsOutliveNativeOwners() {
    for (NativeDisplayListBuilder source :
        new NativeDisplayListBuilder[] {list().rectPath(CLIP, 20, 20, 0, 0, 0),
            list().commandPath(CLIP, 0, 0, 0), list().svgPath(CLIP, "M0 0H10V10H0Z", 2, 0, 0, 0)}) {
      byte[] encoded = source.getPathBytes(0);
      source.close();
      Path built = PlatformRendererContext.decodePath(encoded);
      assertNotNull(built);
      assertEquals(Color.RED, draw(built).getPixel(10, 10));
    }
    assertEquals(0, pathEntries().size());
  }

  @Test
  @UiThreadTest
  public void destroyDuringLoadDoesNotCacheBuiltPaths() {
    NativeDisplayListBuilder source = list().rectPath(CLIP, 20, 20, 0, 0, 0);
    long key = source.getPathKey(0);
    prepareContextForDestroy();
    assertNull(mContext.getOrBuildPath(key, 0, new PlatformRendererContext.PathBytesLoader() {
      @Override
      byte[] load() {
        byte[] bytes = source.getPathBytes(0);
        mContext.destroy();
        return bytes;
      }
    }));
    assertEquals(0, pathEntries().size());
  }

  @Test
  @UiThreadTest
  public void contextDestroyClearsCacheWithoutInvalidatingJavaBindings() {
    NativeDisplayListBuilder source = list().rectPath(CLIP, 20, 20, 0, 0, 0);
    Path retained = source.getPathObject(0, 0);
    NativeDisplayListBuilder lazy = list().rectPath(CLIP, 30, 30, 0, 0, 0);
    assertEquals(1, pathEntries().size());
    prepareContextForDestroy();
    mContext.destroy();
    assertEquals(0, pathEntries().size());
    assertNull(source.getPathObject(0, 0));
    assertNull(lazy.getPathObject(0, 0));
    assertEquals(0, pathEntries().size());
    assertEquals(Color.RED, draw(retained).getPixel(10, 10));
    source.close();
    lazy.close();
    mContext.destroy();
    assertEquals(0, pathEntries().size());
  }

  @Test
  @UiThreadTest
  public void lruCacheBoundsEntriesWithoutInvalidatingRetainedPaths() {
    Path firstPath = null;
    for (int i = 0; i < 300; i++) {
      NativeDisplayListBuilder source = list().rectPath(CLIP, 20, 20, 0, 0, 0);
      Path winding = source.getPathObject(0, 0);
      Path evenOdd = source.getPathObject(0, 1);
      assertNotSame(winding, evenOdd);
      assertSame(winding, source.getPathObject(0, 0));
      if (i == 0) {
        firstPath = winding;
      }
      source.close();
    }
    assertEquals(256, pathEntries().size());
    assertNotNull(firstPath);
    assertEquals(Color.RED, draw(firstPath).getPixel(10, 10));
  }

  @Test
  @UiThreadTest
  @SuppressWarnings("unchecked")
  public void ownerUpdatesUseNewPropertyTableAndRetainContent() {
    NativeDisplayListBuilder state = rendererState();
    NativeDisplayListBuilder initial = list()
                                           .begin(SIGN, 0, 0, 0, 100, 100)
                                           .rectPath(OWNER, 20, 20, 0, 0, 0)
                                           .rectPath(CLIP, 100, 100, 0, 0, 0)
                                           .recordBox(0, 0, 100, 100)
                                           .fill(Color.RED, 0)
                                           .end();
    Path oldClip = initial.getPathObject(0, 0);
    Path contentClip = initial.getPathObject(1, 0);
    initial.installIntoRenderer(state);
    Bitmap first = drawNativeHost();
    assertEquals(Color.GREEN, first.getPixel(10, 10));
    assertEquals(Color.TRANSPARENT, first.getPixel(50, 10));
    Object content = field(mRenderer, "mDisplayListItemsBuffer");
    SparseArray<Path> contentPaths =
        (SparseArray<Path>) field(field(mRenderer, "mDisplayListApplier"), "mPaths");
    assertNotNull(contentPaths);
    assertSame(contentClip, contentPaths.get((1 << 1) | 0));
    NativeDisplayListBuilder update = list().rectPath(OWNER, 20, 20, 1, 40, 0);
    Path newClip = update.getPathObject(0, 1);
    update.installIntoRenderer(state);
    update.close();
    assertSame(newClip, mContext.getOwnerClipPath(SIGN, 1));
    assertNotSame(oldClip, newClip);
    Bitmap second = drawNativeHost();
    assertEquals(Color.TRANSPARENT, second.getPixel(10, 10));
    assertEquals(Color.GREEN, second.getPixel(50, 10));
    assertSame(content, field(mRenderer, "mDisplayListItemsBuffer"));
    assertSame(contentPaths, field(field(mRenderer, "mDisplayListApplier"), "mPaths"));
    list().clearOwnerClip().installIntoRenderer(state);
    Bitmap cleared = drawNativeHost();
    assertEquals(Color.GREEN, cleared.getPixel(10, 10));
    assertEquals(Color.GREEN, cleared.getPixel(50, 10));
    assertSame(content, field(mRenderer, "mDisplayListItemsBuffer"));
    assertSame(contentPaths, field(field(mRenderer, "mDisplayListApplier"), "mPaths"));
    verify(mContext, times(1)).getDisplayListItemsBuffer(SIGN);
    verify(mContext, times(1)).getDisplayListDataBuffer(SIGN);
    assertEquals(Path.FillType.WINDING, oldClip.getFillType());
    assertEquals(Color.RED, draw(oldClip).getPixel(10, 10));
    saveBitmap("path-data-owner-before.png", first);
    saveBitmap("path-data-owner-updated.png", second);
    saveBitmap("path-data-owner-cleared.png", cleared);
  }

  @Test
  @UiThreadTest
  public void multipleOwnerClipPropertiesInOneBatchResolveOnlyFinalClip() {
    NativeDisplayListBuilder state = rendererState();
    clearInvocations(mContext, mHost);
    list()
        .begin(SIGN, 0, 0, 0, 100, 100)
        .rectPath(OWNER, 10, 10, 0, 0, 0)
        .rectPath(OWNER, 20, 20, 1, 40, 40)
        .end()
        .installIntoRenderer(state);
    verify(mContext, times(1)).getOwnerClipPath(SIGN, 1);
    verify(mHost, times(1)).invalidateForRenderer();
    Bitmap bitmap = drawNativeHost();
    assertEquals(Color.TRANSPARENT, bitmap.getPixel(5, 5));
    assertEquals(Color.GREEN, bitmap.getPixel(50, 50));
  }

  @Test
  @UiThreadTest
  public void ownerPlacementIgnoresRenderOffsetAndAnchorsToHostScroll() {
    NativeDisplayListBuilder state = rendererState();
    mRenderer.setLynxFrame(false, 0, 0, 100, 100, 7, 9);
    list()
        .begin(SIGN, 0, 0, 0, 100, 100)
        .rectPath(OWNER, 20, 20, 0, 20, 20)
        .end()
        .installIntoRenderer(state);
    Path path = mContext.getOwnerClipPath(SIGN, 0);
    Bitmap unScrolled = drawNativeHost();
    assertEquals(Color.TRANSPARENT, unScrolled.getPixel(15, 15));
    assertEquals(Color.GREEN, unScrolled.getPixel(25, 25));
    assertEquals(Color.GREEN, unScrolled.getPixel(35, 35));
    assertEquals(Color.TRANSPARENT, unScrolled.getPixel(45, 45));
    assertEquals(new RectF(0, 0, 20, 20), bounds(path));

    when(mHost.getRendererHostScrollX()).thenReturn(15);
    when(mHost.getRendererHostScrollY()).thenReturn(25);
    Bitmap scrolled = drawNativeHost();
    assertEquals(Color.TRANSPARENT, scrolled.getPixel(15, 15));
    assertEquals(Color.GREEN, scrolled.getPixel(25, 25));
    assertEquals(Color.GREEN, scrolled.getPixel(35, 35));
    assertEquals(Color.TRANSPARENT, scrolled.getPixel(45, 45));
  }

  @Test
  @UiThreadTest
  public void scopedClipCoversNativeChildAndRestoresForSibling() {
    NativeDisplayListBuilder source = list()
                                          .begin(SIGN, 0, 0, 0, 100, 100)
                                          .rectPath(CLIP, 20, 20, 0, 10, 10)
                                          .drawView(22, 0, 0)
                                          .end();
    when(mContext.getDisplayListItemsBuffer(SIGN)).thenReturn(source.toItemsBuffer());
    when(mContext.getDisplayListDataBuffer(SIGN)).thenReturn(source.toDataBuffer());
    doAnswer(call -> source.getPathObject(call.getArgument(1), call.getArgument(2)))
        .when(mContext)
        .getPath(eq(SIGN), anyInt(), anyInt());
    mRenderer = new Renderer(mContext, SIGN);
    mRenderer.setRenderHost(mHost);
    View child = new View(mApp);
    child.layout(0, 0, 100, 100);
    child.setBackgroundColor(Color.GREEN);
    Bitmap bitmap = Bitmap.createBitmap(100, 100, Bitmap.Config.ARGB_8888);
    Canvas canvas = new Canvas(bitmap);
    int count = canvas.getSaveCount();
    mRenderer.onDraw(canvas);
    mRenderer.beforeDrawChild(canvas, child);
    child.draw(canvas);
    mRenderer.afterDrawChild(canvas, child);
    mRenderer.afterDispatchDraw(canvas);
    assertEquals(count, canvas.getSaveCount());
    assertEquals(Color.GREEN, bitmap.getPixel(20, 20));
    assertEquals(Color.TRANSPARENT, bitmap.getPixel(50, 50));
    Paint paint = new Paint();
    paint.setColor(Color.BLUE);
    canvas.drawRect(50, 50, 60, 60, paint);
    assertEquals(Color.BLUE, bitmap.getPixel(55, 55));
  }

  @Test
  @UiThreadTest
  public void destroyDuringDrawStillRestoresOwnerClip() {
    NativeDisplayListBuilder state = rendererState();
    list()
        .begin(SIGN, 0, 0, 0, 100, 100)
        .rectPath(OWNER, 20, 20, 0, 0, 0)
        .end()
        .installIntoRenderer(state);
    Canvas canvas = new Canvas(Bitmap.createBitmap(100, 100, Bitmap.Config.ARGB_8888));
    int count = canvas.getSaveCount();
    mRenderer.onDraw(canvas);
    mRenderer.beforeDrawHost(canvas);
    mRenderer.onDestroy();
    mRenderer.afterDispatchDraw(canvas);
    assertEquals(count, canvas.getSaveCount());
  }

  @Test
  @UiThreadTest
  public void invalidOwnerClipDoesNotClearTheLayerClip() {
    NativeDisplayListBuilder state = rendererState();
    list()
        .begin(SIGN, 0, 0, 0, 100, 100)
        .rectPath(OWNER, 20, 20, 0, 0, 0)
        .end()
        .installIntoRenderer(state);
    assertEquals(Color.GREEN, drawNativeHost().getPixel(10, 10));
    list().svgPath(OWNER, "M0 0L10", 1, 0, 0, 0).installIntoRenderer(state);
    assertNull(mContext.getOwnerClipPath(SIGN, 0));
    Bitmap bitmap = drawNativeHost();
    assertEquals(Color.TRANSPARENT, bitmap.getPixel(10, 10));
    assertEquals(Color.TRANSPARENT, bitmap.getPixel(50, 50));
  }
}
